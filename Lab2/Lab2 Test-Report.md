# Lab2 Test-Report

### 一、实验简介

实现自己的HTTP服务器，从网络的角度来看，你的HTTP服务器应该实现以下功能:

1. 创建侦听套接字并将其绑定到端口
2. 等待客户端连接到该端口
3. 接受客户端并获得一个新的连接套接字
4. 读入并解析HTTP请求
5. 开始交付服务
   - 处理HTTP GET/POST请求，如果出现错误返回错误消息。

在基本版本中，每个TCP连接在同一时间只有一个请求。客户机等待响应，当它得到响应时，可能会为 新请求重用TCP连接(或使用新的TCP连接)。这也是普通HTTP服务器所支持的。

### 二、实验环境

**ENV 1:** linux 内核版本为 Linux version 5.4.0-84-generic (buildd@lcy01-amd64-007) ；16GB 内存；CPU 型号为 Intel(R) Core(TM) i7-8750H CPU @ 2.20GHz，2核

**ENV 2:** linux 内核版本为 Linux version 5.4.0-84-generic (buildd@lcy01-amd64-007) ；16GB 内存；CPU 型号为 Intel(R) Core(TM) i7-8750H CPU @ 2.20GHz，4核

### 三、测试过程

### 基本功能测试

- 使用make编译，并执行./http-server --ip 127.0.0.1 --port 8080 --threads 2

![image-20220420122846941](https://s2.loli.net/2022/04/20/uTStbmPvKR4e6j2.png)

- 使用实验脚本测试结果正确：

  ![image-20220420123054246](https://s2.loli.net/2022/04/20/bUlr1acSKfYJsen.png)

- 并发压力测试：ab -n 100 -c 10 http://localhost:8080/

  ![image-20220420123319444](https://s2.loli.net/2022/04/20/3PRHkLZMXrESgjp.png)

### 性能测试

- **Env1**：2核 1，2，3，4，5，10线程 concurren_num：10-100 每个客户端请求数：100

  1：

  ![thread_1](https://s2.loli.net/2022/04/20/tw6EURMzfB8jNeP.png)

  2：![thread_2](https://s2.loli.net/2022/04/20/qNVMpbfOWtCUmy9.png)

  3：

  ![thread_3](https://s2.loli.net/2022/04/20/ZkJrpEKvBW3N9SD.png)

  

  4：

  ![thread_4](https://s2.loli.net/2022/04/20/dlek92MUSsJWLE7.png)

  5：

  ![thread_5](https://s2.loli.net/2022/04/20/nKSYfxr2JV3j6pG.png)

  10：

  ![thread_10](https://s2.loli.net/2022/04/20/PlLBFGohCy9nejk.png)

  总结：在2核CPU中，最快的处理次数出现在2线程的客户端，此时不会出现调度，处理速度应该是最快的，大概是6700个/S

- **Env2**：4核 4，8，10线程 concurren_num：10-100 每个客户端请求数：100

  4：

  ![thread_2-4](https://s2.loli.net/2022/04/20/RZrefAMDaF4qHch.png)

  8：

  ![thread_2-8](https://s2.loli.net/2022/04/20/XcOzkhEKCVjt2oF.png)

  

  10：

  ![thread_2-10](https://s2.loli.net/2022/04/20/pzZQYt43cT1mF75.png)

  总结：在4核CPU中，最快的处理次数出现在8线程的客户端，处理速度应该是最快的，大概是8200个/S

  