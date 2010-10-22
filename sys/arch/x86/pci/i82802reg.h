/*	$NetBSD: i82802reg.h,v 1.2.2.2 2010/10/22 07:21:41 uebayasi Exp $	*/

/*
 * Copyright (c) 2000 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Intel 82802AB/82802AC Firmware Hub
 *
 * see:	ftp://download.intel.com/design/chipsets/datashts/29065804.pdf
 *	and http://www.intel.com/design/chipsets/datashts/29065804.pdf
 */

/*
 * MMIO bases and sizes
 */
#define	I82802AC_REGBASE	0xffb00000
#define	I82802AC_MEMBASE	0xfff00000
#define	I82802AC_WINSIZE	0x00100000
#define	I82802AB_MEMBASE	0xfff80000
#define	I82802AB_WINSIZE	0x00080000

#define I82802_MFG	0x89
#define I82802AB_ID	0xad
#define I82802AC_ID	0xac

/*
 * Intel FWH registers
 */
#define	I82802_T_BLOCK_LK	0xf0002
#define	I82802_T_MINUS01_LK	0xe0002
#define	I82802_T_MINUS02_LK	0xd0002
#define	I82802_T_MINUS03_LK	0xc0002
#define	I82802_T_MINUS04_LK	0xb0002
#define	I82802_T_MINUS05_LK	0xa0002
#define	I82802_T_MINUS06_LK	0x90002
#define	I82802_T_MINUS07_LK	0x80002

#define	I82802_T_MINUS08_LK	0x70002
#define	I82802_T_MINUS09_LK	0x60002
#define	I82802_T_MINUS10_LK	0x50002
#define	I82802_T_MINUS11_LK	0x40002
#define	I82802_T_MINUS12_LK	0x30002
#define	I82802_T_MINUS13_LK	0x20002
#define	I82802_T_MINUS14_LK	0x10002
#define	I82802_T_MINUS15_LK	0x00002

#define	I82802_GPI_REG		0xc0100

#define	I82802_RNG_HSR		0xc015f /* Hardware Status */
#define	I82802_RNG_DSR		0xc0160 /* Data Status */
#define	I82802_RNG_DR		0xc0161 /* Data */

/*
 * T_BLOCK_LK and T_MINUS_* (block locking registers)
 * (table 4-5)
 */
#define	I82802_BLR_RD		0x04
#define	I82802_BLR_LD		0x02
#define	I82802_BLR_WL		0x01

/*
 * General Purpose Inputs Register
 * (table 4-7)
 */					/* PLCC32/TSOP40 pin # */
#define	I82802_GPI_REG_FGPI4	0x10	/*    30 /  7    */
#define	I82802_GPI_REG_FGPI3	0x08	/*     3 / 15    */
#define	I82802_GPI_REG_FGPI2	0x04	/*     4 / 16    */
#define	I82802_GPI_REG_FGPI1	0x02	/*     5 / 17    */
#define	I82802_GPI_REG_FGPI0	0x01	/*     6 / 18    */

/*
 * RNG registers
 */
#define	I82802_RNG_HSR_PRESENT	0x40
#define	I82802_RNG_HSR_ENABLE	0x01
#define	I82802_RNG_DSR_VALID	0x01
