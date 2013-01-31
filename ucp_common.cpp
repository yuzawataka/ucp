#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <map>
#include <vector>
#include <sys/mman.h>
#include <fcntl.h>
#include <error.h>
#include <udt.h>
#include "ucp.h"
#include <msgpack.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

using namespace std;
using namespace boost::filesystem;

#define BOOST_FILESYSTEM_VERSION 3

int ucp_send(UDTSOCKET hnd, struct ucp_header *header, const char *payload)
{
    int rc;
    size_t buffer_len;
    void *buffer;
	UDT::TRACEINFO trace;
    buffer_len = sizeof(struct ucp_header) + header->length;
    buffer = malloc(buffer_len);
	if (buffer == NULL) {
		cout << "buffer alloc failed." << endl;;
		return -1;
	}
    memset(buffer, 0, buffer_len);

    memcpy(buffer, header, sizeof(struct ucp_header));
	if (payload != NULL)
		memcpy((char*)buffer + sizeof(struct ucp_header), payload, header->length);

    // rc = UDT::send(hnd, (const char*)buffer, buffer_len, -1);
	UDT::perfmon(hnd, &trace);
	int64_t send_sz = 0;
	while (buffer_len > send_sz) {
		rc =  UDT::send(hnd, (char*)buffer + send_sz, buffer_len - send_sz, -1);
		if (rc == UDT::ERROR) {
			cout << "ucp_send failed. " << UDT::getlasterror().getErrorMessage() << " recv: " << rc << endl;
			break;
		}
		send_sz += rc;
	}
	UDT::perfmon(hnd, &trace);

#ifdef __DEBUG
	cout << "====== ucp_send ======" << endl;
	cout << "send_sz: " << send_sz << ", payload_len: " << header->length << ", hdr_sz: " << sizeof(ucp_header) << endl;
	show_ucp_header(*header);
	cout << "====== /ucp_send ======" << endl;
#endif

    if (buffer != NULL)
        free(buffer);
    if (rc != buffer_len) {
        return -1;
    } else {
        return rc;
    }
}


int ucp_recv(UDTSOCKET hnd, struct ucp_header *header, char *payload, size_t payload_len)
{
    int rc;
    size_t buffer_len;
	void *buffer;
	UDT::TRACEINFO trace;
    buffer_len = sizeof(struct ucp_header) + payload_len;
    buffer = malloc(buffer_len);
	if (buffer == NULL) {
		cout << "buffer alloc failed." << endl;;
		return -1;
	}
    memset(buffer, 0, buffer_len);
	
	// if (UDT::ERROR == UDT::recv(hnd, (char*)buffer, buffer_len, 0)) {
    //     cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;
    //     return -1;
    // } 

	UDT::perfmon(hnd, &trace);
	int64_t recv_sz = 0;
	while (buffer_len > recv_sz) {
		rc =  UDT::recv(hnd, (char*)buffer + recv_sz, buffer_len - recv_sz, -1);
		if (rc == UDT::ERROR){
			cout << "ucp_recv failed. " << UDT::getlasterror().getErrorMessage() << " recv: " << rc << endl;
			break;
		}
		recv_sz += rc;
	}
	UDT::perfmon(hnd, &trace);

	memcpy(header, buffer, sizeof(struct ucp_header));
	// mmap()
#ifdef __DEBUG
	cout << "====== ucp_recv ======" << endl;
	cout << "recv_sz: " << recv_sz << ", payload_len: " << payload_len << ", hdr_sz: " << sizeof(ucp_header) << endl;
	show_ucp_header(*header);
	cout << "====== /ucp_recv ======" << endl;
#endif
	if (payload != NULL)
		memcpy(payload, (char*)buffer + sizeof(struct ucp_header), payload_len);
	if (buffer != NULL)
		free(buffer);
	return 0;
}

