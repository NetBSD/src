/*	$NetBSD: _sys_siglist.c,v 1.7 1998/01/09 03:15:26 perry Exp $	*/

/*
 * Written by J.T. Conklin, December 12, 1994
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(_sys_siglist, sys_siglist)
__indr_reference(_sys_siglist, __sys_siglist) /* Backwards compat with v.12 */
#else

#undef _sys_siglist
#undef rcsid
#define	_sys_siglist	sys_siglist
#define	rcsid		_rcsid
#include "siglist.c"

#undef _sys_siglist
#undef rcsid
#define	_sys_siglist	__sys_siglist
#define	rcsid		__rcsid
#include "siglist.c"

#endif
