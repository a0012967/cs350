/*
 * Title   : files1
 * Author  : Tim Brecht
 * Date    : Sat Oct  2 08:19:57 EST 1999
 *
 * Some example tests using open, and close.
 * Assumes that files named FILE1 and FILE2 do not exist in current directory
 *
 * Modified: Thu Dec 23 17:05:19 EST 2004
 *   TBB - updated for Winter 2005 term.
 *   TBB - cleaned up and moved into uw-testbin for Winter 2013
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "../lib/testutils.h"

int
main()
{
  int f1, f2;
  int i = 42;
  int j = -999;
  int intbuf = 0;
  int rc = 0;      /* return code */
  int save_errno = 0;

  /*
  rc = fork();
  if (rc == 0) {
      printf("Parent: 0\n");
  }
  else {
      printf("Child: %d\n", rc);
  }
  */

  f1 = open("FILE1", O_RDWR | O_CREAT | O_TRUNC);
  TEST_POSITIVE(f1, "Unable to open FILE1");

  f2 = open("FILE2", O_RDWR | O_CREAT | O_TRUNC);
  TEST_POSITIVE(f2, "Unable to open FILE2");

  TEST_NOT_EQUAL(f1, f2, "fd f1 == f2");

  rc = write(f1, (char *) &i, sizeof(i));
  TEST_EQUAL(rc, sizeof(i), "write to f1 does not write/return proper value");

  rc = write(f2, (char *) &j, sizeof(j));
  TEST_EQUAL(rc, sizeof(j), "write to f2 does not write/return proper value");

  rc = close(f1);
  TEST_EQUAL(rc, SUCCESS, "close f1 failed");

  rc = close(f1);
	save_errno = errno;
  TEST_NEGATIVE(rc, "close f1 second time didn't fail");

  rc = close(f2);
  TEST_EQUAL(rc, SUCCESS, "close f2 failed");

  f1 = open("FILE1", O_RDONLY);
  TEST_POSITIVE(f1, "Unable to open FILE1, after Close");

  f2 = open("FILE2", O_RDONLY);
  TEST_POSITIVE(f1, "Unable to open FILE2, after Close");

  TEST_NOT_EQUAL(f1, f2, "fd f1 == f2");

  rc = read(f1, (char *) &intbuf, sizeof(intbuf));
  TEST_EQUAL(rc, sizeof(intbuf), 
     "read from f1 does not read/return proper value");
  TEST_EQUAL(intbuf, i,
     "read from f1 did not get correct value");

  rc = read(f2, (char *) &intbuf, sizeof(intbuf));
  TEST_EQUAL(rc, sizeof(j), "read from f2 does not read/return proper value");
  TEST_EQUAL(intbuf, j, "read from f2 did not get correct value");

  TEST_STATS();

  exit(0);
}
