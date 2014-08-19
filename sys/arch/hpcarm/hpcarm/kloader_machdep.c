/*	$NetBSD: kloader_machdep.c,v 1.1.6.1 2014/08/20 00:03:02 tls Exp $	*/

/*-
 * Copyright (C) 2012 NONAKA Kimihiro <nonaka@netbsd.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kloader_machdep.c,v 1.1.6.1 2014/08/20 00:03:02 tls Exp $");

#include "debug_kloader.h"
#include "opt_cputypes.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/kloader.h>
#include <machine/pmap.h>

#include <arm/cpufunc.h>
#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
#include <arm/xscale/pxa2x0reg.h>
#endif

kloader_jumpfunc_t kloader_hpcarm_jump __attribute__((__noreturn__));
void kloader_hpcarm_reset(void);
#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
kloader_bootfunc_t kloader_pxa2x0_boot __attribute__((__noreturn__));
#endif

struct kloader_ops kloader_hpcarm_ops = {
	.jump = kloader_hpcarm_jump,
#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
	.boot = kloader_pxa2x0_boot,
#endif
	.reset = kloader_hpcarm_reset,
};

void
kloader_reboot_setup(const char *filename)
{

	__kloader_reboot_setup(&kloader_hpcarm_ops, filename);
}

void
kloader_hpcarm_reset(void)
{
	extern void (*__cpu_reset)(void) __dead;

	__cpu_reset();
	/*NOTREACHED*/
}

void
kloader_hpcarm_jump(kloader_bootfunc_t func, vaddr_t sp,
    struct kloader_bootinfo *kbi, struct kloader_page_tag *tag)
{

	disable_interrupts(I32_bit|F32_bit);	
	cpu_idcache_wbinv_all();

	/* jump to 2nd boot-loader */
	(*func)(kbi, tag);
	__builtin_unreachable();
}

/*
 * Physcal address to virtual address
 */
vaddr_t
kloader_phystov(paddr_t pa)
{
	vaddr_t va;
	int error;

#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
	va = KERNEL_BASE + pa - PXA2X0_SDRAM0_START;
#else
#error	No support KLOADER with specific CPU type.
#endif
	error = pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, 0);
	if (error) {
		printf("%s: map failed: pa=0x%lx, va=0x%lx, error=%d\n",
		    __func__, pa, va, error);
	}
	return va;
}
