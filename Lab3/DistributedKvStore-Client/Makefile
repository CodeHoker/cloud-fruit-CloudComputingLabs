src = $(wildcard ./Src/*.cpp)
src_t = ./Src/main.cpp ./Src/newfun.cpp

target = client server
path_include = ./Include

all:$(target)

server:./Src/echo_server.c
	gcc -o $@ $^ -lpthread -I $(path_include) 
	
# 艹了,编译不默认支持11
client:$(src_t)
	g++ -std=c++11 -o $@ $^ -lpthread -I $(path_include) 
