/*	$NetBSD: vrkiureg.h,v 1.2 2000/09/21 14:17:36 takemura Exp $	*/

/*-
 * Copyright (c) 1999 SASAKI Takesi
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
 * All rights reserved.
 *
 * This code is a part of the PocketBSD.
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

#define	KIUDAT0		0x00
#define	KIUDAT1		0x02
#define	KIUDAT2		0x04
#define	KIUDAT3		0x06
#define	KIUDAT4		0x08
#define	KIUDAT5		0x0a
#define	KIUSCANREP	0x10
#define	KIUSCANS	0x12
#	define	KIUSCANS_SSTAT_MASK	0x0003
#	define	KIUSCANS_SSTAT_SCANNING	0x0003
#	define	KIUSCANS_SSTAT_INTERVAL	0x0002
#	define	KIUSCANS_SSTAT_WAIT	0x0001
#	define	KIUSCANS_SSTAT_STOP	0x0000
#define	KIUWKS		0x14
#define	KIUWKI		0x16
#define	KIUINT		0x18
#define	KIURST		0x1a
#define	KIUSCANLINE	0x1e

#define KIU_NSCANLINE	12
#define KIUDATP		0x00

