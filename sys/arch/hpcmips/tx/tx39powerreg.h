/*	$NetBSD: tx39powerreg.h,v 1.4 2001/06/14 11:09:56 uch Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * TOSHIBA TX3912/3922 Power module
 */

#define	TX39_POWERCTRL_REG	0x1c4

/*
 *	Power Control Register
 */
/* R */
#define	TX39_POWERCTRL_ONBUTN		0x80000000
#define	TX39_POWERCTRL_PWRINT		0x40000000
#define	TX39_POWERCTRL_PWROK		0x20000000
#ifdef TX391X
/* R/W */
#define TX39_POWERCTRL_VIDRF_SHIFT	27
#define TX39_POWERCTRL_VIDRF_MASK	0x3
#define TX39_POWERCTRL_VIDRF(cr)					\
	(((cr) >> TX39_POWERCTRL_VIDRF_SHIFT) &				\
	TX39_POWERCTRL_VIDRF_MASK)
#define TX39_POWERCTRL_VIDRF_SET(cr, val)				\
	((cr) | (((val) << TX39_POWERCTRL_VIDRF_SHIFT) &		\
	(TX39_POWERCTRL_VIDRF_MASK << TX39_POWERCTRL_VIDRF_SHIFT)))
#endif /* TX391X */
#ifdef TX392X
#define	TX39_POWERCTRL_PWROKNMI		0x10000000
#endif /* TX392X */

#define	TX39_POWERCTRL_SLOWBUS		0x04000000
#ifdef TX391X
#define	TX39_POWERCTRL_DIVMOD		0x02000000
#endif /* TX391X */

#define TX39_POWERCTRL_STPTIMERVAL_SHIFT	12
#define TX39_POWERCTRL_STPTIMERVAL_MASK		0xf
#define TX39_POWERCTRL_STPTIMERVAL(cr)					\
	(((cr) >> TX39_POWERCTRL_STPTIMERVAL_SHIFT) &			\
	TX39_POWERCTRL_STPTIMERVAL_MASK)
#define TX39_POWERCTRL_STPTIMERVAL_SET(cr, val)				\
	((cr) | (((val) << TX39_POWERCTRL_STPTIMERVAL_SHIFT) &		\
	(TX39_POWERCTRL_STPTIMERVAL_MASK << TX39_POWERCTRL_STPTIMERVAL_SHIFT)))
#define TX39_POWERCTRL_STPTIMERVAL_MIN		0x0
#define TX39_POWERCTRL_STPTIMERVAL_MAX		0xf

#define	TX39_POWERCTRL_ENSTPTIMER	0x00000800
#define	TX39_POWERCTRL_ENFORCESHUTDWN	0x00000400
#define	TX39_POWERCTRL_FORCESHUTDWN	0x00000200
#define	TX39_POWERCTRL_FORCESHUTDWNOCC	0x00000100
#define	TX39_POWERCTRL_SELC2MS		0x00000080
#ifdef TX392X
#define	TX39_POWERCTRL_WARMSTART	0x00000040
#endif /* TX392X */
#define	TX39_POWERCTRL_BPDBVCC3		0x00000020
#define	TX39_POWERCTRL_STOPCPU		0x00000010
#define	TX39_POWERCTRL_DBNCONBUTN	0x00000008
#define	TX39_POWERCTRL_COLDSTART	0x00000004
#define	TX39_POWERCTRL_PWRCS		0x00000002
#define	TX39_POWERCTRL_VCCON		0x00000001
