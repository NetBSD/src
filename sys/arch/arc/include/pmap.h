/*	$NetBSD: pmap.h,v 1.3 2000/01/23 20:08:31 soda Exp $	*/

#include <mips/pmap.h>

#define pica_trunc_seg(a) mips_trunc_seg(a)
#define pica_round_seg(a) mips_round_seg(a)
