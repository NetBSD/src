/* $NetBSD: aupscreg.h,v 1.2.6.2 2006/04/22 11:37:41 simonb Exp $ */

/*-
 * Copyright (c) 2006 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MIPS_ALCHEMY_DEV_AUPSCREG_H_
#define	_MIPS_ALCHEMY_DEV_AUPSCREG_H_

/*
 * PSC registers (offset from PSCn_BASE).
 */

/* psc_sel: PSC clock and protocol select
 *   CLK [5:4]
 *     00 = pscn_intclk (for SPI, SMBus, I2S Master[PSC3 Only])
 *     01 = PSCn_EXTCLK (for SPI, SMBus, I2S Master)
 *     10 = PSCn_CLK    (for AC97, I2S Slave)
 *     11 = Reserved
 *   PS [2:0]
 *     000 = Protocol disable
 *     001 = Reserved
 *     010 = SPI mode
 *     011 = I2S mode
 *     100 = AC97 mode
 *     101 = SMBus mode
 *     11x = Reserved
 */
#define	AUPSC_SEL			0x00	/* R/W */
#  define	AUPSC_SEL_CLK(x)	((x & 0x03) << 4) /* CLK */
#  define	AUPSC_SEL_PS(x)		(x & 0x07)
#  define	AUPSC_SEL_DISABLE	0
#  define	AUPSC_SEL_SPI		2
#  define	AUPSC_SEL_I2S		3
#  define	AUPSC_SEL_AC97		4
#  define	AUPSC_SEL_SMBUS		5

/* psc_ctrl: PSC control
 *  ENA [1:0]
 *    00 = Disable/Reset
 *    01 = Reserved
 *    10 = Suspend
 *    11 = Enable
 */
#define	AUPSC_CTRL			0x04	/* R/W */
#  define	AUPSC_CTRL_ENA(x)	(x & 0x03)
#  define	AUPSC_CTRL_DISABLE	0
#  define	AUPSC_CTRL_SUSPEND	2
#  define	AUPSC_CTRL_ENABLE	3

/* 0x0008 - 0x002F: Protocol-specific registers */

/* PSC registers size */
#define	AUPSC_SIZE			0x2f


/*
 * SPI Protocol registers
 */
#define	AUPSC_SPICFG			0x08	/* R/W */
#define	AUPSC_SPIMSK			0x0c	/* R/W */
#define	AUPSC_SPIPCR			0x10	/* R/W */
#define	AUPSC_SPISTAT			0x14	/* Read only */
#define	AUPSC_SPIEVNT			0x18	/* R/W */
#define	AUPSC_SPITXRX			0x1c	/* R/W */

/*
 * I2S Protocol registers
 */
#define	AUPSC_I2SCFG			0x08	/* R/W */
#define	AUPSC_I2SMSK			0x0c	/* R/W */
#define	AUPSC_I2SPCR			0x10	/* R/W */
#define	AUPSC_I2SSTAT			0x14	/* Read only */
#define	AUPSC_I2SEVNT			0x18	/* R/W */
#define	AUPSC_I2STXRX			0x1c	/* R/W */
#define	AUPSC_I2SUDF			0x20	/* R/W */

/*
 * AC97 Protocol registers
 */
#define	AUPSC_AC97CFG			0x08	/* R/W */
#define	AUPSC_AC97MSK			0x0c	/* R/W */
#define	AUPSC_AC97PCR			0x10	/* R/W */
#define	AUPSC_AC97STAT			0x14	/* Read only */
#define	AUPSC_AC97EVNT			0x18	/* R/W */
#define	AUPSC_AC97TXRX			0x1c	/* R/W */
#define	AUPSC_AC97CDC			0x20	/* R/W */
#define	AUPSC_AC97RST			0x24	/* R/W */
#define	AUPSC_AC97GPO			0x28	/* R/W */
#define	AUPSC_AC97GPI			0x2c	/* Read only */

/*
 * SMBus Protocol registers
 */
#define	AUPSC_SMBCFG			0x08	/* R/W */
#define	AUPSC_SMBMSK			0x0c	/* R/W */
#define	AUPSC_SMBPCR			0x10	/* R/W */
#define	AUPSC_SMBSTAT			0x14	/* Read only */
#define	AUPSC_SMBEVNT			0x18	/* R/W */
#define	AUPSC_SMBTXRX			0x1c	/* R/W */
#define	AUPSC_SMBTMR			0x20	/* R/W */

#endif	/* _MIPS_ALCHEMY_DEV_AUPSCREG_H_ */
