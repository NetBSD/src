/*	$NetBSD: ftruncate.c,v 1.1.1.1.14.1 2012/10/30 18:55:17 yamt Exp $	*/

#ifndef LINT
static const char rcsid[] = "Id: ftruncate.c,v 1.3 2005/04/27 18:16:45 sra Exp ";
#endif

/*! \file
 * \brief
 * ftruncate - set file size, BSD Style
 *
 * shortens or enlarges the file as neeeded
 * uses some undocumented locking call. It is known to work on SCO unix,
 * other vendors should try.
 * The #error directive prevents unsupported OSes
 */

#include "port_before.h"

#if defined(M_UNIX)
#define OWN_FTRUNCATE
#include <stdio.h>
#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#ifdef _POSIX_SOURCE
#undef _POSIX_SOURCE
#endif

#include <fcntl.h>

#include "port_after.h"

int
__ftruncate(int fd, long wantsize) {
	long cursize;

	/* determine current file size */
	if ((cursize = lseek(fd, 0L, 2)) == -1)
		return (-1);

	/* maybe lengthen... */
	if (cursize < wantsize) {
		if (lseek(fd, wantsize - 1, 0) == -1 ||
		    write(fd, "", 1) == -1) {
			return (-1);
		}
		return (0);
	}

	/* maybe shorten... */
	if (wantsize < cursize) {
		struct flock fl;

		fl.l_whence = 0;
		fl.l_len = 0;
		fl.l_start = wantsize;
		fl.l_type = F_WRLCK;
		return (fcntl(fd, F_FREESP, &fl));
	}
	return (0);
}
#endif

#ifndef OWN_FTRUNCATE
int __bindcompat_ftruncate;
#endif
