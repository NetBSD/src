/* $NetBSD: ultrix_flock.h,v 1.1 1999/07/30 16:03:49 drochner Exp $ */

struct ultrix_flock {
	short l_type;
#define ULTRIX_F_RDLCK 1
#define ULTRIX_F_WRLCK 2
#define ULTRIX_F_UNLCK 3
	short l_whence;
	long l_start;
	long l_len;
	int l_pid;
};
