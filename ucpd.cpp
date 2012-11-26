#include <cstdlib>
#include <netdb.h>
#include <iostream>
#include <cstring>
#include <map>
#include <arpa/inet.h>
#include <udt.h>
#include "ucp.h"

#define EPOLL_TIMEOUT_SEC 60

using namespace std;

static void* dispatch(UDTSOCKET hnd);
static int ucp_response_auth(UDTSOCKET hnd, size_t auth_len, int *token);

int main(int argc, char* argv[])
{
    UDT::startup();
    
    addrinfo hints;
    addrinfo *res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    string service(UDT_PORT);
    
    if (getaddrinfo(NULL, service.c_str(), &hints, &res) != 0) {
        cout << "" << endl;
        return 1;
    }
    UDTSOCKET srv = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    
    if (UDT::ERROR == UDT::bind(srv, res->ai_addr, res->ai_addrlen)) {
        cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }
    freeaddrinfo(res);
    
    if (UDT::ERROR == UDT::listen(srv, 10)) {
        cout << "listen: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }
	cout << "Listen fd = " << srv << endl;

    /* UDT::epoll() 1 */
    int ep;
    set<UDTSOCKET> readfds;
    ep = UDT::epoll_create();
	cout << "Using UDT::epoll() ";
    UDT::epoll_add_usock(ep, srv);
    while (true) {
        int i;
        int nfd;
		int pid;
		int64_t tv = EPOLL_TIMEOUT_SEC * 1000;
        nfd = UDT::epoll_wait(ep, &readfds, NULL, tv);
        for (set<UDTSOCKET>::iterator i = readfds.begin(); i != readfds.end(); ++ i) {
            if (*i == srv) {
                UDTSOCKET hnd;
                sockaddr_storage clientaddr;
                int addrlen = sizeof(clientaddr);
                if (UDT::INVALID_SOCK == (hnd = UDT::accept(srv, (sockaddr*)&clientaddr, &addrlen))) {
                    cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
                    continue;
                }
                UDT::epoll_add_usock(ep, hnd);
            } else {
				cout << "using fork()" << endl;
				if ((pid = fork()) == 0) {
					dispatch(*i);
					_exit(0);
				} else {
					UDT::close(*i);
				}
            }
        }
    }
    UDT::epoll_release(ep);

    UDT::close(srv);
    UDT::cleanup();
  
    return 0;
}


static void* dispatch(UDTSOCKET hnd)
{
	int token;
	int rc;
	cout << "dispatched" << endl;
	rc = ucp_response_auth(hnd, 10, &token);
	cout << "rc: " << rc << endl;
	// ucp_recv_metadata(hnd);
}

// int cli_to_srv(UDTSOCKET hnd)
// {
// 	// recv auth
// 	ucp_recv();
// 	ucp_send();
// 	// recv meta data
// 	rcp_recv();
// 	ucp_send();
// 	// recv chunk
// 	rcp_recv();
// 	ucp_send();
// }

// int srv_to_cli(UDTSOCKET hnd)
// {
// 	// recv auth
// 	ucp_recv();
// 	ucp_send();
// 	// send meta data
// 	ucp_recv();
// 	ucp_send();
// 	// send chunk
// 	ucp_recv();
// 	ucp_send();
// }


static int ucp_response_auth(UDTSOCKET hnd, size_t auth_len, int *token)
{
	int rc;
	ucp_header hdr0, hdr1;
	void *buf0, *buf1;
	cout << "receiving...";
	rc = ucp_recv(hnd, hdr0, (char*)buf0, auth_len);
	if (rc == -1)
		return -1;
	cout << "received." << endl;
	if (hdr0.version != UCP_VERSION)
		return -1;
	int seq = ntohl(hdr1.seq);

	printf ("%s\n", buf0);

	// auth check
	hdr1.version = UCP_VERSION;
	hdr1.op = UCP_OP_AUTH;
	hdr1.token = hdr1.token;
	hdr1.seq += seq;
	hdr1.length = auth_len;
	hdr1.flags = htonl(1);
	
	buf1 = malloc(auth_len);
	if (buf1 == NULL)
		return -1;
	memset(buf1, 3, auth_len);
	rc = ucp_send(hnd, hdr1, (char*)buf1);
	if (buf1 != NULL)
		free(buf1);
	if (rc == -1)
		return -1;
	return 0;
}
