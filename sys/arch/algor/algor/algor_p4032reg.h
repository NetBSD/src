/*	$NetBSD: algor_p4032reg.h,v 1.1 2001/06/01 16:00:03 thorpej Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Memory map and register definitions for the Algorithmics P-4032.
 */

#define	P4032_MEMORY		0x00000000UL	/* onbord DRAM memory */
			/* 	256 MB		*/
#define	P4032_ISAMEM		0x10000000UL	/* ISA window of PCI memory */
			/*	8MB		*/
#define	P4032_PCIMEM		0x11000000UL	/* PCI memory window */
			/*	112MB		*/
#define	P4032_PCIIO		0x1ed00000UL	/* PCI I/O window */
			/*	1MB		*/
#define	P4032_PCICFG		0x1ee00000UL	/* PCI config space */
			/*	1MB		*/
#define	P4032_V962PBC		0x1ef00000UL	/* V962PBC PCI controller */
			/*	64KB		*/
#define	P4032_CFGBOOT_W		0x1fc00000UL	/* configured bootstrap (W) */
			/*	1MB or 512KB	*/
#define	P4032_SOCKET_W		0x1fd00000UL	/* socket EPROM (W) */
			/*	512KB		*/
#define	P4032_FLASH_W		0x1fe00000UL	/* flash (W) */
			/*	1MB		*/
#define	P4032_RTC		0x1ff00000UL	/* RTC */
#define	P4032_PCKBC		0x1ff10000UL	/* keyboard controller */
#define	P4032_LED		0x1ff20010UL	/* LED display (4reg) */
#define	P4032_LCD		0x1ff30000UL	/* LCD display */
#define	P4032_GPIO		0x1ff40000UL	/* General purpose I/O */
#define	P4032_FDC		0x1ff807c0UL	/* floppy controller */
#define	P4032_GAME		0x1ff80800UL	/* game port (unused) */
#define	P4032_COM2		0x1ff80be0UL	/* COM2 */
#define	P4032_LPT		0x1ff80de0UL	/* parallel port */
#define	P4032_COM1		0x1ff80fe0UL	/* COM1 */
#define	P4032_IRR0		0x1ff90000UL	/* interrupt req. 0 */
#define	P4032_IRR1		0x1ff90004UL	/* interrupt req. 1 */
#define	P4032_IRR2		0x1ff90008UL	/* interrupt req. 2 */
#define	P4032_XBAR0		0x1ff9000cUL	/* interrupt crossbar 0 */
#define	P4032_XBAR1		0x1ff90010UL	/* interrupt crossbar 1 */
#define	P4032_XBAR2		0x1ff90014UL	/* interrupt crossbar 2 */
#define	P4032_VERSION		0x1ff9001cUL	/* board version */
#define	P4032_FLOPPY_DMA_ACK	0x1ffa0fd4UL	/* floppy "DMA ACK" */
#define	P4032_BOARD_CONFIG	0x1ffb0000UL	/* board configuration */
#define	P4032_DRAM_CONFIG	0x1ffc0000UL	/* DRAM configuration */
#define	P4032_OPTION		0x1ffd0000UL	/* option register */
#define	P4032_PCIMEM_HI		0x20000000UL	/* PCI memory high window */
			/*	3.5GB		*/

/* IRR0 (8-bit devices) */
#define	IRR0_PCICTLR	0x01	/* PCI controller */
#define	IRR0_FLOPPY	0x02	/* floppy controller */
#define	IRR0_PCKBC	0x04	/* keyboard controller */
#define	IRR0_COM1	0x08	/* COM1 */
#define	IRR0_COM2	0x10	/* COM2 */
#define	IRR0_LPT	0x20	/* centronics */
#define	IRR0_GPIO	0x40	/* general purpose I/O */
#define	IRR0_RTC	0x80	/* real-time clock */

/* IRR1 (error/clear) */
#define	IRR1_DEBUG	0x01	/* debug switch */
#define	IRR1_POWERFAIL	0x02	/* power fail */
#define	IRR1_BUSERR	0x04	/* bus error */
#define	IRR1_LPT_ACK	0x20	/* centronics interrupt ack. */

/* IRR2 (PCI) */
#define	IRR2_FLOPPY_DMA	0x08	/* floppy DMA request */
#define	IRR2_PCIIRQ0	0x10	/* PCIIRQ 0 */
#define	IRR2_PCIIRQ1	0x20	/* PCIIRQ 1 */
#define	IRR2_PCIIRQ2	0x40	/* PCIIRQ 2 */
#define	IRR2_PCIIRQ3	0x80	/* PCIIRQ 3 */

/*
 * The Algorithmics PMON initializes two DMA windows:
 *
 *	PCI 8000.0000 -> Phys 0000.0000 (256MB)
 */
#define	P4032_DMA_PCI_PCIBASE	0x80000000UL
#define	P4032_DMA_PCI_PHYSBASE	0x00000000UL
#define	P4032_DMA_PCI_SIZE	(256 * 1024 * 1024)
