/*	$NetBSD: kloader_machdep.c,v 1.2 2002/02/11 17:13:28 uch Exp $	*/

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
#include <mips/cache.h>

#include <machine/sysconf.h>
#include <machine/kloader.h>

void kloader_mips_jump(kloader_bootfunc_t *, vaddr_t,
    struct kloader_bootinfo *, struct kloader_page_tag *);
void kloader_mips_reset(void);
extern kloader_bootfunc_t kloader_vr_boot;
extern kloader_bootfunc_t kloader_tx_boot;

struct kloader_ops kloader_mips_ops = {
	.jump = kloader_mips_jump,
	.boot = 0,
	.reset = kloader_mips_reset
};

void
kloader_reboot_setup(const char *filename)
{

	kloader_mips_ops.boot = CPUISMIPS3 ? kloader_vr_boot : kloader_tx_boot;

	__kloader_reboot_setup(&kloader_mips_ops, filename);
}

void
kloader_mips_jump(kloader_bootfunc_t func, vaddr_t sp,
    struct kloader_bootinfo *info, struct kloader_page_tag *tag)
{

	mips_icache_sync_all();

	(*func)(info, tag);	/* 2nd-bootloader don't use stack */
	/* NOTREACHED */
}

void
kloader_mips_reset()
{

	(*platform.reboot)(0, 0);
}
