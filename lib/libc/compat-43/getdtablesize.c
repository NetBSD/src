/*	$NetBSD: getdtablesize.c,v 1.8 1998/10/18 13:56:21 kleink Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: getdtablesize.c,v 1.8 1998/10/18 13:56:21 kleink Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <unistd.h>

int
getdtablesize()
{
	return ((int)sysconf(_SC_OPEN_MAX));
}
