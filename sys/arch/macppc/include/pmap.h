/*	$NetBSD: pmap.h,v 1.8 2006/08/05 21:26:48 sanjayl Exp $	*/
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
