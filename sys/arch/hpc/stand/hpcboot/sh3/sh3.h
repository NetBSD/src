/* -*-C++-*-	$NetBSD: sh3.h,v 1.5 2002/02/04 17:38:27 uch Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef _HPCBOOT_SH3_H_
#define _HPCBOOT_SH3_H_
/*
 * SH3 for Windows CE(SH7709, SH7709A) common defines.
 */

/*
 * Address space.
 */
#define SH_P0_START		0x0
#define SH_P1_START		0x80000000
#define SH_P2_START		0xa0000000
#define SH_P3_START		0xc0000000
#define SH_P4_START		0xe0000000

/* 
 * D-RAM(CS3)
 */
#define DRAM_BANK_NUM		2
#define DRAM_BANK_SIZE		0x02000000	/* 32MByte */

#define DRAM_BANK0_START	0x0c000000
#define DRAM_BANK0_SIZE		DRAM_BANK_SIZE
#define DRAM_BANK1_START	0x0e000000
#define DRAM_BANK1_SIZE		DRAM_BANK_SIZE

/* 
 * MMU 
 */
/* 4-way set-associative 32-entry(total 128 entries) */
#define MMU_WAY			4
#define MMU_ENTRY		32

/* Windows CE uses 1Kbyte page for SH3, 4Kbyte for SH4 */
#define SH3_PAGE_SIZE		0x400
#define SH3_PAGE_MASK		(~(SH3_PAGE_SIZE - 1))
#define SH4_PAGE_SIZE		0x1000
#define SH4_PAGE_MASK		(~(SH4_PAGE_SIZE - 1))

#define MMUPTEH			0xfffffff0
#define MMUPTEH_ASID_MASK	0x0000000f
#define MMUPTEH_VPN_MASK	0xfffffc00
#ifdef SH4
#define MMUCR			0xff000010
#else
#define MMUCR			0xffffffe0
#endif
#define MMUCR_AT		0x00000001
#define MMUCR_IX		0x00000002
#define MMUCR_TF		0x00000004
#define MMUCR_RC		0x00000030
#define MMUCR_SV		0x00000100

#define MMUAA			0xf2000000	/* Address array */
#define MMUDA			0xf3000000	/* Data array */

#define MMUAA_VPN_MASK		0x0001f000
#define MMUAA_WAY_SHIFT		8
#define MMUAA_D_VALID		0x00000100
#define MMUAA_D_VPN_MASK	0xfffe0c00
#define MMUAA_D_ASID_MASK	0x0000000f

#define MMUDA_D_PPN_MASK	0xfffffc00

/* 
 * Cache (Windows CE uses normal-mode.)
 */
#define CCR			0xffffffec
#define CCR_CE			0x00000001
#define CCR_WT			0x00000002
#define CCR_CB			0x00000004
#define CCR_CF			0x00000008
#define CCR_RA			0x00000020

#define CCA			0xf0000000
#define CCD			0xf1000000

/*
 * Interrupt
 */
/* R/W 16bit */
#define ICU_ICR0_REG16		0xfffffee0
#define ICU_ICR1_REG16		0x04000010
#define ICU_ICR2_REG16		0x04000012
#define ICU_PINTER_REG16	0x04000014
#define ICU_IPRA_REG16		0xfffffee2
#define ICU_IPRB_REG16		0xfffffee4
#define ICU_IPRC_REG16		0x04000016
#define ICU_IPRD_REG16		0x04000018
#define ICU_IPRE_REG16		0x0400001a
/* R/W 8bit */
#define ICU_IRR0_REG8		0x04000004
/* R 8bit */
#define ICU_IRR1_REG8		0x04000006
#define ICU_IRR2_REG8		0x04000008

#define ICU_ICR0_NMIL		0x8000
#define ICU_ICR0_NMIE		0x0100