void udt_trace(UDT::TRACEINFO& trace)
{
	cout << "+--------------------------------+" << endl;
	cout << "|pktSentTotal: " << trace.pktSentTotal << endl;
	cout << "|pktRecvTotal: " << trace.pktRecvTotal << endl;
	cout << "|pktSndLossTotal: " << trace.pktSndLossTotal << endl;
	cout << "|pktRcvLossTotal: " << trace.pktRcvLossTotal << endl;
	cout << "|pktRetransTotal: " << trace.pktRetransTotal << endl;
	cout << "+--------------------------------+" << endl;
	cout << "|pktSent: " << trace.pktSent << endl;
	cout << "|pktRecv: " << trace.pktRecv << endl;
	cout << "|pktSndLoss: " << trace.pktSndLoss << endl;
	cout << "|pktRcvLoss: " << trace.pktRcvLoss << endl;
	cout << "|pktRetrans: " << trace.pktRetrans << endl;
	cout << "|mbpsSendRate: " << trace.mbpsSendRate << endl;
	cout << "|mbpsRecvRate: " << trace.mbpsRecvRate << endl;
	cout << "+--------------------------------+" << endl;
	cout << "|usPktSndPeriod: " << trace.usPktSndPeriod << endl;
	cout << "|pktFlowWindow: " << trace.pktFlowWindow << endl;
	cout << "|pktCongestionWindow: " << trace.pktCongestionWindow << endl;
	cout << "|pktFlightSize: " << trace.pktFlightSize << endl;
	cout << "|msRTT: " << trace.msRTT << endl;
	cout << "|mbpsBandwidth: " << trace.mbpsBandwidth << endl;
	cout << "|byteAvailSndBuf: " << trace.byteAvailSndBuf << endl;
	cout << "|byteAvailRcvBuf: " << trace.byteAvailRcvBuf << endl;
	cout << "+--------------------------------+" << endl;
}

int ucp_send_dummy(UDTSOCKET hnd, const char *buf, size_t len)
{
    int rc;
    
    rc = UDT::send(hnd, (const char*)buf, len, 0);
    if (rc != len) {
        return -1;
    } else {
        return rc;
    }
}


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


bool check_one_colon(string& str)
{
    unsigned int loc;
    loc = str.find(":", 0);
    if (loc == string::npos)
        return false;
    if (str.find(":", loc + 1) != string::npos)
        return false;
    return true;
}

remote_path check_remote_path(string& str)
{
    remote_path rp;
    string name, host, path;
    unsigned int loc0, loc1, len;
    loc0 = str.find("@", 0);
    loc1 = str.find(":", 0);
    len = str.length();
    if (loc0 != string::npos) {
        rp.name.assign(str, 0, loc0);
        rp.host.assign(str, loc0 + 1, loc1 - loc0 - 1);
        rp.path.assign(str, loc1 + 1, len - loc1);
    } else {
        rp.name = "";
        rp.host.assign(str, 0, loc1);
        rp.path.assign(str, loc1 + 1, len - loc1);
    }
    return rp;
}


int64_t gen_each_meta(string fname, uint32_t fnum, uint64_t chunk_offset, file_info* fi, msgpack::sbuffer* sbuf)
{
	struct stat fs;
	char *list;
	size_t size;
	ssize_t attr_size;
	int64_t datum_sz;
	if (stat(fname.c_str(), &fs) == -1)
		return -1;
	fi->fname = fname;
	fi->fnum = fnum;
	fi->uid = fs.st_uid;
	fi->gid = fs.st_gid;
	fi->mode = fs.st_mode;
	fi->mtime = fs.st_mtime;
	fi->data_begin = chunk_offset;
	if (is_directory(fname)) {
		fi->data_end = fi->data_begin;
		fi->xattr_end = fi->data_end;
		datum_sz = 0;
	} else {
		attr_size = get_xattr(fname, sbuf);
		fi->data_end = fi->data_begin + fs.st_size;
		fi->xattr_end = fi->data_end + attr_size;
		datum_sz = fs.st_size + attr_size;
	}
	return datum_sz;
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

int gen_chunk_meta(vector<string>& flist, struct remote_path& rp, struct chunk_meta& chunk_mta)
{
	int i = 0;
	size_t len = 0;
	struct passwd *pw;
	for (vector<string>::iterator j = flist.begin(); j != flist.end(); j++) {
		file_info fi;
		size_t sz;
		msgpack::sbuffer sbuf;
		if (access(j->c_str(), R_OK) != 0) {
			cout << *j << " is not found, or can't access." << endl;
			return -1;
		}
		sz = gen_each_meta(*j, i, len, &fi, &sbuf);
		len += sz;
		chunk_mta.finfo.push_back(fi);
		i++;
	}
	chunk_mta.chunk_seq = 0;
	chunk_mta.file_count = flist.size();
	chunk_mta.type = UCP_META_TYPE_UPLOAD;
	chunk_mta.user = rp.name;
	chunk_mta.dest_path = rp.path;
	chunk_mta.chunk_size = len;
	return 0;
}	

int update_chunk_meta(chunk_meta& chunk_mta, string& dest_path, int overwrite)
{
	int rc = 0;
	for (list<file_info>::iterator i = chunk_mta.finfo.begin(); i != chunk_mta.finfo.end(); ++i) {
		if (overwrite) {
			i->fname = dest_path;
		} else {
			string prefix(dest_path + string("/"));
			i->fname = prefix.append(i->fname);
		}
		++rc;
	}
	return rc;
}

int gen_request_meta(string fname, struct chunk_meta *chunk_mta)
{
	file_info fi;
	fi.fname = fname;
	chunk_mta->type = UCP_META_TYPE_DOWNLOAD;
	return 0;
}

int gen_whole_chunk(vector<string> fnames, struct chunk_meta *chunk_mta, string tmpfname)
{
	ofstream ofs(tmpfname.c_str());
	char buf[4096];
	streamsize pos = 0;
	int j = 0;

	for (vector<string>::iterator i = fnames.begin(); i != fnames.end(); i++) {
		file_info fi;
		msgpack::sbuffer sbuf;
		map<string, string> xattrs;
		streamsize len = 0;

		// store file name to chunk.
		ofs.write((*i).c_str(), (*i).size());
		len += (*i).size();

		// getting file info data.
		if (gen_each_meta(*i, j, pos, &fi, &sbuf) != 0)
			return -1;

		//memcpy((void*)(chunk_mta->finfo[j]), &fi, sizeof(file_info));
		len += fi.data_end - fi.data_begin;

		// getting file extend attributes.
		// if (get_xattr(*i, &xattrs) <= -1)
		// 	return -1;
		// len += fi.xattr_end - fi.xattr_begin;

		// store blob to chunk.
		ifstream ifs((*i).c_str());
		ifs.seekg(0, ios::beg);
		while (!ifs.eof()) {
			ifs.read(buf, sizeof(buf));
			ofs.write(buf, sizeof(buf));
			ofs.flush();
		}
		ifs.close();

		// store extend attribute to chunk
// 		boost::archive::text_oarchive oa(ofs);
// 		oa << xattrs;
// 		ofs.write(xattrs, xattrs.size());		
		pos += len;
		ofs.seekp(pos);
		j++;
	}
	if (truncate(tmpfname.c_str(), pos) == -1)
		return -1;
	return pos;
}

int get_xattr(string fname, msgpack::sbuffer* sbuf)
{
	char* names;
	char* n;
	ssize_t attr_len, len;
	size_t cur = 0;
	size_t found;
	int rc = 0;
	map<string, string> attr;	

	attr_len = listxattr(fname.c_str(), NULL, 0);
	if (attr_len == -1)
		return -1;
	if (attr_len == 0)
		return 0;

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
		attr.insert(map<string, string>::value_type(name, value_str));		
		if (value != NULL)
			free(value);
	}
	if (names != NULL)
		free(names);
	msgpack::pack(*sbuf, attr);
	return sbuf->size();
}

