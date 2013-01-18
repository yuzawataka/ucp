CFLAGS=-g -Wall -W -D__udt_epoll -D__DEBUG

.SUFFIXES: .cpp .o

all:	ucp ucpd

.cpp:
	g++ -I. -c $< -o $@

ucp:	ucp.o ucp_common.o
	g++ $(CFLAGS) -o ucp ucp.o ucp_common.o -ludt -lpthread -lrsync -lmsgpack -lboost_filesystem

ucpd.o:	ucpd.cpp
	g++ $(CFLAGS) -c ucpd.cpp

ucpd:	ucpd.o ucp_common.o
	g++ $(CFLAGS) -o ucpd ucpd.o ucp_common.o -ludt -lpthread -lpam -lpam_misc -lrsync -lmsgpack -lboost_filesystem

test:	test.cpp
	rm -f *.o test
	g++ -DTEST -D__DEBUG -c ucp_common.cpp
	g++ -DTEST -c test.cpp
	g++ -DTEST -o test test.o ucp_common.o -ludt -lpthread -lboost_filesystem -lmsgpack
	./test

clean:
	rm -f *.o ucp ucpd test
