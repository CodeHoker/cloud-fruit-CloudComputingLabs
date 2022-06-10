# Lab3

### 运行和演示

### 运行

将Lab3clone下来

进入文件夹Lab3

- 先打开三个服务端

  进入文件夹Lab3/final2/DistributedKvStore-Server/make

  第一个服务端使用 ./server打开

  第二个服务端使用./server .../config/config1.txt

  第三个服务端使用./server .../config/config2.txt

- 然后打开一个新的终端进入Lab3/final2/DistributedKvStore-Client2/

  执行客户端：./userServer

![image-20220610201326358](https://s2.loli.net/2022/06/10/fKysHjLpWRg3uJk.png)

### 演示

在客户端终端里面

执行userShowCmdTable

查看指令

![image-20220610202319781](https://s2.loli.net/2022/06/10/JuNecLRyMPtIWSQ.png)

查看userShowClient查看leader客户端和服务端

![image-20220610202829236](https://s2.loli.net/2022/06/10/JQGqU3XFRTxVcN6.png)

使用setCur <IP>:<端口>可以设置当前操作的服务器

我们设置为9191端口的服务器，并可以对数据进行测试

首先查看操作的服务器

![image-20220610203825542](https://s2.loli.net/2022/06/10/HNXjiwa5dtgsuPR.png)

插入数据

使用set <键><值>插入数据

```
set ki vi
#插入键为ki 值为vi的数据
```

两个服务器存储数据

![image-20220610203922904](https://s2.loli.net/2022/06/10/iUtXDRx6zLGuBwd.png)

获取数据ki

![image-20220610204647964](https://s2.loli.net/2022/06/10/z6SAPXrUCL3jYaw.png)



删除数据ki，再获取验证，会返回nil值

![image-20220610204728332](https://s2.loli.net/2022/06/10/rldJgzyOZsEcFbe.png)

flushDb清空数据库所有数据无法获取

![image-20220610204852126](https://s2.loli.net/2022/06/10/7MhY1UE4aZoA9Vp.png)

断掉Leader，修改Leader

![image-20220610210007258](https://s2.loli.net/2022/06/10/8T3bPyxiMqaQUnv.png)

### 指令解析

![image-20220610205002610](https://s2.loli.net/2022/06/10/fKUumglYjHDowx8.png)

- showCmdTable：在客户端打印命令列表

- loadConfig: 在服务器打开后，用于连接客户端和服务端，以及初始化

- showOldCmds: 在当前操作的服务器上打印整个系统使用的所有指令

- beMy：打印当前leader信息

- userShowClients：打印当前运行的所有客户端信息

- showClients：在服务器打印当前运行的所有服务器信息

- slaveof：用于让服务器服从于某台Leader服务器

  ```
  slaveof <IP><端口>
  ```

- userShowServer: 客户端运行的服务器信息

- get：根据键值获取数据

  ```
  get <键>
  #根据规则返回数据
  ```

- printf

- connect：客户端连接某个服务器

- setLeader：用于投票

- userShowCmdTable：打印命令列表

- del：根据键值删除信息

  ```
  del <键>
  ```

- set :插入信息

  ```
  set <键><值>
  ```

- flushDb:请空数据库

- setCur：设置当前操作的服务器

  ```
  setCur <IP><端口>
  ```

- showServer：打印当前操作的服务端信息（在服务端）



