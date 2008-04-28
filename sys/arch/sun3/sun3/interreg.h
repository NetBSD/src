/*	$NetBSD: interreg.h,v 1.13 2008/04/28 20:23:38 martin Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass.
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

#define IREG_CLOCK_ENAB_7  0x80
#define IREG_RESERVED      0x40
#define IREG_CLOCK_ENAB_5  0x20
#define IREG_VIDEO_ENAB    0x10
#define IREG_SOFT_ENAB_3   0x08
#define IREG_SOFT_ENAB_2   0x04
#define IREG_SOFT_ENAB_1   0x02
#define IREG_ALL_ENAB      0x01

#ifdef	_KERNEL

#define IREG_BITS "\20\8CLK7\7RSV6\6CLK5\5VIDEO\4SOFT3\3SOFT2\2SOFT1\1ALL\n"

#ifdef	_SUN3X_
#define	IREG_ADDR	0x61001400
#else
#define	IREG_ADDR	  0x0A0000
#endif

extern volatile u_char *interrupt_reg;

void set_clk_mode(u_char on, u_char off, int enable);

#endif	/* _KERNEL */
