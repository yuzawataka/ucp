/*
ucp - UDT based remote file copy
*/

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <string>
#include <map>
#include <sys/stat.h>
#include <netdb.h>
#include <librsync.h>
#include "udt.h"
#include "ucp.h"

static int local_to_remote(UDTSOCKET hnd, unsigned int* token, 
						   unsigned int* sequence, vector<string>& fnames, remote_path& rp);
static int remote_to_local(UDTSOCKET hnd, unsigned int* token, 
						   unsigned int* sequence, vector<string>& fnames, remote_path& rp);
using namespace std;

#ifdef TEST
int _ucp_main(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	int opt;
	int port = 0;
	bool encryption;

	while((opt = getopt(argc,argv,"hvdep:")) !=-1) {
		switch(opt){
		case 'h':
			// show_usage();
			break;
		case 'v':
			break;
		case 'd':
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'e':
			break;
		default:
			break;
		}
	}

	// for (int i = 0; i < argc; i++)
	// 	cout << "argv[" << i << "]: " << argv[i] << endl;

	int first = optind;
	int last = argc - 1;
	// for (first = optind; optind < argc; optind++)
	// 	last = optind;

	string first_f(argv[first]);
	string last_f(argv[last]);

	remote_path rp;
	bool remote_local;
	string fn_entry;
	vector<string> flist;

	if (check_one_colon(first_f) && check_one_colon(last_f)) {
		cout << "invalid" << endl;
		exit(1);
	} else if (check_one_colon(first_f)) {
		remote_local = true;
		rp = check_remote_path(first_f);
		cout << "Not Implemented." << endl;
		exit(1);
	} else if (check_one_colon(last_f)) {
		remote_local = false;
		rp = check_remote_path(last_f);
		fn_entry = argv[2];
	} else {
		cout << "invalid" << endl;
		exit(1);
	}

    int rc, errno;
    struct addrinfo hints, *peer;
    struct ucp_header header;
    char tid = 0;

    string peer_host(rp.host);

	char p[6];
	if (port == 0)
		port = UDT_PORT;
	snprintf(p, 6, "%d", port);
	string peer_port(p);
	cout << "port: " << peer_port << endl;
    
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

	srand(time(NULL));
	unsigned int token = rand();
	unsigned int sequence = 0;
	cout << "0: token: " << token << ", seq: " << sequence << endl;

	// ========= auth begin =========
	struct ucp_header hdr0, hdr1;
	gen_ucp_header(&hdr0, UCP_OP_AUTH, token, sequence, 0, 1);
	rc = ucp_send(hnd, &hdr0, NULL);
	rc = ucp_recv(hnd, &hdr0, NULL, 0);
	if (check_ucp_header(hdr0, UCP_OP_ACK, token, sequence + 1) != 0){
		show_ucp_header(hdr0);
	}
	token = ntohl(hdr0.token);
	sequence = ntohl(hdr0.seq);
	// ========= auth end   =========

	cout << "1: token: " << token << ", seq: " << sequence << endl;
	
	if (remote_local) {
		cout << "remote->local  name: " << rp.name << ", host: " << rp.host << ", path: " << rp.path << endl;
		remote_to_local(hnd, &token, &sequence, flist, rp);
	} else{
		if (get_file_list(fn_entry, &flist) != 0) {
			cout << fn_entry << " is not found." << endl;
			exit(1);
		}
		cout << "local->remote  name: " << rp.name << ", host: " << rp.host << ", path: " << rp.path << endl;
		local_to_remote(hnd, &token, &sequence, flist, rp);
		cout << "2: token: " << token << ", seq: " << sequence << endl;
	}
	UDT::close(hnd);
    UDT::cleanup();
    return 0;
}

static int local_to_remote(UDTSOCKET hnd, unsigned int* token, unsigned int* sequence, vector<string>& flist, remote_path& rp)
{
	int rc;
	struct chunk_meta chunk_mta;
	gen_chunk_meta(flist, rp, chunk_mta);
	chunk_mta.type = UCP_META_TYPE_UPLOAD;
	cout << "local_to_remote: " << *token  << endl;
	if ((rc = ucp_send_metadata(hnd, &chunk_mta, token, sequence)) == -1) {
		cout << "Sending metadata failed." << endl;
		return -1;
	}
	if ((rc = ucp_send_chunk(hnd, &chunk_mta, token, sequence)) == -1) {
		cout << "Sending chunk failed." << endl;
		return -1;
	}
}

static int remote_to_local(UDTSOCKET hnd, unsigned int* token, unsigned int* sequence, vector<string>& fnames, remote_path& rp)
{
	int rc;
	struct chunk_meta chunk_mta;
	gen_chunk_meta(fnames, rp, chunk_mta);
	chunk_mta.type = UCP_META_TYPE_DOWNLOAD;
	if ((rc = ucp_send_metadata(hnd, &chunk_mta, token, sequence)) == -1) {
		cout << "Sending metadata failed." << endl;
		return -1;
	}
	if ((rc = ucp_recv_metadata(hnd, &chunk_mta, token, sequence)) == -1) {
		cout << "Receiving metadata failed." << endl;
		return -1;
	}
	if ((rc = ucp_recv_chunk(hnd, &chunk_mta, token, sequence)) == -1) {
		cout << "Receiving chunk failed." << endl;
		return -1;
	}
}


void show_usage(void) 
{
	cout << "-v version" << endl;
	cout << "-d debug" << endl;
	cout << "-e encription" << endl;
	cout << "-p port UDT_port" << endl;
	cout << "-h show this usage." << endl;
}
