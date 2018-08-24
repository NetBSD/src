/* $NetBSD: exec.c,v 1.1 2018/08/24 02:01:06 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
#include "efifdt.h"

#include <loadfile.h>

int
exec_netbsd(const char *fname, const char *args)
{
	EFI_PHYSICAL_ADDRESS addr;
	u_long marks[MARK_MAX], alloc_size;
	EFI_STATUS status;
	int fd;

	memset(marks, 0, sizeof(marks));
	fd = loadfile(fname, marks, COUNT_KERNEL | LOAD_NOTE);
	if (fd < 0) {
		printf("boot: %s: %s\n", fname, strerror(errno));
		return EIO;
	}
	close(fd);
	marks[MARK_END] = (((u_long) marks[MARK_END] + sizeof(int) - 1)) & (-sizeof(int));
	alloc_size = marks[MARK_END] - marks[MARK_START] + EFIBOOT_ALIGN;

#ifdef EFIBOOT_ALLOCATE_MAX_ADDRESS
	addr = EFIBOOT_ALLOCATE_MAX_ADDRESS;
	status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress, EfiLoaderData,
	    EFI_SIZE_TO_PAGES(alloc_size), &addr);
#else
	addr = 0;
	status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData,
	    EFI_SIZE_TO_PAGES(alloc_size), &addr);
#endif
	if (EFI_ERROR(status)) {
		printf("Failed to allocate %lu bytes for kernel image (error %lu)\n",
		    alloc_size, status);
		return ENOMEM;
	}

	memset(marks, 0, sizeof(marks));
	marks[MARK_START] = (addr + EFIBOOT_ALIGN) & ~(EFIBOOT_ALIGN - 1);
	fd = loadfile(fname, marks, LOAD_KERNEL);
	if (fd < 0) {
		printf("boot: %s: %s\n", fname, strerror(errno));
		goto cleanup;
	}
	close(fd);

	if (efi_fdt_size() > 0) {
		if (args && *args)
			efi_fdt_bootargs(args);
		efi_fdt_memory_map();	
	}

	efi_cleanup();
	efi_boot_kernel(marks);

	/* This should not happen.. */
	printf("boot returned\n");

cleanup:
	uefi_call_wrapper(BS->FreePages, 2, addr, EFI_SIZE_TO_PAGES(alloc_size));
	return EIO;
}
