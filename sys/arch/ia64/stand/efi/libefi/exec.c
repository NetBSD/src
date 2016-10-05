/* $NetBSD: exec.c,v 1.6.14.1 2016/10/05 20:55:29 skrll Exp $ */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 *	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/sys/boot/efi/libefi/elf_freebsd.c,v 1.13 2004/04/05 23:41:28 imp Exp $"); */

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <sys/param.h>
#include <sys/lock.h>

#include <machine/elf_machdep.h>
#include <machine/bootinfo.h>
#include <machine/ia64_cpu.h>
#include <machine/pte.h>
#include <machine/vmparam.h>

#include <efi.h>
#include <efilib.h>
#include <efiboot.h>

#include <machine/efilib.h>

#include "bootstrap.h"

#define _KERNEL

static int	elf64_exec(struct preloaded_file *amp);

struct file_format ia64_elf = { elf64_loadfile, elf64_exec };

static __inline u_int64_t
disable_ic(void)
{
	u_int64_t psr;
	__asm __volatile("mov %0=psr;;" : "=r" (psr));
	__asm __volatile("rsm psr.ic|psr.i;; srlz.i;;");
	return psr;
}

static __inline void
restore_ic(u_int64_t psr)
{
	__asm __volatile("mov psr.l=%0;; srlz.i" :: "r" (psr));
}

/*
 * Entered with psr.ic and psr.i both zero.
 */
void
enter_kernel(u_int64_t start, struct bootinfo *bi)
{
	u_int64_t		psr;

	__asm __volatile("srlz.i;;");
	__asm __volatile("mov cr.ipsr=%0"
			 :: "r"(IA64_PSR_IC
				| IA64_PSR_DT
				| IA64_PSR_RT
				| IA64_PSR_IT
				| IA64_PSR_BN));
	__asm __volatile("mov cr.iip=%0" :: "r"(start));
	__asm __volatile("mov cr.ifs=r0;;");
	__asm __volatile("mov ar.rsc=0;; flushrs;;");
	__asm __volatile("mov r8=%0" :: "r" (bi));
	__asm __volatile("rfi;;");
}

static int
elf64_exec(struct preloaded_file *fp)
{
	pt_entry_t		pte;
	struct bootinfo		*bi;
	u_int64_t		psr;
	UINTN			mapkey, pages, size;
	UINTN			descsz;
	UINT32			descver;
	EFI_STATUS		status;

	if (strcmp(fp->f_type, ELF64_KERNELTYPE)) return(EFTYPE);

	/*
	 * Allocate enough pages to hold the bootinfo block and the memory
	 * map EFI will return to us. The memory map has an unknown size,
	 * so we have to determine that first. Note that the AllocatePages
	 * call can itself modify the memory map, so we have to take that
	 * into account as well. The changes to the memory map are caused
	 * by splitting a range of free memory into two (AFAICT), so that
	 * one is marked as being loader data.
	 */
	size = 0;
	descsz = sizeof(EFI_MEMORY_DESCRIPTOR);
	BS->GetMemoryMap(&size, NULL, &mapkey, &descsz, &descver);
	size += descsz + ((sizeof(struct bootinfo) + 0x0f) & ~0x0f);
	pages = EFI_SIZE_TO_PAGES(size);
	status = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages,
	    (void*)&bi);
	if (EFI_ERROR(status)) {
		printf("unable to create bootinfo block (status=0x%lx)\n",
		    (long)status);
		return (ENOMEM);
	}

	memset(bi, 0, sizeof(struct bootinfo));
	bi_load(bi, fp, &mapkey, pages);

	printf("Entering %s at 0x%lx...\n", fp->f_name, fp->marks[MARK_ENTRY]);

	status = BS->ExitBootServices(IH, mapkey);
	if (EFI_ERROR(status)) {
		printf("ExitBootServices returned 0x%lx\n", status);
		return (EINVAL);
	}

	psr = disable_ic();

	/*
	 * Region 6 is direct mapped UC and region 7 is direct mapped
	 * WC. The details of this is controlled by the Alt {I,D}TLB
	 * handlers. Here we just make sure that they have the largest 
	 * possible page size to minimise TLB usage.
	 */
	ia64_set_rr(IA64_RR_BASE(6), (6 << 8) | (28 << 2));
	ia64_set_rr(IA64_RR_BASE(7), (7 << 8) | (28 << 2));

	pte = PTE_PRESENT | PTE_MA_WB | PTE_ACCESSED | PTE_DIRTY |
	    PTE_PL_KERN | PTE_AR_RWX;

	__asm __volatile("mov cr.ifa=%0" :: "r"(IA64_RR_BASE(7)));
	__asm __volatile("mov cr.itir=%0" :: "r"(28 << 2));
	__asm __volatile("ptr.i %0,%1" :: "r"(IA64_RR_BASE(7)), "r"(28<<2));
	__asm __volatile("ptr.d %0,%1" :: "r"(IA64_RR_BASE(7)), "r"(28<<2));
	__asm __volatile("srlz.i;;");
	__asm __volatile("itr.i itr[%0]=%1;;" :: "r"(0), "r"(pte));
	__asm __volatile("srlz.i;;");
	__asm __volatile("itr.d dtr[%0]=%1;;" :: "r"(0), "r"(pte));
	__asm __volatile("srlz.i;;");

	enter_kernel(fp->marks[MARK_ENTRY], bi);

	restore_ic(psr);
}