int set_xattr(char* fname, map<string, string>& xattrs)
{
	int rc;
	errno = 0;
	for (map<string, string>::iterator i = xattrs.begin(); i != xattrs.end(); i++) {
		char *cwd;
		cwd = getcwd(NULL, 0);
		rc = setxattr(fname, i->first.c_str(), i->second.c_str(), i->second.size(), XATTR_REPLACE);
		cout << "setxattr rc: " << rc << ", errno: " << errno << ", Current work dir: " << cwd << ", fname: " << fname << ", name: " << i->first.c_str() << ", value: " << i->second.c_str() << ", size:" << i->second.size() << endl;
		switch (errno) {
		case EEXIST:
			cout << fname << " has already extended attributes." << endl;
			break;
		case ENOSPC:
			cout << "No space for set extended attributes." << endl;
			break;
		case EDQUOT:
			cout << "No space for set extended attributes by quota." << endl;
			break;
		case ENOTSUP:
			cout << "Filesystem doesn't support extended attributes." << endl;
			break;
		default:
			cout << strerror(errno) << endl;
			break;
		}
		free(cwd);
	}
	return rc;
}

int set_attr_rename(char *fname, struct file_info* finfo)
{
	errno = 0;
	struct utimbuf times;
	times.actime = finfo->mtime;
	times.modtime = finfo->mtime;

	if (chown(fname, finfo->uid, finfo->gid) != 0) {
		cout << "set_attr: " << strerror(errno) << endl;
		return -1;
	}
	if (chmod(fname, finfo->mode) != 0) {
		cout << "set_attr: " << strerror(errno) << endl;
		return -1;
	}
	if (utime(fname, &times) != 0) {
		cout << "set_attr: " << strerror(errno) << endl;
		return -1;
	}
	if (rename(fname, finfo->fname.c_str()) != 0) {
		cout << "set_attr: " << strerror(errno) << endl;
		return -1;
	}
	return 0;
}

void gen_ucp_header(struct ucp_header* hdr, unsigned char op, uint32_t token, uint32_t seq,
				   uint64_t length, int32_t utility)
{
	hdr->version = UCP_VERSION;
	hdr->op = op;
	hdr->token = htonl(token);
	hdr->seq = htonl(seq);
	hdr->length = length;
	hdr->utility = htonl(utility);
}

