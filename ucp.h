#ifndef UCP_H_
#define UCP_H_

using namespace std;

#define UDT_PORT "9001"

#define UCP_METADATA_SIZE 1024
#define UCP_CHUNK_MAX_SIZE 1024

#define UCP_VERSION 0x00
#define UCP_OP_AUTH 0x01

#define UCP_OP_ACK 0x03
#define UCP_OP_NCK 0x04
#define UCP_OP_TOKEN 0x05
#define UCP_OP_RM2LO 0x06
#define UCP_OP_LO2RM 0x07

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef struct ucp_header {
    u8 version;
    u8 op;
    u32 token;
    u32 seq;
    u64 length;
	u32 flags;
} ucp_header;

typedef struct {
	string id;
	string host;
	string path;
} remote_path;

typedef struct {
	string fname;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	time_t mtime;
	unsigned long long int data_begin;
	unsigned long long int data_end;
	unsigned long long int xattr_begin;
	unsigned long long int xattr_end;
} file_mdata;

int ucp_send_dummy(UDTSOCKET hnd, const char* buf, size_t len);

int ucp_send(UDTSOCKET hnd, ucp_header header, const char* payload);
int ucp_recv(UDTSOCKET hnd, ucp_header header, char* payload, size_t payload_len);

bool check_one_colon(string str);
remote_path check_remote_path(string str);
int gen_each_meta(string fname, unsigned long long int chunk_offset, file_mdata* fm);
int gen_chunk(vector<string> fnames, string tmpfname);
int get_xattr(string fname, map<string, string>* attr);

void udt_status(UDTSOCKET s);


#endif
