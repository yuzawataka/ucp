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
#include "minunit.h"

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
			  rp0.id == "hoge" && rp0.host == "192.168.2.1" && rp0.path == "test/test.txt");
	mu_assert("error, check_remote_path(\"192.168.2.1:test/test.txt\")", 
			  rp1.id.empty() && rp1.host == "192.168.2.1" && rp1.path == "test/test.txt");
	return 0;
}

static char* test_get_xattr()
{
	string test0("test.cpp");
	map<string, string> attrs;
	int rc;
	if ((rc = get_xattr(test0, &attrs)) >= 0) {
		cout << "rc:" << rc  << endl;
		map<string, string>::iterator i = attrs.begin();
		while(i != attrs.end()) {
			cout << (*i).first << ": " << (*i).second << endl;
			++i;
		}
		cout << "OK" << endl;
	} else {
		cout << "NG" << endl;
	}
	return 0;
}


static char* test_gen_each_meta()
{
	string test0("test.cpp");
	file_mdata fm;
	if (gen_each_meta(test0, 0, &fm) == 0) {
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
	mu_assert("error, gen_chunk()",
			  gen_chunk(file_ls, tmpfname) >= 0);
//	close(fd);
	system("/bin/cat test.cpp ucp.cpp ucpd.cpp > /tmp/test_gen_chunk_tmp");
	mu_assert("error, gen_chunk() diff_check",
			  system("/usr/bin/diff -u /tmp/test_gen_chunk_tmp /tmp/test_gen_chunk_orig") == 0);
	unlink("/tmp/test_gen_chunk_tmp");
	unlink(tmpfname.c_str());
	return 0;
}

int tests_run = 0;

static char * all_tests() {
	mu_run_test(test_check_one_colon);
	mu_run_test(test_check_remote_path);
	mu_run_test(test_get_xattr);
	mu_run_test(test_gen_each_meta);
	mu_run_test(test_gen_chunk);
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
