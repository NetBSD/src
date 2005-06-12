/*	$NetBSD: _Exit.c,v 1.2 2005/06/12 05:21:27 lukem Exp $	*/

/*
 * Ben Harris, 2002
 * This file is in the Public Domain
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _Exit.c,v 1.2 2005/06/12 05:21:27 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#include <stdlib.h>
#include <unistd.h>

/*
 * IEEE 1003.1-2001 says:
 * The _Exit() and _exit() functions shall be functionally equivalent.
 */

void
_Exit(int status)
{

	_exit(status);
}
