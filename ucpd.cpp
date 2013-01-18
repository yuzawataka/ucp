#include <cstdlib>
#include <netdb.h>
#include <iostream>
#include <cstring>
#include <map>
#include <arpa/inet.h>
#include <librsync.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <udt.h>
#include <pwd.h>
#include <errno.h>
#include <sys/stat.h>
#include "ucp.h"
// #include <boost/filesystem/path.hpp>
// #include <boost/filesystem/operations.hpp>

#define EPOLL_TIMEOUT_SEC 60
#define BOOST_FILESYSTEM_VERSION 3

using namespace std;
// using namespace boost::filesystem;

#ifdef __udt_epoll
static void* dispatch(UDTSOCKET hnd);
#endif
#ifdef __pthread
static void* dispatch(void *usock);
#endif

int is_dir(string& path);
int is_file(string& path);

int main(int argc, char* argv[])
{
	int opt;
	int port = 0;
	while((opt = getopt(argc,argv,"hvdp:")) !=-1) {
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
		}
	}

	char p[6];
	if (port == 0)
		port = UDT_PORT;
	snprintf(p, 6, "%d", port);
	string service(p);
	cout << "port: " << service << endl;

    UDT::startup();
    
    addrinfo hints;
    addrinfo *res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

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

#ifdef __udt_epoll
	int ep;
	ep = UDT::epoll_create();
    set<UDTSOCKET> readfds;
    set<UDTSOCKET> writefds;
    UDT::epoll_add_usock(ep, srv);
	while (true) {
		int nfd;
		int64_t tv = EPOLL_TIMEOUT_SEC * 1000;
		nfd = UDT::epoll_wait(ep, &readfds, &writefds, tv);
		for (set<UDTSOCKET>::iterator i = readfds.begin(); i != readfds.end(); ++ i) {
			if (*i == srv) {
                UDTSOCKET hnd;
                sockaddr_storage clientaddr;
                int addrlen = sizeof(clientaddr);
                if (UDT::INVALID_SOCK == (hnd = UDT::accept(srv, (sockaddr*)&clientaddr, &addrlen))) {
                    cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
                    continue;
                }
				cout << "accepted: " << hnd << endl;
                UDT::epoll_add_usock(ep, hnd);
			} else {
				cout << "received: " << *i << endl;
				dispatch(*i);
			}
		}
	}
#endif

#ifdef __pthread
	while (true) {
		UDTSOCKET hnd;
		sockaddr_storage clientaddr;
		int addrlen = sizeof(clientaddr);
		if (UDT::INVALID_SOCK == (hnd = UDT::accept(srv, (sockaddr*)&clientaddr, &addrlen)))
			cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
        pthread_t filethread;
		pthread_create(&filethread, NULL, dispatch, new UDTSOCKET(hnd));
		pthread_detach(filethread);
	}
#endif

    UDT::close(srv);
    UDT::cleanup();
  
    return 0;
}

