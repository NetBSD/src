/* $NetBSD: exec.c,v 1.6 2018/09/15 17:06:32 jmcneill Exp $ */

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

#include <sys/reboot.h>

u_long load_offset = 0;

#define	FDT_SPACE	(4 * 1024 * 1024)
#define	FDT_ALIGN	((2 * 1024 * 1024) - 1)

static EFI_PHYSICAL_ADDRESS initrd_addr, dtb_addr;
static u_long initrd_size = 0, dtb_size = 0;

static int
load_file(char *path, EFI_PHYSICAL_ADDRESS *paddr, u_long *psize)
{
	EFI_STATUS status;
	struct stat st;
	ssize_t len;
	int fd;

	if (strlen(path) == 0)
		return 0;

	fd = open(path, 0);
	if (fd < 0) {
		printf("boot: failed to open %s: %s\n", path, strerror(errno));
		return errno;
	}
	if (fstat(fd, &st) < 0) {
		printf("boot: failed to fstat %s: %s\n", path, strerror(errno));
		close(fd);
		return errno;
	}
	if (st.st_size == 0) {
		printf("boot: empty file %s\n", path);
		close(fd);
		return EINVAL;
	}

	*psize = st.st_size;

#ifdef EFIBOOT_ALLOCATE_MAX_ADDRESS
	*paddr = EFIBOOT_ALLOCATE_MAX_ADDRESS;
	status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress, EfiLoaderData,
	    EFI_SIZE_TO_PAGES(*psize), paddr);
#else
	*paddr = 0;
	status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData,
	    EFI_SIZE_TO_PAGES(*psize), paddr);
#endif
	if (EFI_ERROR(status)) {
		printf("Failed to allocate %lu bytes for %s (error %lu)\n",
		    *psize, path, status);
		close(fd);
		return ENOMEM;
	}

	printf("boot: loading %s ", path);
	len = read(fd, (void *)*paddr, *psize);
	close(fd);

	if (len != *psize) {
		if (len < 0)
			printf(": %s\n", strerror(errno));
		else
			printf(": returned %ld (expected %ld)\n", len, *psize);
		return EIO;
	}

	printf("done.\n");

	efi_dcache_flush(*paddr, *psize);

	return 0;
}

int
exec_netbsd(const char *fname, const char *args)
{
	EFI_PHYSICAL_ADDRESS addr;
	u_long marks[MARK_MAX], alloc_size;
	EFI_STATUS status;
	int fd, ohowto;

	load_file(get_initrd_path(), &initrd_addr, &initrd_size);
	load_file(get_dtb_path(), &dtb_addr, &dtb_size);

	memset(marks, 0, sizeof(marks));
	ohowto = howto;
	howto |= AB_SILENT;
	fd = loadfile(fname, marks, COUNT_KERNEL | LOAD_NOTE);
	howto = ohowto;
	if (fd < 0) {
		printf("boot: %s: %s\n", fname, strerror(errno));
		return EIO;
	}
	close(fd);
	marks[MARK_END] = (((u_long) marks[MARK_END] + sizeof(int) - 1)) & (-sizeof(int));
	alloc_size = marks[MARK_END] - marks[MARK_START] + FDT_SPACE + EFIBOOT_ALIGN;

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
	load_offset = (addr + EFIBOOT_ALIGN) & ~(EFIBOOT_ALIGN - 1);
	fd = loadfile(fname, marks, LOAD_KERNEL);
	if (fd < 0) {
		printf("boot: %s: %s\n", fname, strerror(errno));
		goto cleanup;
	}
	close(fd);
	load_offset = 0;

	if (dtb_addr && efi_fdt_set_data((void *)dtb_addr) != 0) {
		printf("boot: invalid DTB data\n");
		goto cleanup;
	}

	if (efi_fdt_size() > 0) {
		efi_fdt_init((marks[MARK_END] + FDT_ALIGN) & ~FDT_ALIGN, FDT_ALIGN + 1);
		efi_fdt_initrd(initrd_addr, initrd_size);
		efi_fdt_bootargs(args);
		efi_fdt_memory_map();
		efi_fdt_fini();
	}

	efi_cleanup();
	efi_boot_kernel(marks);

	/* This should not happen.. */
	printf("boot returned\n");

cleanup:
	uefi_call_wrapper(BS->FreePages, 2, addr, EFI_SIZE_TO_PAGES(alloc_size));
	if (initrd_addr) {
		uefi_call_wrapper(BS->FreePages, 2, initrd_addr, EFI_SIZE_TO_PAGES(initrd_size));
		initrd_addr = 0;
		initrd_size = 0;
	}
	if (dtb_addr) {
		uefi_call_wrapper(BS->FreePages, 2, dtb_addr, EFI_SIZE_TO_PAGES(dtb_size));
		dtb_addr = 0;
		dtb_size = 0;
	}
	return EIO;
}
