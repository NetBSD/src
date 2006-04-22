/*	$NetBSD: ep93xxreg.h,v 1.5.8.1 2006/04/22 11:37:17 simonb Exp $ */

/*
 * Copyright (c) 2004 Jesse Off
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

#ifndef _EP93XXREG_H_
#define _EP93XXREG_H_

/*
 * Physical memory map for the Cirrus Logic EP93XX 
 */

/*
 * FFFF FFFF ---------------------------
 *            Device 12
 *            External SMC CS#0 ROM/SRAM
 * F000 0000 ---------------------------
 *            Device 11
 *            SDRAM CS#2
 * E000 0000 ---------------------------
 *            Device 10
 *            SDRAM CS#1
 * D000 0000 ---------------------------
 *            Device 9
 *            SDRAM CS#0
 * C000 0000 ---------------------------
 *            Device 8
 *            Not used
 * 9000 0000 ---------------------------
 *            Device 7
 *            EP93XX System Registers
 *              8080 0000 - 8094 FFFF
 *                  APB Mapped Registers
 *              8010 0000 - 807F FFFF
 *                  Reserved
 *              8000 0000 - 800F FFFF
 *                  AHB Mapped Registers
 * 8000 0000 ---------------------------
 *            Device 6
 *            External SMC CS#7 ROM/SRAM
 * 7000 0000 ---------------------------
 *            Device 5
 *            External SMC CS#6 ROM/SRAM
 * 6000 0000 ---------------------------
 *            Device 4
 *            PCMCIA/CompactFlash
 *              5000 0000 - 5fff ffff
 *                  Reserved
 *              4c00 0000 - 4fff ffff
 *                  Slot0 Memory space
 *              4800 0000 - 4bff ffff
 *                  Slot0 Attribute space
 *              4400 0000 - 47ff ffff
 *                  Reserved
 *              4000 0000 - 43ff ffff
 *                  Slot0 I/O space
 * 4000 0000 ---------------------------
 *            Device 3
 *            External SMC CS#3 ROM/SRAM
 * 3000 0000 ---------------------------
 *            Device 2
 *            External SMC CS#2 ROM/SRAM
 * 2000 0000 ---------------------------
 *            Device 1
 *            External SMC CS#1 ROM/SRAM
 * 1000 0000 ---------------------------
 *            Device 0
 *            SDRAM CS#3
 * 0000 0000 ---------------------------
 */


/*
 * Virtual memory map for the Cirrus Logic EP93XX integrated devices
 *
 * Some device registers are staticaly mapped on upper address region.
 * because we have to access them before bus_space is initialized.
 * Most device is dynamicaly mapped by bus_space_map().  In this case,
 * the actual mapped (virtual) address are not cared by device drivers.
 */

/*
 * FFFF FFFF ---------------------------
 *            not used
 * F030 0000 ---------------------------
 *            APB bus (2Mbyte)
 * F010 0000 ---------------------------
 *            AHB bus (1Mbyte)
 * F000 0000 ---------------------------
 *            PCMCIA slot0 space
 * E000 0000 ---------------------------
 *            Kernel text and data
 * C000 0000 ---------------------------
 * 0000 0000 ---------------------------
 *
 */

/* Virtual address for I/O space */
#define	EP93XX_IO_VBASE		0xf0000000UL

/* EP93xx System and Peripheral Registers */
#define	EP93XX_AHB_VBASE	0xf0000000UL
#define	EP93XX_AHB_HWBASE	0x80000000UL
#define	EP93XX_AHB_SIZE		0x00100000UL	/* 1Mbyte */
#define  EP93XX_AHB_VIC1	0x000b0000UL
#define  EP93XX_AHB_VIC2	0x000c0000UL
#define   EP93XX_VIC_IRQStatus	0x00000000UL
#define   EP93XX_VIC_FIQStatus	0x00000004UL
#define   EP93XX_VIC_RawIntr	0x00000008UL
#define   EP93XX_VIC_IntSelect	0x0000000cUL
#define   EP93XX_VIC_IntEnable	0x00000010UL
#define   EP93XX_VIC_IntEnClear	0x00000014UL
#define   EP93XX_VIC_SoftInt	0x00000018UL
#define   EP93XX_VIC_SoftIntClear	0x0000001cUL
#define   EP93XX_VIC_Protection	0x00000020UL
#define   EP93XX_VIC_VectAddr	0x00000030UL
#define   EP93XX_VIC_DefVectAddr	0x00000034UL
#define   EP93XX_VIC_VectAddr0	0x00000100UL
#define   EP93XX_VIC_VectCntl0	0x00000200UL
#define   EP93XX_VIC_PeriphID0	0x00000fe0UL
#define  EP93XX_AHB_SMC		0x00080000UL

