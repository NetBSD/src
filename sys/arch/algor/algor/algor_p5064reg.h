/*	$NetBSD: algor_p5064reg.h,v 1.2 2002/02/20 01:34:19 simonb Exp $	*/

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
 * Memory map and register definitions for the Algorithmics P-5064.
 */

#define	P5064_MEMORY		0x00000000UL	/* onboard DRAM memory */
			/* 	256 MB		*/
#define	P5064_ISAMEM		0x10000000UL	/* ISA window of PCI memory */
			/*	8MB		*/
#define	P5064_PCIMEM		0x11000000UL	/* PCI memory window */
			/*	112MB		*/
#define	P5064_PCIIO		0x1d000000UL	/* PCI I/O window */
			/*	16MB		*/
#define	P5064_PCICFG		0x1ee00000UL	/* PCI config space */
			/*	1MB		*/
#define	P5064_V360EPC		0x1ef00000UL	/* V360EPC PCI controller */
			/*	64KB		*/
#define	P5064_CFGBOOT_W		0x1f800000UL	/* configured bootstrap (W) */
			/*	512KB		*/
#define	P5064_SOCKET_W		0x1f900000UL	/* socket EPROM (W) */
			/*	512KB		*/
#define	P5064_FLASH_W		0x1fa00000UL	/* flash (W) */
			/*	1MB		*/
#define	P5064_CFBOOT		0x1fc00000UL	/* configured bootstrap */
			/*	512KB		*/
#define	P5064_SOCKET		0x1fd00000UL	/* socket EPROM */
			/*	512KB		*/
#define	P5064_FLASH		0x1fe00000UL	/* flash */
			/*	1MB		*/
#define	P5064_LED0		0x1ff00000UL	/* LED (1reg) */
#define	P5064_LED1		0x1ff20010UL	/* LED (4reg) */
#define	P5064_LCD		0x1ff30000UL	/* LCD display */
#define	P5064_Z80GPIO		0x1ff40000UL	/* Z80 GPIO (rev B only) */
#define	P5064_Z80GPIO_IACK	0x1ff50000UL	/* intr. ack. for Z80 */
#define	P5064_DBG_UART		0x1ff60000UL	/* UART on debug board */
#define	P5064_LOCINT		0x1ff90000UL	/* local interrupts */
#define	P5064_PANIC		0x1ff90004UL	/* panic interrupts */
#define	P5064_PCIINT		0x1ff90008UL	/* PCI interrupts */
#define	P5064_ISAINT		0x1ff9000cUL	/* ISA interrupts */
#define	P5064_XBAR0		0x1ff90010UL	/* Int. xbar 0 */
#define	P5064_XBAR1		0x1ff90014UL	/* Int. xbar 1 */
#define	P5064_XBAR2		0x1ff90018UL	/* Int. xbar 2 */
#define	P5064_XBAR3		0x1ff9001cUL	/* Int. xbar 3 */
#define	P5064_XBAR4		0x1ff90020UL	/* Int. xbar 4 */
#define	P5064_KBDINT		0x1ff90024UL	/* keyboard interrupts */
#define	P5064_LOGICREV		0x1ff9003cUL	/* logic revision */
#define	P5064_CFG0		0x1ffa0000UL	/* board configuration 0 */
#define	P5064_CFG1		0x1ffb0000UL	/* board configuration 1 */
#define	P5064_DRAMCFG		0x1ffc0000UL	/* DRAM configuration */
#define	P5064_BOARDREV		0x1ffd0000UL	/* board revision */
#define	P5064_PCIMEM_HI		0x20000000UL	/* PCI memory high window */
			/*	3.5GB		*/

/* P5064_LOCINT */
#define	LOCINT_PCIBR		0x01
#define	LOCINT_FLP		0x02
#define	LOCINT_MKBD		0x04
#define	LOCINT_COM1		0x08
#define	LOCINT_COM2		0x10
#define	LOCINT_CENT		0x20
#define	LOCINT_RTC		0x80

/* P5064_PANIC */
#define	PANIC_DEBUG		0x01
#define	PANIC_PFAIL		0x02
#define	PANIC_BERR		0x04
#define	PANIC_ISANMI		0x08
#define	PANIC_IOPERR		0x10
#define	PANIC_CENT		0x20
#define	PANIC_EWAKE		0x40
#define	PANIC_ECODERR		0x80

/* P5064_PCIINT */
#define	PCIINT_EMDINT		0x01
#define	PCIINT_ETH		0x02
#define	PCIINT_SCSI		0x04
#define	PCIINT_USB		0x08
#define	PCIINT_PCI0		0x10
#define	PCIINT_PCI1		0x20
#define	PCIINT_PCI2		0x40
#define	PCIINT_PCI3		0x80

/* P5064_ISAINT */
#define	ISAINT_ISABR		0x01
#define	ISAINT_IDE0		0x02
#define	ISAINT_IDE1		0x04

/* P5064_KBDINT */
#define	KBDINT_KBD		0x01
#define	KBDINT_MOUSE		0x02

/*
 * The Algorithmics PMON initializes two DMA windows:
 *
 *	THE MANUAL CLAIMS THIS:
 *	PCI 0080.0000 -> Phys 0080.0000 (8MB)
 *
 *	THE PMON FIRMWARE DOES THIS:
 *	PCI 0080.0000 -> Phys 0000.0000 (8MB)
 *
 *	PCI 8000.0000 -> Phys 0000.0000 (256MB)
 */
#define	P5064_DMA_ISA_PCIBASE	0x00800000UL
#define	P5064_DMA_ISA_PHYSBASE	0x00000000UL
#define	P5064_DMA_ISA_SIZE	(8 * 1024 * 1024)

#define	P5064_DMA_PCI_PCIBASE	0x80000000UL
#define	P5064_DMA_PCI_PHYSBASE	0x00000000UL
#define	P5064_DMA_PCI_SIZE	(256 * 1024 * 1024)
