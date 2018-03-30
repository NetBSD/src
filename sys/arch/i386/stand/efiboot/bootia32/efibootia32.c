/*	$NetBSD: efibootia32.c,v 1.3.10.1 2018/03/30 06:20:11 pgoyette Exp $	*/

/*-
 * Copyright (c) 2016 Kimihiro Nonaka <nonaka@netbsd.org>
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

#include "efiboot.h"

#include <sys/bootblock.h>

void startprog32_start(physaddr_t, uint32_t, uint32_t *, physaddr_t,
    physaddr_t, physaddr_t, u_long, void *);
extern void (*startprog32)(physaddr_t, uint32_t, uint32_t *, physaddr_t,
    physaddr_t, physaddr_t, u_long, void *);
extern u_int startprog32_size;

void
efi_md_init(void)
{
	EFI_STATUS status;
	EFI_PHYSICAL_ADDRESS addr = EFI_ALLOCATE_MAX_ADDRESS;
	u_int sz = EFI_SIZE_TO_PAGES(startprog32_size);

	status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress,
	    EfiLoaderData, sz, &addr);
	if (EFI_ERROR(status))
		panic("%s: AllocatePages() failed: %d page(s): %" PRIxMAX,
		    __func__, sz, (uintmax_t)status);
	startprog32 = (void *)(u_long)addr;
	CopyMem(startprog32, startprog32_start, startprog32_size);
}

/* ARGSUSED */
void
startprog(physaddr_t entry, uint32_t argc, uint32_t *argv, physaddr_t sp)
{

	(*startprog32)(entry, argc, argv,
	    (physaddr_t)startprog32 + startprog32_size,
	    efi_kernel_start, efi_kernel_start + efi_loadaddr,
	    efi_kernel_size, startprog32);
}

/* ARGSUSED */
void
multiboot(physaddr_t entry, physaddr_t header, physaddr_t sp)
{
	panic("%s: not implemented", __func__);
}
