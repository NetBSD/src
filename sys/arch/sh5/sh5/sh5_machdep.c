/*	$NetBSD: sh5_machdep.c,v 1.1 2002/07/05 13:32:06 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/user.h>

#include <machine/cpu.h>
#include <machine/param.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/cacheops.h>


char	machine_arch[] = MACHINE_ARCH;

/*
 * Initialised by port-specific code to the number of CTC control
 * register ticks per micro-second.
 *
 * This is used, primarily, to implement delay().
 */
u_int _sh5_ctc_ticks_per_us;

void (*__cpu_cache_purge)(vaddr_t, vsize_t);
void (*__cpu_cache_invalidate)(vaddr_t, vsize_t);

void
delay(u_int microseconds)
{
	register_t ctcreg;
	u_int ctc;

	if (microseconds == 0)
		return;

	__asm __volatile("getcon ctc, %0" : "=r"(ctcreg));

	/*
	 * XXX: Assumes CTC is 32-bits wide.
	 */
	ctc = (u_int)ctcreg - (microseconds * _sh5_ctc_ticks_per_us);

	if (ctc > (u_int)ctcreg) {
		/* Counter will wrap-around while we're waiting... */
		do {
			__asm __volatile("getcon ctc, %0" : "=r"(ctcreg));
		} while (ctc >= (u_int)ctcreg);
	} else {
		do {
			__asm __volatile("getcon ctc, %0" : "=r"(ctcreg));
		} while (ctc <= (u_int)ctcreg);
	}
}

/*
 * These variables are needed by /sbin/savecore
 */
u_int32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first NBPG of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf()
{

	dumpsize = 0;
}

