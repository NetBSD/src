/*	$NetBSD: machtype.h,v 1.1 2001/05/11 04:38:22 thorpej Exp $	*/

/*
 * Copyright (c) 2001 Rafal K. Boni
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MACHTYPE__
#define __MACHTYPE__

/*
 * SGI machine types and known subtypes.  Info cribbed from ../README.IPn
 */
#define MACH_SGI_IP2		2	/* IRIS 3000 */
#define MACH_SGI_IP4		4  	/* Generic 4D/50-85 */
#define MACH_SGI_IP5		5	/* 4D/1x0 */
#define MACH_SGI_IP6		6	/* 4D/20 */
#define MACH_SGI_IP7		7	/* 4D/2x0, 4D/3x0, 4D/4x0 */
#define MACH_SGI_IP9		9	/* 4D/210 */
#define MACH_SGI_IP10		10	/* 4D/25 */
#define MACH_SGI_IP12		12	/* 4D/30, 4D/35, Indigo R3K */
#define MACH_SGI_IP17		17	/* Crimson */
#define MACH_SGI_IP19		19	/* Onyx, Challenge M/L(/XL?) */
#define MACH_SGI_IP20		20	/* Indigo R4K */
#define MACH_SGI_IP21		21	/* Power Challenge, Power Onyx */
#define MACH_SGI_IP22		22	/* Indigo2, Indy, Challenge S */
#define MACH_SGI_IP25		25	/* Power Challenge R10K */
#define MACH_SGI_IP26		26	/* Power Indigo2 R8K ("Teton") */
#define MACH_SGI_IP27		27	/* Origin 200, Origin 2000, Onyx2 */
#define MACH_SGI_IP28		28	/* Power Indigo2 R10K ("Pacecar") */
#define MACH_SGI_IP30		30	/* Octane */
#define MACH_SGI_IP32		32	/* O2 ("Moosehead") */
#define MACH_SGI_IP35		35	/* SN1 (?) */

/*
 * SGI machine subtypes 
 */
#define MACH_SGI_IP4_4D_50_70		40	/* 4D/50, 4D/70 */
#define MACH_SGI_IP4_4D_60_80_85	45	/* 4D/60, 4D/80, 4D/85 */

#define MACH_SGI_IP7_4D_3X0		13	/* 4D/3x0 */
#define MACH_SGI_IP7_4D_4X0		15	/* 4D/4x0 */

#define MACH_SGI_IP22_FULLHOUSE		22	/* Indigo2 */
#define MACH_SGI_IP22_GUINESS		24	/* Indy, Challenge S */

#endif /* __MACHTYPE__ */
