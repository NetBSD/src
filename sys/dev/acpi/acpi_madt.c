/*	$NetBSD: acpi_madt.c,v 1.6 2003/07/14 15:47:00 lukem Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_madt.c,v 1.6 2003/07/14 15:47:00 lukem Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/malloc.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_osd.h>
#include <dev/acpi/acpica/Subsystem/actables.h>
#include <dev/acpi/acpica/Subsystem/acnamesp.h>
#include <dev/acpi/acpi_madt.h>

#ifdef ACPI_MADT_DEBUG
static void acpi_madt_print(void);
static ACPI_STATUS acpi_madt_print_entry(APIC_HEADER *, void *);
static void acpi_print_apic_proc(PROCESSOR_APIC *);
static void acpi_print_ioapic(IO_APIC *);
static void acpi_print_intsrc_ovr(INT_SOURCE_OVERRIDE *);
static void acpi_print_intsrc_nmi(INT_IOAPIC_SOURCE_NMI *);
static void acpi_print_lapic_nmi(INT_LAPIC_SOURCE_NMI *);
static void acpi_print_lapic_addr_ovr(LAPIC_ADDR_OVR *);
static void acpi_print_iosapic(IO_SAPIC *);
static void acpi_print_local_sapic(LOCAL_SAPIC *);
static void acpi_print_platint(INT_PLATFORM *);
#endif

ACPI_TABLE_HEADER *AcpiGbl_MADT;

ACPI_STATUS
acpi_madt_map(void)
{
	ACPI_TABLE_HEADER *hdrp, header;
	XSDT_DESCRIPTOR *xp;
	ACPI_POINTER Address;
	ACPI_STATUS  Status = AE_OK;
	int i, len;

	if (AcpiGbl_XSDT == NULL)
		return AE_NO_ACPI_TABLES;

	if (AcpiGbl_MADT != NULL)
		return AE_ALREADY_EXISTS;

	xp = AcpiGbl_XSDT;
	hdrp = &AcpiGbl_XSDT->Header;

	Address.PointerType = AcpiGbl_TableFlags | ACPI_LOGICAL_ADDRESSING;

	len = (hdrp->Length - sizeof (ACPI_TABLE_HEADER))
		/ sizeof (xp->TableOffsetEntry[0]);

	for (i = 0; i < len; i++) {
		Address.Pointer.Value =
		    ACPI_GET_ADDRESS(AcpiGbl_XSDT->TableOffsetEntry[i]);
		Status = AcpiTbGetTableHeader(&Address, &header);
		if (ACPI_FAILURE (Status))
			return Status;
		if (!strncmp(header.Signature, APIC_SIG, 4)) {
			Status = AcpiOsMapMemory(Address.Pointer.Value,
			    (ACPI_SIZE)header.Length, (void *)&AcpiGbl_MADT);
			if (ACPI_FAILURE (Status))
				return Status;
			else
				break;
		}
	}
#ifdef ACPI_MADT_DEBUG
	if (AcpiGbl_MADT != NULL)
		acpi_madt_print();
#endif
	return AcpiGbl_MADT != NULL ? AE_OK : AE_NOT_FOUND;
}

void
acpi_madt_unmap(void)
{
	AcpiOsUnmapMemory(AcpiGbl_MADT, AcpiGbl_MADT->Length);
	AcpiGbl_MADT = NULL;
}

#ifdef ACPI_MADT_DEBUG

static void
acpi_print_apic_proc(PROCESSOR_APIC *p)
{
	printf("processor: id %u apid %u enabled: %s\n",
	    p->ProcessorApicId, p->LocalApicId,
	    p->ProcessorEnabled ? "yes" : "no");
}

static void
acpi_print_ioapic(IO_APIC *p)
{
	printf("ioapic: apid %u address %x vector base %x\n",
	    p->IoApicId, p->IoApicAddress, p->Vector);
}

static void
acpi_print_intsrc_ovr(INT_SOURCE_OVERRIDE *p)
{
	printf("int override: bus %u irq %u int %u\n",
	    p->Bus, p->Source, p->GlobalInt);
}

static void
acpi_print_intsrc_nmi(INT_IOAPIC_SOURCE_NMI *p)
{
	printf("ioapic NMI: int %u\n", p->GlobalInt);
}

static void
acpi_print_lapic_nmi(INT_LAPIC_SOURCE_NMI *p)
{
	printf("lapic NMI: apid %u input %u\n", p->ApicId, p->Lint);
}

static void
acpi_print_lapic_addr_ovr(LAPIC_ADDR_OVR *p)
{
	printf("lapic addr override: %llx\n", p->LocalApicAddress);
}

static void
acpi_print_iosapic(IO_SAPIC *p)
{
	printf("iosapic: sapid %u address %llx vector base %x\n",
	    p->IoSapicId, p->IoSapicAddress, p->Vector);
}

static void
acpi_print_local_sapic(LOCAL_SAPIC *p)
{
	printf("local sapic: cpu %u sapid %u sapeid %u enabled: %s\n",
	    p->ProcessorApicId, p->LocalSapicId, p->LocalSapicEid,
	    p->ProcessorEnabled ? "yes" : "no");
}

static void
acpi_print_platint(INT_PLATFORM *p)
{
	printf("platform int: type %u apid %u apeid %u\n",
	    p->Type, p->ProcessorId, p->ProcessorEid);
}

#endif

void
acpi_madt_walk(ACPI_STATUS (*func)(APIC_HEADER *, void *), void *aux)
{
	char *madtend, *where;
	APIC_HEADER *hdrp;
	APIC_TABLE *ap;

	ap = (APIC_TABLE *)AcpiGbl_MADT;
	madtend = (char *)AcpiGbl_MADT + AcpiGbl_MADT->Length;
	where = (char *)AcpiGbl_MADT + sizeof (APIC_TABLE);
	while (where < madtend) {
		hdrp = (APIC_HEADER *)where;
		if (func(hdrp, aux) != AE_OK)
			break;
		where += hdrp->Length;
	}
}

#ifdef ACPI_MADT_DEBUG
void
acpi_madt_print(void)
{
	APIC_TABLE *ap;

	ap = (APIC_TABLE *)AcpiGbl_MADT;
	printf("\n\nACPI MADT table:\n");
	printf("default local APIC address: %x\n", ap->LocalApicAddress);
	printf("system dual 8259%s present\n",
	    ap->PCATCompat ? "" : " not");
	printf("entries:\n");

	acpi_madt_walk(acpi_madt_print_entry, NULL);
}

static ACPI_STATUS
acpi_madt_print_entry(APIC_HEADER *hdrp, void *aux)
{
	switch (hdrp->Type) {
	case APIC_PROC:
		acpi_print_apic_proc((void *)hdrp);
		break;
	case APIC_IO:
		acpi_print_ioapic((void *)hdrp);
		break;
	case APIC_INTSRC_OVR:
		acpi_print_intsrc_ovr((void *)hdrp);
		break;
	case APIC_INTSRC_NMI:
		acpi_print_intsrc_nmi((void *)hdrp);
		break;
	case APIC_LAPIC_NMI:
		acpi_print_lapic_nmi((void *)hdrp);
		break;
	case APIC_ADDR_OVR:
		acpi_print_lapic_addr_ovr((void *)hdrp);
		break;
	case APIC_IOSAPIC:
		acpi_print_iosapic((void *)hdrp);
		break;
	case APIC_LOCAL_SAPIC:
		acpi_print_local_sapic((void *)hdrp);
		break;
	case APIC_PLAT_INTSRC:
		acpi_print_platint((void *)hdrp);
		break;
	default:
		printf("Unknown MADT entry type %d\n", hdrp->Type);
		break;
	}
	return AE_OK;
}
#endif
