/*	$NetBSD: getdtablesize.c,v 1.7 1998/10/08 13:49:16 kleink Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: getdtablesize.c,v 1.7 1998/10/08 13:49:16 kleink Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(getdtablesize,_getdtablesize);
#endif

int
getdtablesize()
{
	return ((int)sysconf(_SC_OPEN_MAX));
}
