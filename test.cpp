#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
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
#include "minunit.h"
#include <msgpack.hpp>

using namespace std;

static char* test_check_one_colon()
{
	string test0("test:test");
	string test1("test:test:test");
	mu_assert("error, check_one_colon(\"test:test\")", check_one_colon(test0) == true);
	mu_assert("error, check_one_colon(\"test:test:test\")", check_one_colon(test1) == false);
	return 0;
}

static char* test_check_remote_path()
{
	string test0("hoge@192.168.2.1:test/test.txt");
	string test1("192.168.2.1:test/test.txt");

	remote_path rp0, rp1;
	rp0 = check_remote_path(test0);
	rp1 = check_remote_path(test1);
	mu_assert("error, check_remote_path(\"hoge@192.168.2.1:test/test.txt\")", 
			  rp0.name == "hoge" && rp0.host == "192.168.2.1" && rp0.path == "test/test.txt");
	mu_assert("error, check_remote_path(\"192.168.2.1:test/test.txt\")", 
			  rp1.name.empty() && rp1.host == "192.168.2.1" && rp1.path == "test/test.txt");
	return 0;
}

static char* test_get_xattr()
{
	string test0("ucp.cpp");
	int rc;
	msgpack::sbuffer sbuf;
	rc = get_xattr(test0, &sbuf);
	cout << "xattr(" << rc << ","<< sbuf.size() << "): " << sbuf.data() << endl;

	msgpack::unpacked msg;
	msgpack::unpack(&msg, sbuf.data(), sbuf.size());
 	msgpack::object obj = msg.get();
	cout << obj << endl;
	map<string, string> xattrs;
	obj.convert(&xattrs);

	return 0;
}


// static char* test_set_xattr()
// {
// 	string test0("ucp.cpp");
// 	int rc;
// 	msgpack::sbuffer sbuf;
// 	rc = get_xattr(test0, &sbuf);
// 	msgpack::unpacked msg;
// 	msgpack::unpack(&msg, sbuf.data(), sbuf.size());
//  	msgpack::object obj = msg.get();
// 	cout << obj << endl;
// 	map<string, string> xattrs;
// 	obj.convert(&xattrs);

// 	string test1("/tmp/ucp-test.cpp");
// 	system("/bin/touch /tmp/ucp-test.cpp");
// 	rc = set_xattr((char*)test1.c_str(), &xattrs);
// 	rc = get_xattr(test1, &sbuf);
// 	msgpack::unpack(&msg, sbuf.data(), sbuf.size());
//  	obj = msg.get();
// 	cout << obj << endl;
// 	return 0;
// }


static char* test_gen_each_meta()
{
	string test0("ucp.cpp");
	file_info fi;
	msgpack::sbuffer sbuf;
	if (gen_each_meta(test0, 0, 0, &fi, &sbuf) == 0) {
		cout << "OK" << endl;
	} else {
		cout << "NG" << endl;
	}
	return 0;
}

static char* test_gen_chunk()
{
	vector<string> file_ls;
	// char tmpfname[] = "/tmp/ucp_XXXXXX";
	// int fd = mkstemp(tmpfname);
	string tmpfname("/tmp/test_gen_chunk_orig");
	file_ls.push_back("test.cpp");
	file_ls.push_back("ucp.cpp");
	file_ls.push_back("ucpd.cpp");
	// mu_assert("error, gen_chunk()",
	// 		  gen_chunk(file_ls, tmpfname) >= 0);
//	close(fd);
	system("/bin/cat test.cpp ucp.cpp ucpd.cpp > /tmp/test_gen_chunk_tmp");
	// mu_assert("error, gen_chunk() diff_check",
	// 		  system("/usr/bin/diff -u /tmp/test_gen_chunk_tmp /tmp/test_gen_chunk_orig") == 0);
	unlink("/tmp/test_gen_chunk_tmp");
	unlink(tmpfname.c_str());
	return 0;
}

