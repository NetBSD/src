/*	$NetBSD: pmap.h,v 1.13 1997/06/16 07:47:42 jonathan Exp $	*/

#include <mips/pmap.h>

#define pmax_trunc_seg(a) mips_trunc_seg(a)
#define pmax_round_seg(a) mips_round_seg(a)
