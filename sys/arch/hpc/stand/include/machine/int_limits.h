/*	$NetBSD: int_limits.h,v 1.1 2001/04/30 13:41:33 uch Exp $	*/

#if defined ARM
#include "../../../../arm/include/int_limits.h"
#elif defined MIPS
#include "../../../../mips/include/int_limits.h"
#elif defined SHx
#include "../../../../sh3/include/int_limits.h"
#else
#error "unknown architecture"
#endif