#ifdef __udt_epoll
static void* dispatch(UDTSOCKET hnd)
#endif
#ifdef __pthread
static void* dispatch(void *usock)
#endif
{
#ifdef __pthread
	UDTSOCKET hnd = *(UDTSOCKET*)usock;
#endif
	int ep;
    set<UDTSOCKET> readfds;
    set<UDTSOCKET> writefds;
	unsigned int token = 0;
	unsigned int sequence = 0;
	int rc;
	struct chunk_meta chunk_mta;

	cout << "0: token: " << token << ", seq: " << sequence << endl;
	ep = UDT::epoll_create();
    UDT::epoll_add_usock(ep, hnd);

	// =========== auth begin ===========
	struct ucp_header hdr0, hdr1;
	rc = ucp_recv(hnd, &hdr0, NULL, 0);
	if (check_ucp_header(hdr0, UCP_OP_AUTH, token, sequence) != 0){
		show_ucp_header(hdr0);
	}
	token = ntohl(hdr0.token);
	sequence = ntohl(hdr0.seq);
	sequence++;
	gen_ucp_header(&hdr1, UCP_OP_ACK, token, sequence, 0, 1);
	rc = ucp_send(hnd, &hdr1, NULL);
	// =========== auth end   ===========

	cout << "1: token: " << token << ", seq: " << sequence << endl;

	rc = ucp_recv_metadata(hnd, &chunk_mta, &token, &sequence);

	if (chunk_mta.type == UCP_META_TYPE_UPLOAD) {
		cout << "Going to Upload." << endl; 

		struct passwd pw;
		struct passwd *result;
		char *buf;
		size_t buf_sz;
		buf_sz = sysconf(_SC_GETPW_R_SIZE_MAX);
		if (buf_sz == -1)
			buf_sz = 16384;
		buf = (char*)malloc(buf_sz);
		if (buf == NULL) {
			return NULL;
		}
		rc = getpwnam_r(chunk_mta.user.c_str(), &pw, buf, buf_sz, &result);
		if (result == NULL) {
			if (rc == 0) {
				cout << chunk_mta.user << " is not found." << endl;
				UDT::close(hnd);
				return NULL;
			}
		}

		string dest_path;
		if (chunk_mta.dest_path.empty())
			dest_path = pw.pw_dir;
		if (chunk_mta.dest_path.find_first_not_of("/", 0) == 0) {
			dest_path = chunk_mta.dest_path;
		} else {
			dest_path = pw.pw_dir + '/' + chunk_mta.dest_path;
		}

		if (!access(dest_path.c_str(), F_OK) && chunk_mta.file_count <= 1) {
			cout << "only one new file write" << endl;
//			chdir(dest_path.parent_path().c_str());
		}
		if (!access(dest_path.c_str(), F_OK) && chunk_mta.file_count > 1) {
			cout << "Error. No such directory" << endl;
			UDT::close(hnd);
			return NULL;
		}

		if (access(dest_path.c_str(), F_OK) && is_dir(dest_path)) {
			cout << "files write under directory" << endl;
			chdir(dest_path.c_str());
		}
		if (access(dest_path.c_str(), F_OK) && is_file(dest_path)) {
			if (chunk_mta.file_count <= 1) {
				cout << "only one file overwrite" << endl;
//				chdir(dest_path.parent_path().c_str());
			} else {
				cout << "Error. Src many, Dst one" << endl;
				UDT::close(hnd);
				return NULL;
			}
		}


		// dest_path = chunk_mta.dest_path;
		// if (!dest_path.is_absolute()) {
		// 	path home_path(pw.pw_dir);
		// 	dest_path = absolute(dest_path, home_path);
		// }

		// if (!exists(dest_path) && chunk_mta.file_count <= 1) {
		// 	cout << "only one new file write" << endl;
		// 	chdir(dest_path.parent_path().c_str());
		// }
		// if (!exists(dest_path) && chunk_mta.file_count > 1) {
		// 	cout << "Error. No such directory" << endl;
		// 	UDT::close(hnd);
		// 	return NULL;
		// }
		// if (exists(dest_path) && is_directory(dest_path)) {
		// 	cout << "files write under directory" << endl;
		// 	chdir(dest_path.c_str());
		// }
		// if (exists(dest_path) && is_regular_file(dest_path)) {
		// 	if (chunk_mta.file_count <= 1) {
		// 		cout << "only one file overwrite" << endl;
		// 		chdir(dest_path.parent_path().c_str());
		// 	} else {
		// 		cout << "Error. Src many, Dst one" << endl;
		// 		UDT::close(hnd);
		// 		return NULL;
		// 	}
		// }

		cout << "dest_path: " << dest_path << endl;
		cout << "2: token: " << token << ", seq: " << sequence << endl;

		ucp_recv_chunk(hnd, &chunk_mta, &token, &sequence);
		cout << "rc: " << rc << endl;
		if (buf != NULL)
			free(buf);
		UDT::close(hnd);
	} else {
		cout << "Going to Download." << endl; 
		// send meta data
		// send chunk
		UDT::close(hnd);
	}
	return NULL;
}

int is_dir(string& path)
{
	int rc;
	struct stat fs;
	rc = stat(path.c_str(), &fs);
	if (rc == -1)
		return -1;
	if (S_ISDIR(fs.st_mode))
		return 1;
	return 0;
}
			
int is_file(string& path)
{
	int rc;
	struct stat fs;
	rc = stat(path.c_str(), &fs);
	if (rc == -1)
		return -1;
	if (S_ISREG(fs.st_mode))
		return 1;
	return 0;
}
