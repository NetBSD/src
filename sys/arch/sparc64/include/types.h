/*       $NetBSD: types.h,v 1.27.14.1 2019/06/10 22:06:48 christos Exp $        */

#ifndef _SPARC64_TYPES_H_
#define	_SPARC64_TYPES_H_

#include <sparc/types.h>

#ifdef __arch64__
#define	MD_TOPDOWN_INIT(epp)	/* no topdown VM flag for exec by default */
#endif

#define	__HAVE_COMPAT_NETBSD32
#define	__HAVE_UCAS_FULL

#endif
