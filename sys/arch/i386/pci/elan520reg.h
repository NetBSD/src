/*	$NetBSD: elan520reg.h,v 1.1.30.4 2008/01/21 09:37:11 yamt Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * Register definitions for the AMD Elan SC520 System Controller.
 */

#ifndef _I386_PCI_ELAN520REG_H_
#define	_I386_PCI_ELAN520REG_H_

#include <sys/cdefs.h>

#define	MMCR_BASE_ADDR		0xfffef000

/*
 * Am5x86 CPU Registers.
 */
#define	MMCR_REVID		0x0000
#define	MMCR_CPUCTL		0x0002

#define	REVID_PRODID		0xff00	/* product ID */
#define	REVID_PRODID_SHIFT	8
#define	REVID_MAJSTEP		0x00f0	/* stepping major */
#define	REVID_MAJSTEP_SHIFT	4
#define	REVID_MINSTEP		0x000f	/* stepping minor */

#define	PRODID_ELAN_SC520	0x00	/* Elan SC520 */

#define	CPUCTL_CPU_CLK_SPD_MASK	0x03	/* CPU clock speed */
#define	CPUCTL_CACHE_WR_MODE	0x10	/* cache mode (0 = wb, 1 = wt) */

/*
 * Performance Registers
 */
#define MMCR_DBCTL      0x0040  /* SDRAM Buffer Control */

#define	MMCR_DBCTL_RAB_ENB	__BIT(4)	/* enable read-ahead */
#define	MMCR_DBCTL_WB_WM_MASK	__BITS(3,2)	/* write buffer watermark */
#define	MMCR_DBCTL_WB_WM_28DW	__SHIFTIN(0, MMCR_DBCTL_WB_WM_MASK)
#define	MMCR_DBCTL_WB_WM_24DW	__SHIFTIN(1, MMCR_DBCTL_WB_WM_MASK)
#define	MMCR_DBCTL_WB_WM_16DW	__SHIFTIN(2, MMCR_DBCTL_WB_WM_MASK)
#define	MMCR_DBCTL_WB_WM_8DW	__SHIFTIN(3, MMCR_DBCTL_WB_WM_MASK)
#define	MMCR_DBCTL_WB_FLUSH	__BIT(1)	/* write 1 to flush wr buf */
#define	MMCR_DBCTL_WB_ENB	__BIT(0)	/* enable write buffer */
#define MMCR_HBCTL      0x0060  /* Host Bridge Control */
#define	MMCR_HBCTL_PCI_RST		__BIT(15)
#define	MMCR_HBCTL_T_PURGE_RD_ENB	__BIT(10)
#define	MMCR_HBCTL_T_DLYTR_ENB_MASK	__BITS(9,8)
#define	MMCR_HBCTL_T_DLYTR_ENB_WAIT	\
    __SHIFTIN(0, MMCR_HBCTL_T_DLYTR_ENB_MASK)
#define	MMCR_HBCTL_T_DLYTR_ENB_AUTORETRY\
    __SHIFTIN(1, MMCR_HBCTL_T_DLYTR_ENB_MASK)
#define	MMCR_HBCTL_T_DLYTR_ENB_RSVD0	\
    __SHIFTIN(2, MMCR_HBCTL_T_DLYTR_ENB_MASK)
#define	MMCR_HBCTL_T_DLYTR_ENB_RSVD1	\
    __SHIFTIN(3, MMCR_HBCTL_T_DLYTR_ENB_MASK)
#define	MMCR_HBCTL_M_WPOST_ENB		__BIT(3)
#define MMCR_SYSARBCTL  0x0070  /* System Arbiter Control */
#define MMCR_SYSARBCTL_CNCR_MODE_ENB	__BIT(1)
#define MMCR_SYSARBCTL_GNT_TO_INT_ENB	__BIT(0)	/* 1: interrupt when the
    							 * PCI bus arbiter
							 * detects a time-out
							 */

#define	MMCR_PCIARBSTA	0x71	/* PCI Bus Arbiter Status */
#define	MMCR_PCIARBSTA_GNT_TO_STA	__BIT(7)
#define	MMCR_PCIARBSTA_GNT_TO_ID	__BITS(3, 0)

/*
 * PCI Host Bridge Registers
 */
