/*	$NetBSD: int_const.h,v 1.1 2001/04/30 13:41:32 uch Exp $	*/

#if defined ARM
#include "../../../../arm/include/int_const.h"
#elif defined MIPS
#include "../../../../mips/include/int_const.h"
#elif defined SHx
#include "../../../../sh3/include/int_const.h"
#else
#error "unknown architecture"
#endif
