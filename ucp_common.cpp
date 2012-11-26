#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/stat.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <sys/mman.h>
#include <fcntl.h>
#include <udt.h>
#include "ucp.h"

using namespace std;

int ucp_send(UDTSOCKET hnd, ucp_header header, const char* payload)
{
    int rc;
    size_t buffer_len;
    void *buffer;
    
    buffer_len = sizeof(ucp_header) + header.length;
    buffer = malloc(buffer_len);
	if (buffer == NULL)
		return 1;
    memset(buffer, 0, buffer_len);

    memcpy(buffer, &header, sizeof(ucp_header));
    memcpy(buffer + sizeof(ucp_header), payload, header.length);
    
    rc = UDT::send(hnd, (const char*)buffer, buffer_len, 0);
    if (buffer != NULL)
        free(buffer);
    if (rc != buffer_len) {
        return -1;
    } else {
        return rc;
    }
}


int ucp_recv(UDTSOCKET hnd, ucp_header header, char* payload, size_t payload_len)
{
    int rc;
    size_t buffer_len;
	void *buffer;
    buffer_len = sizeof(ucp_header) + payload_len;
    buffer = malloc(buffer_len);
	if (buffer == NULL)
		return 1;
    memset(buffer, 0, buffer_len);
	
	if (UDT::ERROR == UDT::recv(hnd, (char*)buffer, buffer_len, 0)) {
        cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;
        return 1;
    } 
	memcpy(&header, buffer, sizeof(ucp_header));
	memcpy(payload, buffer + sizeof(ucp_header), payload_len);
	if (buffer != NULL)
		free(buffer);
	return 0;
}


int ucp_send_dummy(UDTSOCKET hnd, const char* buf, size_t len)
{
    int rc;
    
    rc = UDT::send(hnd, (const char*)buf, len, 0);
    if (rc != len) {
        return -1;
    } else {
        return rc;
    }
}


// int ucp_recv(void* usocket)
// {
//     unsigned char *buffer; 
//     ucp_header *header;
//     unsigned char *payload;
//     size_t payload_len;
    
//     char buf[1024];

//     cout << "dispatch" << endl;
//     UDTSOCKET hnd = *(UDTSOCKET*)usocket;
//     delete (UDTSOCKET*)usocket;

//     if (UDT::ERROR == UDT::recv(hnd, (char*)buf, sizeof(buf), 0)) {
//         cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;
//         return 0;
//     } 

// }

int gen_meta_data(string fname)
{
	struct stat fs;
	char *list;
	size_t size;
	if (stat(fname.c_str(), &fs) == -1)
		return -1;
	cout << fs.st_mode << endl;
	cout << fs.st_uid << endl;
	cout << fs.st_gid << endl;
	cout << fs.st_size << endl;
	if (listxattr(fname.c_str(), list, size) != -1)
		cout << size << endl;
}

int gen_chunk()
{
}

