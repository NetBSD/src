/*	$NetBSD: pmap.h,v 1.4 2000/01/23 21:01:58 soda Exp $	*/
/*      $OpenBSD: pmap.h,v 1.3 1997/04/19 17:19:58 pefo Exp $ */

#include <mips/include/pmap.h>

#define pmap_resident_count(pmap)       ((pmap)->pm_stats.resident_count)
#define PMAP_PREFER(pa, va)             pmap_prefer((pa), (va))
