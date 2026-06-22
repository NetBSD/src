/*	$NetBSD: sam460ex.h,v 1.4 2026/06/22 12:34:19 rkujawa Exp $	*/

/*
 * Copyright (c) 2012, 2014, 2024, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ACube Sam460ex board definitions.
 */

#ifndef	_EVBPPC_SAM460EX_H_
#define	_EVBPPC_SAM460EX_H_

#include <powerpc/ibm4xx/amcc460ex.h>

/* Boot loader handoff (set in machdep.c initppc()) */
#ifndef _LOCORE
extern paddr_t sam460ex_fdt_pa;
extern uint32_t sam460ex_epapr_magic;

/* Values extracted from the U-Boot/QEMU device tree (0 = absent) */
#define	SAM460EX_NEMAC	2
struct sam460ex_fdt_info {
	uint32_t fi_memsize;
	uint32_t fi_cpu_freq;
	uint32_t fi_timebase_freq;
	uint32_t fi_opb_freq;
	uint32_t fi_uart_freq;
	const char *fi_bootargs;
	/* EMAC MAC addresses (all-zero = absent/not fixed up) */
	uint8_t fi_enaddr[SAM460EX_NEMAC][6];
	bool fi_enaddr_valid[SAM460EX_NEMAC];
};
extern struct sam460ex_fdt_info sam460ex_fdt_info;

bool sam460ex_fdt_parse(paddr_t);
#endif

/*
 * UART input clock for the console and com devices
 */
uint32_t sam460ex_com_freq(void);

/*
 * Console target, selected by the "console=" bootarg
 */
enum sam460ex_console {
	SAM460EX_CONS_COM,
	SAM460EX_CONS_SM502,
	SAM460EX_CONS_PCI,
};
extern enum sam460ex_console sam460ex_console;
extern int sam460ex_console_pci_bdf[3];

/* ePAPR magic passed in r6 by U-Boot and QEMU */
#define	SAM460EX_EPAPR_MAGIC	0x45504150

/*
 * Kernel VA for I/O
 *	0xe0000000  PCIE1 local-config (XCFG: inbound BAR/PIM regs, 16MB)
 *      0xef000000  OPB (16MB)
 *	0xf0000000  PCI memory window  (AMCC460EX_PCIX0_MEM_SIZE)
 *	0xf5000000  PCI I/O window     (16MB entry, 64KB used)
 *	0xf6000000  PCIX config + internal registers (16MB entry)
 *	0xf7000000  PCIE0 config window (ECAM, 16MB)
 *	0xf8000000  PCIE1 config window (ECAM, 16MB)
 *	0xf9000000  PCIE0 memory window (16MB)
 *	0xfa000000  PCIE1 memory window (16MB)
 *	0xfb000000  AHB peripherals: USB OTG/OHCI/EHCI (16MB)
 *	0xfc000000  PCI prefetchable window (POM1, 64MB pinned of 256MB)
 */
#define	SAM460EX_PCIE1XCFG_VA	0xe0000000
#define	SAM460EX_PCIMEM_VA	0xf0000000
#define	SAM460EX_PCIIO_VA	0xf5000000
#define	SAM460EX_PCICFG_VA	0xf6000000
#define	SAM460EX_PCIE0CFG_VA	0xf7000000
#define	SAM460EX_PCIE1CFG_VA	0xf8000000
#define	SAM460EX_PCIE0MEM_VA	0xf9000000
#define	SAM460EX_PCIE1MEM_VA	0xfa000000
#define	SAM460EX_AHB_VA		0xfb000000
#define	SAM460EX_PCIPREFMEM_VA	0xfc000000

#endif	/* _EVBPPC_SAM460EX_H_ */
