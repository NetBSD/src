/*	$NetBSD: pmap.h,v 1.1 1998/02/18 13:48:24 tsubai Exp $	*/

#include <mips/pmap.h>

#define pmax_trunc_seg(a) mips_trunc_seg(a)
#define pmax_round_seg(a) mips_round_seg(a)
