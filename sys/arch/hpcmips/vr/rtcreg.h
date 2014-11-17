/*	$NetBSD: rtcreg.h,v 1.10 2014/11/17 02:15:48 christos Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura. All rights reserved.
 * Copyright (c) 1999-2001 SATO Kazumi. All rights reserved.
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */

#define EPOCHOFF	0			/* epoch offset */
#ifndef EPOCHYEAR
#define EPOCHYEAR	1850	/* XXX */	/* WINCE epoch year */
#endif
#define EPOCHMONTH	1			/* WINCE epoch month of year */
#define EPOCHDATE	1			/* WINCE epoch date of month */

/*
 *	RTC (Real Time Clock Unit) Registers definitions.
 *		start 0x0B0000C0 (Vr4102-4121)
 *		start 0x0F000100 (Vr4122-4131)
 *		start 0x0B0000C0 (Vr4181)
 */
#define RTC_NO_REG_W		0xffffffff

#define ETIME_L_REG_W		0x000	/* Elapsed Time L */
#define ETIME_M_REG_W		0x002	/* Elapsed Time M */
#define ETIME_H_REG_W		0x004	/* Elapsed Time H */

#define		ETIME_L_HZ		0x8000	/* 1 HZ */


#define ECMP_L_REG_W		0x008	/* Elapsed Compare L */
#define ECMP_M_REG_W		0x00a	/* Elapsed Compare M */
#define ECMP_H_REG_W		0x00c	/* Elapsed Compare H */


#define RTCL1_L_REG_W		0x010	/* RTC Long 1 L */
#define RTCL1_H_REG_W		0x012	/* RTC Long 1 H */

#define		RTCL1_L_HZ		0x8000	/* 1 HZ */


#define RTCL1_CNT_L_REG_W	0x014	/* RTC Long 1 Count L */
#define RTCL1_CNT_H_REG_W	0x016	/* RTC Long 1 Count H */


#define RTCL2_L_REG_W		0x018	/* RTC Long 2 L */
#define RTCL2_H_REG_W		0x01a	/* RTC Long 2 H */

#define		RTCL2_L_HZ		0x8000	/* 1 HZ */


#define RTCL2_CNT_L_REG_W	0x01c	/* RTC Long 2 Count L */
#define RTCL2_CNT_H_REG_W	0x01e	/* RTC Long 2 Count H */


#define VR4102_TCLK_L_REG_W	0x100	/* TCLK L */
#define VR4102_TCLK_H_REG_W	0x102	/* TCLK H */
#define VR4122_TCLK_L_REG_W	0x020	/* TCLK L */
#define VR4122_TCLK_H_REG_W	0x022	/* TCLK H */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define TCLK_L_REG_W		VR4102_TCLK_L_REG_W	/* TCLK L */
#define TCLK_H_REG_W		VR4102_TCLK_H_REG_W	/* TCLK H */
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define TCLK_L_REG_W		VR4122_TCLK_L_REG_W	/* TCLK L */
#define TCLK_H_REG_W		VR4122_TCLK_H_REG_W	/* TCLK H */
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define TCLK_L_REG_W		RTC_NO_REG_W
#define TCLK_H_REG_W		RTC_NO_REG_W
#endif /* VRGROUP_4181 */
#endif /* defined SINGLE_VRIP_BASE */


#define VR4102_TCLK_CNT_L_REG_W	0x104	/* TCLK Count L */
#define VR4102_TCLK_CNT_H_REG_W	0x106	/* TCLK Count H */
#define VR4122_TCLK_CNT_L_REG_W	0x024	/* TCLK Count L */
#define VR4122_TCLK_CNT_H_REG_W	0x026	/* TCLK Count H */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define TCLK_CNT_L_REG_W	VR4102_TCLK_CNT_L_REG_W	/* TCLK Count L */
#define TCLK_CNT_H_REG_W	VR4102_TCLK_CNT_L_REG_W	/* TCLK Count H */
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define TCLK_CNT_L_REG_W	VR4122_TCLK_CNT_L_REG_W	/* TCLK Count L */
#define TCLK_CNT_H_REG_W	VR4122_TCLK_CNT_H_REG_W	/* TCLK Count H */
#endif /* VRGROUP_4122_4131 */
#if defined VRGROUP_4181
#define TCLK_CNT_L_REG_W	RTC_NO_REG_W
#define TCLK_CNT_H_REG_W	RTC_NO_REG_W
#endif /* VRGROUP_4181 */
#endif /* defined SINGLE_VRIP_BASE */


#define VR4102_RTCINT_REG_W		0x11e	/* RTC intr reg. */
#define VR4122_RTCINT_REG_W		0x03e	/* RTC intr reg. */
#define VR4181_RTCINT_REG_W		0x11e	/* RTC intr reg. */
#if defined SINGLE_VRIP_BASE
#if defined VRGROUP_4102_4121
#define RTCINT_REG_W		VR4102_RTCINT_REG_W	/* RTC intr reg. */
#endif /* VRGROUP_4102_4121 */
#if defined VRGROUP_4122_4131
#define RTCINT_REG_W		VR4122_RTCINT_REG_W	/* RTC intr reg. */
#endif /* VRGROUP_4122 */
#if defined VRGROUP_4181
#define RTCINT_REG_W		VR4181_RTCINT_REG_W	/* RTC intr reg. */
#endif /* VRGROUP_4181 */
#endif /* defined SINGLE_VRIP_BASE */

#define		RTCINT_TCLOCK		(1<<3)	/* TClock */
#define		RTCINT_RTCLONG2		(1<<2)	/* RTC Long 2 */
#define		RTCINT_RTCLONG1		(1<<1)	/* RTC Long 1 */
#define		RTCINT_ELAPSED		(1)	/* Elapsed time */
#define		RTCINT_ALL		(RTCINT_TCLOCK|RTCINT_RTCLONG2|RTCINT_RTCLONG1|RTCINT_ELAPSED)

/* END rtcreg.h */
