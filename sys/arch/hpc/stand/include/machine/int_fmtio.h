/*	$NetBSD: int_fmtio.h,v 1.1.10.2 2001/04/30 13:41:33 uch Exp $	*/

#if defined ARM
#include "../../../../arm/include/int_fmtio.h"
#elif defined MIPS
#include "../../../../mips/include/int_fmtio.h"
#elif defined SHx
#include "../../../../sh3/include/int_fmtio.h"
#else
#error "unknown architecture"
#endif
