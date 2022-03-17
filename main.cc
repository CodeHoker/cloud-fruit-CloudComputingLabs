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

/*
int64_t now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char* argv[])
{
  init_neighbors();

  FILE* fp = fopen(argv[1], "r");
  char puzzle[128];
  int total_solved = 0;
  int total = 0;
  bool (*solve)(int) = solve_sudoku_basic;
  if (argv[2] != NULL)
    if (argv[2][0] == 'a')
      solve = solve_sudoku_min_arity;
    else if (argv[2][0] == 'c')
      solve = solve_sudoku_min_arity_cache;
    else if (argv[2][0] == 'd')
      solve = solve_sudoku_dancing_links;
  int64_t start = now();
  while (fgets(puzzle, sizeof puzzle, fp) != NULL) {
    if (strlen(puzzle) >= N) {
      ++total;
      input(puzzle);
      init_cache();
      //if (solve_sudoku_min_arity_cache(0)) {
      //if (solve_sudoku_min_arity(0))
      //if (solve_sudoku_basic(0)) {
      if (solve(0)) {
        ++total_solved;
        if (!solved())
          assert(0);
      }
      else {
        printf("No: %s", puzzle);
      }
    }
  }
  int64_t end = now();
  double sec = (end-start)/1000000.0;
  printf("%f sec %f ms each %d\n", sec, 1000*sec/total, total_solved);

  return 0;
}
*/
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

char **buf;             //缓冲区，存放题目
int n_pthread;          //线程数量
int total = 0;          //解决问题总数
int n_data = 0;         //缓冲区剩余题目个数
int use_ptr = 0;        //消费下标
int fill_ptr = 0;       //生产下标
int cur_print = 0;      //当前打印者编号
int finish_num = 0;     //当前打开的文件 线程完成的数量

std::list<char *> file_list; //文件名队列

int64_t start=0;
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
    printf("文件读取中\n");
    while(1){
        scanf("%s", tmp);
        pthread_mutex_lock(&lock_file);
        file_list.push_back(tmp);
        while(file_list.size()>=2){
          printf("等待文件使用\n");
          pthread_cond_wait(&filein,&lock_file);

        }
        pthread_cond_signal(&fileout);
        pthread_mutex_unlock(&lock_file);
      }
}
 //2.初始化线程
void init()
{
    //获取cpu核心数
    int n = getCpuInfo(); 
    n_pthread = n;

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
  printf("生产者启动\n");
  while(1){

    //从队列中获取一个文件
    printf("获取文件中\n");
    pthread_mutex_lock(&lock_write);
    while(file_list.size() == 0){
      printf("等待文件读取\n");
      pthread_cond_wait(&fileout,&lock_write); 
    };
    pthread_cond_signal(&filein);
    pthread_mutex_unlock(&lock_write);
    pthread_mutex_lock(&lock_file);
    strcpy(filename,file_list.front());
    file_list.pop_front();
    pthread_mutex_unlock(&lock_file);
    if(strcmp(filename, "quit") == 0){
      //等待解题进程结束，并退出
        for (int i = 0; i < n_pthread; ++i){
            pthread_join(tid[i], NULL);
        }
            int64_t end = now();
            int64_t sec = (end-start)/1000000.0;
            printf("%f sec %f ms each %d\n", sec, 1000*sec/total, finish_num);
            program_end();
            printf("退出\n");
            exit(0);
        }
    if(access(filename, F_OK) == -1){
            printf("文件不存在\n");
            continue;
        }
    FILE *fp=fopen(filename,"r");
    total=0; //遇到新文件重头计数
    cur_print=0; //遇到新文件重头打印
    while(1){
      //加锁
      pthread_mutex_lock(&visit_buf);
      //当缓存区满就开始等待
      while(n_data==n_pthread){
        printf("等待问题解决\n");
        pthread_cond_wait(&empty, &visit_buf);
      }
      //读取fp中的一行到缓存区
      if(fgets(buf[fill_ptr], 83, fp) != NULL){
                fill_ptr = (fill_ptr + 1) % n_pthread;
                ++n_data;
            }
      else{
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
    printf("执行%d\n",getpid());  
    while (1)
    {
        pthread_mutex_lock(&visit_buf);

        while (n_data == 0){
            printf("等待问题输入\n");
            pthread_cond_wait(&full, &visit_buf);
        }

        ++total;                              //当前数据的行数
        //int record_print = total;           //调试用
        int myturn = (total - 1) % n_pthread; //应该的打印顺序

        //读一行题放到board里面
        char *data = get();
        for (int i = 0; i < 81; ++i){
            board[i] = data[i] - '0';
            
        }
        printf("\n");
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&visit_buf);

        //解决问题
        solve_sudoku_dancing_links(board);
        printf("解决问题: %d\n",total);
        if(n_data==0){
          finish_num+=1;
        }
        //输出顺序
        pthread_mutex_lock(&lock_print);
        while (myturn != cur_print){    //没有轮到则在自己的条件变量上等待
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
    init();

    for (int i = 0; i < n_pthread; ++i){
        pthread_create(&tid[i], NULL, solver, NULL);        //解题
    }
    pthread_create(&file_thread,NULL,file_handler,NULL);
    pthread_create(&producer,NULL,put,NULL);
     //读文件名
    

    for (int i = 0; i < n_pthread; ++i){
        pthread_join(tid[i], NULL);
    }
    int64_t end = now();
    int64_t sec = (end-start)/1000000.0;
    printf("%f sec %f ms each %d\n", sec, 1000*sec/total, finish_num);
    program_end();
    return 0;
}
