/*	$NetBSD: int_mwgwtypes.h,v 1.1 2001/04/30 13:41:33 uch Exp $	*/

#if defined ARM
#include "../../../../arm/include/int_mwgwtypes.h"
#elif defined MIPS
#include "../../../../mips/include/int_mwgwtypes.h"
#elif defined SHx
#include "../../../../sh3/include/int_mwgwtypes.h"
#else
#error "unknown architecture"
#endif
