/* $NetBSD: asm.h,v 1.3.4.1 2019/12/24 17:30:28 martin Exp $ */

#ifndef _AARCH64_ASM_H_
#define _AARCH64_ASM_H_

#include <arm/asm.h>

#define	fp	x29
#define	lr	x30

/*
 * Add a speculation barrier after the 'eret'.
 * Some aarch64 cpus speculatively execute instructions after 'eret',
 * and this potentiates side-channel attacks.
 */
#define	ERET	\
	eret; dsb sy; isb

#endif /* !_AARCH64_ASM_H_ */
