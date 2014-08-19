/* $NetBSD: locore.h,v 1.1.4.2 2014/08/20 00:02:39 tls Exp $ */

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

#include <sys/types.h>

#include <sys/cpu.h>
#include <sys/lwp.h>
#include <sys/bus.h>

#include <aarch64/armreg.h>
#include <aarch64/frame.h>

struct mainbus_attach_args {
	const char *mba_name;
	bus_space_tag_t mba_memt;
	bus_dma_tag_t mba_dmat;
	bus_addr_t mba_addr;
	bus_size_t mba_size;
	int mba_intr;
	int mba_intrbase;
	int mba_package;
};

void	userret(struct lwp *, struct trapframe *);
void	lwp_trampoline(void);
void	cpu_dosoftints(void);
void	dosoftints(void);
void	cpu_switchto_softint(struct lwp *, int);
void	cpu_send_ipi(struct cpu_info *, int);

extern paddr_t physical_start;
extern paddr_t physical_end;

extern const pcu_ops_t pcu_fpu_ops;

static inline bool
fpu_used_p(lwp_t *l)
{
	KASSERT(l == curlwp);
	return pcu_valid_p(&pcu_fpu_ops);
}

static inline void
fpu_discard(lwp_t *l, bool usesw)
{
	KASSERT(l == curlwp);
	pcu_discard(&pcu_fpu_ops, usesw);
}

static inline void
fpu_save(lwp_t *l)
{
	KASSERT(l == curlwp);
	pcu_save(&pcu_fpu_ops);
}

static inline void cpsie(register_t psw) __attribute__((__unused__));
static inline register_t cpsid(register_t psw) __attribute__((__unused__));

static inline void
cpsie(register_t psw)
{
	if (!__builtin_constant_p(psw)) {
		reg_daif_write(psw);
	} else {
		reg_daifset_write(psw);
	}
}

static inline void
enable_interrupts(register_t psw)
{
	reg_daif_write(psw);
}

static inline register_t
cpsid(register_t psw)
{
	register_t oldpsw = reg_daif_read();
	if (!__builtin_constant_p(psw)) {
		reg_daif_write(oldpsw & ~psw);
	} else {
		reg_daifclr_write(psw);
	}
	return oldpsw;
}

static const paddr_t VTOPHYS_FAILED = (paddr_t) -1L;

static inline paddr_t
vtophys(vaddr_t va)
{
	const uint64_t daif = reg_daif_read();
	/*
	 * Use the address translation instruction to do the lookup.
	 */
	reg_daifset_write(DAIF_I|DAIF_F);
	__asm __volatile("at\ts1e1r, %0" :: "r"(va));
	paddr_t pa = reg_par_el1_read();
	pa = (pa & PAR_F) ? VTOPHYS_FAILED : (pa & PAR_PA);
	reg_daif_write(daif);
	return pa;
}

static inline paddr_t
vtophysw(vaddr_t va)
{
	const uint64_t daif = reg_daif_read();
	/*
	 * Use the address translation instruction to do the lookup.
	 */
	reg_daifset_write(DAIF_I|DAIF_F);
	__asm __volatile("at\ts1e1w, %0" :: "r"(va));
	paddr_t pa = reg_par_el1_read();
	pa = (pa & PAR_F) ? VTOPHYS_FAILED : (pa & PAR_PA);
	reg_daif_write(daif);
	return pa;
}

#elif defined(__arm__)

#include <arm/locore.h>

#endif /* __aarch64__/__arm__ */

#endif /* _AARCH64_LOCORE_H_ */
