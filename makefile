test:
	g++ -o bin/test test.cpp -Iinclude -luv -std=c++11 -Wall -Werror -ggdb
test-enet:
	g++ -o bin/test-enet test-enet.cpp -Iinclude -luv -lenet -std=c++11 -Wall -Werror -ggdb
test-sqlite:
	g++ -o bin/test-sqlite test-sqlite.cpp -Iinclude -luv -lsqlite3 -std=c++11 -Wall -Werror -ggdb