int check_ucp_header(struct ucp_header hdr, unsigned char op, uint32_t token, uint32_t seq)
{
	if (hdr.version != UCP_VERSION)
		return UCP_ERR_VERSION_UNMATCH;
	switch (hdr.op) {
	case UCP_OP_AUTH:
		break;
	case UCP_OP_AUTH_ACK:
		break;
	case UCP_OP_AUTH_NCK:
		break;
	case UCP_OP_PREMETA:
		break;
	case UCP_OP_META:
		break;
	case UCP_OP_META_ACK:
		break;
	case UCP_OP_META_NCK:
		break;
	case UCP_OP_CHUNK:
		break;
	case UCP_OP_CHUNK_ACK:
		break;
	case UCP_OP_CHUNK_NCK:
		break;
	case UCP_OP_BLOB:
		break;
	case UCP_OP_BLOB_ACK:
		break;
	case UCP_OP_BLOB_NCK:
		break;
	case UCP_OP_XATR:
		break;
	case UCP_OP_XATR_ACK:
		break;
	case UCP_OP_XATR_NCK:
		break;
	case UCP_OP_ACK:
		break;
	case UCP_OP_NCK:
		break;
	case UCP_OP_RETRY:
		break;
	case UCP_OP_ABORT:
		break;
	}
	if (hdr.op != op)
		return UCP_ERR_OP_UNMATCH;
	if (hdr.token != htonl(token))
		return UCP_ERR_TOKEN_UNMATCH;
	if (hdr.seq != htonl(seq))
		return UCP_ERR_SEQ_UNMATCH;
	return 0;
}

void show_ucp_header(struct ucp_header ucp_hdr)
{
	cout << "--------------------" << endl;
	cout << "version: ";
	if (ucp_hdr.version == UCP_VERSION) {
		cout << "0" << endl;
	} else {
		cout << "Err" << endl;
	}
	cout << "op: ";
	switch (ucp_hdr.op)
	{ 
	case UCP_OP_AUTH:
		cout << "UCP_OP_AUTH" << endl;
		break;
	case UCP_OP_AUTH_ACK:
		cout << "UCP_OP_AUTH_ACK" << endl;
		break;
	case UCP_OP_AUTH_NCK:
		cout << "UCP_OP_AUTH_NCK" << endl;
		break;
	case UCP_OP_PREMETA:
		cout << "UCP_OP_PREMETA" << endl;
		break;
	case UCP_OP_META:
		cout << "UCP_OP_META" << endl;
		break;
	case UCP_OP_META_ACK:
		cout << "UCP_OP_META_ACK" << endl;
		break;
	case UCP_OP_META_NCK:
		cout << "UCP_OP_META_NCK" << endl;
		break;
	case UCP_OP_CHUNK:
		cout << "UCP_OP_CHUNK" << endl;
		break;
	case UCP_OP_CHUNK_ACK:
		cout << "UCP_OP_CHUNK_ACK" << endl;
		break;
	case UCP_OP_CHUNK_NCK:
		cout << "UCP_OP_CHUNK_NCK" << endl;
		break;
	case UCP_OP_BLOB:
		cout << "UCP_OP_BLOB" << endl;
		break;
	case UCP_OP_BLOB_ACK:
		cout << "UCP_OP_BLOB_ACK" << endl;
		break;
	case UCP_OP_BLOB_NCK:
		cout << "UCP_OP_BLOB_NCK" << endl;
		break;
	case UCP_OP_XATR:
		cout << "UCP_OP_XATR" << endl;
		break;
	case UCP_OP_XATR_ACK:
		cout << "UCP_OP_XATR_ACK" << endl;
		break;
	case UCP_OP_XATR_NCK:
		cout << "UCP_OP_XATR_NCK" << endl;
		break;
	case UCP_OP_ACK:
		cout << "UCP_OP_ACK" << endl;
		break;
	case UCP_OP_NCK:
		cout << "UCP_OP_NCK" << endl;
		break;
	case UCP_OP_RETRY:
		cout << "UCP_OP_RETRY" << endl;
		break;
	case UCP_OP_ABORT:
		cout << "UCP_OP_ABORT" << endl;
		break;
	default:
		cout << "Err" << endl;
		break;
	}
	cout << "token: " << ntohl(ucp_hdr.token) << endl;
	cout << "seq: " << ntohl(ucp_hdr.seq) << endl;
	cout << "length: " << ucp_hdr.length << endl;
	cout << "utility: " << ntohl(ucp_hdr.utility) << endl;
	cout << "--------------------" << endl;
}

