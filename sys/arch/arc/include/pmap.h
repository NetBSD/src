/*	$NetBSD: pmap.h,v 1.5.6.2 2000/11/20 20:00:37 bouyer Exp $	*/
/*      $OpenBSD: pmap.h,v 1.3 1997/04/19 17:19:58 pefo Exp $ */

#include <mips/pmap.h>

#define pmap_resident_count(pmap)       ((pmap)->pm_stats.resident_count)
#define PMAP_PREFER(pa, va)             pmap_prefer((pa), (va))
