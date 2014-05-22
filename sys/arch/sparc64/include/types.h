/*       $NetBSD: types.h,v 1.25.112.1 2014/05/22 11:40:09 yamt Exp $        */

#include <sparc/types.h>

#ifdef __arch64__
#define	MD_TOPDOWN_INIT(epp)	/* no topdown VM flag for exec by default */
#endif
