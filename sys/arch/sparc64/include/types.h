/*       $NetBSD: types.h,v 1.25.122.2 2017/12/03 11:36:45 jdolecek Exp $        */

#ifndef _SPARC64_TYPES_H_
#define	_SPARC64_TYPES_H_

#include <sparc/types.h>

#ifdef __arch64__
#define	MD_TOPDOWN_INIT(epp)	/* no topdown VM flag for exec by default */
#endif

#define	__HAVE_COMPAT_NETBSD32

#endif
