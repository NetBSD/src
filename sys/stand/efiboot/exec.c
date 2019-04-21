/* $NetBSD: exec.c,v 1.10 2019/04/21 22:30:41 thorpej Exp $ */

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
#include "efienv.h"
#include "efifdt.h"
#include "efiacpi.h"

#include <sys/reboot.h>

u_long load_offset = 0;

#define	FDT_SPACE	(4 * 1024 * 1024)
#define	FDT_ALIGN	((2 * 1024 * 1024) - 1)

static EFI_PHYSICAL_ADDRESS initrd_addr, dtb_addr;
static u_long initrd_size = 0, dtb_size = 0;

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

static const char default_efibootplist_path[] = "/etc/efiboot.plist";

/* This is here because load_file() is here. */
void
load_efibootplist(bool default_fallback)
{
	EFI_PHYSICAL_ADDRESS plist_addr = 0;
	u_long plist_size = 0;
	prop_dictionary_t plist = NULL, oplist = NULL;
	bool load_quietly = false;

	const char *path = get_efibootplist_path();
	if (path == NULL || strlen(path) == 0) {
		if (!default_fallback)
			return;
		path = default_efibootplist_path;
		load_quietly = true;
	}

	/*
	 * Fudge the size so we can ensure the resulting buffer
	 * is NUL-terminated for convenience.
	 */
	if (load_file(path, 1, load_quietly, &plist_addr, &plist_size) != 0 ||
	    plist_addr == 0) {
		/* Error messages have already been displayed. */
		goto out;
	}
	char *plist_buf = (char *)((uintptr_t)plist_addr);
	plist_buf[plist_size - 1] = '\0';

	plist = prop_dictionary_internalize(plist_buf);
	if (plist == NULL) {
		printf("boot: unable to parse plist '%s'\n", path);
		goto out;
	}

out:
	oplist = efibootplist;

	/*
	 * If we had a failure, create an empty one for
	 * convenience.  But a failure should not clobber
	 * an in-memory plist we already have.
	 */
	if (plist == NULL &&
	    (oplist == NULL || prop_dictionary_count(oplist) == 0))
		plist = prop_dictionary_create();

#ifdef EFIBOOT_DEBUG
	printf(">> load_efibootplist: oplist = 0x%lx, plist = 0x%lx\n",
	    (u_long)oplist, (u_long)plist);
#endif

	if (plist_addr) {
		uefi_call_wrapper(BS->FreePages, 2, plist_addr,
		    EFI_SIZE_TO_PAGES(plist_size));
	}

	if (plist) {
		efibootplist = plist;
		efi_env_from_efibootplist();

		if (oplist)
			prop_object_release(oplist);
	}
}

static void
apply_overlay(void *dtbo)
{

	if (!efi_fdt_overlay_is_compatible(dtbo)) {
		printf("boot: incompatible overlay\n");
	}

	int fdterr;

	if (efi_fdt_overlay_apply(dtbo, &fdterr) != 0) {
		printf("boot: error %d applying overlay\n", fdterr);
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

	apply_overlay((void *)(uintptr_t)dtbo_addr);

out:
	if (dtbo_addr) {
		uefi_call_wrapper(BS->FreePages, 2, dtbo_addr,
		    EFI_SIZE_TO_PAGES(dtbo_size));
	}
}

#define	DT_OVERLAYS_PROP	"device-tree-overlays"

static void
load_fdt_overlays(void)
{
	/*
	 * We support loading device tree overlays specified in efiboot.plist
	 * using the following schema:
	 *
	 *	<key>device-tree-overlays</key>
	 *	<array>
	 *		<string>/path/to/some/overlay.dtbo</string>
	 *		<string>hd0e:/some/other/overlay.dtbo</string>
	 *	</array>
	 *
	 * The overlays are loaded in array order.
	 */
	prop_array_t overlays = prop_dictionary_get(efibootplist,
	    DT_OVERLAYS_PROP);
	if (overlays == NULL) {
#ifdef EFIBOOT_DEBUG
		printf("boot: no device-tree-overlays\n");
#endif
		return;
	}
	if (prop_object_type(overlays) != PROP_TYPE_ARRAY) {
		printf("boot: invalid %s\n", DT_OVERLAYS_PROP);
		return;
	}

	prop_object_iterator_t iter = prop_array_iterator(overlays);
	prop_string_t pathobj;
	while ((pathobj = prop_object_iterator_next(iter)) != NULL) {
		if (prop_object_type(pathobj) != PROP_TYPE_STRING) {
			printf("boot: invalid %s entry\n", DT_OVERLAYS_PROP);
			continue;
		}
		apply_overlay_file(prop_string_cstring_nocopy(pathobj));
	}
	prop_object_iterator_release(iter);
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
		efi_fdt_init((marks[MARK_END] + FDT_ALIGN) & ~FDT_ALIGN, FDT_ALIGN + 1);
		load_fdt_overlays();
		efi_fdt_initrd(initrd_addr, initrd_size);
		efi_fdt_bootargs(args);
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