#define	MMCR_HBMSTIRQCTL	0x66	/* Host Bridge Master Interrupt Ctrl */

#define	MMCR_HBMSTIRQCTL_RSVD0			__BITS(15, 14)

/* Interrupt Selects
 *
 * 0: generate maskable interrupt (see MMCR_PCIHOSTMAP)
 * 1: generate NMI
 */
/* Master Retry Time-Out */
#define	MMCR_HBMSTIRQCTL_M_RTRTO_IRQ_SEL	__BIT(13)
/* Master Target Abort */
#define	MMCR_HBMSTIRQCTL_M_TABRT_IRQ_SEL	__BIT(12)
/* Master Abort */
#define	MMCR_HBMSTIRQCTL_M_MABRT_IRQ_SEL	__BIT(11)
/* Master System Error */
#define	MMCR_HBMSTIRQCTL_M_SERR_IRQ_SEL		__BIT(10)
/* Master Received PERR */
#define	MMCR_HBMSTIRQCTL_M_RPER_IRQ_SEL		__BIT(9)
/* Master Detected PERR */
#define	MMCR_HBMSTIRQCTL_M_DPER_IRQ_SEL		__BIT(8)
#define	MMCR_HBMSTIRQCTL_RSVD1			__BITS(7, 6)

/* Interrupt Enables */
/* Master Retry Time-Out */
#define	MMCR_HBMSTIRQCTL_M_RTRTO_IRQ_ENB	__BIT(5)	
/* Master Target Abort */
#define	MMCR_HBMSTIRQCTL_M_TABRT_IRQ_ENB	__BIT(4)	
/* Master Abort */
#define	MMCR_HBMSTIRQCTL_M_MABRT_IRQ_ENB	__BIT(3)	
/* Master System Error */
#define	MMCR_HBMSTIRQCTL_M_SERR_IRQ_ENB		__BIT(2)
/* Master Received PERR */
#define	MMCR_HBMSTIRQCTL_M_RPER_IRQ_ENB		__BIT(1)	
/* Master Detected PERR */
#define	MMCR_HBMSTIRQCTL_M_DPER_IRQ_ENB		__BIT(0)	

/* Host Bridge Target Interrupt Ctrl.  16 bits. */
#define	MMCR_HBTGTIRQCTL	0x62

#define	MMCR_HBTGTIRQCTL_RSVD0			__BITS(15, 11)

/* Interrupt Selects
 *
 * 0: generate maskable interrupt (see MMCR_PCIHOSTMAP)
 * 1: generate NMI
 */
#define	MMCR_HBTGTIRQCTL_T_DLYTO_IRQ_SEL	__BIT(10)
#define	MMCR_HBTGTIRQCTL_T_APER_IRQ_SEL		__BIT(9)
#define	MMCR_HBTGTIRQCTL_T_DPER_IRQ_SEL		__BIT(8)
#define	MMCR_HBTGTIRQCTL_RSVD1			__BITS(7, 3)

/* Interrupt Enables */
/* Target Delayed Transaction Time-out */
#define	MMCR_HBTGTIRQCTL_T_DLYTO_IRQ_ENB	__BIT(2)
/* Target Address Parity */
#define	MMCR_HBTGTIRQCTL_T_APER_IRQ_ENB		__BIT(1)
/* Target Data Parity */
#define	MMCR_HBTGTIRQCTL_T_DPER_IRQ_ENB		__BIT(0)

/* Host Bridge Master Interrupt Status.  16 bits. */
#define	MMCR_HBMSTIRQSTA	0x68

/* Host Bridge Master Interrupt Address */
#define	MMCR_MSTINTADD		0x6c

#define	MMCR_HBMSTIRQSTA_RSVD0			__BITS(15, 12)
#define	MMCR_HBMSTIRQSTA_M_CMD_IRQ_ID		__BITS(11, 8)
#define	MMCR_HBMSTIRQSTA_RSVD1			__BITS(7, 6)
#define	MMCR_HBMSTIRQSTA_M_RTRTO_IRQ_STA	__BIT(5)	
#define	MMCR_HBMSTIRQSTA_M_TABRT_IRQ_STA	__BIT(4)	
#define	MMCR_HBMSTIRQSTA_M_MABRT_IRQ_STA	__BIT(3)	
#define	MMCR_HBMSTIRQSTA_M_SERR_IRQ_STA		__BIT(2)
#define	MMCR_HBMSTIRQSTA_M_RPER_IRQ_STA		__BIT(1)	
#define	MMCR_HBMSTIRQSTA_M_DPER_IRQ_STA		__BIT(0)	

