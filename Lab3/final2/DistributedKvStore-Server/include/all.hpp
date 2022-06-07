#ifndef ALL_HPP
#define ALL_HPP
#include <iostream>
#include <bits/stdc++.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <netinet/tcp.h>

// 好像头文件的引用关系也有影响...
// 把先被使用的类放在前面
#include "RedisCommand.hpp"
#include "RedisClient.hpp"
#include "RedisServer.hpp"
#include "SinglePC.hpp"
#include "Net.hpp"

const bool openDebug = false;
const bool debugElection = true;

#endif