static char* test_gen_chunk_meta()
{
	string test0("root@192.168.2.1:hoge");
	remote_path rp;
	struct chunk_meta chunk_mta;
	vector<string> flist;
	msgpack::sbuffer sbuf;
	msgpack::unpacked msg;

	rp = check_remote_path(test0);
	flist.push_back("test.cpp");
	flist.push_back("ucp.cpp");
	flist.push_back("ucpd.cpp");
	gen_chunk_meta(flist, rp, chunk_mta);
	msgpack::pack(sbuf, chunk_mta);
	msgpack::unpack(&msg, sbuf.data(), sbuf.size());
 	msgpack::object obj = msg.get();
	cout << obj << endl;

	return 0;
}

static char* test_update_chunk_meta()
{
	string test0("root@192.168.2.1:hoge");
	remote_path rp;
	struct chunk_meta chunk_mta;
	vector<string> flist;
	msgpack::sbuffer sbuf;
	msgpack::unpacked msg;

	rp = check_remote_path(test0);
	flist.push_back("test.cpp");
	flist.push_back("ucp.cpp");
	flist.push_back("ucpd.cpp");
	gen_chunk_meta(flist, rp, chunk_mta);
	string dest_path("/root/hoge");
	update_chunk_meta(chunk_mta, dest_path, 0);
	msgpack::pack(sbuf, chunk_mta);
	msgpack::unpack(&msg, sbuf.data(), sbuf.size());
 	msgpack::object obj = msg.get();
	cout << obj << endl;

	return 0;
}

// static char* test_read_chunk_meta()
// {
// 	struct chunk_meta_test chunk_mta;
// 	ifstream file("/tmp/save.dat");
// 	boost::archive::text_iarchive ia(file);
// }

// static char* test_gen_whole_chunk()
// {
// 	chunk_meta *chunk_mta;
// 	vector<string> file_ls;
// 	string tmpfname("/tmp/test_gen_whole_chunk_orig");
// 	file_ls.push_back("test.cpp");
// 	file_ls.push_back("ucp.cpp");
// 	file_ls.push_back("ucpd.cpp");
// 	cout << "chunk_meta: " << sizeof(chunk_meta) << endl; 
// 	cout << "file_info: " << sizeof(file_info) * file_ls.size() << endl;
// 	chunk_mta = (chunk_meta*)malloc(sizeof(chunk_meta) + 
// 									  sizeof(file_info) * file_ls.size());
// 	gen_whole_chunk(file_ls, chunk_mta, tmpfname);
// 	if (chunk_mta != NULL)
// 		free(chunk_mta);
// 	return 0;
// }

// static char* test_send_chunk()
// {
// 	vector<string> fnames;
// 	fnames.push_back("test.cpp");
// 	fnames.push_back("ucp.cpp");
// 	fnames.push_back("ucpd.cpp");

// 	struct chunk_meta chunk_mta;
// 	gen_chunk_meta(fnames, chunk_mta);
// 	int hnd = 0;
// 	unsigned int token = 1000;
// 	unsigned int seq = 0;

// 	ucp_send_chunk(hnd, &chunk_mta, &token, &seq);

// 	return 0;
// }	


static char* test_get_file_list()
{
	vector<string> flist;
	string str("/home/yuzawataka/bin");
	get_file_list(str, &flist);
	for (vector<string>::iterator i = flist.begin(); i != flist.end(); i++) {
		cout << *i << endl;
	}
	// gen_file_list("/home/yuzawataka/test.py", &flist);
	// gen_file_list("/home/yuzawataka/nofile", &flist);
	// gen_file_list("../ucp/", &flist);
	return 0;
}

int tests_run = 0;

static char * all_tests() {
	// mu_run_test(test_check_one_colon);
	// mu_run_test(test_check_remote_path);
	// mu_run_test(test_get_xattr);
	// mu_run_test(test_set_xattr);
	// mu_run_test(test_gen_each_meta);
	// mu_run_test(test_gen_chunk);
	// mu_run_test(test_get_xattr);
	mu_run_test(test_gen_chunk_meta);
	mu_run_test(test_update_chunk_meta);
	// mu_run_test(test_gen_chunk_meta_hoge);
	// mu_run_test(test_gen_whole_chunk);
	// mu_run_test(test_get_file_list);
	return 0;
}

int main(int argc, char **argv) {
	char *result = all_tests();
	if (result != 0) {
		printf("%s\n", result);
	}
	else {
		printf("ALL TESTS PASSED\n");
	}
	printf("Tests run: %d\n", tests_run);
 
	return result != 0;
}
