/*	$NetBSD: acpi_madt.c,v 1.13 2004/05/01 12:03:48 kochi Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: acpi_madt.c,v 1.13 2004/05/01 12:03:48 kochi Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/malloc.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_madt.h>

#ifdef ACPI_MADT_DEBUG
static void acpi_madt_print(void);
static ACPI_STATUS acpi_madt_print_entry(APIC_HEADER *, void *);
static void acpi_print_apic_proc(MADT_PROCESSOR_APIC *);
static void acpi_print_ioapic(MADT_IO_APIC *);
static void acpi_print_intsrc_ovr(MADT_INTERRUPT_OVERRIDE *);
static void acpi_print_intsrc_nmi(MADT_NMI_SOURCE *);
static void acpi_print_lapic_nmi(MADT_LOCAL_APIC_NMI *);
static void acpi_print_lapic_addr_ovr(MADT_ADDRESS_OVERRIDE *);
static void acpi_print_iosapic(MADT_IO_SAPIC *);
static void acpi_print_local_sapic(MADT_LOCAL_SAPIC *);
static void acpi_print_platint(MADT_INTERRUPT_SOURCE *);
#endif

static ACPI_TABLE_HEADER *madt_header;

ACPI_STATUS
acpi_madt_map(void)
{
	ACPI_STATUS  rv;

	if (madt_header != NULL)
		return AE_ALREADY_EXISTS;

	rv = AcpiGetFirmwareTable(APIC_SIG, 1, ACPI_LOGICAL_ADDRESSING,
	    &madt_header);

	if (ACPI_FAILURE(rv))
		return rv;

#ifdef ACPI_MADT_DEBUG
	acpi_madt_print();
#endif

	return AE_OK;
}

void
acpi_madt_unmap(void)
{
	AcpiOsUnmapMemory(madt_header, madt_header->Length);
	madt_header = NULL;
}

#ifdef ACPI_MADT_DEBUG

static void
acpi_print_apic_proc(MADT_PROCESSOR_APIC *p)
{
	printf("processor: id %u apid %u enabled: %s\n",
	    p->ProcessorId, p->LocalApicId,
	    p->ProcessorEnabled ? "yes" : "no");
}

static void
acpi_print_ioapic(MADT_IO_APIC *p)
{
	printf("ioapic: apid %u address %x vector base %x\n",
	    p->IoApicId, p->Address, p->Interrupt);
}

static void
acpi_print_intsrc_ovr(MADT_INTERRUPT_OVERRIDE *p)
{
	printf("int override: bus %u irq %u int %u\n",
	    p->Bus, p->Source, p->Interrupt);
}

static void
acpi_print_intsrc_nmi(MADT_NMI_SOURCE *p)
{
	printf("ioapic NMI: int %u\n", p->Interrupt);
}

static void
acpi_print_lapic_nmi(MADT_LOCAL_APIC_NMI *p)
{
	printf("lapic NMI: apid %u input %u\n", p->ProcessorId, p->Lint);
}

static void
acpi_print_lapic_addr_ovr(MADT_ADDRESS_OVERRIDE *p)
{
	printf("lapic addr override: %llx\n", (unsigned long long)p->Address);
}

static void
acpi_print_iosapic(MADT_IO_SAPIC *p)
{
	printf("iosapic: sapid %u address %llx vector base %x\n",
	    p->IoSapicId, p->Address, p->InterruptBase);
}

static void
acpi_print_local_sapic(MADT_LOCAL_SAPIC *p)
{
	printf("local sapic: CPU %u sapid %u sapeid %u enabled: %s\n",
	    p->ProcessorId, p->LocalSapicId, p->LocalSapicEid,
	    p->ProcessorEnabled ? "yes" : "no");
}

static void
acpi_print_platint(MADT_INTERRUPT_SOURCE *p)
{
	printf("platform int: type %u apid %u apeid %u\n",
	    p->InterruptType, p->ProcessorId, p->ProcessorEid);
}

#endif

void
acpi_madt_walk(ACPI_STATUS (*func)(APIC_HEADER *, void *), void *aux)
{
	char *madtend, *where;
	APIC_HEADER *hdrp;

	madtend = (char *)madt_header + madt_header->Length;
	where = (char *)madt_header + sizeof (MULTIPLE_APIC_TABLE);
	while (where < madtend) {
		hdrp = (APIC_HEADER *)where;
		if (func(hdrp, aux) != AE_OK)
			break;
		where += hdrp->Length;
	}
}

#ifdef ACPI_MADT_DEBUG
static void
acpi_madt_print(void)
{
	MULTIPLE_APIC_TABLE *ap;

	ap = (MULTIPLE_APIC_TABLE *)madt_header;
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
	case APIC_PROCESSOR:
		acpi_print_apic_proc((void *)hdrp);
		break;
	case APIC_IO:
		acpi_print_ioapic((void *)hdrp);
		break;
	case APIC_XRUPT_OVERRIDE:
		acpi_print_intsrc_ovr((void *)hdrp);
		break;
	case APIC_NMI:
		acpi_print_intsrc_nmi((void *)hdrp);
		break;
	case APIC_LOCAL_NMI:
		acpi_print_lapic_nmi((void *)hdrp);
		break;
	case APIC_ADDRESS_OVERRIDE:
		acpi_print_lapic_addr_ovr((void *)hdrp);
		break;
	case APIC_IO_SAPIC:
		acpi_print_iosapic((void *)hdrp);
		break;
	case APIC_LOCAL_SAPIC:
		acpi_print_local_sapic((void *)hdrp);
		break;
	case APIC_XRUPT_SOURCE:
		acpi_print_platint((void *)hdrp);
		break;
	default:
		printf("Unknown MADT entry type %d\n", hdrp->Type);
		break;
	}
	return AE_OK;
}
#endif
