/*	$NetBSD: rd94.h,v 1.2 2001/03/30 09:10:49 ur Exp $	*/
/*	$OpenBSD: pica.h,v 1.4 1996/09/14 15:58:28 pefo Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Per Fogelstrom
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
 *	This product includes software developed under OpenBSD by
 *	Per Fogelstrom.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#ifndef	_RD94_H_
#define	_RD94_H_ 1

/*
 * I/O map
 */

#define	RD94_P_LOCAL_IO_BASE	0x80000000	/* I/O Base address */
#define	RD94_V_LOCAL_IO_BASE	0xe0000000
#define	RD94_S_LOCAL_IO_BASE	0x00040000	/* Size */
#define RD94SYS RD94_V_LOCAL_IO_BASE

#define	RD94_SYS_CONFIG		(RD94SYS+0x000)	/* Global config register */
#define	RD94_SYS_RFAILADDR	(RD94SYS+0x010)	/* Remote failed address */
#define	RD94_SYS_MFAILADDR	(RD94SYS+0x018)	/* Memory failed address */
#define	RD94_SYS_INVALIDADDR	(RD94SYS+0x020)	/* Invalid address */
#define	RD94_SYS_TL_BASE	(RD94SYS+0x028)	/* DMA transl. table base */
#define	RD94_SYS_TL_LIMIT	(RD94SYS+0x030)	/* DMA transl. table limit */
#define	RD94_SYS_TL_IVALID	(RD94SYS+0x038)	/* DMA transl. cache inval */
#define	RD94_SYS_INTSTAT0	(RD94SYS+0x040)	/* Int0 status (DMA?) */
#define	RD94_SYS_INTSTAT1	(RD94SYS+0x048)	/* Int1 status (LB) */
#define	RD94_SYS_INTSTAT2	(RD94SYS+0x050)	/* Int2 status (PCI/EISA) */
#define	RD94_SYS_INTSTAT3	(RD94SYS+0x058)	/* Int3 status (Timer) */
#define	RD94_SYS_INTSTAT4	(RD94SYS+0x060)	/* Int4 status (IPI) */
#define	RD94_SYS_CPUID		(RD94SYS+0x070)	/* CPU number */
#define	RD94_SYS_NMISRC		(RD94SYS+0x078)	/* NMI source */
#define	RD94_SYS_EXT_IMASK	(RD94SYS+0x0f8)	/* External int enable mask */
#define	RD94_SYS_DMA0_REGS	(RD94SYS+0x100)	/* DMA ch0 base address */
#define	RD94_SYS_DMA1_REGS	(RD94SYS+0x120)	/* DMA ch0 base address */
#define	RD94_SYS_DMA2_REGS	(RD94SYS+0x140)	/* DMA ch0 base address */
#define	RD94_SYS_DMA3_REGS	(RD94SYS+0x160)	/* DMA ch0 base address */
#define	RD94_SYS_IT_VALUE	(RD94SYS+0x1a8)	/* Interval timer reload */
#define	RD94_SYS_IPI		(RD94SYS+0x1b8)	/* IPI register */
#define	RD94_SYS_ECCDIAG	(RD94SYS+0x1c8)	/* ECC diagnostics */
#define	RD94_SYS_PCI_CONFADDR	(RD94SYS+0x518)	/* PCI configuration address */
#define	RD94_SYS_PCI_CONFDATA	(RD94SYS+0x520)	/* PCI configuration data */
#define	RD94_SYS_PCI_INTMASK	(RD94SYS+0x530)	/* PCI interrupt mask */
#define	RD94_SYS_PCI_INTSTAT	(RD94SYS+0x538)	/* PCI interrupt status */
#define	RD94_SYS_BEEP_DIVISOR	(RD94SYS+0x5A8)	/* Beep frequency divisor */
#define	RD94_SYS_BEEP_CNTL	(RD94SYS+0x5AC)	/* Beep control */
#define	RD94_SYS_ERR_STAT	(RD94SYS+0x5BC)	/* System error status */

#define RD94LB RD94_V_LOCAL_IO_BASE
#define	RD94_SYS_SONIC		(RD94LB+0x1000)	/* SONIC base address */
#define	RD94_SYS_SCSI		(RD94LB+0x2000)	/* SCSI base address */
#define	RD94_SYS_CLOCK		(RD94LB+0x4000)	/* Clock base address */
#define	RD94_SYS_KBD		(RD94LB+0x5000)	/* Keybrd/mouse base address */
#define	RD94_SYS_COM1		(RD94LB+0x6000)	/* Com port 1 */
#define	RD94_SYS_COM2		(RD94LB+0x7000)	/* Com port 2 */
#define	RD94_SYS_PAR1		(RD94LB+0x8000)	/* Parallel port 1 */
#define	RD94_SYS_NVRAM		(RD94LB+0x9000)	/* Unprotected NV-ram */
#define	RD94_SYS_PNVRAM		(RD94LB+0xa000)	/* Protected NV-ram */
#define	RD94_SYS_NVPROM		(RD94LB+0xb000)	/* Read only NV-ram */
#define	RD94_SYS_FLOPPY		(RD94LB+0xC000)	/* Floppy base address */
#define	RD94_SYS_SOUND		(RD94LB+0x10000)/* Sound port */
#define	RD94_SYS_THERMOMETER	(RD94LB+0x12000)/* DS1620 thermometer */

#define	RD94_SYS_LB_LED		(RD94LB+0xE000)	/* LED/self-test register */
#define	RD94_SYS_LB_IE1		(RD94LB+0xF000)	/* Local bus int enable */
#define	RD94_SYS_LB_IE2		(RD94LB+0xF002)	/* Local bus int enable */
#define RD94_SYS_LB_IE_PAR1	0x0001		/* Parallel port enable */
#define	RD94_SYS_LB_IE_FLOPPY	0x0002		/* Floppy ctrl enable */
#define	RD94_SYS_LB_IE_SOUND	0x0004		/* Sound port enable */
#define	RD94_SYS_LB_IE_SONIC	0x0008		/* Ethernet ctrl enable */
#define	RD94_SYS_LB_IE_SCSI	0x0010		/* Scsi crtl enable */
#define RD94_SYS_LB_IE_KBD	0x0040		/* Keyboard ctrl enable */
#define RD94_SYS_LB_IE_MOUSE	0x0080		/* Mouse ctrl enable */
#define	RD94_SYS_LB_IE_COM1	0x0100		/* Serial port 1 enable */
#define	RD94_SYS_LB_IE_COM2	0x0200		/* Serial port 2 enable */

#define	RD94_P_PCI_IO		0x90000000	/* PCI I/O control */
#define	RD94_V_PCI_IO		0xe2000000
#define	RD94_S_PCI_IO		0x01000000

#define	RD94_P_PCI_MEM		0x100000000LL	/* PCI Memory control */
#define	RD94_V_PCI_MEM		0xe3000000
#define	RD94_S_PCI_MEM		0x40000000

#endif	/* _RD94_H_ */
