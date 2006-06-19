/*	$NetBSD: pb1000reg.h,v 1.1.52.1 2006/06/19 03:44:02 chap Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Memory map and register definitions for the Alchemy Semiconductor Pb1000.
 */

/* RAM addresses */
#define	PB1000_SDRAM_BASE	0x00000000
#define	PB1000_SDRAM_SIZE	0x04000000

/* Static RAM */
#define	PB1000_SRAM_BASE	0x1c000000
#define	PB1000_SRAM_SIZE	0x00040000

/* PCMCIA addresses */
#define	PB1000_PCMCIA_CONTROL	0x1e000000
#define	PB1000_PCMCIA_DATA	0x1e000004
#define	PB1000_PCMCIA_STATUS	0x1e000008

/* CPLD addresses */
#define	PB1000_CPLD_AUX0	0x1e00000c
#define	PB1000_CPLD_AUX1	0x1e000010
#define	PB1000_CPLD_AUX2	0x1e000014

/* Flash addresses */
#define	PB1000_FLASH_BASE	0x1f800000
#define	PB1000_FLASH_SIZE	0x00800000
