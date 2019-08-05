/* $NetBSD: asm.h,v 1.4 2019/08/05 16:24:48 joerg Exp $ */

#ifndef _AARCH64_ASM_H_
#define _AARCH64_ASM_H_

#include <arm/asm.h>

#ifdef __aarch64__
#define	fp	x29
#define	lr	x30
#endif

#endif /* !_AARCH64_ASM_H_ */