#define	EP93XX_APB_VBASE	0xf0100000UL
#define	EP93XX_APB_HWBASE	0x80800000UL
#define	EP93XX_APB_SIZE		0x00200000UL	/* 2Mbyte */
#define  EP93XX_APB_GPIO	0x00040000UL
#define  EP93XX_APB_GPIO_SIZE	0x000000d0UL
#define  EP93XX_APB_SSP		0x000a0000UL
#define  EP93XX_APB_SSP_SIZE	0x00000018UL
#define   EP93XX_SSP_SSPCR0	0x00000000UL
#define   EP93XX_SSP_SSPCR1	0x00000004UL
#define   EP93XX_SSP_SSPDR	0x00000008UL
#define   EP93XX_SSP_SSPSR	0x0000000cUL
#define   EP93XX_SSP_SSPCPSR	0x00000010UL
#define   EP93XX_SSP_SSPIIR	0x00000014UL
#define   EP93XX_SSP_SSPICR	0x00000014UL
#define  EP93XX_APB_SYSCON	0x00130000UL
#define  EP93XX_APB_SYSCON_SIZE	0x000000c0UL
#define   EP93XX_SYSCON_PwrSts	0x00000000UL
#define   EP93XX_SYSCON_PwrCnt	0x00000004UL
#define    PwrCnt_UARTBAUD	0x20000000UL
#define   EP93XX_SYSCON_TEOI	0x00000018UL
#define   EP93XX_SYSCON_ClkSet1	0x00000020UL
#define   EP93XX_SYSCON_ClkSet2	0x00000024UL
#define   EP93XX_SYSCON_DeviceCfg	0x00000080UL
#define   EP93XX_SYSCON_ChipID	0x00000094UL
#define  EP93XX_APB_TIMERS	0x00010000UL
#define  EP93XX_APB_UART1	0x000c0000UL
#define  EP93XX_APB_UART2	0x000d0000UL
#define  EP93XX_APB_UART_SIZE	0x00000220UL
#define   EP93XX_UART_Flag	0x00000018UL
#define   EP93XX_UART_Data	0x00000000UL
#define  EP93XX_APB_RTC		0x00120000UL
#define  EP93XX_APB_RTC_SIZE	0x00000112UL
#define  EP93XX_APB_WDOG	0x00140000UL
#define  EP93XX_APB_WDOG_SIZE	0x00000008UL

/* EP93xx PCMCIA space */
#define	EP93XX_PCMCIA0_VBASE	0xe0000000UL
#define	EP93XX_PCMCIA0_HWBASE	0x40000000UL
#define	EP93XX_PCMCIA_SIZE	0x10000000UL
#define  EP93XX_PCMCIA_IO	0x00000000UL
#define  EP93XX_PCMCIA_IO_SIZE	0x04000000UL
#define  EP93XX_PCMCIA_ATTRIBUTE	0x08000000UL
#define  EP93XX_PCMCIA_ATTRIBUTE_SIZE	0x04000000UL
#define  EP93XX_PCMCIA_COMMON	0x0c000000UL
#define  EP93XX_PCMCIA_COMMON_SIZE	0x04000000UL

#define NIRQ			64
#define VIC_NIRQ		32

#define	EP93XX_INTR_bit31	31
#define	EP93XX_INTR_bit30	30
#define	EP93XX_INTR_bit29	29
#define	EP93XX_INTR_bit28	28
#define	EP93XX_INTR_bit27	27
#define	EP93XX_INTR_bit26	26
#define	EP93XX_INTR_bit25	25
#define	EP93XX_INTR_bit24	24
#define	EP93XX_INTR_bit23	23
#define	EP93XX_INTR_bit22	22
#define	EP93XX_INTR_bit21	21
#define	EP93XX_INTR_bit20	20
#define	EP93XX_INTR_bit19	19
#define	EP93XX_INTR_bit18	18
#define	EP93XX_INTR_bit17	17
#define	EP93XX_INTR_bit16	16
#define	EP93XX_INTR_bit15	15
#define	EP93XX_INTR_bit14	14
#define	EP93XX_INTR_bit13	13
#define	EP93XX_INTR_bit12	12
#define	EP93XX_INTR_bit11	11
#define	EP93XX_INTR_bit10	10
#define	EP93XX_INTR_bit9	9
#define	EP93XX_INTR_bit8	8
#define	EP93XX_INTR_bit7	7
#define	EP93XX_INTR_bit6	6
#define	EP93XX_INTR_bit5	5
#define	EP93XX_INTR_bit4	4
#define	EP93XX_INTR_bit3	3
#define	EP93XX_INTR_bit2	2
#define	EP93XX_INTR_bit1	1
#define	EP93XX_INTR_bit0	0

#endif /* _EP93XXREG_H_ */