int get_file_list(string& fname, vector<string>* flist)
{
	if (!exists(fname)) {
		return -1;
	}

	if (is_regular_file(fname)) {
		flist->push_back(fname);
		return 0;
	}
	if (is_directory(fname)) {
		flist->push_back(fname);
		recursive_directory_iterator i = recursive_directory_iterator(fname);
		recursive_directory_iterator end = recursive_directory_iterator();
		for (; i != end; ++i) {
			flist->push_back((*i).path().string());
		}
		return 0;
	}
}

int ucp_send_metadata(UDTSOCKET hnd, struct chunk_meta* chunk_mta, unsigned int* token, unsigned int* sequence)
{
	int rc;
	unsigned int tok = *token;
	unsigned int seq = *sequence;
	cout << "ucp_send_metadata: token: " << tok << ", seq: " << seq << endl;
	struct ucp_header hdr0, hdr1, hdr2, hdr3;
	memset((void*)&hdr0, 0, sizeof(struct ucp_header));
	memset((void*)&hdr1, 0, sizeof(struct ucp_header));
	memset((void*)&hdr2, 0, sizeof(struct ucp_header));
	memset((void*)&hdr3, 0, sizeof(struct ucp_header));

	msgpack::zone zone;
	msgpack::object obj(*chunk_mta, &zone);
	cout << obj << endl;
	msgpack::sbuffer sbuf;
	msgpack::pack(sbuf, obj);
	unsigned int metadata_sz = (int)(sbuf.size());

	// Send PREMATA
	seq++;
	gen_ucp_header(&hdr0, UCP_OP_PREMETA, tok, seq, 0, metadata_sz);
	cout << "ucp_send_metadata 0: " << metadata_sz << endl; 
	show_ucp_header(hdr0);
	rc = ucp_send(hnd, &hdr0, NULL);

	rc = ucp_recv(hnd, &hdr1, NULL, 0);
	// Check ACK
	seq++;
	if ((rc = check_ucp_header(hdr1, UCP_OP_ACK, tok, seq)) != 0) {
		cout << "ucp_send_metadata: recv0 NG (rc=" << rc << ")" << endl;
		ucp_header hdr;
		gen_ucp_header(&hdr, UCP_OP_ABORT, tok, seq, 0, 1);
		rc = ucp_send(hnd, &hdr, NULL);
		return -1;
	}
	cout << "ucp_send_metadata 1: " << metadata_sz << endl; 
	show_ucp_header(hdr1);
	seq = ntohl(hdr1.seq);

	// Send META
	seq++;
	gen_ucp_header(&hdr2, UCP_OP_META, tok, seq, sbuf.size(), 1);
	rc = ucp_send(hnd, &hdr2, sbuf.data());

	// Check META_ACK or META_NCK
	seq++;
	rc = ucp_recv(hnd, &hdr3, NULL, 0);
	if ((rc = check_ucp_header(hdr3, UCP_OP_META_ACK, tok, seq)) != 0) {
		cout << "ucp_send_metadata: recv1 NG (rc=" << rc << ")" << endl;
		return -1;
	}
	cout << "ucp_send_metadata 2" << endl; 
	show_ucp_header(hdr3);

	memcpy(token, &tok, sizeof(int));
	memcpy(sequence, &seq, sizeof(int));

	return rc;

UCP_ABORT:
	return -1;
}

