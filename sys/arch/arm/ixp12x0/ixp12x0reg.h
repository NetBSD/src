/*	$NetBSD: ixp12x0reg.h,v 1.3 2003/02/17 20:51:52 ichiro Exp $ */

/*
 * Copyright (c) 2002, 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
 * All rights reserved.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _IXP12X0REG_H_
#define _IXP12X0REG_H_

/*
 * Physical memory map for the Intel IXP12X0 
 */

/*
 * FFFF FFFF ---------------------------
 *            Device 6
 *            SDRAM
 *              FF00 0000 - FF00 0014
 *                  SDRAM Control Register
 *              D000 0000 - DFFF FFFF
 *                  Prefetch 256MB
 *              C000 0000 - CFFF FFFF
 *                  non-Prefetch 256MB
 * C000 0000 ---------------------------
 *            Device 5
 *            AMBA Translation (ATU)
 * B000 0000 ---------------------------
 *            Device 4
 *            Reserved
 * A000 0000 ---------------------------
 *            Device 3
 *            StrongARM Core System
 * 9000 0000 ---------------------------
 *            Device 2
 *            Reserved
 * 8000 0000 ---------------------------
 *            Device 1
 *            PCI UNIT
 *              6000 0000 - 7FFF FFFF
 *                  PCI Memory Cycle Access
 *              5400 0000 - 5400 FFFF
 *                  PCI I/O Cycle Access
 *              5300 0000 - 53BF FFFF
 *                  PCI Type0 Configuration Cycle Access
 *              5200 0000 - 52FF FFFF
 *                  PCI Type1 Configuration Cycle Access
 *              4200 0000 - 4200 03FF
 *                  Local PCI Configuration Space
 * 4000 0000 ---------------------------
 *            Device 0
 *            SRAM UNIT
 * 0000 0000 ---------------------------
 */


/*
 * Virtual memory map for the Intel IXP12X0 integrated devices
 *
 * IXP12x0 processors have many device registers at very lower addresses.
 * To make user process space wider, we map the registers at lower address
 * to upper address using adress translation of virtual memory system.
 *
 * Some device registers are staticaly mapped on upper address region.
 * because we have to access them before bus_space is initialized.
 * Most device is dinamicaly mapped by bus_space_map().  In this case,
 * the actual mapped (virtual) address are not cared by device drivers.
 */

/*
 * FFFF FFFF ---------------------------
 *
 * F400 0000 ---------------------------
 *            PCI Type 0 Configuration Cycle Access
 *            VA F300 0000 == PA 5300 0000 (16Mbyte)
 * F300 0000 ---------------------------
 *            PCI Type 1 Configuration Cycle Access
 *            VA F200 0000 == PA 5200 0000 (16Mbyte)
 * F200 0000 ---------------------------
 *            not used
 * F001 1000 ---------------------------
 *            PCI Registers Accessible Through StrongARM Core
 *            VA F001 0000 == PA 4200 0000 (4kbyte)
 *              F001 0300 - F001 036F  TIMER
 *              F001 0200 - F001 0293  PCI_FIQ
 *              F001 0180 - F001 0193  PCI_IRQ
 * F001 0000 ---------------------------
 *            StrongARM System and Peripheral Registers
 *            VA F001 0000 == PA 9000 0000 (64kbyte)
 *              F000 3400 - F000 3C03  UART
 *              F000 3400 - F000 3C03  UART
 *              F000 2000 - F000 3003  RTC
 *              F000 1C00 - F000 1C03  GPIO_DATA
 *              F000 1800 - F000 1C03  GPIO
 *              F000 1400 - F000 1403  IRQ
 *              F000 1000 - F000 1003  FIQ
 *              F000 0C00 - F000 0C03  PLL_CFG (not used at this addres)
 * F000 0000 ---------------------------
 *            Kernel text and data
 * C000 0000 ---------------------------
 *            L2 tables for user process (XXX should be fixed)
 * 8000 0000 ---------------------------
 *            PCI Registers Accessible Through Memory
 *            VA 6000 0000 == PA 6000 0000 (512Mbyte)
 * 6000 0000 ---------------------------
 *            PCI Registers Accessible Through I/O
 *            VA 5400 0000 == PA 5400 0000 (1Mbyte)
 * 5400 0000 ---------------------------
 * 0000 0000 ---------------------------
 *
 */

/* Virtual address for I/O space */
#define	IXP12X0_IO_VBASE	0xf0000000UL

/* StrongARM System and Peripheral Registers */
#define	IXP12X0_SYS_VBASE	IXP12X0_IO_VBASE
						/* va=0xf0000000 */
#define	IXP12X0_SYS_HWBASE	0x90000000UL
#define	IXP12X0_SYS_SIZE	0x00010000UL	/* 64Kbyte */

#define	IXP12X0_PLL_CFG		(IXP12X0_IO_VBASE + 0x0c00)
#define	 IXP12X0_PLL_CFG_CCF	0x1f

/* PCI Registers Accessible Through StrongARM Core */
#define	IXP12X0_PCI_VBASE	(IXP12X0_IO_VBASE + IXP12X0_SYS_SIZE)
						/* va=0xf0010000 */
#define	IXP12X0_PCI_HWBASE	0x42000000UL
#define	IXP12X0_PCI_SIZE	0x00001000UL	/* 4Kbyte */

/* PCI I/O Space */
#define	IXP12X0_PCI_IO_HWBASE	0x54000000UL	/* VA == PA */
#define	IXP12X0_PCI_IO_VBASE	IXP12X0_PCI_IO_HWBASE
#define	IXP12X0_PCI_IO_SIZE	0x00100000UL	/* 1Mbyte */