#define ICU_ICR1_MAI		0x8000
#define ICU_ICR1_IRQLVL		0x4000
#define ICU_ICR1_BLMSK		0x2000
#define ICU_ICR1_IRLSEN		0x1000
#define ICU_ICR1_IRQ51S		0x0800
#define ICU_ICR1_IRQ50S		0x0400
#define ICU_ICR1_IRQ41S		0x0200
#define ICU_ICR1_IRQ40S		0x0100
#define ICU_ICR1_IRQ31S		0x0080
#define ICU_ICR1_IRQ30S		0x0040
#define ICU_ICR1_IRQ21S		0x0020
#define ICU_ICR1_IRQ20S		0x0010
#define ICU_ICR1_IRQ11S		0x0008
#define ICU_ICR1_IRQ10S		0x0004
#define ICU_ICR1_IRQ01S		0x0002
#define ICU_ICR1_IRQ00S		0x0001

#define ICU_SENSE_SELECT_MASK		0x3
#define ICU_SENSE_SELECT_FALLING_EDGE	0x0
#define ICU_SENSE_SELECT_RAISING_EDGE	0x1
#define ICU_SENSE_SELECT_LOW_LEVEL	0x2
#define ICU_SENSE_SELECT_RESERVED	0x3

#define ICU_ICR2_PINT15S	0x8000
#define ICU_ICR2_PINT14S	0x4000
#define ICU_ICR2_PINT13S	0x2000
#define ICU_ICR2_PINT12S	0x1000
#define ICU_ICR2_PINT11S	0x0800
#define ICU_ICR2_PINT10S	0x0400
#define ICU_ICR2_PINT9S		0x0200
#define ICU_ICR2_PINT8S		0x0100
#define ICU_ICR2_PINT7S		0x0080
#define ICU_ICR2_PINT6S		0x0040
#define ICU_ICR2_PINT5S		0x0020
#define ICU_ICR2_PINT4S		0x0010
#define ICU_ICR2_PINT3S		0x0008
#define ICU_ICR2_PINT2S		0x0004
#define ICU_ICR2_PINT1S		0x0002
#define ICU_ICR2_PINT0S		0x0001

#define ICU_IPR_MASK		0xf

/*
 * Bus State Controller
 */
#define SH3_BSC_BCR1_REG		0xffffff60
#define SH3_BSC_BCR2_REG		0xffffff62
#define SH3_BSC_WCR1_REG		0xffffff64
#define SH3_BSC_WCR2_REG		0xffffff66
#define SH3_BSC_MCR_REG			0xffffff68
#define SH3_BSC_DCR_REG			0xffffff6a
#define SH3_BSC_PCR_REG			0xffffff6c
#define SH3_BSC_RTCSR_REG		0xffffff6e
#define SH3_BSC_RTCNT_REG		0xffffff70
#define SH3_BSC_RTCOR_REG		0xffffff72
#define SH3_BSC_RFCR_REG		0xffffff74
#define SH3_BSC_BCR3_REG		0xffffff7e

/*
 * Pin Function Controller
 */
#define SH3_PACR_REG16			0xa4000100
#define SH3_PBCR_REG16			0xa4000102
#define SH3_PCCR_REG16			0xa4000104
#define SH3_PDCR_REG16			0xa4000106
#define SH3_PECR_REG16			0xa4000108
#define SH3_PFCR_REG16			0xa400010a
#define SH3_PGCR_REG16			0xa400010c
#define SH3_PHCR_REG16			0xa400010e
#define SH3_PJCR_REG16			0xa4000110
#define SH3_PKCR_REG16			0xa4000112
#define SH3_PLCR_REG16			0xa4000114
#define SH3_SCPCR_REG16			0xa4000116

/*
 * I/O port
 */
