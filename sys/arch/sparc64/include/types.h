/*       $NetBSD: types.h,v 1.25.122.1 2014/08/20 00:03:25 tls Exp $        */

#include <sparc/types.h>

#ifdef __arch64__
#define	MD_TOPDOWN_INIT(epp)	/* no topdown VM flag for exec by default */
#endif
