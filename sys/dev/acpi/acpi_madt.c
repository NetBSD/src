/*	$NetBSD: acpi_madt.c,v 1.13.12.2 2008/01/21 09:42:30 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: acpi_madt.c,v 1.13.12.2 2008/01/21 09:42:30 yamt Exp $");

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
static ACPI_STATUS acpi_madt_print_entry(ACPI_SUBTABLE_HEADER *, void *);
static void acpi_print_lapic(ACPI_MADT_LOCAL_APIC *);
static void acpi_print_ioapic(ACPI_MADT_IO_APIC *);
static void acpi_print_intsrc_ovr(ACPI_MADT_INTERRUPT_OVERRIDE *);
static void acpi_print_intsrc_nmi(ACPI_MADT_NMI_SOURCE *);
static void acpi_print_lapic_nmi(ACPI_MADT_LOCAL_APIC_NMI *);
static void acpi_print_lapic_ovr(ACPI_MADT_LOCAL_APIC_OVERRIDE *);
static void acpi_print_iosapic(ACPI_MADT_IO_SAPIC *);
static void acpi_print_local_sapic(ACPI_MADT_LOCAL_SAPIC *);
static void acpi_print_platint(ACPI_MADT_INTERRUPT_SOURCE *);
#endif

static ACPI_TABLE_HEADER *madt_header;

ACPI_STATUS
acpi_madt_map(void)
{
	ACPI_STATUS  rv;

	if (madt_header != NULL)
		return AE_ALREADY_EXISTS;

	rv = AcpiGetTable(ACPI_SIG_MADT, 1, &madt_header);

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
	madt_header = NULL;
}

#ifdef ACPI_MADT_DEBUG

static void
acpi_print_lapic(ACPI_MADT_LOCAL_APIC *p)
{
	printf("lapic: processor id %u apid %u enabled: %s\n",
	    p->ProcessorId, p->Id,
	    (p->LapicFlags & ACPI_MADT_ENABLED) ? "yes" : "no");
}

static void
acpi_print_ioapic(ACPI_MADT_IO_APIC *p)
{
	printf("ioapic: apid %u address 0x%x vector base 0x%x\n",
	    p->Id, p->Address, p->GlobalIrqBase);
}

static void
acpi_print_intsrc_ovr(ACPI_MADT_INTERRUPT_OVERRIDE *p)
{
	printf("int override: bus %u src int %u global int %u\n",
	    p->Bus, p->SourceIrq, p->GlobalIrq);
}

static void
acpi_print_intsrc_nmi(ACPI_MADT_NMI_SOURCE *p)
{
	printf("ioapic NMI: int %u\n", p->GlobalIrq);
}

static void
acpi_print_lapic_nmi(ACPI_MADT_LOCAL_APIC_NMI *p)
{
	printf("lapic NMI: cpu id %u input %u\n", p->ProcessorId, p->Lint);
}

static void
acpi_print_lapic_ovr(ACPI_MADT_LOCAL_APIC_OVERRIDE *p)
{
	printf("lapic addr override: 0x%llx\n", (unsigned long long)p->Address);
}

static void
acpi_print_iosapic(ACPI_MADT_IO_SAPIC *p)
{
	printf("iosapic: sapid %u address 0x%llx int vector base 0x%x\n",
	    p->Id, (unsigned long long)p->Address, p->GlobalIrqBase);
}

static void
acpi_print_local_sapic(ACPI_MADT_LOCAL_SAPIC *p)
{
	printf("local sapic: cpu id %u sapid %u sapeid %u enabled: %s\n",
	    p->ProcessorId, p->Id, p->Eid,
	    (p->LapicFlags & ACPI_MADT_ENABLED) ? "yes" : "no");
}

static void
acpi_print_platint(ACPI_MADT_INTERRUPT_SOURCE *p)
{
	printf("platform int: type %u cpu id %u cpu eid %u vector %u int %u%s\n",
	    p->Type, p->Id, p->Eid, p->IoSapicVector, p->GlobalIrq,
	    (p->Flags & ACPI_MADT_CPEI_OVERRIDE) ? " CPEI" : "");
}

#endif

void
acpi_madt_walk(ACPI_STATUS (*func)(ACPI_SUBTABLE_HEADER *, void *), void *aux)
{
	char *madtend, *where;
	ACPI_SUBTABLE_HEADER *hdrp;

	madtend = (char *)madt_header + madt_header->Length;
	where = (char *)madt_header + sizeof (ACPI_TABLE_MADT);
	while (where < madtend) {
		hdrp = (ACPI_SUBTABLE_HEADER *)where;
		if (ACPI_FAILURE(func(hdrp, aux)))
			break;
		where += hdrp->Length;
	}
}

#ifdef ACPI_MADT_DEBUG
static void
acpi_madt_print(void)
{
	ACPI_TABLE_MADT *ap;

	ap = (ACPI_TABLE_MADT *)madt_header;
	printf("\n\nACPI MADT table:\n");
	/* printf("default local APIC address: %x\n", ap->LocalApicAddress); */
	printf("system dual 8259%s present\n",
	    (ap->Flags & ACPI_MADT_PCAT_COMPAT) ? "" : " not");
	printf("entries:\n");

	acpi_madt_walk(acpi_madt_print_entry, NULL);
}

static ACPI_STATUS
acpi_madt_print_entry(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	switch (hdrp->Type) {
	case ACPI_MADT_TYPE_LOCAL_APIC:
		acpi_print_lapic((void *)hdrp);
		break;
	case ACPI_MADT_TYPE_IO_APIC:
		acpi_print_ioapic((void *)hdrp);
		break;
	case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
		acpi_print_intsrc_ovr((void *)hdrp);
		break;
	case ACPI_MADT_TYPE_NMI_SOURCE:
		acpi_print_intsrc_nmi((void *)hdrp);
		break;
	case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
		acpi_print_lapic_nmi((void *)hdrp);
		break;
	case ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE:
		acpi_print_lapic_ovr((void *)hdrp);
		break;
	case ACPI_MADT_TYPE_IO_SAPIC:
		acpi_print_iosapic((void *)hdrp);
		break;
	case ACPI_MADT_TYPE_LOCAL_SAPIC:
		acpi_print_local_sapic((void *)hdrp);
		break;
	case ACPI_MADT_TYPE_INTERRUPT_SOURCE:
		acpi_print_platint((void *)hdrp);
		break;
	default:
		printf("Unknown MADT entry type %d\n", hdrp->Type);
		break;
	}
	return AE_OK;
}
#endif
