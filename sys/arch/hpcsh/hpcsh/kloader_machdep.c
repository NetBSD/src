/*	$NetBSD: kloader_machdep.c,v 1.3 2002/02/11 17:32:35 uch Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>

#include <sh3/mmureg.h>
#include <sh3/cache.h>
#include <sh3/cache_sh3.h>
#include <sh3/cache_sh4.h>

#include <machine/kloader.h>

/* 
 * 2nd-bootloader. Make sure that PIC and its size is lower than page size.
 */
#define KLOADER_SH_BOOT(cpu, product)					\
void									\
kloader_sh ## cpu ## _boot(struct kloader_bootinfo *kbi,		\
    struct kloader_page_tag *p)						\
{									\
	int tmp;							\
									\
	/* Disable interrupt. block exception. */			\
	__asm__ __volatile__(						\
		"stc	sr, %1;"					\
		"or	%0, %1;"					\
		"ldc	%1, sr" : : "r"(0x500000f0), "r"(tmp));		\
									\
	/* Now I run on P1, TLB flush. and disable. */			\
	SHREG_MMUCR = MMUCR_TF;						\
									\
	do {								\
		u_int32_t *dst =(u_int32_t *)p->dst;			\
		u_int32_t *src =(u_int32_t *)p->src;			\
		u_int32_t sz = p->sz / sizeof (int);			\
		while (sz--)						\
			*dst++ = *src++;				\
	} while ((p = (struct kloader_page_tag *)p->next) != 0);	\
									\
	SH ## product ## _CACHE_FLUSH();				\
									\
	/* jump to kernel entry. */					\
	__asm__ __volatile__(						\
		"mov	%0, r4;"					\
		"mov	%1, r5;"					\
		"jmp	@%3;"						\
		"mov	%2, r6;"					\
		: :							\
		"r"(kbi->argc),						\
		"r"(kbi->argv),						\
		"r"(&kbi->bootinfo),					\
		"r"(kbi->entry));					\
	/* NOTREACHED */						\
}

void kloader_sh_jump(kloader_bootfunc_t *, vaddr_t,
    struct kloader_bootinfo *, struct kloader_page_tag *);
kloader_bootfunc_t kloader_sh3_boot;
kloader_bootfunc_t kloader_sh4_boot;

struct kloader_ops kloader_sh_ops = {
	.jump = kloader_sh_jump,
	.boot = 0
};

void
kloader_reboot_setup(const char *filename)
{
#ifdef SH4
	kloader_sh_ops.boot = kloader_sh4_boot;
#else
	kloader_sh_ops.boot = kloader_sh3_boot;
#endif
	__kloader_reboot_setup(&kloader_sh_ops, filename);
}

void
kloader_sh_jump(kloader_bootfunc_t func, vaddr_t sp,
    struct kloader_bootinfo *info, struct kloader_page_tag *tag)
{

	sh_icache_sync_all();	/* also flush d-cache */

	__asm__ __volatile__(
	    	"mov	%0, r4;"
		"mov	%1, r5;"
		"jmp	@%2;"
		"mov	%3, sp"
		: : "r"(info), "r"(tag), "r"(func), "r"(sp));
	/* NOTREACHED */
}

KLOADER_SH_BOOT(3, 7709A)
KLOADER_SH_BOOT(4, 7750)
