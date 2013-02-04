CFLAGS=-g -Wall -W -D__udt_epoll -D__DEBUG

.SUFFIXES: .cpp .o

all:	ucp ucpd

.cpp:
	g++ -I. -c $< -o $@

ucp:	ucp.o ucp_common.o
	g++ $(CFLAGS) -o ucp ucp.o ucp_common.o -ludt -lpthread -lmsgpack -lboost_filesystem -lboost_system -lattr

ucpd.o:	ucpd.cpp
	g++ $(CFLAGS) -c ucpd.cpp

ucpd:	ucpd.o ucp_common.o
	g++ $(CFLAGS) -o ucpd ucpd.o ucp_common.o -ludt -lpthread -lmsgpack -lboost_filesystem -lboost_system -lattr

test:	test.cpp
	rm -f *.o test
	g++ -DTEST -D__DEBUG -c ucp_common.cpp
	g++ -DTEST -c test.cpp
	g++ -DTEST -o test test.o ucp_common.o -ludt -lpthread -lmsgpack -lboost_filesystem -lboost_system
	./test

clean:
	rm -f *.o ucp ucpd test
