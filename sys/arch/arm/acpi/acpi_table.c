/* $NetBSD: acpi_table.c,v 1.1 2018/10/12 22:15:04 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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
 * ACPI table functions for use in early boot before ACPICA can be safely
 * initialized.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_table.c,v 1.1 2018/10/12 22:15:04 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <machine/acpi_machdep.h>

#include <arch/arm/acpi/acpi_table.h>

ACPI_STATUS
acpi_table_map(ACPI_PHYSICAL_ADDRESS pa, void **hdrp)
{
	ACPI_TABLE_HEADER *header;
	ACPI_STATUS rv;
	UINT32 length;

	rv = acpi_md_OsMapMemory(pa, sizeof(*header), (void **)&header);
	if (ACPI_FAILURE(rv))
		return rv;
	length = header->Length;
	acpi_md_OsUnmapMemory(header, sizeof(*header));

	return acpi_md_OsMapMemory(pa, length, hdrp);
}

void
acpi_table_unmap(ACPI_TABLE_HEADER *hdrp)
{
	acpi_md_OsUnmapMemory(hdrp, hdrp->Length);
}

ACPI_STATUS
acpi_table_find(const char *sig, void **hdrp)
{
	ACPI_TABLE_RSDP *rsdp;
	ACPI_TABLE_XSDT *xsdt;
	ACPI_TABLE_HEADER *header;
	ACPI_PHYSICAL_ADDRESS pa;
	ACPI_STATUS rv;

	/* RDSP contains a pointer to the XSDT */
	pa = 0;
	rv = acpi_md_OsMapMemory(acpi_md_OsGetRootPointer(), sizeof(*rsdp), (void **)&rsdp);
	if (ACPI_FAILURE(rv))
		return rv;
	if (memcmp(rsdp->Signature, ACPI_SIG_RSDP, sizeof(rsdp->Signature)) == 0)
		pa = rsdp->XsdtPhysicalAddress;
	acpi_md_OsUnmapMemory(rsdp, sizeof(*rsdp));
	if (pa == 0)
		return AE_NOT_FOUND;

	/* XSDT contains pointers to other tables */
	rv = acpi_table_map(pa, (void **)&xsdt);
	if (ACPI_FAILURE(rv))
		return rv;
	const u_int entries = (xsdt->Header.Length - sizeof(ACPI_TABLE_HEADER)) / ACPI_XSDT_ENTRY_SIZE;
	for (u_int n = 0; n < entries; n++) {
		rv = acpi_table_map(xsdt->TableOffsetEntry[n], (void **)&header);
		if (ACPI_FAILURE(rv))
			continue;
		if (memcmp(header->Signature, sig, sizeof(header->Signature)) == 0) {
			acpi_table_unmap((ACPI_TABLE_HEADER *)xsdt);
			*hdrp = header;
			return AE_OK;
		}
	}

	/* Not found */
	acpi_table_unmap((ACPI_TABLE_HEADER *)xsdt);
	return AE_NOT_FOUND;
}