#define SH3_PADR_REG8			0xa4000120
#define SH3_PBDR_REG8			0xa4000122
#define SH3_PCDR_REG8			0xa4000124
#define SH3_PDDR_REG8			0xa4000126
#define SH3_PEDR_REG8			0xa4000128
#define SH3_PFDR_REG8			0xa400012a
#define SH3_PGDR_REG8			0xa400012c
#define SH3_PHDR_REG8			0xa400012e
#define SH3_PJDR_REG8			0xa4000130
#define SH3_PKDR_REG8			0xa4000132
#define SH3_PLDR_REG8			0xa4000134
#define SH3_SCPDR_REG8			0xa4000136

/*
 * TMU
 */
#define SH3_TOCR_REG8			0xfffffe90
#define TOCR_TCOE		0x01
#define SH3_TSTR_REG8			0xfffffe92
#define TSTR_STR2		0x04
#define TSTR_STR1		0x02
#define TSTR_STR0		0x01
#define SH3_TCOR0_REG			0xfffffe94
#define SH3_TCNT0_REG			0xfffffe98
#define SH3_TCR0_REG16			0xfffffe9c
#define SH3_TCOR1_REG			0xfffffea0
#define SH3_TCNT1_REG			0xfffffea4
#define SH3_TCR1_REG16			0xfffffea8
#define SH3_TCOR2_REG			0xfffffeac
#define SH3_TCNT2_REG			0xfffffeb0
#define SH3_TCR2_REG16			0xfffffeb4
#define SH3_TCPR2_REG			0xfffffeb8
#define TCR_ICPF	0x0200
#define TCR_UNF		0x0100
#define TCR_ICPE1	0x0080
#define TCR_ICPE0	0x0040
#define TCR_UNIE	0x0020
#define TCR_CKEG1	0x0010
#define TCR_CKEG0	0x0008
#define TCR_TPSC2	0x0004
#define TCR_TPSC1	0x0002
#define TCR_TPSC0	0x0001

#define TCR_TPSC_P4	0x0000
#define TCR_TPSC_P16	0x0001
#define TCR_TPSC_P64	0x0002
#define TCR_TPSC_P256	0x0003

/*   suspend/resume external Interrupt. 
 *  (don't block) use under privilege mode.
 */
__BEGIN_DECLS
u_int32_t suspendIntr(void);
void resumeIntr(u_int32_t);
__END_DECLS

/*
 * SCI
 */
#ifdef SH4
#define SCI_SCSMR_REG8			0xffe00000
#define SCI_SCBRR_REG8			0xffe00004
#define SCI_SCSCR_REG8			0xffe00008
#define SCI_SCTDR_REG8			0xffe0000c
#define SCI_SCSSR_REG8			0xffe00010
#define SCI_SCRDR_REG8			0xffe00014
#else
#define SCI_SCRSR_REG8			/* can't access from CPU */
#define SCI_SCTSR_REG8			/* can't access from CPU */
#define SCI_SCSMR_REG8			0xfffffe80
#define SCI_SCBRR_REG8			0xfffffe82
#define SCI_SCSCR_REG8			0xfffffe84
#define SCI_SCTDR_REG8			0xfffffe86
#define SCI_SCSSR_REG8			0xfffffe88
#define SCI_SCRDR_REG8			0xfffffe8a
#define SCI_SCPCR_REG16			0xa4000116
#define SCI_SCPDR_REG16			0xa4000136
#endif
#define SCI_SCSSR_TDRE			0x80

#define SCI_TX_BUSY()							\
	while ((VOLATILE_REF8(SCI_SCSSR_REG8) & SCI_SCSSR_TDRE) == 0)

#define SCI_PUTC(c)							\
__BEGIN_MACRO								\
	SCI_TX_BUSY();							\
	VOLATILE_REF8(SCI_SCTDR_REG8) = (c);				\
	VOLATILE_REF8(SCI_SCSSR_REG8) &= ~SCI_SCSSR_TDRE;		\
__END_MACRO

