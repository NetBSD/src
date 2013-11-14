/*       $NetBSD: types.h,v 1.26 2013/11/14 12:11:13 martin Exp $        */

#include <sparc/types.h>

#ifdef __arch64__
#define	MD_TOPDOWN_INIT(epp)	/* no topdown VM flag for exec by default */
#endif
