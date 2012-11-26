CFLAGS=-g -Wall -W -DEPOLL

.SUFFIXES: .cpp .o

all:	ucp ucpd

.cpp:
	g++ -I. -c $< -o $@

ucp:	ucp.o ucp_common.o
	g++ $(CFLAGS) -o ucp ucp.o ucp_common.o -ludt -lpthread -lossp-uuid++

ucpd.o:	ucpd.cpp
	g++ $(CFLAGS) -c ucpd.cpp

ucpd:	ucpd.o ucp_common.o
	g++ $(CFLAGS) -o ucpd ucpd.o ucp_common.o -ludt -lpthread

test:	test.cpp
	g++ -DTEST -c ucp_common.cpp
	g++ -DTEST -c test.cpp
	g++ -DTEST -o test test.o ucp_common.o -ludt -lpthread
	./test

clean:
	rm -f *.o ucp ucpd
