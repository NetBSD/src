/*	$NetBSD: clock.c,v 1.1.8.2 2008/01/21 09:39:01 yamt Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>
#include <dev/isa/isareg.h>
#include <dev/ic/i8253reg.h>
#include <powerpc/spr.h>

#include "boot.h"

u_long ns_per_tick = NS_PER_TICK;

static inline u_quad_t mftb(void);
static inline void mfrtc(u_long *, u_long *);

static inline u_quad_t
mftb(void)
{
	u_long scratch;
	u_quad_t tb;

	__asm volatile ("1: mftbu %0; mftb %0+1; mftbu %1; cmpw %0,%1; bne 1b"
	    : "=r"(tb), "=r"(scratch));
	return (tb);
}

static inline void
mfrtc(u_long *up, u_long *lp)
{
	u_long scratch;

	__asm volatile ("1: mfspr %0,%3; mfspr %1,%4; mfspr %2,%3;"
	    "cmpw %0,%2; bne 1b"
	    : "=r"(*up), "=r"(*lp), "=r"(scratch)
	    : "n"(SPR_RTCU_R), "n"(SPR_RTCL_R));
}

/*
 * Wait for about n microseconds (at least!).
 */
void
delay(u_int n)
{
	u_quad_t tb;
	u_long tbh, tbl, scratch;
	unsigned int cpuvers;

	__asm volatile ("mfpvr %0" : "=r"(cpuvers));
	cpuvers >>= 16; 
	/*cpuvers = MPC601; for now */

	if (cpuvers == MPC601) {
		mfrtc(&tbh, &tbl);
		while (n >= 1000000) {
			tbh++;
			n -= 1000000;
		}
		tbl += n * 1000;
		if (tbl >= 1000000000) {
			tbh++;
			tbl -= 1000000000;
		}
		__asm volatile ("1: mfspr %0,%3; cmplw %0,%1; blt 1b; bgt 2f;"
		    "mfspr %0,%4; cmplw %0,%2; blt 1b; 2:"
		    : "=&r"(scratch)
		    : "r"(tbh), "r"(tbl), "n"(SPR_RTCU_R), "n"(SPR_RTCL_R));
	} else {
		tb = mftb();
		tb += (n * 1000 + ns_per_tick - 1) / ns_per_tick;
		tbh = tb >> 32;
		tbl = tb;
		__asm volatile ("1: mftbu %0; cmpw %0,%1; blt 1b; bgt 2f;"
		                  "mftb %0; cmpw %0,%2; blt 1b; 2:"
		                  : "=&r"(scratch)
		                  : "r"(tbh), "r"(tbl));
	}
}