int ucp_recv_metadata(UDTSOCKET hnd, struct chunk_meta* chunk_mta, unsigned int* token, unsigned int* sequence)
{
	int rc;
	unsigned int tok = *token;
	unsigned int seq = *sequence;
	cout << "ucp_recv_metadata: token: " << tok << ", seq: " << seq << endl;
	struct ucp_header hdr0, hdr1, hdr2, hdr3;
	memset((void*)&hdr0, 0, sizeof(struct ucp_header));
	memset((void*)&hdr1, 0, sizeof(struct ucp_header));
	memset((void*)&hdr2, 0, sizeof(struct ucp_header));
	memset((void*)&hdr3, 0, sizeof(struct ucp_header));

	int metadata_sz = 0;
	// Check PREMETA
	rc = ucp_recv(hnd, &hdr0, NULL, 0);
	seq++;
	if ((rc = check_ucp_header(hdr0, UCP_OP_PREMETA, tok, seq)) != 0) {
		cout << "ucp_recv_metadata: recv0 NG (rc=" << rc << ")" << ntohl(hdr0.seq) << endl;
		return -1;
	}
	metadata_sz = ntohl(hdr0.utility);
	cout << "ucp_recv_metadata 0: " << metadata_sz << endl; 
	show_ucp_header(hdr0);
	tok = ntohl(hdr0.token);

	cout << "token: " << tok << ", seq: " << seq << endl;
	
	char *metadata;
	metadata = (char *)malloc(metadata_sz);
	seq = ntohl(hdr0.seq);

	// Send ACK
	seq++;
	gen_ucp_header(&hdr1, UCP_OP_ACK, tok, seq, 0, 1);
	cout << "ucp_recv_metadata 1" << endl; 
	show_ucp_header(hdr1);
	rc = ucp_send(hnd, &hdr1, NULL);

	// Recv META
	seq++;
	rc = ucp_recv(hnd, &hdr2, metadata, metadata_sz);
	if ((rc = check_ucp_header(hdr2, UCP_OP_META, tok, seq)) != 0) {
		cout << "ucp_recv_metadata: recv1 NG (rc=" << rc << ")" << endl;
		switch (rc) {
		case UCP_ERR_VERSION_UNMATCH:
			cout << " UCP_ERR_VERSION_UNMATCH" << endl;
			break;
		case UCP_ERR_OP_UNMATCH:
			cout << " UCP_ERR_OP_UNMATCH" << endl;
			break;
		case UCP_ERR_TOKEN_UNMATCH:
			cout << " UCP_ERR_TOKEN_UNMATCH" << endl;
			break;
		case UCP_ERR_SEQ_UNMATCH:
			cout << " UCP_ERR_SEQ_UNMATCH" << endl;
			break;
		}
		return -1;
	}
	cout << "ucp_recv_metadata 2" << endl; 
	show_ucp_header(hdr2);

	msgpack::unpacked result;
	msgpack::unpack(&result, metadata, metadata_sz);
	msgpack::object obj = result.get();
	cout << obj << endl;
	obj.convert(chunk_mta);

	// Check File Infomation
    // Send META_ACK or META_NCK
	seq = ntohl(hdr2.seq);
	seq++;
	gen_ucp_header(&hdr3, UCP_OP_META_ACK, tok, seq, 0, 1);
	rc = ucp_send(hnd, &hdr3, NULL);

	memcpy(token, &tok, sizeof(int));
	memcpy(sequence, &seq, sizeof(int));

	return rc;

UCP_ABORT:
	return -1;
}

