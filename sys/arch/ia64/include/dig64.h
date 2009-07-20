/*	$NetBSD: dig64.h,v 1.2 2009/07/20 04:41:37 kiyohara Exp $	*/

/*-
 * Copyright (c) 2002 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _MACHINE_DIG64_H_
#define	_MACHINE_DIG64_H_

/*
 * This header file written refer to 'DIG64 Desriptions for Primary Console &
 * Debug Port Devices'.
 */

/* ACPI GAS (Generic Address Structure) */
struct dig64_gas {
	uint8_t		addr_space;
	uint8_t		bit_width;
	uint8_t		bit_offset;
	uint8_t		_reserved_;
	/*
	 * XXX using a 64-bit type for the address would cause padding and
	 * using __packed would cause unaligned accesses...
	 */
	uint32_t	addr_low;
	uint32_t	addr_high;
};

struct dig64_hcdp_entry {
	uint8_t		type;
#define	DIG64_HCDP_CONSOLE		DIG64_ENTRYTYPE_TYPE0
#define	DIG64_HCDP_DBGPORT		DIG64_ENTRYTYPE_TYPE1
	uint8_t		databits;
	uint8_t		parity;
#define	DIG64_HCDP_PARITY_NO		1
#define	DIG64_HCDP_PARITY_EVEN		2
#define	DIG64_HCDP_PARITY_ODD		3
#define	DIG64_HCDP_PARITY_MARK		4
#define	DIG64_HCDP_PARITY_SPACE		5
	uint8_t		stopbits;
#define	DIG64_HCDP_STOPBITS_1		1
#define	DIG64_HCDP_STOPBITS_15		2
#define	DIG64_HCDP_STOPBITS_2		3
	uint8_t		pci_segment;
	uint8_t		pci_bus;
	uint8_t		pci_device:5;
	uint8_t		_reserved1_:3;
	uint8_t		pci_function:3;
	uint8_t		_reserved2_:3;
	uint8_t		interrupt:1;
	uint8_t		pci_flag:1;
	/*
	 * XXX using a 64-bit type for the baudrate would cause padding and
	 * using __packed would cause unaligned accesses...
	 */
	uint32_t	baud_low;
	uint32_t	baud_high;
	struct dig64_gas address;
	uint16_t	pci_devid;
	uint16_t	pci_vendor;
	uint32_t	irq;
	uint32_t	pclock;
	uint8_t		pci_interface;
	uint8_t		_reserved3_[7];
};


/* Device Specific Structures */

struct dig64_vga_spec {
	uint8_t		num;	/*Number of Extended Address Space Descriptors*/
	struct {
		uint8_t	data[56];
	} edesc[0];
} __packed;


/* Interconnect Specific Structure */

#define DIG64_FLAGS_INTR_LEVEL		(0 << 0)	/* Level Triggered */
#define DIG64_FLAGS_INTR_EDGE		(1 << 0)	/* Edge Triggered */
#define DIG64_FLAGS_INTR_ACTH		(0 << 1)	/* Intr Active High */
#define DIG64_FLAGS_INTR_ACTL		(1 << 1)	/* Intr Active Low */
#define DIG64_FLAGS_TRANS_DENSE		(0 << 3)	/* Dense Transration */
#define DIG64_FLAGS_TRANS_SPARSE	(1 << 3)	/* Sparse Transration */
#define DIG64_FLAGS_TYPE_STATIC		(0 << 4)	/* Type Static */
#define DIG64_FLAGS_TYPE_TRANS		(1 << 4)	/* Type Translation */
#define DIG64_FLAGS_INTR_SUPP		(1 << 6)	/* Intrrupt supported */
#define DIG64_FLAGS_MMIO_TRA_VALID	(1 << 8)
#define DIG64_FLAGS_IOPORT_TRA_VALID	(1 << 9)

struct dig64_acpi_spec {
	uint8_t		type;		/* = 0 indicating ACPI */
	uint8_t		resv;		/* must be 0 */
	uint16_t	length;		/* of the ACPI Specific Structure */
	uint32_t	uid;
	uint32_t	hid;
	uint32_t	acpi_gsi;	/* ACPI Global System Interrupt */
	uint64_t	mmio_tra;
	uint64_t	ioport_tra;
	uint16_t	flags;
} __packed;

struct dig64_pci_spec {
	uint8_t		type;		/* = 1 indicating PCI */
	uint8_t		resv;		/* must be 0 */
	uint16_t	length;		/* of the PCI Specific Structure */
	uint8_t		sgn;		/* PCI Segment Group Number */
	uint8_t		bus;		/* PCI Bus Number */
	uint8_t		device;		/* PCI Device Number */
	uint8_t		function;	/* PCI Function Number */
	uint16_t	device_id;
	uint16_t	vendor_id;
	uint32_t	acpi_gsi;	/* ACPI Global System Interrupt */
	uint64_t	mmio_tra;
	uint64_t	ioport_tra;
	uint16_t	flags;
} __packed;


struct dig64_pcdp_entry {
	uint8_t		type;
	uint8_t		primary;
	uint16_t	length;		/* in bytes */
	uint16_t	index;
#define	DIG64_PCDP_CONOUTDEV		0
#define	DIG64_PCDP_NOT_VALID		1
#define	DIG64_PCDP_CONOUTDEV2		2
#define	DIG64_PCDP_CONINDEV		3

	union {
		/*
		 * Interconnect Specific Structure,
		 *   and Device Specific Structure(s)
		 */
		uint8_t	type;
#define DIG64_PCDP_SPEC_ACPI		0
		struct dig64_acpi_spec acpi;
#define DIG64_PCDP_SPEC_PCI		1
		struct dig64_pci_spec pci;
	} specs;
} __packed;

struct dig64_hcdp_table {
	char		signature[4];
#define	HCDP_SIGNATURE	"HCDP"
	uint32_t	length;
	uint8_t		revision;	/* It is PCDP, if '3' or greater. */
	uint8_t		checksum;
	char		oem_id[6];
	char		oem_tbl_id[8];
	uint32_t	oem_rev;
	char		creator_id[4];
	uint32_t	creator_rev;
	uint32_t	entries;	/* Number of Type0 and Type1 Entries. */
	union dev_desc {	/* Device Descriptor */
		uint8_t type;
#define	DIG64_ENTRYTYPE_TYPE0		0	/* (UART | Bidirect) */
#define	DIG64_ENTRYTYPE_TYPE1		1	/* (UART | Debug Port) */
#define	DIG64_ENTRYTYPE_BIDIRECT	(0<<0)	/* bidirectional console */
#define	DIG64_ENTRYTYPE_DEBUGPORT	(1<<0)	/* debug port */
#define	DIG64_ENTRYTYPE_OUTONLY		(2<<0)	/* console output-only */
#define	DIG64_ENTRYTYPE_INONLY		(3<<0)	/* console input-only */
#define	DIG64_ENTRYTYPE_UART		(0<<3)
#define	DIG64_ENTRYTYPE_VGA		(1<<3)
#define	DIG64_ENTRYTYPE_VENDOR		(1<<7)	/* Vendor specific */
		struct dig64_hcdp_entry uart;
		struct dig64_pcdp_entry pcdp;
	} entry[0];
};

#endif
