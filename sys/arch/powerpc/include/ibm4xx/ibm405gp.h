/*	$NetBSD: ibm405gp.h,v 1.9 2003/09/23 15:19:05 shige Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Eduardo Horvath for Wasabi Systems, Inc.
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

#ifndef _IBM4XX_IBM405GP_H_
#define	_IBM4XX_IBM405GP_H_

/*
 * Memory and PCI addresses
 */

/* Local Memory and Peripherals */
#define	IBM405GP_LOCAL_MEM_START	0x00000000
#define	IBM405GP_LOCAL_MEM_END		0x7fffffff

/* PCI Memory - 1.625GB */
#define	IBM405GP_PCI_MEM_START		0x80000000
#define	IBM405GP_PCI_MEM_END		0xe7ffffff

/* PCI I/O - PCI I/O accesses from 0 to 64kB-1 (64kB) */
#define	IBM405GP_PCI_IO_LOW_START	0xe8000000
#define	IBM405GP_PCI_IO_LOW_END		0xe800ffff

/* PCI I/O - PCI I/O accesses from 8MB to 64MB-1 (56MB) */
#define	IBM405GP_PCI_IO_HIGH_START	0xe8800000
#define	IBM405GP_PCI_IO_HIGH_END	0xebffffff

/* PCI I/O - PLB side of PCI I/O address space */
#define	IBM405GP_PLB_PCI_IO_START	0xe8000000
/* PCI I/O - PCI side of PCI I/O address space */
#define	IBM405GP_PCI_PCI_IO_START	0x00000000

#define	IBM405GP_PCIC0_BASE		0xeec00000

/* PCI Interrupt Acknowledge (read: 0xeed00000 0xeed00003 - 4 bytes) */
#define	IBM405GP_PCIIA0			0xeed00000

/* PCI Special Cycle (write: 0xeed00000 0xeed00003 - 4 bytes) */
#define	IBM405GP_PCISC0			0xeed00000
#define	IBM405GP_PCIL0_BASE		0xef400000

/*
 * Internal peripheral addresses
 */

#define	IBM405GP_UART0_BASE		0xef600300
#define	IBM405GP_UART1_BASE		0xef600400
#define	IBM405GP_IIC0_BASE		0xef600500
#define	IBM405GP_OPBA0_BASE		0xef600600
#define	IBM405GP_GPIO0_BASE		0xef600700
#define	IBM405GP_EMAC0_BASE		0xef600800


/* Expansion ROM - 254MB */
#define	IBM405GP_EXPANSION_ROM_START	0xf0000000
#define	IBM405GP_EXPANSION_ROM_END	0xffdfffff

/* Boot ROM - 2MB */
#define	IBM405GP_BOOT_ROM_START		0xffe00000
#define	IBM405GP_BOOT_ROM_END		0xffffffff

#ifndef _LOCORE
void ibm4xx_show_pci_map(void);
void ibm4xx_setup_pci(void);
#endif /* _LOCORE */
#endif	/* _IBM4XX_IBM405GP_H_ */
