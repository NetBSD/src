/* $NetBSD: locore.h,v 1.2.12.1 2018/04/07 04:12:11 pgoyette Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AARCH64_LOCORE_H_
#define _AARCH64_LOCORE_H_

#ifdef __aarch64__

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#endif

#ifdef _LOCORE

#define ENABLE_INTERRUPT	\
	msr daifclr, #((DAIF_I|DAIF_F) >> DAIF_SETCLR_SHIFT)
#define DISABLE_INTERRUPT	\
	msr daifset, #((DAIF_I|DAIF_F) >> DAIF_SETCLR_SHIFT)

#else /* _LOCORE */

#include <sys/types.h>

#include <aarch64/armreg.h>

#ifdef MULTIPROCESSOR
/* for compatibility arch/arm/pic/pic.c */
extern u_int arm_cpu_max;
#endif

/* for compatibility arch/arm */
#define I32_bit			DAIF_I
#define F32_bit			DAIF_F
#define cpsie(psw)		daif_enable((psw))
#define cpsid(psw)		daif_disable((psw))


#define ENABLE_INTERRUPT()	daif_enable(DAIF_I|DAIF_F)
#define DISABLE_INTERRUPT()	daif_disable(DAIF_I|DAIF_F)

#define DAIF_MASK		(DAIF_D|DAIF_A|DAIF_I|DAIF_F)

static inline void __unused
daif_enable(register_t psw)
{
	if (!__builtin_constant_p(psw)) {
		reg_daif_write(reg_daif_read() & ~psw);
	} else {
		reg_daifclr_write((psw & DAIF_MASK) >> DAIF_SETCLR_SHIFT);
	}
}

static inline register_t __unused
daif_disable(register_t psw)
{
	register_t oldpsw = reg_daif_read();
	if (!__builtin_constant_p(psw)) {
		reg_daif_write(oldpsw | psw);
	} else {
		reg_daifset_write((psw & DAIF_MASK) >> DAIF_SETCLR_SHIFT);
	}
	return oldpsw;
}

static inline void
arm_dsb(void)
{
	__asm __volatile("dsb sy" ::: "memory");
}

static inline void
arm_isb(void)
{
	__asm __volatile("isb" ::: "memory");
}

#endif /* _LOCORE */

#elif defined(__arm__)

#include <arm/locore.h>

#endif /* __aarch64__/__arm__ */

#endif /* _AARCH64_LOCORE_H_ */
