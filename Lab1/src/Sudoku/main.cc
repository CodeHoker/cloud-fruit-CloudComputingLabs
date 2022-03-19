#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <list>
#include <sys/time.h>
#include "tools.h"
#include "sudoku.h"

//1. 定义所需变量，初始化线程
//2. 设计生产者，消费者
//3. 设计解题函数
//4. 主程序读取文件进程

//1.定义所需变量
pthread_mutex_t visit_buf = PTHREAD_MUTEX_INITIALIZER;    //缓冲区锁
pthread_mutex_t lock_print = PTHREAD_MUTEX_INITIALIZER;   //打印锁
pthread_mutex_t lock_write = PTHREAD_MUTEX_INITIALIZER;      //数据写入锁
pthread_mutex_t lock_file = PTHREAD_MUTEX_INITIALIZER;    //文件队列锁
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;          //生产者挂起
pthread_cond_t full = PTHREAD_COND_INITIALIZER;           //消费者挂起
pthread_cond_t *print_order;                              //控制输出顺序
pthread_cond_t filein;                                   //控制生产者接收数据
pthread_cond_t fileout;                                   //控制接收线程接收数据
pthread_t *tid;                                           //解题线程
pthread_t file_thread;                                    //读文件线程
pthread_t producer;                                       //读取文件数据进入缓存区线程

char **buf;              //缓冲区，存放题目
int n_pthread;          //线程数量
int total = 0;          //解决问题总数
int n_data = 0;         //缓冲区剩余题目个数
int use_ptr = 0;        //消费下标
int fill_ptr = 0;       //生产下标
int cur_print = 0;      //当前打印者编号
int finish_num = 0;     //当前打开的文件 线程完成的数量
bool flag_end_file=false; //判断当前是否已经不再有输入
bool data_empty=false; //判断是否已无题目输入
char *data;   //获取问题
FILE *fp;     //获取文件名
std::list<char *> file_list; //文件名队列

int64_t start;
//释放开辟的动态数组空间
void program_end()
{
    free(print_order);
    free(tid);
    for (int i = 0; i < n_pthread; ++i)
        free(buf[i]);
    free(buf);
}

//获取当前时间
int64_t  now()
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return tv.tv_sec * 1000000 + tv.tv_usec;
}
void *put(void *argv);
//处理从屏幕读入的文件名，加入文件队列
int num=0;
void *file_handler(void *argv) 
{
    char tmp[20];
    //printf("文件读取中\n");
    while(scanf("%s", tmp)!=EOF){
        //scanf("%s", tmp);
        char *tmp_name = (char *)malloc(40);
        strcpy(tmp_name, tmp);
        pthread_mutex_lock(&lock_file);
        //printf("存储文件%s\n",tmp_name);
        file_list.push_back(tmp_name);
        pthread_cond_signal(&fileout);
        pthread_mutex_unlock(&lock_file);
      }
  //printf("file end\n");
  flag_end_file=true;
  pthread_cond_signal(&fileout);
  pthread_exit(NULL);
}
 //2.初始化线程
void init(int x)
{
    //获取cpu核心数
    int n = x; 
    n_pthread = n;
    //printf("CPU核数%d\n",n);

    //缓冲区开辟n行
    buf = (char **)malloc(n * sizeof(char *));
    for (int i = 0; i < n_pthread; ++i)
        buf[i] = (char *)malloc(83);

    //n个条件变量控制输出
    print_order = (pthread_cond_t *)malloc(n * sizeof(pthread_cond_t));

    //n个线程号
    tid = (pthread_t *)malloc(n * sizeof(pthread_t));

    //初始化 输出顺序的条件变量
    for (int i = 0; i < n_pthread; ++i){
        print_order[i] = PTHREAD_COND_INITIALIZER;
    }
}

