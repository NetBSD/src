/*	$NetBSD: pread.c,v 1.1.1.2.4.2 2014/05/22 15:45:00 yamt Exp $	*/

#include "file.h"
#ifndef lint
#if 0
FILE_RCSID("@(#)$File: pread.c,v 1.2 2013/04/02 16:23:07 christos Exp $")
#else
__RCSID("$NetBSD: pread.c,v 1.1.1.2.4.2 2014/05/22 15:45:00 yamt Exp $");
#endif
#endif  /* lint */
#include <fcntl.h>
#include <unistd.h>

ssize_t
pread(int fd, void *buf, size_t len, off_t off) {
	if (lseek(fd, off, SEEK_SET) == (off_t)-1)
		return -1;

	return read(fd, buf, len);
}
