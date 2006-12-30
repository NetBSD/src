/*	$NetBSD: pmap.h,v 1.7.18.1 2006/12/30 20:46:29 yamt Exp $	*/
#ifndef _MACPPC_PMAP_H_
#define _MACPPC_PMAP_H_

#include <powerpc/pmap.h>

/* Mac specific routines */
#if defined (PPC_OEA64_BRIDGE)
#ifndef _LOCORE
int pmap_setup_segment0_map(int use_large_pages, ...);
#endif
#endif

#endif /* _MACPPC_PMAP_H_ */
