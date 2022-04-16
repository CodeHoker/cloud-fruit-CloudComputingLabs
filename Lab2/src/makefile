httpserver:main.o threadpool.o httpcon.o
	gcc $^ -lpthread -o httpserver
main.o:main.cpp
	gcc -c main.cpp  -o main.o
threadpool.o:locker.cpp
	gcc -c locker.cpp -o threadpool.o
httpcon.o :httpcon.cpp
	gcc -c httpcon.cpp -o httpcon.o
clean: 
	rm httpserver main.o httpcon.o threadpool.o
