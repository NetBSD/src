/* $NetBSD: exec.c,v 1.19 2020/10/10 19:17:39 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jason R. Thorpe
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
#include "efiacpi.h"
#include "efirng.h"
#include "module.h"
#include "overlay.h"

#include <sys/param.h>
#include <sys/reboot.h>

extern char twiddle_toggle;

u_long load_offset = 0;

#define	FDT_SPACE	(4 * 1024 * 1024)
#define	FDT_ALIGN	((2 * 1024 * 1024) - 1)

static EFI_PHYSICAL_ADDRESS initrd_addr, dtb_addr, rndseed_addr, efirng_addr;
static u_long initrd_size = 0, dtb_size = 0, rndseed_size = 0, efirng_size = 0;

static int
load_file(const char *path, u_long extra, bool quiet_errors,
    EFI_PHYSICAL_ADDRESS *paddr, u_long *psize)
{
	EFI_STATUS status;
	struct stat st;
	ssize_t len;
	ssize_t expectedlen;
	int fd;

	if (strlen(path) == 0)
		return 0;

	fd = open(path, 0);
	if (fd < 0) {
		if (!quiet_errors) {
			printf("boot: failed to open %s: %s\n", path,
			    strerror(errno));
		}
		return errno;
	}
	if (fstat(fd, &st) < 0) {
		printf("boot: failed to fstat %s: %s\n", path, strerror(errno));
		close(fd);
		return errno;
	}
	if (st.st_size == 0) {
		if (!quiet_errors) {
			printf("boot: empty file %s\n", path);
		}
		close(fd);
		return EINVAL;
	}

	expectedlen = st.st_size;
	*psize = st.st_size + extra;

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
		    *psize, path, (u_long)status);
		close(fd);
		*paddr = 0;
		return ENOMEM;
	}

	printf("boot: loading %s ", path);
	len = read(fd, (void *)(uintptr_t)*paddr, expectedlen);
	close(fd);

	if (len != expectedlen) {
		if (len < 0) {
			printf(": %s\n", strerror(errno));
		} else {
			printf(": returned %ld (expected %ld)\n", len,
			    expectedlen);
		}
		return EIO;
	}

	printf("done.\n");

	efi_dcache_flush(*paddr, *psize);

	return 0;
}

static void
apply_overlay(const char *path, void *dtbo)
{

	if (!efi_fdt_overlay_is_compatible(dtbo)) {
		printf("boot: %s: incompatible overlay\n", path);
		return;
	}

	int fdterr;

	if (efi_fdt_overlay_apply(dtbo, &fdterr) != 0) {
		printf("boot: %s: error %d applying overlay\n", path, fdterr);
	}
}

static void
apply_overlay_file(const char *path)
{
	EFI_PHYSICAL_ADDRESS dtbo_addr;
	u_long dtbo_size;

	if (strlen(path) == 0)
		return;

	if (load_file(path, 0, false, &dtbo_addr, &dtbo_size) != 0 ||
	    dtbo_addr == 0) {
		/* Error messages have already been displayed. */
		goto out;
	}

	apply_overlay(path, (void *)(uintptr_t)dtbo_addr);

out:
	if (dtbo_addr) {
		uefi_call_wrapper(BS->FreePages, 2, dtbo_addr,
		    EFI_SIZE_TO_PAGES(dtbo_size));
	}
}

static void
load_fdt_overlays(void)
{
	if (!dtoverlay_enabled)
		return;

	dtoverlay_foreach(apply_overlay_file);
}

static void
load_module(const char *module_name)
{
	EFI_PHYSICAL_ADDRESS addr;
	u_long size;
	char path[PATH_MAX];

	snprintf(path, sizeof(path), "%s/%s/%s.kmod", module_prefix,
	    module_name, module_name);

	if (load_file(path, 0, false, &addr, &size) != 0 || addr == 0 || size == 0)
		return;

	efi_fdt_module(module_name, (u_long)addr, size);
}

static void
load_modules(const char *kernel_name)
{
	if (!module_enabled)
		return;

	module_init(kernel_name);
	module_foreach(load_module);
}

static void
generate_efirng(void)
{
	EFI_PHYSICAL_ADDRESS addr;
	u_long size = EFI_PAGE_SIZE;
	EFI_STATUS status;

	/* Check whether the RNG is available before bothering.  */
	if (!efi_rng_available())
		return;

	/*
	 * Allocate a page.  This is the smallest unit we can pass into
	 * the kernel conveniently.
	 */
#ifdef EFIBOOT_ALLOCATE_MAX_ADDRESS
	addr = EFIBOOT_ALLOCATE_MAX_ADDRESS;
	status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress,
	    EfiLoaderData, EFI_SIZE_TO_PAGES(size), &addr);
#else
	addr = 0;
	status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages,
	    EfiLoaderData, EFI_SIZE_TO_PAGES(size), &addr);
#endif
	if (EFI_ERROR(status)) {
		Print(L"Failed to allocate page for EFI RNG output: %r\n",
		    status);
		return;
	}

	/* Fill the page with whatever the EFI RNG will do.  */
	if (efi_rng((void *)(uintptr_t)addr, size)) {
		uefi_call_wrapper(BS->FreePages, 2, addr, size);
		return;
	}

	/* Success!  */
	efirng_addr = addr;
	efirng_size = size;
}

int
exec_netbsd(const char *fname, const char *args)
{
	EFI_PHYSICAL_ADDRESS addr;
	u_long marks[MARK_MAX], alloc_size;
	EFI_STATUS status;
	int fd, ohowto;

	load_file(get_initrd_path(), 0, false, &initrd_addr, &initrd_size);
	load_file(get_dtb_path(), 0, false, &dtb_addr, &dtb_size);
	generate_efirng();

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
		    alloc_size, (u_long)status);
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

#ifdef EFIBOOT_ACPI
	if (efi_acpi_available()) {
		efi_acpi_create_fdt();
	} else
#endif
	if (dtb_addr && efi_fdt_set_data((void *)(uintptr_t)dtb_addr) != 0) {
		printf("boot: invalid DTB data\n");
		goto cleanup;
	}

	if (efi_fdt_size() > 0) {
		/*
		 * Load the rndseed as late as possible -- after we
		 * have committed to using fdt and executing this
		 * kernel -- so that it doesn't hang around in memory
		 * if we have to bail or the kernel won't use it.
		 */
		load_file(get_rndseed_path(), 0, false,
		    &rndseed_addr, &rndseed_size);

		efi_fdt_init((marks[MARK_END] + FDT_ALIGN) & ~FDT_ALIGN, FDT_ALIGN + 1);
		load_modules(fname);
		load_fdt_overlays();
		efi_fdt_initrd(initrd_addr, initrd_size);
		efi_fdt_rndseed(rndseed_addr, rndseed_size);
		efi_fdt_efirng(efirng_addr, efirng_size);
		efi_fdt_bootargs(args);
		efi_fdt_system_table();
		efi_fdt_gop();
		efi_fdt_memory_map();
	}

	efi_cleanup();

	if (efi_fdt_size() > 0) {
		efi_fdt_fini();
	}

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
