/*       $NetBSD: types.h,v 1.28 2019/04/06 03:06:27 thorpej Exp $        */

#ifndef _SPARC64_TYPES_H_
#define	_SPARC64_TYPES_H_

#include <sparc/types.h>

#ifdef __arch64__
#define	MD_TOPDOWN_INIT(epp)	/* no topdown VM flag for exec by default */
#endif

#define	__HAVE_COMPAT_NETBSD32
#define	__HAVE_UCAS_FULL

#endif
