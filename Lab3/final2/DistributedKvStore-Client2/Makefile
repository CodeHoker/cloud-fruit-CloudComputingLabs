src = $(wildcard ./Src/*.cpp)
src_t = ./Src/main.cpp ./Src/newfun.cpp

target = userServer
path_include = ./Include

all:$(target)

# 艹了,编译不默认支持11
userServer:$(src)
	g++ -std=c++11 -o $@ $^ -I $(path_include) 
