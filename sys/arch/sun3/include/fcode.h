/*	$NetBSD: fcode.h,v 1.1 2023/12/27 17:35:37 thorpej Exp $	*/

#ifndef _SUN3_FCODE_H_
#define	_SUN3_FCODE_H_

#include <m68k/fcode.h>

/*
 * On the sun3, Function Code 3 is control space.  On the sun3x, it's
 * Function Code 4.
 */
#ifdef _SUN3X_
#define	FC_CONTROL	FC_UNDEF4
#else
#define	FC_CONTROL	FC_UNDEF3
#endif /* _SUN3X_ */

#endif /* _SUN3_FCODE_H_ */