int64_t ucp_send_chunk(UDTSOCKET hnd, struct chunk_meta* chunk_mta, unsigned int* token, unsigned int* sequence)
{
	int rc;
	unsigned int tok = *token;
	unsigned int seq = *sequence;

	struct ucp_header hdr;
	memset((void*)&hdr, 0, sizeof(struct ucp_header));

	uint64_t send_sz = 0;
	for (list<struct file_info>::iterator i = chunk_mta->finfo.begin(); i != chunk_mta->finfo.end(); ++i) {
		cout << "Sending... " << i->fname.c_str() << endl;
		struct ucp_header hdr0;
		memset((void*)&hdr0, 0, sizeof(struct ucp_header));
		if (access(i->fname.c_str(), R_OK) != 0) {
			cout << i->fname << " is not found, or can't access." << endl;
			return -1;
		}
		if (S_ISDIR(i->mode)) {
			cout <<  i->fname << " is dir." << endl;
		} else {
			cout <<  i->fname << " is file." << endl;
			// sending file chunk
			fstream ifs(i->fname.c_str(), ios::in | ios::binary);
			int64_t offset = 0;
#ifndef TEST
			if (UDT::ERROR == (rc = UDT::sendfile(hnd, ifs, offset, i->data_end - i->data_begin))) {
				cout << "ucp_send_chunk failed. " << UDT::getlasterror().getErrorMessage() << " recv: " << rc << endl;
				return -1;
			}
			send_sz += rc;
			// Recv BLOB_ACK or BLOB_NCK
			seq++;
			rc = ucp_recv(hnd, &hdr0, NULL, 0);
			if (check_ucp_header(hdr0, UCP_OP_BLOB_ACK, tok, seq) != 0) {
				cout << "ucp_send_chunk: recv0 NG (rc=" << rc << ")" << endl;
				switch (rc) {
				case UCP_ERR_VERSION_UNMATCH:
					cout << " UCP_ERR_VERSION_UNMATCH" << endl;
					break;
				case UCP_ERR_OP_UNMATCH:
					cout << " UCP_ERR_OP_UNMATCH" << endl;
					break;
				case UCP_ERR_TOKEN_UNMATCH:
					cout << " UCP_ERR_TOKEN_UNMATCH" << endl;
					break;
				case UCP_ERR_SEQ_UNMATCH:
					cout << " UCP_ERR_SEQ_UNMATCH" << endl;
					break;
				}
				return -1;
			}
			if (i->fnum != ntohl(hdr0.utility)) {
				cout << "File number unmatch(blob)." << "(" << i->fnum << "|" << ntohl(hdr0.utility) << endl;
				return -1;
			}
#endif
			// sending extended attributes
			if (i->data_end != i->xattr_end) {
				struct ucp_header hdr1;
				memset((void*)&hdr1, 0, sizeof(struct ucp_header));
				msgpack::sbuffer sbuf;
				rc = get_xattr(i->fname, &sbuf);
				cout << "xattr(" << rc << ","<< sbuf.size() << "): " << sbuf.data() << endl;
#ifndef TEST
				if (rc =  UDT::send(hnd, (char*)(sbuf.data()), sbuf.size(), -1))
					return -1;
				// Recv XATR_ACK or XATR_NCK
				seq++;
				rc = ucp_recv(hnd, &hdr1, NULL, 0);
				if (check_ucp_header(hdr1, UCP_OP_XATR_ACK, tok, seq) != 0) {
					cout << "ucp_send_chunk: recv0 NG (rc=" << rc << ")" << endl;
					switch (rc) {
					case UCP_ERR_VERSION_UNMATCH:
						cout << " UCP_ERR_VERSION_UNMATCH" << endl;
						break;
					case UCP_ERR_OP_UNMATCH:
						cout << " UCP_ERR_OP_UNMATCH" << endl;
						break;
					case UCP_ERR_TOKEN_UNMATCH:
						cout << " UCP_ERR_TOKEN_UNMATCH" << endl;
						break;
					case UCP_ERR_SEQ_UNMATCH:
						cout << " UCP_ERR_SEQ_UNMATCH" << endl;
						break;
					}
					return -1;
				}
				if (i->fnum != ntohl(hdr1.utility)) {
					cout << "File number unmatch(xattr)." << "(" << i->fnum << "|" << ntohl(hdr1.utility) << endl;
					return -1;
				}
				send_sz += rc;
#endif
			}
		}
	}
	if (chunk_mta->chunk_size != send_sz) {
		cout << "send_sz is unmatched." << endl;
		cout << " send_chunk: " << send_sz << ", chunk_sz: " << chunk_mta->chunk_size << endl;
		return -1;
	}
#ifndef TEST
	// Recv CHUNK_ACK or CHUNK_NCK (receipt of all)
	seq++;
	rc = ucp_recv(hnd, &hdr, NULL, 0);
	if (check_ucp_header(hdr, UCP_OP_CHUNK_ACK, tok, seq) != 0) {
		cout << "ucp_send_chunk: recv0 NG (rc=" << rc << ")" << endl;
		switch (rc) {
		case UCP_ERR_VERSION_UNMATCH:
			cout << " UCP_ERR_VERSION_UNMATCH" << endl;
			break;
		case UCP_ERR_OP_UNMATCH:
			cout << " UCP_ERR_OP_UNMATCH" << endl;
			break;
		case UCP_ERR_TOKEN_UNMATCH:
			cout << " UCP_ERR_TOKEN_UNMATCH" << endl;
			break;
			case UCP_ERR_SEQ_UNMATCH:
				cout << " UCP_ERR_SEQ_UNMATCH" << endl;
				break;
		}
		return -1;
	}
#endif
	memcpy(token, &tok, sizeof(int));
	memcpy(sequence, &seq, sizeof(int));

	return rc;

UCP_ABORT:
	return -1;
}

