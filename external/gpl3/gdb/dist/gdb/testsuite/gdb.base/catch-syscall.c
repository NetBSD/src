/* This file is used to test the 'catch syscall' feature on GDB.
 
   Please, if you are going to edit this file DO NOT change the syscalls
   being called (nor the order of them).  If you really must do this, then
   take a look at catch-syscall.exp and modify there too.

   Written by Sergio Durigan Junior <sergiodj@linux.vnet.ibm.com>
   September, 2008 */

#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/stat.h>

/* These are the syscalls numbers used by the test.  */

int close_syscall = SYS_close;
int chroot_syscall = SYS_chroot;
/* GDB had a bug where it couldn't catch syscall number 0 (PR 16297).
   In most GNU/Linux architectures, syscall number 0 is
   restart_syscall, which can't be called from userspace.  However,
   the "read" syscall is zero on x86_64.  */
int read_syscall = SYS_read;
int pipe_syscall = SYS_pipe;
int write_syscall = SYS_write;
int exit_group_syscall = SYS_exit_group;

int
main (void)
{
	int fd[2];
	char buf1[2] = "a";
	char buf2[2];

	/* A close() with a wrong argument.  We are only
	   interested in the syscall.  */
	close (-1);

	chroot (".");

	pipe (fd);

	write (fd[1], buf1, sizeof (buf1));
	read (fd[0], buf2, sizeof (buf2));

	/* The last syscall.  Do not change this.  */
	_exit (0);
}