/* PCI Memory Space */
#define IXP12X0_PCI_MEM_HWBASE	0x60000000UL	/* VA == PA */
#define IXP12X0_PCI_MEM_VBASE	IXP12X0_PCI_MEM_HWBASE
#define	IXP12X0_PCI_MEM_SIZE	0x20000000UL

/* PCI Type0/1 Configuration address */
#define	IXP12X0_PCI_TYPE0_HWBASE	0x53000000UL
#define	IXP12X0_PCI_TYPE0_VBASE		0xf3000000UL
#define	IXP12X0_PCI_TYPE0_SIZE		0x01000000UL	/* 16MB */

#define	IXP12X0_PCI_TYPE1_HWBASE	0x52000000UL
#define	IXP12X0_PCI_TYPE1_VBASE		0xf2000000UL
#define	IXP12X0_PCI_TYPE1_SIZE		0x01000000UL	/* 16MB */

/*
 * SlowPort I/O Register
 */
/* see. arch/evbarm/ixm1200/ixm1200reg.h */

/* Physical register base addresses */
/* #define	IXP12X0_GPIO_VBASE */
#define	IXP12X0_GPIO_HWBASE	0x90001800UL
#define	IXP12X0_GPIO_SIZE	0x00000800UL

/* Interrupts */
#define	IXP12X0_FIQ_VBASE	(IXP12X0_IO_VBASE + 0x1000)
#define	IXP12X0_FIQ_HWBASE	0x90001000UL
#define	IXP12X0_FIQ_SIZE	0x00000004UL
#define	IXP12X0_IRQ_VBASE	(IXP12X0_IO_VBASE + 0x1400)
#define	IXP12X0_IRQ_HWBASE	0x90001400UL
#define	IXP12X0_IRQ_SIZE	0x00000004UL

/*
 * Interrupt index assignment
 *
 *
 *     FIQ/IRQ bitmap in "StrongARM System and Peripheral Registers"
 *
 *        3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 * bit    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 
 *       +-+-------------------------------------------+-+-+-+-+-+-+-+---+
 *       |M|                                           |U|S|R|S|U|C|P|   |
 *       |B|                                           |A|D|T|R|E|I|C|   |
 *       |Z|                    RES                    |R|R|C|A|N|N|I|RES|
 *       | |                                           |T|A| |M|G|T| |   |
 *       | |                                           | |M| | | | | |   |
 *       +-+-------------------------------------------+-+-+-+-+-+-+-+---+
 *        3
 * index  1                                             8 7 6 5 4 3 2
 *
 *
 * We Map a software interrupt queue index to the unused bits in the
 * IRQ/FIQ registers. (in "StrongARM System and Peripheral Registers")
 *
 * XXX will need to revisit this if those bits are ever used in future
 * steppings).
 *
 *        3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 * bit    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 
 *       +-+-+-+-+-+-----------------------------------+-+-+-+-+-+-+-+---+
 *       |M|S|C|N|S|                                   |U|S|R|S|U|C|P|   |
 *       |B|O|L|E|E|                                   |A|D|T|R|E|I|C|   |
 *       |Z|F|O|T|R|                RES                |R|R|C|A|N|N|I|RES|
 *       | |T|C| |I|                                   |T|A| |M|G|T| |   |
 *       | | |K| |A|                                   | |M| | | | | |   |
 *       | | | | |L|                                   | | | | | | | |   |
 *       +-+-+-+-+-+-----------------------------------+-+-+-+-+-+-+-+---+
 *        3 3 2 2 2
 * index  1 0 9 8 7                                     8 7 6 5 4 3 2
 *
 */

#define NIRQ			64
#define SYS_NIRQ		32

#define	IXP12X0_INTR_MBZ	31
#define	IXP12X0_INTR_bit30	30
#define	IXP12X0_INTR_bit29	29
#define	IXP12X0_INTR_bit28	28
#define	IXP12X0_INTR_bit27	27
#define	IXP12X0_INTR_bit26	26
#define	IXP12X0_INTR_bit25	25
#define	IXP12X0_INTR_bit24	24
#define	IXP12X0_INTR_bit23	23
#define	IXP12X0_INTR_bit22	22
#define	IXP12X0_INTR_bit21	21
#define	IXP12X0_INTR_bit20	20
#define	IXP12X0_INTR_bit19	19
#define	IXP12X0_INTR_bit18	18
#define	IXP12X0_INTR_bit17	17
#define	IXP12X0_INTR_bit16	16
#define	IXP12X0_INTR_bit15	15
#define	IXP12X0_INTR_bit14	14
#define	IXP12X0_INTR_bit13	13
#define	IXP12X0_INTR_bit12	12
#define	IXP12X0_INTR_bit11	11
#define	IXP12X0_INTR_bit10	10
#define	IXP12X0_INTR_bit9	9
#define	IXP12X0_INTR_UART	8
#define	IXP12X0_INTR_SDRAM	7
#define	IXP12X0_INTR_RTC	6
#define	IXP12X0_INTR_SRAM	5
#define	IXP12X0_INTR_UENG	4
#define	IXP12X0_INTR_CINT	3
#define	IXP12X0_INTR_PCI	2
#define	IXP12X0_INTR_bit1	1
#define	IXP12X0_INTR_bit0	0

#define	IXP12X0_INTR_MASK					\
				((1U << IXP12X0_INTR_MBZ)	\
				 | (1U << IXP12X0_INTR_UART)	\
				 | (1U << IXP12X0_INTR_SDRAM)	\
				 | (1U << IXP12X0_INTR_RTC)	\
				 | (1U << IXP12X0_INTR_SRAM)	\
				 | (1U << IXP12X0_INTR_UENG)	\
				 | (1U << IXP12X0_INTR_CINT))

#endif /* _IXP12X0REG_H_ */
