/*	$NetBSD: pmap.h,v 1.14 2000/01/09 15:34:43 ad Exp $	*/

#ifndef _PMAX_PMAP_H_
#define _PMAX_PMAP_H_

#include <mips/pmap.h>

#define pmax_trunc_seg(a)	mips_trunc_seg(a)
#define pmax_round_seg(a)	mips_round_seg(a)

#endif	/* !_PMAX_PMAP_H_ */
