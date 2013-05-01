/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

Developed by Semihalf

********************************************************************************
Marvell BSD License

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
            this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef _ARMADAXPREG_H_
#define _ARMADAXPREG_H_

#include <arm/marvell/mvsocreg.h>
#include <evbarm/marvell/marvellvar.h>

#define ARMADAXP_UNITID_DDR		MVSOC_UNITID_DDR
#define ARMADAXP_UNITID_DEVBUS		MVSOC_UNITID_DEVBUS
#define ARMADAXP_UNITID_MBUS		MVSOC_UNITID_MBUS
#define ARMADAXP_UNITID_MLMB		MVSOC_UNITID_MLMB
#define ARMADAXP_UNITID_PEX		MVSOC_UNITID_PEX
#define ARMADAXP_UNITID_USB		0x5	/* USB registers */
#define ARMADAXP_UNITID_GBE0		0x7
#define ARMADAXP_UNITID_GBE2		0x3
#define ARMADAXP_UNITID_PEX0		MVSOC_UNITID_PEX
#define ARMADAXP_UNITID_PEX1		0x8
#define ARMADAXP_UNITID_PEX2		MVSOC_UNITID_PEX
#define ARMADAXP_UNITID_PEX3		0x8
#define ARMADAXP_UNITID_SATA		0xa

#define ARMADAXP_ATTR_PEXx0_MEM		0xe8
#define ARMADAXP_ATTR_PEXx0_IO		0xe0
#define ARMADAXP_ATTR_PEXx1_MEM		0xd8
#define ARMADAXP_ATTR_PEXx1_IO		0xd0
#define ARMADAXP_ATTR_PEXx2_MEM		0xb8
#define ARMADAXP_ATTR_PEXx2_IO		0xb0
#define ARMADAXP_ATTR_PEXx3_MEM		0x78
#define ARMADAXP_ATTR_PEXx3_IO		0x70
#define ARMADAXP_ATTR_PEX2_MEM		0xf8
#define ARMADAXP_ATTR_PEX2_IO		0xf0
#define ARMADAXP_ATTR_PEX3_MEM		0xf8
#define ARMADAXP_ATTR_PEX3_IO		0xf0

#define ARMADAXP_MLMB_NWINDOW		19
#define ARMADAXP_MLMB_NREMAP		8

#define ARMADAXP_PEX00_BASE		0x40000
#define ARMADAXP_PEX01_BASE		0x44000
#define ARMADAXP_PEX02_BASE		0x48000
#define ARMADAXP_PEX03_BASE		0x4c000
#define ARMADAXP_PEX10_BASE		0x80000
#define ARMADAXP_PEX11_BASE		0x84000
#define ARMADAXP_PEX12_BASE		0x88000
#define ARMADAXP_PEX13_BASE		0x8c000
#define ARMADAXP_PEX2_BASE		0x42000
#define ARMADAXP_PEX3_BASE		0x82000

#define ARMADAXP_IRQ_PEX00		58	/* PCIe Port0.0 INTA/B/C/D */
#define ARMADAXP_IRQ_PEX01		59	/* PCIe Port0.1 INTA/B/C/D */
#define ARMADAXP_IRQ_PEX02		60	/* PCIe Port0.2 INTA/B/C/D */
#define ARMADAXP_IRQ_PEX03		61	/* PCIe Port0.3 INTA/B/C/D */
#define ARMADAXP_IRQ_PEX10		62	/* PCIe Port1.0 INTA/B/C/D */
#define ARMADAXP_IRQ_PEX11		63	/* PCIe Port1.1 INTA/B/C/D */
#define ARMADAXP_IRQ_PEX12		64	/* PCIe Port1.2 INTA/B/C/D */
#define ARMADAXP_IRQ_PEX13		65	/* PCIe Port1.3 INTA/B/C/D */
#define ARMADAXP_IRQ_PEX2		99	/* PCIe Port2 INTA/B/C/D */
#define ARMADAXP_IRQ_PEX3		103	/* PCIe Port3 INTA/B/C/D */

#define ARMADAXP_IRQ_SATA0		55
#define ARMADAXP_IRQ_SATA1		57

#define ARMADAXP_IRQ_SPI0		30	/* SPI0 */
#define ARMADAXP_IRQ_TWSI0		31	/* TWSI0 */
#define ARMADAXP_IRQ_TWSI1		32	/* TWSI1 */
#define ARMADAXP_IRQ_TIMER0		37	/* Timer0 */
#define ARMADAXP_IRQ_UART0INT		41	/* UART0 */
#define ARMADAXP_IRQ_UART1INT		42	/* UART1 */
#define ARMADAXP_IRQ_UART2INT		43	/* UART2 */
#define ARMADAXP_IRQ_UART3INT		44	/* UART3 */
/*
 * According to functional specification (MV-S107021-00B.pdf), interrupt number
 * for USB0_int is 47, in fact this number is 45. Because of that interrupt
 * number definitions are not equivalent to ArmadaXP functional specification
 * (MV-S107021-00B.pdf).
 */
#define ARMADAXP_IRQ_USB0INT		45	/* USB2 */
#define ARMADAXP_IRQ_USB1INT		46	/* USB1 */
#define ARMADAXP_IRQ_USB2INT		47	/* USB0 */
#define ARMADAXP_IRQ_RTCINT		50	/* RTC */