/* The PCI master interrupts that NetBSD is interested in. */
#define	MMCR_MSTIRQ_ACT	(MMCR_HBMSTIRQCTL_M_RTRTO_IRQ_ENB |\
			 MMCR_HBMSTIRQCTL_M_TABRT_IRQ_ENB |\
			 MMCR_HBMSTIRQCTL_M_MABRT_IRQ_ENB |\
			 MMCR_HBMSTIRQCTL_M_SERR_IRQ_ENB |\
			 MMCR_HBMSTIRQCTL_M_RPER_IRQ_ENB |\
			 MMCR_HBMSTIRQCTL_M_DPER_IRQ_ENB)

/* Host Bridge Target Interrupt Status.  16 bits. */
#define	MMCR_HBTGTIRQSTA	0x64

#define	MMCR_HBTGTIRQSTA_RSVD0			__BITS(15, 12)
/* Target Interrupt Identification */
#define	MMCR_HBTGTIRQSTA_T_IRQ_ID		__BITS(11, 8)
#define	MMCR_HBTGTIRQSTA_RSVD1			__BITS(7, 3)
/* Status bits.  Write 1 to clear. */
#define	MMCR_HBTGTIRQSTA_T_DLYTO_IRQ_STA	__BIT(2)
#define	MMCR_HBTGTIRQSTA_T_APER_IRQ_STA		__BIT(1)
#define	MMCR_HBTGTIRQSTA_T_DPER_IRQ_STA		__BIT(0)

/* The PCI target interrupts that NetBSD is interested in. */
#define	MMCR_TGTIRQ_ACT	(MMCR_HBTGTIRQSTA_T_DLYTO_IRQ_STA |\
			 MMCR_HBTGTIRQSTA_T_APER_IRQ_STA |\
			 MMCR_HBTGTIRQSTA_T_DPER_IRQ_STA)

#define	MMCR_PCIHOSTMAP	0x0d14	/* PCI Host Bridge Interrupt Mapping */

#define	MMCR_PCIHOSTMAP_PCI_NMI_ENB	__BIT(8)
#define	MMCR_PCIHOSTMAP_PCI_IRQ_MAP	__BITS(4, 0)

/* Programmable Interrupt Controller.  8 bits. */
#define	MMCR_PICICR			0xd00
#define	MMCR_PICICR_NMI_DONE		__BIT(7)
#define	MMCR_PICICR_NMI_ENB		__BIT(6)
#define	MMCR_PICICR_RSVD0		__BITS(5, 3)
#define	MMCR_PICICR_S2_GINT_MODE	__BIT(2)
#define	MMCR_PICICR_S1_GINT_MODE	__BIT(1)
#define	MMCR_PICICR_M_GINT_MODE		__BIT(0)

#define	MMCR_MPICMODE		0xd02
#define	MMCR_SL1PICMODE		0xd03
#define	MMCR_SL2PICMODE		0xd04

#define	MMCR_WPVMAP		0xd44
#define	MMCR_WPVMAP_RSVD0	__BITS(7, 5)
/* map write-protection violations to an Elan SC520 interrupt priority,
 * 1 through 22
 */ 
#define	MMCR_WPVMAP_INT_MAP	__BITS(4, 0)
/* no bits set -> disable */
#define	MMCR_WPVMAP_INT_OFF	0
/* all bits set -> NMI */
#define	MMCR_WPVMAP_INT_NMI	MMCR_WPVMAP_INT_MAP

#define	MMCR_ADDDECCTL		0x80
#define	MMCR_ADDDECCTL_WPV_INT_ENB	__BIT(7)

