CXXFLAGS+=-O2 -ggdb -DDEBUG
CXXFLAGS+=-Wall -Wextra

all: sudoku_solve

sudoku_solve: main.cc neighbor.cc sudoku_basic.cc sudoku_min_arity.cc sudoku_min_arity_cache.cc sudoku_dancing_links.cc tool.cc
	g++ -O2 -o $@ $^ -lpthread
