/*
 * invalid calls to open()
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include "config.h"
#include "test.h"
#include "../lib/testutils.h"
static
void
open_badflags(void)
{
	int fd1, fd2;
	// creates FILE1
	fd1 = open("FILE1", O_CREAT | O_RDONLY);
	// FILE1 exists, so flags O_CREAT & O_EXCL should
	// cause an  EEXIST errno (9)
	fd2 = open("FILE1", O_CREAT | O_EXCL);
	TEST_NOT_EQUAL(fd1, fd2, "Test different fds failed: same fds returned\n");
//	report_test(fd, errno, EINVAL, "open null: with bad flags");
}

static
void
open_empty(void)
{
	int rv;
	rv = open("", O_RDONLY);
//	report_test2(rv, errno, 0, EINVAL, "open empty string");
	if (rv>=0) {
		close(rv);
	}
}

void
test_open(void)
{
//	test_open_path();

	open_badflags();
	open_empty();
}

void
test_reopen_different_fds(void){
  /* Check that we can create the file */
  int rc, f1, f2, f3;

  rc = open("FILE1", O_RDWR | O_CREAT | O_TRUNC);
//  TEST_POSITIVE(rc, "Unable to create FILE1 (assumes that it doesn't exist)");

  /* Check that we can create the file */
  rc = open("FILE2", O_RDWR | O_CREAT | O_TRUNC);
  //TEST_POSITIVE(rc, "Unable to create FILE2 (assumes that it doesn't exist)");

  /* check that we can open the same file many times */
  f1 = open("FILE1", O_RDWR);
  //TEST_POSITIVE(f1, "Unable to open FILE1 first time");

  f2 = open("FILE1", O_RDWR);
  //TEST_POSITIVE(f2, "Unable to open FILE1 second time");

  f3 = open("FILE1", O_RDWR);
  //TEST_POSITIVE(f3, "Unable to open FILE1 third time");
  if (f3 != f2 && f2 != f3){
	
  }else {
	while (f1 > 0 )f1++;	
}
  /* Check that f1 != f2 != f3 */
//  TEST_NOT_EQUAL(f1, f2, "Using same fd for multiple opens f1 = f2");
  //TEST_NOT_EQUAL(f2, f3, "Using same fd for multiple opens f2 = f3");


}


int main(int argc, char **argv) {

	test_open();
	test_reopen_different_fds();
  

}