#define	MMCR_WPVSTA		0x82
#define	MMCR_WPVSTA_WPV_STA		__BIT(15)
#define	MMCR_WPVSTA_WPV_RSVD0		__BITS(14, 10)
#define	MMCR_WPVSTA_WPV_MSTR		__BITS(9, 8)
#define	MMCR_WPVSTA_WPV_MSTR_CPU	__SHIFTIN(0, MMCR_WPVSTA_WPV_MSTR)
#define	MMCR_WPVSTA_WPV_MSTR_PCI	__SHIFTIN(1, MMCR_WPVSTA_WPV_MSTR)
#define	MMCR_WPVSTA_WPV_MSTR_GP		__SHIFTIN(2, MMCR_WPVSTA_WPV_MSTR)
#define	MMCR_WPVSTA_WPV_MSTR_RSVD	__SHIFTIN(3, MMCR_WPVSTA_WPV_MSTR)
#define	MMCR_WPVSTA_WPV_RSVD1		__BITS(7, 4)
#define	MMCR_WPVSTA_WPV_WINDOW		__BITS(3, 0)

#define	MMCR_PAR(__i)		(0x88 + 4 * (__i))
#define	MMCR_PAR_TARGET		__BITS(31, 29)
#define	MMCR_PAR_TARGET_OFF	__SHIFTIN(0, MMCR_PAR_TARGET)
#define	MMCR_PAR_TARGET_GPIO	__SHIFTIN(1, MMCR_PAR_TARGET)
#define	MMCR_PAR_TARGET_GPMEM	__SHIFTIN(2, MMCR_PAR_TARGET)
#define	MMCR_PAR_TARGET_PCI	__SHIFTIN(3, MMCR_PAR_TARGET)
#define	MMCR_PAR_TARGET_BOOTCS	__SHIFTIN(4, MMCR_PAR_TARGET)
#define	MMCR_PAR_TARGET_ROMCS1	__SHIFTIN(5, MMCR_PAR_TARGET)
#define	MMCR_PAR_TARGET_ROMCS2	__SHIFTIN(6, MMCR_PAR_TARGET)
#define	MMCR_PAR_TARGET_SDRAM	__SHIFTIN(7, MMCR_PAR_TARGET)
#define	MMCR_PAR_ATTR		__BITS(28, 26)
#define	MMCR_PAR_ATTR_NOEXEC	__SHIFTIN(__BIT(2), MMCR_PAR_ATTR)
#define	MMCR_PAR_ATTR_NOCACHE	__SHIFTIN(__BIT(1), MMCR_PAR_ATTR)
#define	MMCR_PAR_ATTR_NOWRITE	__SHIFTIN(__BIT(0), MMCR_PAR_ATTR)
#define	MMCR_PAR_PG_SZ		__BIT(25)
#define	MMCR_PAR_SZ_ST_ADR	__BITS(24, 0)
#define	MMCR_PAR_4KB_SZ		__BITS(24, 18)
#define	MMCR_PAR_4KB_ST_ADR	__BITS(17, 0)
#define	MMCR_PAR_64KB_SZ	__BITS(24, 14)
#define	MMCR_PAR_64KB_ST_ADR	__BITS(13, 0)
#define	MMCR_PAR_IO_SZ		__BITS(24, 16)
#define	MMCR_PAR_IO_ST_ADR	__BITS(15, 0)

/*
 * General Purpose Bus Registers
 */
#define	MMCR_GPECHO		0x0c00	/* GP echo mode */
#define	MMCR_GPCSDW		0x0c01	/* GP chip sel data width */
#define	MMCR_CPCSQUAL		0x0c02	/* GP chip sel qualification */
#define	MMCR_GPCSRT		0x0c08	/* GP chip sel recovery time */
#define	MMCR_GPCSPW		0x0c09	/* GP chip sel pulse width */
#define	MMCR_GPCSOFF		0x0c0a	/* GP chip sel offset */
#define	MMCR_GPRDW		0x0c0b	/* GP read pulse width */
#define	MMCR_GPRDOFF		0x0c0c	/* GP read offset */
#define	MMCR_GPWRW		0x0c0d	/* GP write pulse width */
#define	MMCR_GPWROFF		0x0c0e	/* GP write offset */
#define	MMCR_GPALEW		0x0c0f	/* GPALE pulse width */
#define	MMCR_GPALEOFF		0x0c10	/* GPALE offset */

#define	GPECHO_GP_ECHO_ENB	0x01	/* GP bus echo mode enable */

/*
 * Programmable Input/Output Registers
 */
