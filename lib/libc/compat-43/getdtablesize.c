/*	$NetBSD: getdtablesize.c,v 1.4 1997/07/13 18:50:08 christos Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: getdtablesize.c,v 1.4 1997/07/13 18:50:08 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <unistd.h>

int
getdtablesize()
{
	return sysconf(_SC_OPEN_MAX);
}
