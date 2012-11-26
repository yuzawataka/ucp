/*
ucp - UDT based remote file copy
*/

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cstring>
#include <string>
#include <map>
#include <netdb.h>
#include <ossp/uuid++.hh>
#include "udt.h"
#include "ucp.h"

static int local_to_remote(UDTSOCKET hnd, int token, vector<string> fnames);
static int remote_to_local(UDTSOCKET hnd, int token, vector<string> fnames);
static int ucp_request_auth(UDTSOCKET hnd, int token, int seq);

using namespace std;

#ifdef TEST
int _ucp_main(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
//	GetOpt getopt (argc, argv, "vp:");


    int rc, errno;
    struct addrinfo hints, *peer;
    ucp_header header;
    char tid = 0;
    string peer_host("localhost");
    string peer_port(UDT_PORT);
    
    uuid id;
    id.make(UUID_MAKE_V4);
    // cout << id.string() << endl;  
    // cout << id.integer() << endl;
    // cout << id.binary() << endl;
    
    // cout << "unsigned long long:" << sizeof(unsigned long long) << endl;
  
    if (UDT::ERROR == UDT::startup()) {
        cout << "init: " << UDT::getlasterror().getErrorMessage() << endl;
        return -1;
    }      

    if (memset(&hints, 0, sizeof(struct addrinfo)) == NULL) {
        return -1;
    }
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    UDTSOCKET hnd = UDT::socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
    if (getaddrinfo(peer_host.c_str(), peer_port.c_str(), &hints, &peer) != 0) {
        cout << "incorrect server/peer address. " << argv[1] << ":" << argv[2] << endl;
        return -1;
    }
    
    if (UDT::ERROR == UDT::connect(hnd, peer->ai_addr, peer->ai_addrlen)) {
        cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
        return -1;
    }

	freeaddrinfo(peer);

	int token = 1;
	vector<string> fnames;
		
	if (true) {
		local_to_remote(hnd, token, fnames);
	} else{
		remote_to_local(hnd, token, fnames);
	}
    
    // string buf("This is a test.");
    // header.token = htonl(256);
    // header.op = UCP_OP_TOKEN;
    // header.seq = htonl(1);
    // header.length = htonl(buf.size());

    // char buf2[1024];

    // rc = ucp_send_dummy(hnd, buf.c_str(), buf.size());
    // // rc = ucp_send(hnd, header, buf.c_str());
	// rc = UDT::recv(hnd, buf2, sizeof(buf2), 0);
	// udt_status(hnd);
	// cout << buf2 << ": " << strlen(buf2) << endl;
    
    UDT::cleanup();
    return 0;
}

static int local_to_remote(UDTSOCKET hnd, int token, vector<string> fnames)
{
	// send auth
	ucp_request_auth(hnd, token, 0);
	// // send meta data 
	// ucp_send();
	// ucp_recv();
	// // send chunk
	// ucp_send();
	// ucp_recv();
}

static int remote_to_local(UDTSOCKET hnd, int token, vector<string> fnames)
{
	// send auth
	ucp_request_auth(hnd, token, 0);
	// // recv meta data 
	// ucp_send();
	// ucp_recv();
	// // recv chunk
	// ucp_send();
	// ucp_recv();
}


static int ucp_request_auth(UDTSOCKET hnd, int token, int seq)
{
	int rc;
	ucp_header hdr0, hdr1;
	u64 length = 10;
	void *buf0, *buf1;

	hdr0.version = UCP_VERSION;
	hdr0.op = UCP_OP_AUTH;
	hdr0.token = htonl(token);
	hdr0.seq = htonl(seq);
	hdr0.length = length;
	hdr0.flags = htonl(1);

	buf0 = malloc(10);
	if (buf0 == NULL)
		return 1;
	memset(buf0, 1, 10);
	strcpy((char*)buf0, "This Test");
	rc = ucp_send(hnd, hdr0, (char*)buf0);
	if (buf0 != NULL)
		free(buf0);
	if (rc == -1)
		return 1;
	buf1 = malloc(10);
	if (buf1 == NULL)
		return 1;
	cout << "receiving...";
	rc = ucp_recv(hnd, hdr1, (char*)buf1, sizeof(buf1));
	if (rc == -1)
		return 1;
	cout << "received." << endl;
	// check auth

	if (buf1 != NULL)
		free(buf1);

	return 0;
}
