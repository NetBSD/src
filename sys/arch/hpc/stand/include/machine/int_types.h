/*	$NetBSD: int_types.h,v 1.2.8.2 2001/04/30 13:41:34 uch Exp $	*/

#if defined ARM
#include "../../../../arm/include/int_types.h"
#elif defined MIPS
#include "../../../../mips/include/int_types.h"
#elif defined SHx
#include "../../../../sh3/include/int_types.h"
#else
#error "unknown architecture"
#endif
