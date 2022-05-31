/*	$NetBSD: plumicureg.h,v 1.5 2022/05/31 11:22:33 andvar Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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
 * PLUM2 INTERRUPT CONTROLLER UNIT
 */
#define	PLUM_INT_REGBASE		0x8000
#define	PLUM_INT_REGSIZE		0x1000

/* 
 *  Interrupt status register 
 */
#define	PLUM_INT_INTSTA_REG		0x000
#define PLUM_INT_INTSTA_EXTINT		0x00000080
#define PLUM_INT_INTSTA_SMINT		0x00000040
#define PLUM_INT_INTSTA_USBWAKE		0x00000020
#define PLUM_INT_INTSTA_USBINT		0x00000010
#define PLUM_INT_INTSTA_DISPINT		0x00000008
#define PLUM_INT_INTSTA_C2SCINT		0x00000004
#define PLUM_INT_INTSTA_C1SCINT		0x00000002
#define PLUM_INT_INTSTA_PCCINT		0x00000001

/* 
 *  Interrupt enable register 
 */
#define	PLUM_INT_INTIEN_REG		0x010
#define PLUM_INT_INTIEN			0x00000001

/*
 *  External interrupts 
 */
/* outside input interrupt status register */
#define	PLUM_INT_EXTINTS_REG		0x100

#define	PLUM_INT_EXTINTS_IO3INT1	0x00000020
#define	PLUM_INT_EXTINTS_IO3INT0	0x00000010
#define	PLUM_INT_EXTINTS_IO5INT3	0x00000008
#define	PLUM_INT_EXTINTS_IO5INT2	0x00000004
#define	PLUM_INT_EXTINTS_IO5INT1	0x00000002
#define	PLUM_INT_EXTINTS_IO5INT0	0x00000001

/* outside input interrupt status register (after the mask) */
#define	PLUM_INT_EXTINTM_REG		0x104

/* interrupt enable register from the outside input */
#define	PLUM_INT_EXTIEN_REG		0x110

#define	PLUM_INT_EXTIEN_IENIO3INT1	0x00000020
#define	PLUM_INT_EXTIEN_IENIO3INT0	0x00000010
#define	PLUM_INT_EXTIEN_IENIO5INT3	0x00000008
#define	PLUM_INT_EXTIEN_IENIO5INT2	0x00000004
#define	PLUM_INT_EXTIEN_IENIO5INT1	0x00000002
#define	PLUM_INT_EXTIEN_IENIO5INT0	0x00000001

#define	PLUM_INT_EXTIEN_SENIO3INT1	0x00002000
#define	PLUM_INT_EXTIEN_SENIO3INT0	0x00001000
#define	PLUM_INT_EXTIEN_SENIO5INT3	0x00000800
#define	PLUM_INT_EXTIEN_SENIO5INT2	0x00000400
#define	PLUM_INT_EXTIEN_SENIO5INT1	0x00000200
#define	PLUM_INT_EXTIEN_SENIO5INT0	0x00000100

/*
 *  PC-card interrupts
 */
/* PC-card interrupt status register */
#define	PLUM_INT_PCCINTS_REG		0x200

#define PLUM_INT_PCCINTS_C2RI		0x00000008
#define PLUM_INT_PCCINTS_C2IO		0x00000004
#define PLUM_INT_PCCINTS_C1RI		0x00000002
#define PLUM_INT_PCCINTS_C1IO		0x00000001

/* PC-card interrupt status register (masked) */
#define	PLUM_INT_PCCINTM_REG		0x204
/* PC-card interrupt enable register */
#define	PLUM_INT_PCCIEN_REG		0x210

#define PLUM_INT_PCCIEN_SENC2RI		0x00000800
#define PLUM_INT_PCCIEN_SENC2IO		0x00000400
#define PLUM_INT_PCCIEN_SENC1RI		0x00000200
#define PLUM_INT_PCCIEN_SENC1IO		0x00000100
#define PLUM_INT_PCCIEN_IENC2RI		0x00000008
#define PLUM_INT_PCCIEN_IENC2IO		0x00000004
#define PLUM_INT_PCCIEN_IENC1RI		0x00000002
#define PLUM_INT_PCCIEN_IENC1IO		0x00000001

/* PC-card interrupt detection register */
#define	PLUM_INT_PCCLKSL_REG		0x220
#define	PLUM_INT_PCCLKSL_RTC		0x00000001 /*(for suspend mode)*/

/*
 *  USB interrupts
 */
/* USB interrupt enable register */
#define	PLUM_INT_USBINTEN_REG		0x310

/* master-enables the USB interrupts */
#define	PLUM_INT_USBINTEN_IEN		0x00000002
/* enables the clock restart request interrupts */
#define	PLUM_INT_USBINTEN_WIEN		0x00000001

/*
 *  SmartMedia interrupts
 */
/* SmartMedia interrupt enable register */
#define	PLUM_INT_SMIEN_REG		0x410

#define	PLUM_INT_SMIEN			0x00000001
