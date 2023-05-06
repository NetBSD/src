/*	$Id: at91pioreg.h,v 1.3 2023/05/06 22:17:28 andvar Exp $	*/
/*	$NetBSD: at91pioreg.h,v 1.3 2023/05/06 22:17:28 andvar Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
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

#ifndef	_AT91GPIOREG_H_
#define	_AT91GPIOREG_H_

#define	PIO_PER		0x00U	/* 00: PIO Enable Register		*/
#define	PIO_PDR		0x04U	/* 04: PIO Disable Register		*/
#define	PIO_PSR		0x08U	/* 08: PIO Status Register 		*/
#define	PIO_OER		0x10U	/* 10: PIO Output Enable Register	*/
#define	PIO_ODR		0x14U	/* 14: PIO Output Disable Register	*/
#define	PIO_OSR		0x18U	/* 18: PIO Output Status Register	*/
#define	PIO_IFER	0x20U	/* 20: PIO Glitch Inp. Filter Ena. Reg	*/
#define	PIO_IFDR	0x24U	/* 24: PIO Glitch Inp. Filter Dis. Reg	*/
#define	PIO_IFSR	0x28U	/* 28: PIO Glitch Inp. Filter Sta. Reg	*/
#define	PIO_SODR	0x30U	/* 30: PIO Set Output Data Reg		*/
#define	PIO_CODR	0x34U	/* 34: PIO Clr Output Data Reg		*/
#define	PIO_ODSR	0x38U	/* 38: PIO Output Data Status Reg	*/
#define	PIO_PDSR	0x3CU	/* 3C: PIO Pin Data Status Reg		*/
#define	PIO_IER		0x40U	/* 40: PIO Interrupt Enable Reg		*/
#define	PIO_IDR		0x44U	/* 44: PIO Interrupt Disable Reg	*/
#define	PIO_IMR		0x48U	/* 48: PIO Interrupt Mask Reg		*/
#define	PIO_ISR		0x4CU	/* 4C: PIO Interrupt Status Reg		*/
#define	PIO_MDER	0x50U	/* 50: PIO Multi-driver Enable Reg	*/
#define	PIO_MDDR	0x54U	/* 54: PIO Multi-driver Disable Reg	*/
#define	PIO_MDSR	0x58U	/* 58: PIO Multi-driver Status Reg	*/
#define	PIO_PUDR	0x60U	/* 60: PIO Pull-up Disable Reg		*/
#define	PIO_PUER	0x64U	/* 64: PIO Pull-up Enable Reg		*/
#define	PIO_PUSR	0x68U	/* 68: PIO Pull-up Status Reg		*/
#define	PIO_ASR		0x70U	/* 70: PIO Peripheral A Select Reg	*/
#define	PIO_BSR		0x74U	/* 74: PIO Peripheral B Select Reg	*/
#define	PIO_ABSR	0x78U	/* 78: PIO AB Status Reg		*/
#define	PIO_OWER	0xA0U	/* A0: PIO Output Write Enable		*/
#define	PIO_OWDR	0xA4U	/* A4: PIO Output Write Disable		*/
#define	PIO_OWSR	0xA8U	/* A8: PIO Output Write Status Reg	*/
#define	PIO_VERSION	0xFCU	/* FC: version register	(AT91SAM92xx)	*/

#endif	/* _AT91GPIOREG_H_ */
