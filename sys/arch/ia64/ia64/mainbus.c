/*	$NetBSD: mainbus.c,v 1.9.14.1 2017/12/03 11:36:20 jdolecek Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author:
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.9.14.1 2017/12/03 11:36:20 jdolecek Exp $");

#include "acpica.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#include <actables.h>


static int mainbus_match(device_t, cfdata_t, void *);
static void mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mainbus, 0, mainbus_match, mainbus_attach, NULL, NULL);


/*
 * Probe for the mainbus; always succeeds.
 */
static int
mainbus_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

/*
 * Attach the mainbus.
 */
static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
#if NACPICA > 0
	struct acpibus_attach_args aaa;
#endif
	ACPI_PHYSICAL_ADDRESS rsdp_ptr;
	ACPI_MADT_LOCAL_SAPIC *entry;
	ACPI_TABLE_MADT *table;
	ACPI_TABLE_RSDP *rsdp;
	ACPI_TABLE_XSDT *xsdt;
	char *end, *p;
	int tables, i;

	aprint_naive("\n");
	aprint_normal("\n");

	if ((rsdp_ptr = AcpiOsGetRootPointer()) == 0)
		panic("cpu not found");

	rsdp = (ACPI_TABLE_RSDP *)IA64_PHYS_TO_RR7(rsdp_ptr);
	xsdt = (ACPI_TABLE_XSDT *)IA64_PHYS_TO_RR7(rsdp->XsdtPhysicalAddress);

	tables = (UINT64 *)((char *)xsdt + xsdt->Header.Length) -
	    xsdt->TableOffsetEntry;

	for (i = 0; i < tables; i++) {
		int len;
		char *sig;

		table = (ACPI_TABLE_MADT *)
		    IA64_PHYS_TO_RR7(xsdt->TableOffsetEntry[i]);

		sig = table->Header.Signature;
		if (strncmp(sig, ACPI_SIG_MADT, ACPI_NAME_SIZE) != 0)
			continue;
		len = table->Header.Length;
		if (ACPI_FAILURE(AcpiTbChecksum((void *)table, len)))
			continue;

		end = (char *)table + table->Header.Length;
		p = (char *)(table + 1);
		while (p < end) {
			entry = (ACPI_MADT_LOCAL_SAPIC *)p;

			if (entry->Header.Type == ACPI_MADT_TYPE_LOCAL_SAPIC)
				config_found_ia(self, "cpubus", entry, 0);

			p += entry->Header.Length;
		}
	}

#if NACPICA > 0
	acpi_probe();

	aaa.aa_iot = IA64_BUS_SPACE_IO;
	aaa.aa_memt = IA64_BUS_SPACE_MEM;
	aaa.aa_pc = 0;
	aaa.aa_pciflags =
	    PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY |
	    PCI_FLAGS_MWI_OKAY;
	aaa.aa_ic = 0;
	aaa.aa_dmat = NULL;	/* XXX */
	aaa.aa_dmat64 = NULL;	/* XXX */
	config_found_ia(self, "acpibus", &aaa, 0);
#endif

	return;
}