#define	MMCR_PIOPFS15_0		0x0c20	/* PIO15-PIO0 pin func sel */
#define	MMCR_PIOPFS31_16	0x0c22	/* PIO31-PIO16 pin func sel */
#define	MMCR_CSPFS		0x0c24	/* chip sel pin func sel */
#define	MMCR_CLKSEL		0x0c26	/* clock select */
#define	MMCR_DSCTL		0x0c28	/* drive strength control */
#define	MMCR_PIODIR15_0		0x0c2a	/* PIO15-PIO0 direction */
#define	MMCR_PIODIR31_16	0x0c2c	/* PIO31-PIO16 direction */
#define	MMCR_PIODATA15_0	0x0c30	/* PIO15-PIO0 data */
#define	MMCR_PIODATA31_16	0x0c32	/* PIO31-PIO16 data */
#define	MMCR_PIOSET15_0		0x0c34	/* PIO15-PIO0 set */
#define	MMCR_PIOSET31_16	0x0c36	/* PIO31-PIO16 set */
#define	MMCR_PIOCLR15_0		0x0c38	/* PIO15-PIO0 clear */
#define	MMCR_PIOCLR31_16	0x0c3a	/* PIO31-PIO16 clear */

#define	ELANSC_PIO_NPINS	32	/* total number of PIO pins */

/*
 * Watchdog Timer Registers.
 */
#define	MMCR_WDTMRCTL		0x0cb0	/* watchdog timer control */
#define	MMCR_WDTMRCNTL		0x0cb2	/* watchdog timer count low */
#define	MMCR_WDTMRCNTH		0x0cb4	/* watchdog timer count high */

#define	WDTMRCTL_EXP_SEL_MASK	0x00ff	/* exponent select */
#define	WDTMRCTL_EXP_SEL14	0x0001	/*	496us/492us */
#define	WDTMRCTL_EXP_SEL24	0x0002	/*	508ms/503ms */
#define	WDTMRCTL_EXP_SEL25	0x0004	/*	1.02s/1.01s */
#define	WDTMRCTL_EXP_SEL26	0x0008	/*	2.03s/2.01s */
#define	WDTMRCTL_EXP_SEL27	0x0010	/*	4.07s/4.03s */
#define	WDTMRCTL_EXP_SEL28	0x0020	/*	8.13s/8.05s */
#define	WDTMRCTL_EXP_SEL29	0x0040	/*	16.27s/16.11s */
#define	WDTMRCTL_EXP_SEL30	0x0080	/*	32.54s/32.21s */
#define	WDTMRCTL_IRQ_FLG	0x1000	/* interrupt request */
#define	WDTMRCTL_WRST_ENB	0x4000	/* watchdog timer reset enable */
#define	WDTMRCTL_ENB		0x8000	/* watchdog timer enable */

#define	WDTMRCTL_UNLOCK1	0x3333
#define	WDTMRCTL_UNLOCK2	0xcccc

#define	WDTMRCTL_RESET1		0xaaaa
#define	WDTMRCTL_RESET2		0x5555

/*
 * Reset Generation Registers.
 */
#define	MMCR_SYSINFO		0x0d70	/* system board information */
#define	MMCR_RESCFG		0x0d72	/* reset configuration */
#define	MMCR_RESSTA		0x0d74	/* reset status */

#define	RESCFG_SYS_RST		0x01	/* software system reset */
#define	RESCFG_GP_RST		0x02	/* assert GP bus reset */
#define	RESCFG_PRG_RST_ENB	0x04	/* programmable reset enable */
#define	RESCFG_ICE_ON_RST	0x08	/* enter AMDebug(tm) on reset */

#define	RESSTA_PWRGOOD_DET	0x01	/* POWERGOOD reset detect */
#define	RESSTA_PRGRST_DET	0x02	/* programmable reset detect */
#define	RESSTA_SD_RST_DET	0x04	/* CPU shutdown reset detect */
#define	RESSTA_WDT_RST_DET	0x08	/* watchdog timer reset detect */
#define	RESSTA_ICE_SRST_DET	0x10	/* AMDebug(tm) soft reset detect */
#define	RESSTA_ICE_HRST_DET	0x20	/* AMDebug(tm) soft reset detect */
#define	RESSTA_SCP_RST		0x40	/* SCP reset detect */

#endif /* _I386_PCI_ELAN520REG_H_ */
