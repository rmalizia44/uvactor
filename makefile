all:
	g++ -o bin/test test.cpp -Iinclude -luv -std=c++11 -Wall -Werror -ggdb