#define ARMADAXP_IRQ_GBE0_TH_RXTX_Int	8	/* GBE0_TH_RXTX_Int */
#define ARMADAXP_IRQ_GBE1_TH_RXTX_Int	10	/* GBE1_TH_RXTX_Int */
#define ARMADAXP_IRQ_GBE2_TH_RXTX_Int	12	/* GBE2_TH_RXTX_Int */
#define ARMADAXP_IRQ_GBE3_TH_RXTX_Int	14	/* GBE3_TH_RXTX_Int */

/*
 * Physical address of integrated peripherals
 */

#undef UNITID2PHYS
#define UNITID2PHYS(uid)	((ARMADAXP_UNITID_ ## uid) << 16)

/*
 * Real-Time Clock Unit Registers
 */
#define ARMADAXP_RTC_BASE		(MVSOC_DEVBUS_BASE + 0x0300)

/*
 * TWSI Interface Registers
 */
#define ARMADAXP_TWSI0_BASE		(MVSOC_DEVBUS_BASE + 0x1000)
#define ARMADAXP_TWSI1_BASE		(MVSOC_DEVBUS_BASE + 0x1100)

/* SPI Interface Registers
+ */
#define ARMADAXP_SPI0_BASE		(MVSOC_DEVBUS_BASE + 0x0600)
#define ARMADAXP_SPI1_BASE		(MVSOC_DEVBUS_BASE + 0x0680)

/*
 * UART Interface Registers
 */					/* NS16550 compatible */
#define ARMADAXP_COM2_BASE		(MVSOC_DEVBUS_BASE + 0x2200)
#define ARMADAXP_COM3_BASE		(MVSOC_DEVBUS_BASE + 0x2300)

/*
 * PCI Express Interface Registers
 */
#define ARMADAXP_PEX_BASE	(UNITID2PHYS(PEX))	/* 0x40000 */

/*
 * USB 2.0 Interface Registers
 */
#define ARMADAXP_USB_BASE	(UNITID2PHYS(USB))	/* 0x50000 */
#define ARMADAXP_USB0_BASE	(ARMADAXP_USB_BASE)
#define ARMADAXP_USB1_BASE	(ARMADAXP_USB_BASE + 0x1000)

/*
 * Gigabit Ethernet Registers
 */
#define ARMADAXP_GBE0_BASE	(UNITID2PHYS(GBE0))	/* 0x70000 */
#define ARMADAXP_GBE1_BASE	(ARMADAXP_GBE0_BASE + 0x4000)
#define ARMADAXP_GBE2_BASE	(UNITID2PHYS(GBE2))	/* 0x30000 */
#define ARMADAXP_GBE3_BASE	(ARMADAXP_GBE2_BASE + 0x4000)

/*
 * Serial-ATA Host Controller (SATAHC) Registers
 */
#define ARMADAXP_SATAHC_BASE	(UNITID2PHYS(SATA))	/* 0xa0000 */

/*
 * SoC MISC Registers
 */
#define	ARMADAXP_MISC_SAR_LO		0x30	/* Sample At Reset Low */
#define	ARMADAXP_MISC_SAR_HI		0x34	/* Sample At Reset High */

/* Multiprocessor Interrupt Controller Registers */
#define	ARMADAXP_MLMB_MPIC_BASE			0x20a00
#define	ARMADAXP_MLMB_MPIC_CPU_BASE		0x21800
#define	ARMADAXP_MLMB_MPIC_CTRL			0x0
#define	ARMADAXP_MLMB_MPIC_SOFT_INT		0x4
#define	ARMADAXP_MLMB_MPIC_ERR_CAUSE		0x20
#define	ARMADAXP_MLMB_MPIC_ISE			0x30
#define	ARMADAXP_MLMB_MPIC_ICE			0x34
#define	ARMADAXP_MLMB_MPIC_ISCR_BASE		0x100

#define	ARMADAXP_MLMB_MPIC_DOORBELL		0x78
#define	ARMADAXP_MLMB_MPIC_DOORBELL_MASK		0x7c
#define	ARMADAXP_MLMB_MPIC_CTP			0xb0
#define	ARMADAXP_MLMB_MPIC_IIACK			0xb4
#define	ARMADAXP_MLMB_MPIC_ISM			0xb8
#define	ARMADAXP_MLMB_MPIC_ICM			0xbc
#define	ARMADAXP_MLMB_MPIC_ERR_MASK		0xec0

/* Multiprocessor Interrupt Controller Shifts */
#define	MPIC_CTP_SHIFT			28 /* Global priority level field */
#define	MPIC_ISCR_SHIFT			24 /* IRQ source priority level field */

/* Cache Main Control */
#define ARMADAXP_L2_BASE		0x8000
#define ARMADAXP_L2_CTRL		0x100
#define ARMADAXP_L2_AUX_CTRL		0x104
#define ARMADAXP_L2_CNTR_CTRL		0x200
#define ARMADAXP_L2_CNTR_CONF(x)	(0x204 + (x) * 0xc)
#define ARMADAXP_L2_INT_CAUSE		0x220
#define ARMADAXP_L2_CFU			0x228
#define ARMADAXP_L2_INV_WAY		0x778
#define L2_ENABLE			(1 << 0)
#define L2_WBWT_MODE_MASK		(3 << 0)
#define L2_REP_STRAT_MASK		(3 << 27)
#define L2_REP_STRAT_SEMIPLRU		(3 << 27)
#define L2_ALL_WAYS			0xffffffff

#endif /* _ARMADAXPREG_H_ */
