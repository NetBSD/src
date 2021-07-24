/* $NetBSD: acpi_dev.c,v 1.2 2021/07/24 11:36:41 jmcneill Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_dev.c,v 1.2 2021/07/24 11:36:41 jmcneill Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/mman.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#define	_COMPONENT	ACPI_BUS_COMPONENT
ACPI_MODULE_NAME	("acpi_dev")

static dev_type_read(acpi_read);

const struct cdevsw acpi_cdevsw = {
	.d_open		= nullopen,
	.d_close	= nullclose,
	.d_read		= acpi_read,
	.d_write	= nowrite,
	.d_ioctl	= noioctl,
	.d_stop		= nostop,
	.d_tty		= notty,
	.d_poll		= nopoll,
	.d_mmap		= nommap,
	.d_kqfilter	= nokqfilter,
	.d_discard	= nodiscard,
	.d_flag		= D_OTHER | D_MPSAFE,
};

/*
 * acpi_find_table_rsdp --
 *
 * 	Returns true if the RSDP table is found and overlaps the specified
 * 	physical address. The table's physical start address and length
 * 	are placed in 'paddr' and 'plen' when found.
 *
 */
static bool
acpi_find_table_rsdp(ACPI_PHYSICAL_ADDRESS pa,
    ACPI_PHYSICAL_ADDRESS *paddr, uint32_t *plen)
{
	ACPI_PHYSICAL_ADDRESS table_pa;

	table_pa = AcpiOsGetRootPointer();
	if (table_pa == 0) {
		return false;
	}
	if (pa >= table_pa && pa < table_pa + sizeof(ACPI_TABLE_RSDP)) {
		*paddr = table_pa;
		*plen = sizeof(ACPI_TABLE_RSDP);
		return true;
	}

	return false;
}

/*
 * acpi_find_table_sdt --
 *
 * 	Returns true if the XSDT/RSDT table is found and overlaps the
 * 	specified physical address. The table's physical start address
 * 	and length are placed in 'paddr' and 'plen' when found.
 *
 */
static bool
acpi_find_table_sdt(ACPI_PHYSICAL_ADDRESS pa,
    ACPI_PHYSICAL_ADDRESS *paddr, uint32_t *plen)
{
	ACPI_PHYSICAL_ADDRESS table_pa;
	ACPI_TABLE_RSDP *rsdp;
	ACPI_TABLE_HEADER *thdr;
	uint32_t table_len;

	table_pa = AcpiOsGetRootPointer();
	KASSERT(table_pa != 0);

	/*
	 * Find the XSDT/RSDT using the RSDP.
	 */
	rsdp = AcpiOsMapMemory(table_pa, sizeof(ACPI_TABLE_RSDP));
	if (rsdp == NULL) {
		return false;
	}
	if (rsdp->Revision > 1 && rsdp->XsdtPhysicalAddress) {
		table_pa = rsdp->XsdtPhysicalAddress;
	} else {
		table_pa = rsdp->RsdtPhysicalAddress;
	}
	AcpiOsUnmapMemory(rsdp, sizeof(ACPI_TABLE_RSDP));
	if (table_pa == 0) {
		return false;
	}

	/*
	 * Map the XSDT/RSDT and check access.
	 */
	thdr = AcpiOsMapMemory(table_pa, sizeof(ACPI_TABLE_HEADER));
	if (thdr == NULL) {
		return false;
	}
	table_len = thdr->Length;
	AcpiOsUnmapMemory(thdr, sizeof(ACPI_TABLE_HEADER));
	if (pa >= table_pa && pa < table_pa + table_len) {
		*paddr = table_pa;
		*plen = table_len;
		return true;
	}

	return false;
}

/*
 * acpi_find_table --
 *
 * 	Find an ACPI table that overlaps the specified physical address.
 * 	Returns true if the table is found and places the table start
 * 	address into 'paddr' and the length into 'plen'.
 *
 */
static bool
acpi_find_table(ACPI_PHYSICAL_ADDRESS pa,
    ACPI_PHYSICAL_ADDRESS *paddr, uint32_t *plen)
{
	ACPI_TABLE_DESC *tdesc;
	bool found_table;
	uint32_t i;

	/* Check for RSDP access. */
	if (acpi_find_table_rsdp(pa, paddr, plen)) {
		return true;
	}

	/* Check for XSDT/RSDT access. */
	if (acpi_find_table_sdt(pa, paddr, plen)) {
		return true;
	}

	/* Check for root table access. */
	found_table = false;
	AcpiUtAcquireMutex(ACPI_MTX_TABLES);
	for (i = 0; i < AcpiGbl_RootTableList.CurrentTableCount; i++) {
		tdesc = &AcpiGbl_RootTableList.Tables[i];
		if (pa >= tdesc->Address &&
		    pa < tdesc->Address + tdesc->Length) {
			*paddr = tdesc->Address;
			*plen = tdesc->Length;
			found_table = true;
			break;
		}
	}
	AcpiUtReleaseMutex(ACPI_MTX_TABLES);
	return found_table;
}

/*
 * acpi_read --
 *
 * 	Read data from an ACPI configuration table that resides in
 * 	physical memory. Only supports reading one table at a time.
 *
 */
static int
acpi_read(dev_t dev, struct uio *uio, int flag)
{
	ACPI_PHYSICAL_ADDRESS pa, table_pa;
	uint32_t table_len;
	uint8_t *data;
	int error;
	size_t len;

	if (uio->uio_rw != UIO_READ) {
		return EPERM;
	}

	/* Make sure this is a read of a known table */
	if (!acpi_find_table(uio->uio_offset, &table_pa, &table_len)) {
		return EIO;
	}

	/* Copy the contents of the table to user-space */
	pa = uio->uio_offset;
	len = uimin(table_len - (pa - table_pa), uio->uio_resid);
	data = AcpiOsMapMemory(pa, len);
	if (data == NULL) {
		return ENOMEM;
	}
	error = uiomove(data, len, uio);
	AcpiOsUnmapMemory(data, len);

	return error;
}
