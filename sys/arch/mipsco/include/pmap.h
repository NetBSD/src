/*	$NetBSD: pmap.h,v 1.1 2000/08/12 22:58:32 wdk Exp $	*/

#include <mips/pmap.h>

#define pmax_trunc_seg(a) mips_trunc_seg(a)
#define pmax_round_seg(a) mips_round_seg(a)