#define SCI_PRINT(s)							\
__BEGIN_MACRO								\
	char *__s =(char *)(s);						\
	int __i;							\
	for (__i = 0; __s[__i] != '\0'; __i++) {			\
		char __c = __s[__i];					\
		if (__c == '\n')					\
			SCI_PUTC('\r');					\
		SCI_PUTC(__c);						\
	}								\
__END_MACRO

/* 
 * SCIF
 */
#ifdef SH4
#define SCIF_SCSMR2_REG16		0xffe80000
#define SCIF_SCBRR2_REG8		0xffe80004
#define SCIF_SCSCR2_REG16		0xffe80008
#define SCIF_SCFTDR2_REG8		0xffe8000c
#define SCIF_SCFSR2_REG16		0xffe80010
#define SCIF_SCFRDR2_REG8		0xffe80014
#define SCIF_SCFCR2_REG16		0xffe80018
#define SCIF_SCFDR2_REG16		0xffe8001c
#define SCIF_SCSPTR2_REG16		0xffe80020
#define SCIF_SCLSR2_REG16		0xffe80024
#define	SCIF_SCSSR2_REG16		SCIF_SCFSR2_REG16
#else
#define SCIF_SCSMR2_REG8		0xa4000150	/* R/W */
#define SCIF_SCBRR2_REG8		0xa4000152	/* R/W */
#define SCIF_SCSCR2_REG8		0xa4000154	/* R/W */
#define SCIF_SCFTDR2_REG8		0xa4000156	/* W */
#define SCIF_SCSSR2_REG16		0xa4000158	/* R/W(0 write only) */
#define SCIF_SCFRDR2_REG8		0xa400015a	/* R */
#define SCIF_SCFCR2_REG8		0xa400015c	/* R/W */
#define SCIF_SCFDR2_REG16		0xa400015e	/* R */
#endif

/* Transmit FIFO Data Empty */
#define SCIF_SCSSR2_TDFE		0x00000020
/* Transmit End */
#define SCIF_SCSSR2_TEND		0x00000040

/* simple serial console macros. */
#define SCIF_TX_BUSY()							\
	while ((VOLATILE_REF16(SCIF_SCSSR2_REG16) & SCIF_SCSSR2_TDFE) == 0)

#define SCIF_PUTC(c)							\
__BEGIN_MACRO								\
	SCIF_TX_BUSY();							\
	/*  wait until previous transmit done. */			\
	VOLATILE_REF8(SCIF_SCFTDR2_REG8) = (c);				\
	/* Clear transmit FIFO empty flag */				\
	VOLATILE_REF16(SCIF_SCSSR2_REG16) &=				\
	~(SCIF_SCSSR2_TDFE | SCIF_SCSSR2_TEND);				\
__END_MACRO

#define SCIF_PRINT(s)							\
__BEGIN_MACRO								\
	char *__s =(char *)(s);						\
	int __i;							\
	for (__i = 0; __s[__i] != '\0'; __i++) {			\
		char __c = __s[__i];					\
		if (__c == '\n')					\
			SCIF_PUTC('\r');				\
		SCIF_PUTC(__c);						\
	}								\
__END_MACRO

#define SCIF_PRINT_HEX(h)						\
__BEGIN_MACRO								\
	u_int32_t __h =(u_int32_t)(h);					\
	int __i;							\
	SCIF_PUTC('0'); SCIF_PUTC('x');					\
	for (__i = 0; __i < 8; __i++, __h <<= 4) {			\
		int __n =(__h >> 28) & 0xf;				\
		char __c = __n > 9 ? 'A' + __n - 10 : '0' + __n;	\
		SCIF_PUTC(__c);						\
	}								\
	SCIF_PUTC('\r'); SCIF_PUTC('\n');				\
__END_MACRO

/* 
 * Product dependent headers
 */
#include <sh3/sh_7707.h>
#include <sh3/sh_7709.h>
#include <sh3/sh_7709a.h>
#include <sh3/sh_7750.h>

#endif // _HPCBOOT_SH3_H_
