/*	$NetBSD: fcode.h,v 1.1 2023/12/27 17:35:34 thorpej Exp $	*/

#ifndef _HP300_FCODE_H_
#define	_HP300_FCODE_H_

#include <m68k/fcode.h>

/* Function Code 3 is used to invalidate TLB entries in the HP MMU. */
#define	FC_PURGE	FC_UNDEF3

#endif /* _HP300_FCODE_H_ */