void udt_status(UDTSOCKET s)
{

	int32_t val = 0;
	int size = sizeof(int32_t);
	int val2;
	int size2 = sizeof(int);
	bool b;
	int size3 = sizeof(bool);
	linger l;
	int size4 = sizeof(linger);
	linger val64;
	int size5 = sizeof(int64_t);

	cout << "------------------------------" << endl;
	val2 = 0;
	UDT::getsockopt(s, 0, UDT_MSS, &val2, &size2);
	cout << "UDT_MSS: " << val2 << endl;

	UDT::getsockopt(s, 0, UDT_SNDSYN, &b, &size3);
	cout << "UDT_SNDSYN: " << b << endl;

	UDT::getsockopt(s, 0, UDT_RCVSYN, &b, &size3);
	cout << "UDT_RCVSYN: " << b << endl;

	val2 = 0;
	UDT::getsockopt(s, 0, UDT_FC, &val2, &size2);
	cout << "UDT_FC: " << val2 << endl;

	val2 = 0;
	UDT::getsockopt(s, 0, UDT_SNDBUF, &val2, &size2);
	cout << "UDT_SNDBUF: " << val2 << endl;

	val2 = 0;
	UDT::getsockopt(s, 0, UDT_RCVBUF, &val2, &size2);
	cout << "UDT_RCVBUF: " << val2 << endl;

	val2 = 0;
	UDT::getsockopt(s, 0, UDP_SNDBUF, &val2, &size2);
	cout << "UDP_SNDBUF: " << val2 << endl;

	val2 = 0;
	UDT::getsockopt(s, 0, UDP_SNDBUF, &val2, &size2);
	cout << "UDP_SNDBUF: " << val2 << endl;

	UDT::getsockopt(s, 0, UDT_LINGER, &l, &size4);
	cout << "UDT_LINGER: " << val2 << endl;

	UDT::getsockopt(s, 0, UDT_RENDEZVOUS, &b, &size3);
	cout << "UDT_RENDEZVOUS: " << b << endl;

	val2 = 0;
	UDT::getsockopt(s, 0, UDT_SNDTIMEO, &val2, &size2);
	cout << "UDT_SNDTIMEO: " << val2 << endl;

	val2 = 0;
	UDT::getsockopt(s, 0, UDT_RCVTIMEO, &val2, &size2);
	cout << "UDT_RCVTIMEO: " << val2 << endl;

	UDT::getsockopt(s, 0, UDT_REUSEADDR, &b, &size3);
	cout << "UDT_REUSEADDR: " << b << endl;

	// UDT::getsockopt(s, 0, UDT_MAXBW, &val64, &size5);
	// cout << "UDT_REUSEADDR: " << val64 << endl;

	val = 0;
	UDT::getsockopt(s, 0, UDT_STATE, &val, &size);
	char buf[256];
	switch (val) {
	case 1: 
		strcpy(buf, "INIT");
		break;
	case 2:
		strcpy(buf, "OPENED");
		break;
	case 3:
		strcpy(buf, "LISTENING");
		break;
	case 4:
		strcpy(buf, "CONNECTING");
		break;
	case 5:
		strcpy(buf, "CONNECTED");
		break;
	case 6:
		strcpy(buf, "BROKEN");
		break;
	case 7:
		strcpy(buf, "CLOSING");
		break;
	case 8:
		strcpy(buf, "CLOSED");
		break;
	case 9:
		strcpy(buf, "NONEXIST");
	};
	cout << "UDT_STATE: " << buf << endl;

	val = 0;
	UDT::getsockopt(s, 0, UDT_EVENT, &val, &size);
	cout << "UDT_EVENT: " << val << endl;
	val = 0;
	UDT::getsockopt(s, 0, UDT_SNDDATA, &val, &size);
	cout << "UDT_SNDDATA: " << val << endl;
	val = 0;
	UDT::getsockopt(s, 0, UDT_RCVDATA, &val, &size);
	cout << "UDT_RCVDATA: " << val << endl;
	cout << "------------------------------" << endl;
}


bool check_one_colon(string str)
{
    unsigned int loc;
    loc = str.find(":", 0);
    if (loc == string::npos)
        return false;
    if (str.find(":", loc + 1) != string::npos)
        return false;
    return true;
}

remote_path check_remote_path(string str)
{
    remote_path rp;
    string id, host, path;
    unsigned int loc0, loc1, len;
    loc0 = str.find("@", 0);
    loc1 = str.find(":", 0);
    len = str.length();
    if (loc0 != string::npos) {
        rp.id.assign(str, 0, loc0);
        rp.host.assign(str, loc0 + 1, loc1 - loc0 - 1);
        rp.path.assign(str, loc1 + 1, len - loc1);
    } else {
        rp.id = "";
        rp.host.assign(str, 0, loc1);
        rp.path.assign(str, loc1 + 1, len - loc1);
    }
    return rp;
}


