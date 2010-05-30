/*	$NetBSD: kloader_machdep.c,v 1.1.4.2 2010/05/30 05:16:47 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kloader_machdep.c,v 1.1.4.2 2010/05/30 05:16:47 rmind Exp $");

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

kloader_jumpfunc_t kloader_evbsh3_jump __attribute__((__noreturn__));
kloader_bootfunc_t kloader_evbsh3_sh3_boot __attribute__((__noreturn__));
kloader_bootfunc_t kloader_evbsh3_sh4_boot __attribute__((__noreturn__));

static struct kloader_ops kloader_evbsh3_ops = {
	.jump = kloader_evbsh3_jump,
	.boot = NULL
};

void
kloader_reboot_setup(const char *filename)
{

#if defined(SH3) && defined(SH4)
#error "don't define both SH3 and SH4"
#elif defined(SH3)
	kloader_evbsh3_ops.boot = kloader_evbsh3_sh3_boot;
#elif defined(SH4)
	kloader_evbsh3_ops.boot = kloader_evbsh3_sh4_boot;
#endif

	__kloader_reboot_setup(&kloader_evbsh3_ops, filename);
}

void
kloader_evbsh3_jump(kloader_bootfunc_t func, vaddr_t sp,
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
#define KLOADER_EVBSH3_BOOT(cpu, product)				\
void									\
kloader_evbsh3_sh ## cpu ## _boot(struct kloader_bootinfo *kbi,		\
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
		"jmp	@%0;"						\
		" nop;"							\
		: : "r"(kbi->entry));					\
									\
	/* NOTREACHED */						\
	KLOADER_NORETURN;						\
}

#ifdef SH3
#if defined(SH7708)
KLOADER_EVBSH3_BOOT(3, 7708A)
#elif defined(SH7708S)
KLOADER_EVBSH3_BOOT(3, 7708S)
#elif defined(SH7708R)
KLOADER_EVBSH3_BOOT(3, 7708R)
#elif defined(SH7709)
KLOADER_EVBSH3_BOOT(3, 7709)
#elif defined(SH7709A)
KLOADER_EVBSH3_BOOT(3, 7709A)
#elif defined(SH7706)
KLOADER_EVBSH3_BOOT(3, 7706)
#else
#error "unsupported SH3 variants"
#endif
#endif 

#ifdef SH4
#if defined(SH7750)
KLOADER_EVBSH3_BOOT(4, 7750)
#elif defined(SH7750S)
KLOADER_EVBSH3_BOOT(4, 7750S)
#elif defined(SH7750R)
KLOADER_EVBSH3_BOOT(4, 7750R)
#elif defined(SH7751)
KLOADER_EVBSH3_BOOT(4, 7751)
#elif defined(SH7751R)
KLOADER_EVBSH3_BOOT(4, 7751R)
#else
#error "unsupported SH4 variants"
#endif
#endif
