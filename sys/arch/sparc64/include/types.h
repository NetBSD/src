/*       $NetBSD: types.h,v 1.27 2017/01/26 15:55:10 christos Exp $        */

#ifndef _SPARC64_TYPES_H_
#define	_SPARC64_TYPES_H_

#include <sparc/types.h>

#ifdef __arch64__
#define	MD_TOPDOWN_INIT(epp)	/* no topdown VM flag for exec by default */
#endif

#define	__HAVE_COMPAT_NETBSD32

#endif
