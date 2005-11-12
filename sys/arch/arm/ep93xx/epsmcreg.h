/*	$NetBSD: epsmcreg.h,v 1.1 2005/11/12 05:33:23 hamajima Exp $	*/

/*
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*	Cirrus Logic EP9315
	Static Memory Controller register
	http://www.cirrus.com/jp/pubs/manual/EP9315_Users_Guide.pdf	*/

#ifndef	_EPSMCREG_H_
#define	_EPSMCREG_H_

#define	EP93XX_SMC_BCR0		0x00	/* Bank0 Configuration Register (R/W) */
#define	EP93XX_SMC_BCR1		0x04	/* Bank1 Configuration Register (R/W) */
#define	EP93XX_SMC_BCR2		0x08	/* Bank2 Configuration Register (R/W) */
#define	EP93XX_SMC_BCR3		0x0c	/* Bank3 Configuration Register (R/W) */
#define	EP93XX_SMC_BCR6		0x18	/* Bank6 Configuration Register (R/W) */
#define	EP93XX_SMC_BCR7		0x1c	/* Bank7 Configuration Register (R/W) */
#define	 EP93XX_SMC_IDCY_MASK		0x0000000f	/* Idle Cycle */
#define	 EP93XX_SMC_IDCY_SHIFT		0
#define	 EP93XX_SMC_WST1_MASK		0x000003e0	/* Wait State 1 */
#define	 EP93XX_SMC_WST1_SHIFT		5
#define	 EP93XX_SMC_RBLE		(1<<10)		/* Read Byte Lane Enable */
#define	 EP93XX_SMC_WST2_MASK		0x0000f800	/* Wait State 2 */
#define	 EP93XX_SMC_WST2_SHIFT		11
#define	 EP93XX_SMC_WPERR		(1<<25)		/* Write Protect Error status */
#define	 EP93XX_SMC_WP			(1<<26)		/* Write Protect */
#define	 EP93XX_SMC_PME			(1<<27)		/* Page Mode Enable */
#define	 EP93XX_SMC_MW_MASK		0x30000000	/* Memory Width */
#define	 EP93XX_SMC_MW_SHIFT		28
#define	 EP93XX_SMC_EBIBRKDIS		(1<<30)		/* EBI Break Disable */

#define	EP93XX_PCMCIA0_Attribute 0x20	/* PCMCIA Attribute (R/W) */
#define	EP93XX_PCMCIA0_Common	0x24	/* PCMCIA Common (R/W) */
#define	EP93XX_PCMCIA0_IO	0x28	/* PCMCIA IO (R/W) */
#define	EP93XX_PCMCIA1_Attribute 0x30	/* PCMCIA Attribute (R/W) */
#define	EP93XX_PCMCIA1_Common	0x34	/* PCMCIA Common (R/W) */
#define	EP93XX_PCMCIA1_IO	0x38	/* PCMCIA IO (R/W) */
#define	 EP93XX_PCMCIA_WIDTH_16		(1<<31)		/* width */
#define	 EP93XX_PCMCIA_ACCESS_MASK	0x00ff0000	/* access time */
#define	 EP93XX_PCMCIA_ACCESS_SHIFT	16
#define	 EP93XX_PCMCIA_HOLD_MASK	0x00000f00	/* Hole time */
#define	 EP93XX_PCMCIA_HOLD_SHIFT	8
#define	 EP93XX_PCMCIA_PRECHARGE_MASK	0x000000ff	/* pre-charge delay */
#define	 EP93XX_PCMCIA_PRECHARGE_SHIFT	0

#define	EP93XX_PCMCIA_Ctrl	0x40	/* PCMCIA Control */
#define	 EP93XX_PCMCIA_WEN		(1<<4)		/* External wait state */
#define	 EP93XX_PCMCIA_RST		(1<<2)		/* Reset PC card */
#define	 EP93XX_PCMCIA_EN_DISABLE	0x0		/* PC card disabled */
#define	 EP93XX_PCMCIA_EN_CF		0x1		/* CF mode */
#define	 EP93XX_PCMCIA_EN_PCMCIA	0x2		/* PCMCIA mode */
#define	 EP93XX_PCMCIA_EN_MASK		0x3

#endif	/* _EPSMCREG_H_ */
