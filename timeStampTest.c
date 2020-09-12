/* TinyFS demo file
 *  * Foaad Khosmood, Cal Poly / modified Winter 2014
 *   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tinyFS.h"
#include "libTinyFS.h"
#include "TinyFS_errno.h"

/* This program will create 2 files (of sizes 200 and 1000) to be read from or stored in the TinyFS file system. */
int main ()
{
  fileDescriptor aFD;

/* try to mount the disk */
  if (tfs_mount (DEFAULT_DISK_NAME) < 0)	/* if mount fails */
    {
      tfs_mkfs (DEFAULT_DISK_NAME, DEFAULT_DISK_SIZE);	/* then make a new disk */
      if (tfs_mount (DEFAULT_DISK_NAME) < 0)	/* if we still can't open it... */
	{
	  perror ("failed to open disk");	/* then just exit */
	  return;
	}
    }

/* now afile tests */
  aFD = tfs_openFile ("bfile");

/* * * * * * * * HERE * * * * * * * * * * */
  return_creation_time(aFD);
  /* * * * * * * * HERE * * * * * * * * * * */

  printf ("\nend of demo\n\n");
  return 0;
}
