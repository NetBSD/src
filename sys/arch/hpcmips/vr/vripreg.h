/*	$NetBSD: vripreg.h,v 1.1.1.1 1999/09/16 12:23:33 takemura Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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

/* XXX, We aren't interested in I/O register size. */

#define VRIP_BCU_ADDR		0x0b000000

/* XXX,
#define VRIP_DMAAU_ADDR		0x0b000020
*/
#define VRIP_DCU_ADDR		0x0b000040

#define VRIP_CMU_ADDR		0x0b000060

#define VRIP_ICU_ADDR		0x0b000080

#define VRIP_PMU_ADDR		0x0b0000a0

#define VRIP_RTC_ADDR		0x0b0000c0

#define VRIP_DSU_ADDR		0x0b0000e0

#define VRIP_GIU_ADDR		0x0b000100

#define VRIP_PIU_ADDR		0x0b000120

#define VRIP_AIU_ADDR		0x0b000000	/* XXX */

#define VRIP_KIU_ADDR		0x0b000180

#define VRIP_DSIU_ADDR		0x0b0001a0

#define VRIP_LED_ADDR		0x0b000240

#define VRIP_SIU_ADDR		0x0c000000

#define VRIP_HSP_ADDR		0x0c000020

#define VRIP_FIR_ADDR		0x0b000000	/* XXX */


/* reserved 			22-31 */
#define VRIP_INTR_DSIU		21
#define VRIP_INTR_FIR		20
#define VRIP_INTR_TCLK		19
#define VRIP_INTR_HSP		18
#define VRIP_INTR_LED		17
#define VRIP_INTR_RTCL2		16
/* reserved 			15,14 */
#define VRIP_INTR_DOZEPIU	13
/* reserved 			12 */
#define VRIP_INTR_SOFT		11
#define VRIP_INTR_WRBERR	10
#define VRIP_INTR_SIU		9
#define VRIP_INTR_GIU		8
#define VRIP_INTR_KIU		7
#define VRIP_INTR_AIU		6
#define VRIP_INTR_PIU		5
/* reserved 			4	VRC4171 use this ??? */
#define VRIP_INTR_ETIMER	3
#define VRIP_INTR_RTCL1		2
#define VRIP_INTR_POWER		1
#define VRIP_INTR_BAT		0
