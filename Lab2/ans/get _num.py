#测试脚本生成图片
import matplotlib.pyplot as plt
x=[i for i in range(10,110,10)]
y=[]
for i in range(10,110,10):
    with open("C:\\Users\\15409\\Desktop\\网易\\ans_4-10\\"+str(i)+".txt") as f:
        for line in f:
            if "Requests per second" in line:
                print(line)
                print(float(line[24:28]))
                y.append(float(line[24:28]))
        #if "Requests per second" in str1:
            #print(str1)
plt.plot(x,y,linewidth=1)
plt.title("server-thread-2")
plt.xlabel("concurrent_num")
plt.ylabel("Requests per second")
plt.savefig("image\\thread_2-"+str(10)+".png")
plt.show()
