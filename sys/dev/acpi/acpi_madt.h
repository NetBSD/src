/*	$NetBSD: acpi_madt.h,v 1.1 2003/01/05 01:03:44 fvdl Exp $	*/

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

/*
 * MADT entry types not defined in actbl.h
 */
#define APIC_INTSRC_OVR		2
#define APIC_INTSRC_NMI		3
#define APIC_LAPIC_NMI		4
#define APIC_ADDR_OVR		5
#define APIC_IOSAPIC		6
#define APIC_LOCAL_SAPIC	7
#define APIC_PLAT_INTSRC	8

#pragma pack(1)

/*
 * MADT entry structures not defined in actbl.h
 */

/*
 * Interrupt source override.
 */
typedef struct
{
    APIC_HEADER		Header;
    UINT8		Bus;		/* 0 (ISA) */
    UINT8		Source;		/* Legacy IRQ */
    UINT32		GlobalInt;	/* Glocal IO APIC int */
    UINT32_BIT		Polarity:2;	/* See MPS_INTPO* in mpbiosreg.h */
    UINT32_BIT		Trigger:2;	/* See MPS_INTTR* in mpbiosreg.h */
    UINT32_BIT		Reserved:12;	/* Must be zero */
} INT_SOURCE_OVERRIDE;

/*
 * I/O APIC NMI source structure.
 */
typedef struct
{
    APIC_HEADER		Header;
    UINT32_BIT		Polarity:2;	/* See above */
    UINT32_BIT		Trigger:2;	/* See above */
    UINT32_BIT		Reserved:12;
    UINT32		GlobalInt;
} INT_IOAPIC_SOURCE_NMI;

/*
 * Local APIC NMI mapping.
 */
typedef struct
{
    APIC_HEADER		Header;
    UINT8		ApicId;		/* Local APIC Id */
    UINT32_BIT		Polarity:2;	/* See above */
    UINT32_BIT		Trigger:2;	/* See above */
    UINT32_BIT		Reserved:12;
    UINT8		Lint;		/* Local APIC input */
} INT_LAPIC_SOURCE_NMI;

/*
 * 64-bit local APIC address override.
 */
typedef struct
{
    APIC_HEADER		Header;
    UINT16		Reserved;		/* Must be zero */
    UINT64		LocalApicAddress;	/* Phys addr of local APIC */
} LAPIC_ADDR_OVR;

/*
 * I/O SAPIC.
 */
typedef struct
{
    APIC_HEADER		Header;
    UINT8		IoSapicId;
    UINT8		Reserved;
    UINT32		Vector;			/* Vectors start at this off */
    UINT64		IoSapicAddress;		/* Phys addr of SAPIC */
} IO_SAPIC;

/*
 * Local SAPIC.
 */
typedef struct
{
    APIC_HEADER		Header;
    UINT8		ProcessorApicId;
    UINT8		LocalSapicId;
    UINT8		LocalSapicEid;
    UINT8		Reserved[3];
    UINT32_BIT		ProcessorEnabled:1;
    UINT32_BIT		Reserved1:31;
} LOCAL_SAPIC;

/*
 * Platform interrupt source.
 */
typedef struct
{
    APIC_HEADER		Header;
    UINT32_BIT		Polarity:2;	/* See above */
    UINT32_BIT		Trigger:2;	/* See above */
    UINT32_BIT		Reserved:12;
    UINT8		Type;		/* See below */
    UINT8		ProcessorId;
    UINT8		ProcessorEid;
    UINT8		IoSapicVector;
    UINT32		GlobalInt;
    UINT32		Reserved1;
} INT_PLATFORM;

#define ACPI_PLATFORM_INT_PMI	1
#define ACPI_PLATFORM_INT_INIT	2
#define ACPI_PLATFORM_INT_CERR	3

extern ACPI_TABLE_HEADER *AcpiGbl_MADT;

ACPI_STATUS acpi_madt_map(void);
void acpi_madt_unmap(void);
void acpi_madt_walk(ACPI_STATUS (*)(APIC_HEADER *));
