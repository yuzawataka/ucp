#ifndef UCP_H_
#define UCP_H_

#include <msgpack.hpp>

using namespace std;

#define UDT_PORT 9000

#define UCP_METADATA_SIZE 1024
#define UCP_CHUNK_MAX_SIZE 1024

#define UCP_VERSION 0x00

#define UCP_OP_AUTH 0x01
#define UCP_OP_AUTH_ACK 0x02
#define UCP_OP_AUTH_NCK 0x03
#define UCP_OP_PREMETA 0x04
#define UCP_OP_META 0x05
#define UCP_OP_META_ACK 0x06
#define UCP_OP_META_NCK 0x07
#define UCP_OP_CHUNK 0x08
#define UCP_OP_CHUNK_ACK 0x09
#define UCP_OP_CHUNK_NCK 0x0a
#define UCP_OP_BLOB 0x0b
#define UCP_OP_BLOB_ACK 0x0c
#define UCP_OP_BLOB_NCK 0x0d
#define UCP_OP_XATR 0x0e
#define UCP_OP_XATR_ACK 0x0f
#define UCP_OP_XATR_NCK 0x10
#define UCP_OP_DIR 0x11
#define UCP_OP_DIR_ACK 0x12
#define UCP_OP_DIR_NCK 0x13
#define UCP_OP_ACK 0x14
#define UCP_OP_NCK 0x15
#define UCP_OP_RETRY 0x16
#define UCP_OP_ABORT 0x17

enum {
	UCP_ERR_VERSION_UNMATCH,
	UCP_ERR_OP_UNMATCH,
	UCP_ERR_TOKEN_UNMATCH,
	UCP_ERR_SEQ_UNMATCH,
};

enum {
	UCP_FLIST_FILE,
	UCP_FLIST_DIR
};

#define UCP_META_TYPE_UPLOAD 0x00
#define UCP_META_TYPE_DOWNLOAD 0x01

struct ucp_header {
    unsigned char version;
    unsigned char op;
    uint32_t token;
    uint32_t seq;
    uint64_t length;
	int32_t utility;
	unsigned char encrypt;
	unsigned char _unused;
} __attribute__ ((packed));

// MessagePack
struct file_info {
    string fname;
    uint32_t fnum;
    uid_t uid;
    gid_t gid;
    mode_t mode;
    time_t mtime;
    uint64_t data_begin;
    uint64_t data_end;
    uint64_t xattr_end;
    MSGPACK_DEFINE(fname, fnum, uid, gid, mode, mtime, data_begin, data_end, xattr_end);
};

struct chunk_meta {
	unsigned char type;
    uint32_t chunk_seq;
    uint32_t file_count;
	string user;
	string dest_path;
    uint64_t chunk_size;
    list<struct file_info> finfo;
    MSGPACK_DEFINE(chunk_seq, file_count, type, user, dest_path, chunk_size, finfo);
};

struct remote_path {
	string name;
	string host;
	string path;
};

int ucp_send_dummy(UDTSOCKET hnd, const char *buf, size_t len);

int ucp_send(UDTSOCKET hnd, struct ucp_header *header, const char *payload);
int ucp_recv(UDTSOCKET hnd, struct ucp_header *header, char *payload, size_t payload_len);

bool check_one_colon(string& str);
remote_path check_remote_path(string& str);
int64_t gen_each_meta(string fname, uint32_t fnum, uint64_t chunk_offset,
					  file_info* fi, msgpack::sbuffer* sbuf);
int gen_chunk(vector<string> fnames, string tmpfname);
int gen_whole_chunk(vector<string> fnames, chunk_meta *chunk_mta, string tmpfname);
int get_xattr(string fname, msgpack::sbuffer* sbuf);
int set_xattr(char *fname, map<string, string>& xattrs);
int set_attr_rename(char *fname, struct file_info* finfo);
int gen_chunk_meta(vector<string>& flist, remote_path& rp, chunk_meta& chunk_mta);
int update_chunk_meta(chunk_meta& chunk_mta, string& dest_path, int overwrite);
void gen_ucp_header(struct ucp_header* hdr, unsigned char op, uint32_t token, uint32_t seq, 
					uint64_t length, int32_t utility);
int check_ucp_header(struct ucp_header hdr, unsigned char op, uint32_t token, uint32_t seq);
int ucp_send_metadata(UDTSOCKET hnd, struct chunk_meta* chunk_mta, unsigned int* token, unsigned int* sequence);
int ucp_recv_metadata(UDTSOCKET hnd, struct chunk_meta* chunk_mta, unsigned int* token, unsigned int* sequence);

int64_t ucp_send_file(UDTSOCKET hnd, string fname, uint64_t chunk_len, int32_t token, int32_t sequence);
int64_t ucp_recv_file(UDTSOCKET hnd, string fname, uint64_t chunk_len, int32_t token, int32_t sequence);
int64_t ucp_send_chunk(UDTSOCKET hnd, struct chunk_meta* chunk_mta, unsigned int* token, unsigned int* sequence);
int64_t ucp_recv_chunk(UDTSOCKET hnd, struct chunk_meta* chunk_mta, unsigned int* token, unsigned int* sequence);

void udt_status(UDTSOCKET s);
void udt_trace(UDT::TRACEINFO& trace);
void show_ucp_header(struct ucp_header hdr);

int get_file_list(string& fname, vector<string>* flist);
#endif
