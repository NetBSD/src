/*	$NetBSD: amcc405ex.h,v 1.1.6.2 2010/08/11 22:52:34 yamt Exp $	*/

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

#ifndef _IBM4XX_AMCC405EX_H_
#define	_IBM4XX_AMCC405EX_H_

/*
 * Memory and PCI addresses
 */

/* Local Memory and Peripherals */
#define	AMCC405EX_LOCAL_MEM_START	0x00000000
#define	AMCC405EX_LOCAL_MEM_END		0x7fffffff

/* EBC - 256MB */
#define	AMCC405EX_EBC_START		0x80000000
#define	AMCC405EX_EBC_END		0x8fffffff

/* PCI Express - 1.6GB */
#define	AMCC405EX_PCIE_MEM_START	0x90000000
#define	AMCC405EX_PCIE_MEM_END		0xef5fffff

/*
 * Internal peripheral addresses
 */

#define AMCC405EX_OPB_BASE		0xef600000
#define	AMCC405EX_GPT0_BASE		0xef600000
#define	AMCC405EX_UART0_BASE		0xef600200
#define	AMCC405EX_UART1_BASE		0xef600300
#define	AMCC405EX_IIC0_BASE		0xef600400
#define	AMCC405EX_IIC1_BASE		0xef600500
#define	AMCC405EX_SCP0_BASE		0xef600600
#define	AMCC405EX_OPBA0_BASE		0xef600700
#define	AMCC405EX_GPIO0_BASE		0xef600800
#define	AMCC405EX_EMAC0_BASE		0xef600900
#define	AMCC405EX_EMAC1_BASE		0xef600a00
#define	AMCC405EX_RGMIIB0_BASE		0xef600b00

#define	AMCC405EX_PKATRNG0_BASE		0xef610000
#define	AMCC405EX_PCIEIH0_BASE		0xef620000
#define	AMCC405EX_USBOTG0_BASE		0xef6c0000
#define	AMCC405EX_SECURITY0_BASE	0xef700000


/* Expansion ROM - 254MB */
#define	AMCC405EX_EXPANSION_ROM_START	0xf0000000
#define	AMCC405EX_EXPANSION_ROM_END	0xffdfffff

/* Boot ROM - 2MB */
#define	AMCC405EX_BOOT_ROM_START	0xffe00000
#define	AMCC405EX_BOOT_ROM_END		0xffffffff

#ifndef _LOCORE
void ibm4xx_show_pci_map(void);
void ibm4xx_setup_pci(void);
#endif /* _LOCORE */
#endif	/* _IBM4XX_AMCC405EX_H_ */