//3.生产者
void *put(void *argv)
{
  char filename[100];
  //printf("生产者启动\n");
  while(1){
    //从队列中获取一个文件
    //printf("获取文件中\n");
    pthread_mutex_lock(&lock_file);
    while(file_list.size() == 0){
         if(flag_end_file == true){ //如果当前文件不在读入
        //等待解题进程结束，并退出
          data_empty=true;
          for (int i = 0; i < n_pthread; ++i){
              pthread_cond_signal(&full);
          }
          pthread_mutex_unlock(&lock_file);
          //printf("退出put\n");
          pthread_exit(NULL);
      }
      //printf("等待文件读取\n");
      pthread_cond_wait(&fileout,&lock_file); 
    }
    strcpy(filename,file_list.front());
    file_list.pop_front();
    //printf("获取文件为%s\n",filename);
    pthread_mutex_unlock(&lock_file); 
    
    if(access(filename, F_OK) == -1){
            printf("文件不存在\n");
            continue;
        }
    
    fp=fopen(filename,"r");
    char puzzle[128];
    while(1){
      //加锁

      pthread_mutex_lock(&visit_buf);
      //当缓存区满就开始等待
      while(n_data==n_pthread){
        //printf("等待问题解决\n");
        pthread_cond_wait(&empty, &visit_buf);
      }
      //读取fp中的一行到缓存区
      if(fgets(puzzle,sizeof puzzle, fp) != NULL){
              if(strlen(puzzle)>=81){
                
                for (int i = 0; i < 81; i++) {
                    buf[fill_ptr][i] = puzzle[i];
                }
                //printf("读取文件%s到buf\n",filename);
                fill_ptr = (fill_ptr + 1) % n_pthread;
                ++n_data;
                //printf("data:%d\n", n_data);
                finish_num+=1;

              }
                
            }
      else {
          pthread_mutex_unlock(&visit_buf);
          break;
      }
     
      pthread_cond_signal(&full);
      pthread_mutex_unlock(&visit_buf);
      
    }

  }

}
//消费者
char *get()
{
    char *tmp = buf[use_ptr];
    use_ptr = (use_ptr + 1) % n_pthread;
    n_data--;
    return tmp;
}
//解题执行函数
void *solver(void *arg)
{
    int board[81]; 
    //printf("线程%ld开始执行\n", pthread_self());
    while (1)
    {
        
        pthread_mutex_lock(&visit_buf);

        while (n_data == 0){
          //printf("%d\n", data_empty);
          if(data_empty){
            //printf("%ld线程结束\n",pthread_self());
            //printf("文件个数%d\n",file_list.size());
            pthread_mutex_unlock(&visit_buf);
            pthread_exit(NULL);
          }
            //printf("等待问题输入\n");
            pthread_cond_wait(&full, &visit_buf);
        }

        ++total;                              //当前数据的行数
        //int record_print = total;           //调试用
        int myturn = (total - 1) % n_pthread; //应该的打印顺序

        //读一行题放到board里面
        data = get();
        for (int i = 0; i < 81; ++i){
            board[i] = data[i] - '0';
            
        }
        
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&visit_buf);

        //解决问题
        solve_sudoku_dancing_links(board);
        //printf("解决问题: %d\n",finish_num);
        //输出顺序
        pthread_mutex_lock(&lock_print);
        while (myturn != cur_print){    //没有轮到则在自己的条件变量上等待
            //printf("%ld线程睡在print上\n",pthread_self());
            pthread_cond_wait(&print_order[myturn], &lock_print);
        }
        //打印到屏幕 注释掉可以省不少时间
        for (int i = 0; i < 81; ++i)
            printf("%d", board[i]);
        printf("\n");

        cur_print = (cur_print + 1) % n_pthread;                     //下一个该打印的编号
        pthread_cond_signal(&print_order[(myturn + 1) % n_pthread]); //唤醒下一个，如果对方在睡的话
        //这里也可以只用一个条件变量，到这里用broadcast唤醒所有其他线程，但是效率可能会低一点，没有尝试-.-
        pthread_mutex_unlock(&lock_print);
    }
}
int main(int argc, char *argv[])
{
    init(5);
    start=now();
    pthread_create(&file_thread,NULL,file_handler,NULL);
    pthread_create(&producer,NULL,put,NULL);
    for (int i = 0; i < n_pthread; ++i){
        pthread_create(&tid[i], NULL, solver, NULL);        //解题
    }
     //读文件名
    for (int i = 0; i < n_pthread; ++i){
        pthread_join(tid[i], NULL);
    }
    int64_t end = now();
    printf("%ld,%ld\n", end,start);
    double sec = (end-start)/1000000.0;

    printf("%lf sec %lf ms each num=%d\n",sec,1000*sec/finish_num,finish_num);
    program_end();
    return 0;
}
