/*	$NetBSD: sa11x0_gpioreg.h,v 1.2 2001/02/23 04:31:19 ichiro Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA.
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
 * SA-11x0 GPIO Register 
 */

/* GPIO pin-level register */
#define SAGPIO_PLR	0

/* GPIO pin direction register */
#define SAGPIO_PDR	1

/* GPIO pin output set register */
#define SAGPIO_PSR	2

/* GPIO pin output clear register */
#define SAGPIO_PCR	3

/* GPIO rising-edge detect register */
#define SAGPIO_RER	4

/* GPIO falling-edge detect register */
#define SAGPIO_FER	5

/* GPIO edge-detect status register */
#define SAGPIO_EDR	6

/* GPIO alternate function register */
#define SAGPIO_AFR	7

/*
 * iPAQ H3600 specific parameter
 */
/*
port	I/O(Active)	desc
0	I(L)	button detect: power-on
1	I(L)	cpu-interrupt
2...9	O	LCD DATA(8-15)
10	I(L)	PCMCIA Socket1 inserted detection
11	I(L)	PCMCIA slot1 IRQ
12	O	clock select 0 for audio codec
13	O	clock select 1 for audio codec
14	I/O	UDA1341 L3DATA
15	O	UDA1341 L3MODE
16	O	UDA1341 L3SCLK
17	I(L)	PCMCIA Socket0 inserted detection
18	I(L)	button detect: center button
19	I	Stereo audio codev external clock
20	I(H)	Battery fault
21	I(L)	PCMCIA slot0 IRQ	
22	I(L)	expansion pack lock/unlock signal
23	I(H)	RS-232 DCD
24	I(H)	expansion pach shared IRQ
25	I(H)	RS-232 CTS
26	O(H)	RS-232 RTS
27	O(L)	Indicates presence of expansion pack inserted
 */


/*
 * JORNADA720 specific parameter
 */

