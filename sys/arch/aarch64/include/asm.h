/* $NetBSD: asm.h,v 1.5 2019/12/20 07:16:43 ryo Exp $ */

#ifndef _AARCH64_ASM_H_
#define _AARCH64_ASM_H_

#include <arm/asm.h>

#ifdef __aarch64__
#define	fp	x29
#define	lr	x30

/*
 * Add a speculation barrier after the 'eret'.
 * Some aarch64 cpus speculatively execute instructions after 'eret',
 * and this potentiates side-channel attacks.
 */
#define	ERET	\
	eret; dsb sy; isb

#endif

#endif /* !_AARCH64_ASM_H_ */
