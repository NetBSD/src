/*	$NetBSD: types.h,v 1.1 2001/10/16 15:38:52 uch Exp $	*/

#include <mips/types.h>

#define	__HAVE_GENERIC_SOFT_INTERRUPTS

typedef int int128_t __attribute__((__mode__(TI)));
typedef unsigned int u_int128_t __attribute__((__mode__(TI)));
