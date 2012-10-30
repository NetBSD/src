/*	$NetBSD: getdtablesize.c,v 1.9.56.1 2012/10/30 18:58:43 yamt Exp $	*/

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: getdtablesize.c,v 1.9.56.1 2012/10/30 18:58:43 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <unistd.h>

int
getdtablesize(void)
{
	return ((int)sysconf(_SC_OPEN_MAX));
}
