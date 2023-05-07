/*	$NetBSD: machdep.h,v 1.4 2023/05/07 12:41:48 skrll Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#ifndef _RISCV_MACHDEP_H_
#define _RISCV_MACHDEP_H_

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.h,v 1.4 2023/05/07 12:41:48 skrll Exp $");

#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/siginfo.h>

static inline paddr_t
riscv_kern_vtophys(vaddr_t va)
{
	extern unsigned long kern_vtopdiff;

	return va - kern_vtopdiff;
}

static inline vaddr_t
riscv_kern_phystov(paddr_t pa)
{
	extern unsigned long kern_vtopdiff;

	return pa + kern_vtopdiff;
}

#define KERN_VTOPHYS(va)	riscv_kern_vtophys((vaddr_t)va)
#define KERN_PHYSTOV(pa)	riscv_kern_phystov((paddr_t)pa)

extern	paddr_t physical_start;
extern	paddr_t physical_end;

void	uartputc(int);
int	uartgetc(void);

paddr_t	init_mmu(paddr_t);
void	init_riscv(register_t, paddr_t);

void	riscv_timer_frequency_set(uint32_t);	// which header?
void	riscv_timer_register(void (*timerfn)(void));
void	riscv_intr_set_handler(void (*intr_handler)(struct trapframe *,
	    register_t, register_t, register_t));

int	riscv_timer_intr(void *arg);

void	cpus_fdt_md_attach(device_t, device_t, void *);

void    pt_dump(void (*)(const char *, ...));


#endif	/* _RISCV_MACHDEP_H_ */
