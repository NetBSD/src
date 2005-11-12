/*	$NetBSD: epgpioreg.h,v 1.1 2005/11/12 05:33:23 hamajima Exp $	*/

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
	GPIO Interface register
	http://www.cirrus.com/jp/pubs/manual/EP9315_Users_Guide.pdf	*/

#ifndef	_EPGPIOREG_H_
#define	_EPGPIOREG_H_

/* Port A
7:0	EGPIO[7:0]
6	I2S port 2 SDO2
5	I2S port 1 SDI1
4	I2S port 1 SDO1
3	HDLC clock or TENn
2	DMARQ
1	RTC 32.768kHz reference clock
0	Modem Ring Indicator */
#define	EP93XX_GPIO_PADR	0x00	/* Data (R/W) */
#define	EP93XX_GPIO_PADDR	0x10	/* Data Direction (R/W) */
#define	EP93XX_GPIO_AIntEn	0x9c	/* Interrupt Enable (R/W) */
#define	EP93XX_GPIO_AIntType1	0x90	/* Interrupt edge or level (R/W) */
#define	EP93XX_GPIO_AIntType2	0x94	/* rising/falling edge, high/low level (R/W) */
#define	EP93XX_GPIO_AEOI	0x98	/* clear interrupt (W) */
#define	EP93XX_GPIO_ADB		0xa8	/* Interrupt debounce enable (R/W) */
#define	EP93XX_GPIO_RawIntStsA	0xa4	/* Raw Interrupt Status (R) */
#define	EP93XX_GPIO_IntStsA	0xa0	/* Masked interrupt Status (R) */

/* Port B
7:0	EGPIO[15:8]
7	DASPn
5	I2S port 2 SDI2 */
#define	EP93XX_GPIO_PBDR	0x04	/* Data (R/W) */
#define	EP93XX_GPIO_PBDDR	0x14	/* Data Direction (R/W) */
#define	EP93XX_GPIO_BIntEn	0xb8	/* Interrupt Enable (R/W) */
#define	EP93XX_GPIO_BIntType1	0xac	/* Interrupt edge or level (R/W) */
#define	EP93XX_GPIO_BIntType2	0xb0	/* rising/falling edge, high/low level (R/W) */
#define	EP93XX_GPIO_BEOI	0xb4	/* clear interrupt (W) */
#define	EP93XX_GPIO_BDB		0xc4	/* Interrupt debounce enable (R/W) */
#define	EP93XX_GPIO_RawIntStsB	0xc0	/* Raw Interrupt Status (R) */
#define	EP93XX_GPIO_IntStsB	0xbc	/* Masked interrupt Status (R) */

/* Port C
7:0	ROW[7:0]	Key Matrix row pin */
#define	EP93XX_GPIO_PCDR	0x08	/* Data (R/W) */
#define	EP93XX_GPIO_PCDDR	0x18	/* Data Direction (R/W) */

/* Port D
7:0	COL[7:0]	Key Matrix column pin */
#define	EP93XX_GPIO_PDDR	0x0c	/* Data (R/W) */
#define	EP93XX_GPIO_PDDDR	0x1c	/* Data Direction (R/W) */

/* Port E
7:5	IDEDA[2:0]	IDE control pin
4	IDECS1n		IDE control pin
3	IDECS0n		IDE control pin
2	DIORn		IDE control pin
1	RDLED		Red LED pin
0	GRLED		Green LED pin */
#define	EP93XX_GPIO_PEDR	0x20	/* Data (R/W) */
#define	EP93XX_GPIO_PEDDR	0x24	/* Data Direction (R/W) */

/* Port F
7	VS2		PCMCIA pin
6	READY		PCMCIA pin
5	VS1		PCMCIA pin
4	MCBVD2		PCMCIA pin
3	MCBVD1		PCMCIA pin
2	MCCD2		PCMCIA pin
1	MCCD1		PCMCIA pin
0	WP		PCMCIA pin */
#define	EP93XX_GPIO_PFDR	0x30	/* Data (R/W) */
#define	EP93XX_GPIO_PFDDR	0x34	/* Data Direction (R/W) */
#define	EP93XX_GPIO_FIntEn	0x58	/* Interrupt Enable (R/W) */
#define	EP93XX_GPIO_FIntType1	0x4c	/* Interrupt edge or level (R/W) */
#define	EP93XX_GPIO_FIntType2	0x50	/* rising/falling edge, high/low level (R/W) */
#define	EP93XX_GPIO_FEOI	0x54	/* clear interrupt (W) */
#define	EP93XX_GPIO_FDB		0x64	/* Interrupt debounce enable (R/W) */
#define	EP93XX_GPIO_RawIntStsF	0x60	/* Raw Interrupt Status (R) */
#define	EP93XX_GPIO_IntStsF	0x5c	/* Masked interrupt Status (R) */

/* Port G
7:4	DD[15:12]	IDE data pin
3:2	SLA[1:0]	PCMCIA voltage control pin
1	EEDAT		EEPROM data pin
0	EECLK		EEPROM clock pin */
#define	EP93XX_GPIO_PGDR	0x38	/* Data (R/W) */
#define	EP93XX_GPIO_PGDDR	0x3c	/* Data Direction (R/W) */

/* Port H
7:0	DD[7:0]		IDE data pin */
#define	EP93XX_GPIO_PHDR	0x40	/* Data (R/W) */
#define	EP93XX_GPIO_PHDDR	0x44	/* Data Direction (R/W) */

/* EEPROM interface pin drive type (R/W) */
#define	EP93XX_GPIO_EEDrive	0xc8
#define	 EP93XX_GPIO_DATOD	(1<<1)	/* EEDAT pin */
#define	 EP93XX_GPIO_CLKOD	(1<<0)	/* EECLK pin */

/* Interrupt */
#define	EP93XX_GPIO0_INTR	19	/* Port F bit 0 */
#define	EP93XX_GPIO1_INTR	20	/* Port F bit 1 */
#define	EP93XX_GPIO2_INTR	21	/* Port F bit 2 */
#define	EP93XX_GPIO3_INTR	22	/* Port F bit 3 */
#define	EP93XX_GPIO4_INTR	47	/* Port F bit 4 */
#define	EP93XX_GPIO5_INTR	48	/* Port F bit 5 */
#define	EP93XX_GPIO6_INTR	49	/* Port F bit 6 */
#define	EP93XX_GPIO7_INTR	50	/* Port F bit 7 */
#define	EP93XX_GPIO_INTR	59	/* Port A or B */

#endif	/* _EPGPIOREG_H_ */
