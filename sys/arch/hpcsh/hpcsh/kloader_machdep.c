/*	$NetBSD: kloader_machdep.c,v 1.13 2006/01/20 03:55:55 uwe Exp $	*/

/*-
 * Copyright (c) 2001, 2002, 2004 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kloader_machdep.c,v 1.13 2006/01/20 03:55:55 uwe Exp $");

#include "debug_kloader.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <sh3/mmu.h>
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>
#include <sh3/cache.h>
#include <sh3/cache_sh3.h>
#include <sh3/cache_sh4.h>

#include <machine/kloader.h>

/* make gcc believe __attribute__((__noreturn__)) claim */
#define KLOADER_NORETURN for (;;) continue

kloader_jumpfunc_t kloader_hpcsh_jump __attribute__((__noreturn__));
kloader_bootfunc_t kloader_hpcsh3_boot __attribute__((__noreturn__));
kloader_bootfunc_t kloader_hpcsh4_boot __attribute__((__noreturn__));

struct kloader_ops kloader_hpcsh_ops = {
	.jump = kloader_hpcsh_jump,
	.boot = NULL
};


void
kloader_reboot_setup(const char *filename)
{

	kloader_hpcsh_ops.boot = CPU_IS_SH3 ? kloader_hpcsh3_boot
					    : kloader_hpcsh4_boot;

	__kloader_reboot_setup(&kloader_hpcsh_ops, filename);
}

void
kloader_hpcsh_jump(kloader_bootfunc_t func, vaddr_t sp,
    struct kloader_bootinfo *info, struct kloader_page_tag *tag)
{

	sh_icache_sync_all();	/* also flushes d-cache */

	__asm volatile(
	    	"mov	%0, r4;"
		"mov	%1, r5;"
		"jmp	@%2;"
		" mov	%3, sp"
		: : "r"(info), "r"(tag), "r"(func), "r"(sp));

	/* NOTREACHED */
	KLOADER_NORETURN;
}

/*
 * 2nd-stage bootloader.  Fetches new kernel out of the page tags
 * chain and copies it to its intended location in memory.  Make sure
 * this function is position independent and fits into a single page.
 */
#define KLOADER_HPCSH_BOOT(cpu, product)				\
void									\
kloader_hpcsh ## cpu ## _boot(struct kloader_bootinfo *kbi,		\
    struct kloader_page_tag *p)						\
{									\
	int tmp;							\
									\
	/* Disable interrupts, block exceptions. */			\
	__asm volatile(							\
		"stc	sr, %0;"					\
		"or	%1, %0;"					\
		"ldc	%0, sr"						\
		: "=r"(tmp)						\
		: "r"(PSL_MD | PSL_BL | PSL_IMASK));			\
									\
	/* We run on P1, flush and disable TLB. */			\
	SH ## cpu ## _TLB_DISABLE;					\
									\
	do {								\
		uint32_t *dst = (uint32_t *)p->dst;			\
		uint32_t *src = (uint32_t *)p->src;			\
		uint32_t sz = p->sz / sizeof (uint32_t);		\
		while (sz--)						\
			*dst++ = *src++;				\
	} while ((p = (struct kloader_page_tag *)p->next) != 0);	\
									\
	SH ## product ## _CACHE_FLUSH();				\
									\
	/* Jump to the kernel entry point. */				\
	__asm volatile(							\
		"mov	%0, r4;"					\
		"mov	%1, r5;"					\
		"jmp	@%3;"						\
		" mov	%2, r6;"					\
		: :							\
		"r"(kbi->argc),						\
		"r"(kbi->argv),						\
		"r"(&kbi->bootinfo),					\
		"r"(kbi->entry));					\
									\
	/* NOTREACHED */						\
	KLOADER_NORETURN;						\
}

#ifdef SH3
KLOADER_HPCSH_BOOT(3, 7709A)
#endif 

#ifdef SH4
KLOADER_HPCSH_BOOT(4, 7750)
#endif
