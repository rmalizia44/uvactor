test:
	g++ -o bin/test test.cpp -Iinclude -luv -std=c++11 -Wall -Werror -ggdb
test-enet:
	g++ -o bin/test-enet test-enet.cpp -Iinclude -luv -lenet -std=c++11 -Wall -Werror -ggdb