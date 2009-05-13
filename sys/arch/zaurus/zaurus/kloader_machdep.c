/*	$NetBSD: kloader_machdep.c,v 1.3.6.2 2009/05/13 17:18:55 jym Exp $	*/

/*-
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kloader_machdep.c,v 1.3.6.2 2009/05/13 17:18:55 jym Exp $");

#include "debug_kloader.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/disklabel.h>

#include <machine/kloader.h>
#include <machine/pmap.h>

#include <arm/cpufunc.h>
#include <arm/xscale/pxa2x0reg.h>

#include <zaurus/zaurus/zaurus_var.h>

#ifndef KERNEL_BASE
#define	KERNEL_BASE	0xc0000000
#endif

kloader_jumpfunc_t kloader_zaurus_jump __attribute__((__noreturn__));
kloader_bootfunc_t kloader_zaurus_boot __attribute__((__noreturn__));
void kloader_zaurus_reset(void);

struct kloader_ops kloader_zaurus_ops = {
	.jump = kloader_zaurus_jump,
	.boot = kloader_zaurus_boot,
	.reset = kloader_zaurus_reset,
};

void
kloader_reboot_setup(const char *filename)
{

	__kloader_reboot_setup(&kloader_zaurus_ops, filename);
}

void
kloader_zaurus_reset(void)
{

	zaurus_restart();
	/*NOTREACHED*/
}

void
kloader_zaurus_jump(kloader_bootfunc_t func, vaddr_t sp,
    struct kloader_bootinfo *kbi, struct kloader_page_tag *tag)
{
	extern int kloader_howto;
	void (*bootinfop)(void *, void *) = (void *)(0xc0200000 - PAGE_SIZE);
	uint32_t *bootmagicp = (uint32_t *)(0xc0200000 - BOOTARGS_BUFSIZ);
	vaddr_t top, ptr;
	struct bootinfo *bootinfo;
	struct btinfo_howto *bi_howto;
	struct btinfo_rootdevice *bi_rootdv;

	disable_interrupts(I32_bit|F32_bit);	

	/* copy 2nd boot-loader to va=pa page */
	memmove(bootinfop, func, PAGE_SIZE);

	/*
	 * make bootinfo
	 */
	memset(bootmagicp, 0, BOOTARGS_BUFSIZ);
	bootinfo = (struct bootinfo *)(bootmagicp + 1);
	bootinfo->nentries = 0;
	top = ptr = (vaddr_t)bootinfo->info;

	/* pass to howto for new kernel */
	bi_howto = (struct btinfo_howto *)ptr;
	bi_howto->common.len = sizeof(struct btinfo_howto);
	bi_howto->common.type = BTINFO_HOWTO;
	bi_howto->howto = kloader_howto;
	bootinfo->nentries++;
	ptr += bi_howto->common.len;

	/* set previous root device for new boot device */
	if (root_device != NULL 
	 && device_class(root_device) == DV_DISK
	 && !device_is_a(root_device, "dk")) {
		bi_rootdv = (struct btinfo_rootdevice *)ptr;
		bi_rootdv->common.len = sizeof(struct btinfo_rootdevice);
		bi_rootdv->common.type = BTINFO_ROOTDEVICE;
		snprintf(bi_rootdv->devname, sizeof(bi_rootdv->devname), "%s%c",
		    device_xname(root_device), (int)DISKPART(rootdev) + 'a');
		bootinfo->nentries++;
		ptr += bi_rootdv->common.len;
	}

	if (bootinfo->nentries > 0)
		*bootmagicp = BOOTARGS_MAGIC;
	cpu_idcache_wbinv_all();

	/* jump to 2nd boot-loader */
	(*bootinfop)(kbi, tag);

	/*NOTREACHED*/
	for (;;)
		continue;
}

/*
 * Physcal address to virtual address
 */
vaddr_t
kloader_phystov(paddr_t pa)
{
	vaddr_t va;
	int error;

	va = KERNEL_BASE + pa - 0xa0000000UL;
	error = pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, 0);
	if (error) {
		printf("%s: map failed: pa=0x%lx, va=0x%lx, error=%d\n",
		    __func__, pa, va, error);
	}
	return va;
}
