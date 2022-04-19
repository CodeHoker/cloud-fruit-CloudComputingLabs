src = $(wildcard ./src/*.cpp)

target:http-server

http-server:$(src)
	g++ -o $@ $^ -pthread