/*       $NetBSD: types.h,v 1.25.128.1 2014/05/18 17:45:26 rmind Exp $        */

#include <sparc/types.h>

#ifdef __arch64__
#define	MD_TOPDOWN_INIT(epp)	/* no topdown VM flag for exec by default */
#endif
