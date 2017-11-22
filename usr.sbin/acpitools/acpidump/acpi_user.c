/* $NetBSD: acpi_user.c,v 1.2.38.1 2017/11/22 15:35:52 martin Exp $ */

/*-
 * Copyright (c) 1999 Doug Rabson
 * Copyright (c) 2000 Mitsuru IWASAKI <iwasaki@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$FreeBSD: src/usr.sbin/acpi/acpidump/acpi_user.c,v 1.15 2009/08/25 20:35:57 jhb Exp $
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: acpi_user.c,v 1.2.38.1 2017/11/22 15:35:52 martin Exp $");

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/sysctl.h>

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "acpidump.h"

static char	machdep_acpi_root[] = "hw.acpi.root";
static int      acpi_mem_fd = -1;

struct acpi_user_mapping {
	LIST_ENTRY(acpi_user_mapping) link;
	vm_offset_t     pa;
	caddr_t         va;
	size_t          size;
};

LIST_HEAD(acpi_user_mapping_list, acpi_user_mapping) maplist;

static void
acpi_user_init(void)
{

	if (acpi_mem_fd == -1) {
		acpi_mem_fd = open("/dev/mem", O_RDONLY);
		if (acpi_mem_fd == -1)
			err(EXIT_FAILURE, "opening /dev/mem");
		LIST_INIT(&maplist);
	}
}

static struct acpi_user_mapping *
acpi_user_find_mapping(vm_offset_t pa, size_t size)
{
	struct	acpi_user_mapping *map;

	/* First search for an existing mapping */
	for (map = LIST_FIRST(&maplist); map; map = LIST_NEXT(map, link)) {
		if (map->pa <= pa && map->size >= pa + size - map->pa)
			return map;
	}

	/* Then create a new one */
	size = round_page(pa + size) - trunc_page(pa);
	pa = trunc_page(pa);
	map = malloc(sizeof(struct acpi_user_mapping));
	if (!map)
		errx(EXIT_FAILURE, "out of memory");
	map->pa = pa;
	map->va = mmap(0, size, PROT_READ, MAP_SHARED, acpi_mem_fd, pa);
	map->size = size;
	if ((intptr_t) map->va == -1)
		err(EXIT_FAILURE, "can't map address");
	LIST_INSERT_HEAD(&maplist, map, link);

	return map;
}

static ACPI_TABLE_RSDP *
acpi_get_rsdp(u_long addr)
{
	ACPI_TABLE_RSDP rsdp;
	size_t len;

	/* Read in the table signature and check it. */
	pread(acpi_mem_fd, &rsdp, 8, addr);
	if (memcmp(rsdp.Signature, "RSD PTR ", 8))
		return (NULL);

	/* Read the entire table. */
	pread(acpi_mem_fd, &rsdp, sizeof(rsdp), addr);

	/* Check the standard checksum. */
	if (acpi_checksum(&rsdp, ACPI_RSDP_CHECKSUM_LENGTH) != 0)
		return (NULL);

	/* Check extended checksum if table version >= 2. */
	if (rsdp.Revision >= 2 &&
	    acpi_checksum(&rsdp, ACPI_RSDP_XCHECKSUM_LENGTH) != 0)
		return (NULL);

	/* If the revision is 0, assume a version 1 length. */
	if (rsdp.Revision == 0)
		len = ACPI_RSDP_REV0_SIZE;
	else
		len = rsdp.Length;

	return (acpi_map_physical(addr, len));
}

static ACPI_TABLE_RSDP *
acpi_scan_rsd_ptr(void)
{
#if defined(__amd64__) || defined(__i386__)
	ACPI_TABLE_RSDP *rsdp;
	u_long		addr, end;

	/*
	 * On ia32, scan physical memory for the RSD PTR if above failed.
	 * According to section 5.2.2 of the ACPI spec, we only consider
	 * two regions for the base address:
	 * 1. EBDA (1 KB area addressed by the 16 bit pointer at 0x40E
	 * 2. High memory (0xE0000 - 0xFFFFF)
	 */
	addr = ACPI_EBDA_PTR_LOCATION;
	pread(acpi_mem_fd, &addr, sizeof(uint16_t), addr);
	addr <<= 4;
	end = addr + ACPI_EBDA_WINDOW_SIZE;
	for (; addr < end; addr += 16)
		if ((rsdp = acpi_get_rsdp(addr)) != NULL)
			return rsdp;
	addr = ACPI_HI_RSDP_WINDOW_BASE;
	end = addr + ACPI_HI_RSDP_WINDOW_SIZE;
	for (; addr < end; addr += 16)
		if ((rsdp = acpi_get_rsdp(addr)) != NULL)
			return rsdp;
#endif /* __amd64__ || __i386__ */
	return NULL;
}

/*
 * Public interfaces
 */
ACPI_TABLE_RSDP *
acpi_find_rsd_ptr(void)
{
	ACPI_TABLE_RSDP	*rsdp;
	u_long		addr = 0;
	size_t		len = sizeof(addr);

	acpi_user_init();

	if (sysctlbyname(machdep_acpi_root, &addr, &len, NULL, 0) != 0)
		addr = 0;
	if (addr != 0 && (rsdp = acpi_get_rsdp(addr)) != NULL)
		return rsdp;

	return acpi_scan_rsd_ptr();
}

void *
acpi_map_physical(vm_offset_t pa, size_t size)
{
	struct	acpi_user_mapping *map;

	map = acpi_user_find_mapping(pa, size);
	return (map->va + (pa - map->pa));
}

ACPI_TABLE_HEADER *
dsdt_load_file(char *infile)
{
	ACPI_TABLE_HEADER *sdt;
	uint8_t		*dp;
	struct stat	 sb;

	if ((acpi_mem_fd = open(infile, O_RDONLY)) == -1)
		errx(EXIT_FAILURE, "opening %s", infile);

	LIST_INIT(&maplist);

	if (fstat(acpi_mem_fd, &sb) == -1)
		errx(EXIT_FAILURE, "fstat %s", infile);

	dp = mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, acpi_mem_fd, 0);
	if (dp == NULL)
		errx(EXIT_FAILURE, "mmap %s", infile);

	sdt = (ACPI_TABLE_HEADER *)dp;
	if (strncmp((char *)dp, ACPI_SIG_DSDT, 4) != 0)
	    errx(EXIT_FAILURE, "DSDT signature mismatch");

	if (sdt->Length > sb.st_size)
	    errx(EXIT_FAILURE,
		"corrupt DSDT: table size (%"PRIu32" bytes), file size "
		"(%"PRIu64" bytes)",
		sdt->Length, sb.st_size);

        if (acpi_checksum(sdt, sdt->Length) != 0)
	    errx(EXIT_FAILURE, "DSDT checksum mismatch");

	return sdt;
}
