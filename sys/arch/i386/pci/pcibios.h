/*	$NetBSD: pcibios.h,v 1.12 2007/12/25 18:33:33 perry Exp $	*/

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

/*
 * Data structure definitions for the PCI BIOS interface.
 */

/*
 * PCI BIOS return codes.
 */
#define	PCIBIOS_SUCCESS			0x00
#define	PCIBIOS_SERVICE_NOT_PRESENT	0x80
#define	PCIBIOS_FUNCTION_NOT_SUPPORTED	0x81
#define	PCIBIOS_BAD_VENDOR_ID		0x83
#define	PCIBIOS_DEVICE_NOT_FOUND	0x86
#define	PCIBIOS_BAD_REGISTER_NUMBER	0x87
#define	PCIBIOS_SET_FAILED		0x88
#define	PCIBIOS_BUFFER_TOO_SMALL	0x89

/*
 * PCI IRQ Routing Table definitions.
 */

/*
 * Slot entry (per PCI 2.1)
 */
struct pcibios_linkmap {
	uint8_t		link;
	uint16_t	bitmap;
} __packed;

struct pcibios_intr_routing {
	uint8_t		bus;
	uint8_t		device;
	struct pcibios_linkmap linkmap[4];	/* INT[A:D]# */
	uint8_t		slot;
	uint8_t		reserved;
} __packed;

/*
 * $PIR header.  Reference:
 *
 *	http://www.microsoft.com/whdc/hwdev/archive/BUSBIOS/pciirq.mspx
 */
struct pcibios_pir_header {
	uint32_t	signature;		/* $PIR */
	uint16_t	version;
	uint16_t	tablesize;
	uint8_t		router_bus;
	uint8_t		router_devfunc;
	uint16_t	exclusive_irq;
	uint32_t	compat_router;		/* PCI vendor/product */
	uint32_t	miniport;
	uint8_t		reserved[11];
	uint8_t		checksum;
} __packed;

#define	PIR_DEVFUNC_DEVICE(devfunc)	(((devfunc) >> 3) & 0x1f)
#define	PIR_DEVFUNC_FUNCTION(devfunc)	((devfunc) & 7)

void	pcibios_init(void);

extern struct pcibios_pir_header pcibios_pir_header;
extern struct pcibios_intr_routing *pcibios_pir_table;
extern int pcibios_pir_table_nentries;
extern int pcibios_max_bus;

#ifdef PCIBIOSVERBOSE
extern int pcibiosverbose;

#define	PCIBIOS_PRINTV(arg) \
	do { \
		if (pcibiosverbose) \
			aprint_normal arg; \
	} while (0)
#define	PCIBIOS_PRINTVN(n, arg) \
	do { \
		 if (pcibiosverbose > (n)) \
			aprint_normal arg; \
	} while (0)
#else
#define	PCIBIOS_PRINTV(arg)
#define	PCIBIOS_PRINTVN(n, arg)
#endif
