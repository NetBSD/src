/*	$NetBSD: tm.h,v 1.3 2000/02/16 11:23:49 tsutsui Exp $	*/

#ifdef mipseb
#define TARGET_ENDIAN_DEFAULT MASK_BIG_ENDIAN
#else
#ifdef mipsel
#define TARGET_ENDIAN_DEFAULT 0
#endif
#endif

#include "mips/netbsd.h"
