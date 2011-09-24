/* This file is used to test the 'catch syscall' feature on GDB.
 
   Please, if you are going to edit this file DO NOT change the syscalls
   being called (nor the order of them).  If you really must do this, then
   take a look at catch-syscall.exp and modify there too.

   Written by Sergio Durigan Junior <sergiodj@linux.vnet.ibm.com>
   September, 2008 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int
main (void)
{
	/* A close() with a wrong argument.  We are only
	   interested in the syscall.  */
	close (-1);

	chroot (".");

	/* The last syscall.  Do not change this.  */
	_exit (0);
}
