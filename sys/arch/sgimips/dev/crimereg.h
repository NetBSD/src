/*	$NetBSD: crimereg.h,v 1.7 2003/10/04 09:19:23 tsutsui Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * O2 CRIME register definitions
 */

#define CRIME_BASE		0x14000000 /* all registers 64-bit access */

/* Offset 0x00 -- revision register */
#define CRIME_REV         (CRIME_BASE+0x000)
#define CRIME_ID_IDBITS   0xf0
#define CRIME_ID_IDSHIFT  4
#define CRIME_ID_REV      0x0f
#define CRIME_REV_PETTY   0x0
#define CRIME_REV_11      0x11
#define CRIME_REV_13      0x13
#define CRIME_REV_14      0x14

/* offset 0x08 -- control register.  Only lower 14 bits are valid*/
#define CRIME_CONTROL			  (CRIME_BASE+0x008)
#define CRIME_CONTROL_TRITON_SYSADC       0x2000
#define CRIME_CONTROL_CRIME_SYSADC        0x1000
#define CRIME_CONTROL_HARD_RESET          0x0800
#define CRIME_CONTROL_SOFT_RESET          0x0400
#define CRIME_CONTROL_DOG_ENABLE          0x0200
#define CRIME_CONTROL_ENDIANESS           0x0100 /* assert for BE */
#define CRIME_CONTROL_CQUEUE_HWM          0x000f
#define CRIME_CONTROL_CQUEUE_SHFT         0
#define CRIME_CONTROL_WBUF_HWM            0x00f0
#define CRIME_CONTROL_WBUF_SHFT           8

/*
 * macros to manipulate CRIME High Water Mark bits in
 * the CRIME control register.  Examples:
 *
 * foo = CRM_CONTROL_GET_CQUEUE_HWM(*(__uint64_t *)CRM_CONTROL)
 * CRM_CONTROL_SET_CQUEUE_HWM(*(__uint64_t *)CRM_CONTROL, 4)
 *
 * foo = CRM_CONTROL_GET_WBUF_HWM(*(__uint64_t *)CRM_CONTROL)
 * CRM_CONTROL_SET_WBUF_HWM(*(__uint64_t *)CRM_CONTROL, 4)
 */
#define CRM_CONTROL_GET_CQUEUE_HWM(x)   \
	(((x) & CRM_CONTROL_CQUEUE_HWM) >> CRM_CONTROL_CQUEUE_SHFT)
#define CRM_CONTROL_SET_CQUEUE_HWM(x,v) \
	(((v) << CRM_CONTROL_CQUEUE_SHFT) | ((x) & ~CRM_CONTROL_CQUEUE_HWM))

#define CRM_CONTROL_GET_WBUF_HWM(x)     \
	(((x) & CRM_CONTROL_WBUF_HWM) >> CRM_CONTROL_WBUF_SHFT)
#define CRM_CONTROL_SET_WBUF_HWM(x,v)   \
	(((v) << CRM_CONTROL_WBUF_SHFT) | ((x) & ~CRM_CONTROL_WBUF_HWM))


/* Offset 0x010 -- interrupt status register.  All 32 bits valid */
#define CRIME_INTSTAT		(CRIME_BASE+0x010)
#define CRIME_INT_VICE		0x80000000
#define CRIME_INT_SOFT2		0x40000000 /* Also CPU_SysCorErr */
#define CRIME_INT_SOFT1		0x20000000
#define CRIME_INT_SOFT0		0x10000000
#define CRIME_INT_RE5		0x08000000
#define CRIME_INT_RE4		0x04000000
#define CRIME_INT_RE3		0x02000000
#define CRIME_INT_RE2		0x01000000
#define CRIME_INT_RE1		0x00800000
#define CRIME_INT_RE0		0x00400000
#define CRIME_INT_MEMERR	0x00200000
#define CRIME_INT_CRMERR	0x00100000
#define CRIME_INT_GBE3		0x00080000
#define CRIME_INT_GBE2		0x00040000
#define CRIME_INT_GBE1		0x00020000
#define CRIME_INT_GBE0		0x00010000
#define CRIME_INT_PCI_SHARED2	0x00008000 /* from here, actually mace irqs */
#define CRIME_INT_PCI_SHARED1	0x00004000
#define CRIME_INT_PCI_SHARED0	0x00002000
#define CRIME_INT_PCI_SLOT2	0x00001000
#define CRIME_INT_PCI_SLOT1	0x00000800
#define CRIME_INT_PCI_SLOT0	0x00000400
#define CRIME_INT_PCI_SCSI1	0x00000200
#define CRIME_INT_PCI_SCSI0	0x00000100
#define CRIME_INT_PCI_BRIDGE	0x00000080
#define CRIME_INT_PERIPH_AUD	0x00000040
#define CRIME_INT_PERIPH_MISC	0x00000020
#define CRIME_INT_PERIPH_SERIAL	0x00000010
#define CRIME_INT_ETHERNET	0x00000008
#define CRIME_INT_VID_OUT	0x00000004
#define CRIME_INT_VID_IN2	0x00000002
#define CRIME_INT_VID_IN1	0x00000001

/* Masks, hard interrupts, soft interrupts.  Don't know what to do with these */
#define CRIME_INTMASK		(CRIME_BASE+0x018)
#define CRIME_SOFTINT		(CRIME_BASE+0x020)
#define CRIME_HARDINT		(CRIME_BASE+0x028)

/*
 * Offset 0x030 -- watchdog register.  33 bits are valid 
 * Bit   32:  power on reset
 * Bit	  31:  warm reset
 * Write zero here to reset watchdog
 */

#define CRIME_DOG		(CRIME_BASE+0x030)
#define CRIME_WATCHDOG		CRIME_DOG
#define CRIME_TIME		(CRIME_BASE+0x038)
#define CRIME_TIME_MASK		0x0000ffffffffffff
#define CRIME_CPU_ERROR_ADDR	(CRIME_BASE+0x040)
#define CRIME_CPU_ERROR_STAT	(CRIME_BASE+0x048)
#define CRIME_CPU_ERROR_ENA	(CRIME_BASE+0x050)
#define CRIME_VICE_ERROR_ADDR	(CRIME_BASE+0x058)
#define CRIME_MEM_CONTROL	(CRIME_BASE+0x200)
#define CRIME_MEM_BANK_CTRL0	(CRIME_BASE+0x208)
#define CRIME_MEM_BANK_CTRL1	(CRIME_BASE+0x218)
#define CRIME_MEM_BANK_CTRL2	(CRIME_BASE+0x210)
#define CRIME_MEM_BANK_CTRL3	(CRIME_BASE+0x228)
#define CRIME_MEM_BANK_CTRL4	(CRIME_BASE+0x220)
#define CRIME_MEM_BANK_CTRL5	(CRIME_BASE+0x238)
#define CRIME_MEM_BANK_CTRL6	(CRIME_BASE+0x230)
#define CRIME_MEM_BANK_CTRL7	(CRIME_BASE+0x248)
#define CRIME_MEM_REFRESH_CNTR	(CRIME_BASE+0x248)
#define CRIME_MEM_ERROR_STAT	(CRIME_BASE+0x250)
#define CRIME_MEM_ERROR_ADDR	(CRIME_BASE+0x258)
#define CRIME_MEM_ERROR_ECC_SYN	(CRIME_BASE+0x260)
#define CRIME_MEM_ERROR_ECC_CHK	(CRIME_BASE+0x268)
#define CRIME_MEM_ERROR_ECC_REPL (CRIME_BASE+0x270)

#define McGriff CRIME_DOG /* Baseball compatibility */
