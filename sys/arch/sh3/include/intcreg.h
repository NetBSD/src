/* $NetBSD: intcreg.h,v 1.2.12.1 2000/08/08 18:35:54 msaitoh Exp $ */

/*-
 * Copyright (C) 1999 SAITOH Masanobu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_INTCREG_H__
#define _SH3_INTCREG_H__

#ifndef _BYTE_ORDER
#error Define _BYTE_ORDER!
#endif

/*
 * Interrupt Controller
 */
struct sh3_intc {
	/* Interrupt control register (0xFFFFFEE0) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if _BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned char NMIL:1;
			unsigned char	  :6;
			unsigned char NMIE:1;
			unsigned char	  :8;
#else  /* _BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned char	  :8;
			unsigned char NMIE:1;
			unsigned char	  :6;
			unsigned char NMIL:1;
#endif
		} BIT;
	} ICR;

	/* Interrupt priority setting register A (FFFFFEE2) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if _BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short TMU0 :4;
			unsigned short TMU1 :4;
			unsigned short TMU2 :4;
			unsigned short RTC  :4;
#else  /* _BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short RTC  :4;
			unsigned short TMU2 :4;
			unsigned short TMU1 :4;
			unsigned short TMU0 :4;
#endif
		} BIT;
	} IPRA;

	/* Interrupt priority setting register B (FFFFFEE4) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if _BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short WDT  :4;
			unsigned short REF  :4;
			unsigned short SCI  :4;
			unsigned short	    :4;
#else  /* _BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short	    :4;
			unsigned short SCI  :4;
			unsigned short REF  :4;
			unsigned short WDT  :4;
#endif
		} BIT;
	} IPRB;
};

/* address definitions for interrupt controller (INTC)*/

#if !defined(SH4)

/* SH3 definition */

#define SHREG_ICR0	(*(volatile unsigned short *)0xfffffee0)
#define SHREG_IPRA	(*(volatile unsigned short *)0xfffffee2)
#define SHREG_IPRB	(*(volatile unsigned short *)0xfffffee4)

#if defined(SH7709) || defined(SH7709A)
#define SHREG_ICR1	(*(volatile unsigned short *)0xa4000010)
#define SHREG_ICR2	(*(volatile unsigned short *)0xa4000012)
#define SHREG_PINTER	(*(volatile unsigned short *)0xa4000014)
#define SHREG_IPRC	(*(volatile unsigned short *)0xa4000016)
#define SHREG_IPRD	(*(volatile unsigned short *)0xa4000018)
#define SHREG_IPRE	(*(volatile unsigned short *)0xa400001a)
#define SHREG_IRR0	(*(volatile unsigned char *)0xa4000004)
#define SHREG_IRR1	(*(volatile unsigned char *)0xa4000006)
#define SHREG_IRR2	(*(volatile unsigned char *)0xa4000008)

#define IPRC_IRQ3_MASK	0xf000
#define IPRC_IRQ2_MASK	0x0f00
#define IPRC_IRQ1_MASK	0x00f0
#define IPRC_IRQ0_MASK	0x000f

#define IPRD_PINT07_MASK	0xf000
#define IPRD_PINT8F_MASK	0x0f00
#define IPRD_IRQ5_MASK	0x00f0
#define IPRD_IRQ4_MASK	0x000f

#define IPRE_DMAC_MASK	0xf000
#define IPRE_IRDA_MASK	0x0f00
#define IPRE_SCIF_MASK	0x00f0
#define IPRE_ADC_MASK	0x000f

#endif

#else

/* SH4 definitions */

#define SHREG_ICR	(*(volatile unsigned short *)0xffd00000)
#define SHREG_IPRA	(*(volatile unsigned short *)0xffd00004)
#define SHREG_IPRB	(*(volatile unsigned short *)0xffd00008)
#define SHREG_IPRC	(*(volatile unsigned short *)0xffd0000c)


#define IPRC_GPIO_MASK	0xf000
#define IPRC_DMAC_MASK	0x0f00
#define IPRC_SCIF_MASK	0x00f0
#define IPRC_HUDI_MASK	0x000f

#endif

#define IPRA_TMU0_MASK	0xf000
#define IPRA_TMU1_MASK	0x0f00
#define IPRA_TMU2_MASK	0x00f0
#define IPRA_RTC_MASK	0x000f

#define IPRB_WDT_MASK	0xf000
#define IPRB_REF_MASK	0x0f00
#define IPRB_SCI_MASK	0x00f0


#endif /* !_SH3_INTCREG_H__ */