int gen_each_meta(string fname, unsigned long long int chunk_offset, file_mdata* fm)
{
	struct stat fs;
	char *list;
	size_t size;
	ssize_t attr_size;
	if (stat(fname.c_str(), &fs) == -1)
		return -1;
	attr_size = listxattr(fname.c_str(), NULL, 0);
	fm->fname = fname;
	fm->uid = fs.st_uid;
	fm->gid = fs.st_gid;
	fm->mode = fs.st_mode;
	fm->mtime = fs.st_mtime;
	fm->data_begin = chunk_offset;
	fm->data_end = fm->data_begin + fs.st_size;
	fm->xattr_begin = fm->data_end + 1;
	fm->xattr_end = fm->xattr_begin + attr_size;
#ifdef __DEBUG
	cout << "fname: " << fm->fname << endl;
	cout << "uid: " << fm->uid << endl;
	cout << "gid: " << fm->gid << endl;
	cout << "mode: " << fm->mode << endl;
	cout << "mtime: " << fm->mtime << endl;
	cout << "data_begin: " << fm->data_begin << endl;
	cout << "data_end: " << fm->data_end << endl;
	cout << "xattr_begin: " << fm->xattr_begin << endl;
	cout << "xattr_end: " << fm->xattr_end << endl;
#endif
	return 0;
}

int gen_chunk_mmap(vector<string> fnames, int ofd)
{
	int sz = 104857600;
	int offset = 0;

	for (vector<string>::iterator i = fnames.begin(); i != fnames.end(); i++) {
		int ifd = open((*i).c_str(), O_RDONLY);
		cout << "in fd: " << ifd << endl;
		void *ip = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, ifd, 0);
		cout << "0" << endl;
		void *op = mmap(NULL, sz, PROT_WRITE, MAP_PRIVATE, ofd, 0);
		cout << "1" << endl;
		memcpy(ip, op, sz);
		// while (sz--) {
		// 	*op++ = *ip++;
		// }
		cout << "2" << endl;
	}
}

int gen_chunk(vector<string> fnames, string tmpfname)
{
	ofstream ofs(tmpfname.c_str());
	char buf[4096];
	streamsize pos = 0;

	for (vector<string>::iterator i = fnames.begin(); i != fnames.end(); i++) {
		ifstream ifs((*i).c_str());
		streamsize len = ifs.seekg(0, ios::end).tellg();
		ifs.seekg(0, ios::beg);
#ifdef __DEBUG
		cout << len << endl;
#endif
		while (!ifs.eof()) {
			ifs.read(buf, sizeof(buf));
			ofs.write(buf, sizeof(buf));
			ofs.flush();
		}
		ifs.close();
		pos += len;
		ofs.seekp(pos);
#ifdef __DEBUG
		cout << "point: " << ofs.tellp() << endl;
#endif
	}
	if (truncate(tmpfname.c_str(), pos) == -1)
		return -1;
	return pos;
}

int split_chunk(string tmpfname)
{
}

int get_xattr(string fname, map<string, string>* attr)
{
	char* names;
	char* n;
	ssize_t attr_len, len;
	size_t cur = 0;
	size_t found;
	int rc = 0;
	
	attr_len = listxattr(fname.c_str(), NULL, 0);
	if (attr_len == -1)
		return -1;
	if (attr_len == 0)
		return 0;
#ifdef __DEBUG
	cout << "attr_len: " << attr_len << endl;
#endif
	rc += attr_len;
	names = (char *)malloc(attr_len);
	if (!names)
		return -1;
	listxattr(fname.c_str(), names, attr_len);

	for (n = names; n != names + attr_len; n = strchr(n, '\0') + 1) {
		string name = string(n);
		size_t len = getxattr(fname.c_str(), name.c_str(), NULL, 0);
		char* value;
		value = (char*)malloc(len);
		if (!value)
			return -1;
		getxattr(fname.c_str(), name.c_str(), value, len);
		rc += len;
		string value_str = string(value, len);
		attr->insert(map<string, string>::value_type(name, value_str));		
		free(value);
#ifdef __DEBUG
		cout << "name" << name << "(" << len << "): " << value_str << endl;
#endif
	}
	free(names);
	return rc;
}
