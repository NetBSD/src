/*	$NetBSD: getdtablesize.c,v 1.6 1998/02/27 17:43:20 perry Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: getdtablesize.c,v 1.6 1998/02/27 17:43:20 perry Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <unistd.h>

int
getdtablesize()
{
	return ((int)sysconf(_SC_OPEN_MAX));
}
