/*	$NetBSD: clockreg.h,v 1.10.96.1 2011/06/06 09:05:37 jruoho Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: clockreg.h 1.14 91/01/18$
 *
 *	@(#)clockreg.h	8.2 (Berkeley) 1/12/94
 */

/*
 * MC6840 PTM registers
 */

struct clkreg {
	u_char	clk_dummy1;
	u_char	clk_cr3;
#define	clk_cr1	clk_cr3
	u_char	clk_dummy2;
	u_char	clk_cr2;
#define	clk_sr	clk_cr2
	u_char	clk_dummy3;
	u_char	clk_msb1;
	u_char	clk_dummy4;
	u_char	clk_lsb1;
	u_char	clk_dummy5;
	u_char	clk_msb2;
	u_char	clk_dummy6;
	u_char	clk_lsb2;
	u_char	clk_dummy7;
	u_char	clk_msb3;
	u_char	clk_dummy8;
	u_char	clk_lsb3;
};

/* base/offsets for register access (for locore.s) */
#define	CLKBASE		IIOPOFF(0x5F8000)
#define	CLKCR1		0x1
#define	CLKCR2		0x3
#define	CLKCR3		CLKCR1
#define	CLKSR		CLKCR2
#define	CLKMSB1		0x5
#define	CLKMSB2		0x9
#define	CLKMSB3		0xD

/* output of counter 3 clocks counter 2 */

#define	CLK_OENAB	0x80	/* output enable */
#define	CLK_IENAB	0x40	/* interrupt enable */
#define	CLK_8BIT	0x04	/* 8 bit mode */
#define	CLK_RESET	0x01	/* chip reset (CR1 only) */
#define	CLK_CR1		0x01	/* select CR1 (CR2 only) */
#define	CLK_CR3		0x00	/* select CR3 (CR2 only) */
#define CLK_INT1	0x01	/* interrupt flag for timer 1 (SR only) */
#define CLK_INT2	0x02	/* interrupt flag for timer 2 (SR only) */
#define CLK_INT3	0x04	/* interrupt flag for timer 3 (SR only) */
#define	CLK_INTR	0x80	/* composite interrupt flag (SR only) */

#define CLK_RESOLUTION	4	/* 4 usec resolution (250 kHz) */
#define	CLK_INTERVAL	2500	/* 10msec interval at 250 kHz */
#ifdef NOTDEF
#define CLK_INTERVAL	5000	/* 20msec interval at 250 kHz */
#endif
