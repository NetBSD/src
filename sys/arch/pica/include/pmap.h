/*	$NetBSD: pmap.h,v 1.2 1996/03/23 03:42:52 jonathan Exp $	*/

#include <mips/pmap.h>

#define pica_trunc_seg(a) mips_trunc_seg(a)
#define pica_round_seg(a) mips_round_seg(a)
