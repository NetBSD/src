/*	$NetBSD: getdtablesize.c,v 1.5 1997/07/21 14:06:26 jtc Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: getdtablesize.c,v 1.5 1997/07/21 14:06:26 jtc Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <unistd.h>

int
getdtablesize()
{
	return sysconf(_SC_OPEN_MAX);
}