int64_t ucp_recv_chunk(UDTSOCKET hnd, struct chunk_meta* chunk_mta, unsigned int* token, unsigned int* sequence)
{
	int rc;
	unsigned int tok = *token;
	unsigned int seq = *sequence;

	struct ucp_header hdr;
	memset((void*)&hdr, 0, sizeof(struct ucp_header));

	uint64_t recv_sz = 0;
	for (list<struct file_info>::iterator i = chunk_mta->finfo.begin(); i != chunk_mta->finfo.end(); ++i) {
		cout << "Receiving... " << i->fname.c_str() << endl;
		struct ucp_header hdr0;
		memset((void*)&hdr0, 0, sizeof(struct ucp_header));
		if (S_ISDIR(i->mode)) {
			cout <<  i->fname << " is dir." << endl;
		} else {
			cout <<  i->fname << " is file." << endl;
			char* tmpfile;
			char tmp_sfx[] = ".XXXXXX";
			tmpfile = (char*)malloc(i->fname.size() + strlen(tmp_sfx) + 1);
			strcpy(tmpfile, i->fname.c_str());
			strcpy(tmpfile + i->fname.size(), tmp_sfx);
			mkstemp(tmpfile);
			fstream ofs(tmpfile, ios::out | ios::binary | ios::trunc);
			int64_t offset = 0;
			
			// receiving file chunk	
			if (UDT::ERROR == (rc = UDT::recvfile(hnd, ofs, offset, i->data_end - i->data_begin))){
				cout << "ucp_recv_chunk failed. " << UDT::getlasterror().getErrorMessage() << " recv: " << rc << endl;
				return -1;
			}
			ofs.close();
			cout << "recv_sz(file): " << rc << endl;
			recv_sz += rc;
			cout << "chunk: " << tmpfile << endl;
			// Send BLOB_ACK or BLOB_NCK
			seq++;
			gen_ucp_header(&hdr0, UCP_OP_BLOB_ACK, tok, seq, 0, i->fnum);
			rc = ucp_send(hnd, &hdr0, NULL);
			
			// receiving extended attributes
			if (i->data_end != i->xattr_end) {
				struct ucp_header hdr1;
				memset((void*)&hdr1, 0, sizeof(struct ucp_header));
				
				char* xattrs_buf;
				map<string, string> xattrs;
				msgpack::unpacked msg;
				msgpack::object obj;
				
				xattrs_buf = (char*)malloc(i->xattr_end - i->data_end);
				rc =  UDT::recv(hnd, xattrs_buf, i->xattr_end - i->data_end, -1);
				cout << "recv_sz(xattr): " << rc << endl;
				recv_sz += rc;
				msgpack::unpack(&msg, (const char*)xattrs_buf, i->xattr_end - i->data_end);
				obj = msg.get();
				cout << "xattrs: " << obj << endl;
				obj.convert(&xattrs);
				if (xattrs_buf != NULL)
					free(xattrs_buf);
				if (set_xattr(tmpfile, xattrs) != 0) {
					return -1;
				}
				// Sending XATR_ACK or XATR_NCK
				seq++;
				gen_ucp_header(&hdr1, UCP_OP_XATR_ACK, tok, seq, 0, i->fnum);
				rc = ucp_send(hnd, &hdr1, NULL);
			}
			if (set_attr_rename(tmpfile, &*i) != 0)
				return -1;
			if (tmpfile != NULL)
				free(tmpfile);
		}
	}
	if (chunk_mta->chunk_size != recv_sz) {
		cout << "recv_sz is unmatched." << endl;
		cout << " recv_chunk: " << recv_sz << ", chunk_sz: " << chunk_mta->chunk_size << endl;
		return -1;
	}

	// Send CHUNK_ACK or CHUNK_NCK (receipt of all)	
	seq++;
	gen_ucp_header(&hdr, UCP_OP_CHUNK_ACK, tok, seq, 0, 1);
	rc = ucp_send(hnd, &hdr, NULL);

	memcpy(token, &tok, sizeof(int));
	memcpy(sequence, &seq, sizeof(int));

	return rc;

UCP_ABORT:
	return -1;
}

int64_t ucp_send_file(UDTSOCKET hnd, string fname, uint64_t chunk_len, int token, int seq)
{
	int64_t rc;
	fstream ifs(fname.c_str(), ios::in | ios::binary);
	ifs.seekg(0, ios::end);
	int64_t sz = ifs.tellg();
	ifs.seekg(0, ios::beg);
	int64_t offset = 0;

	if (sz != chunk_len) {
		cout << "ucp_send_chunk chunk size error." << endl;
		ifs.close();
		return -1;
	}

	void *buffer;
	buffer = malloc(chunk_len);
	ifs.read((char*)buffer, chunk_len);

	// if (chunk_len >= UCP_CHUNK_MAX_SIZE)

	if (UDT::ERROR == (rc = UDT::sendfile(hnd, ifs, offset, chunk_len))) {
		cout << "ucp_send_chunk send_file failed." << UDT::getlasterror().getErrorMessage()  << endl;
		goto SEND_CLOSE;
	}
	if (rc != chunk_len) {
		goto SEND_CLOSE;
	}
SEND_CLOSE:
	ifs.close();
	return rc;
}

int64_t ucp_recv_file(UDTSOCKET hnd, string fname, uint64_t chunk_len, int token, int seq)
{
	int64_t rc;
	fstream ofs(fname.c_str(), ios::out | ios::binary | ios::trunc);
	int64_t offset = 0;

	void *buffer;
	buffer = malloc(chunk_len);

	// // if (chunk_len >= UCP_CHUNK_MAX_SIZE)
	
	if (UDT::ERROR == (rc = UDT::recvfile(hnd, ofs, offset, chunk_len))){
		cout << "ucp_recv_chunk failed. " << UDT::getlasterror().getErrorMessage() << " recv: " << rc << endl;
		goto RECV_CLOSE;
	}

	if (rc != chunk_len) {
		goto RECV_CLOSE;
	}

RECV_CLOSE:
	ofs.close();
	return rc;
}
