/*	$NetBSD: fpu.c,v 1.19 2003/07/15 03:36:17 lukem Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
 * Floating Point Unit (MC68881/882)
 * Probe for the FPU at autoconfig time.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.19 2003/07/15 03:36:17 lukem Exp $");

#include "opt_fpu_emulate.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/frame.h>

#include <sun3/sun3/machdep.h>

static int fpu_probe __P((void));

static char *fpu_descr[] = {
#ifdef	FPU_EMULATE
	"emulator", 		/* 0 */
#else
	"no math support",	/* 0 */
#endif
	"mc68881",		/* 1 */
	"mc68882",		/* 2 */
	"mc68040 internal",	/* 3 */
	"mc68060 internal",	/* 4 */
	"unknown type" };	/* 5 */

void
initfpu()
{
	char *descr;
	int maxtype = sizeof(fpu_descr) / sizeof(fpu_descr[0]) - 1;

	/* Set the FPU bit in the "system enable register" */
	enable_fpu(1);

	fputype = fpu_probe();
	if (fputype < 0 || fputype > maxtype)
		fputype = FPU_UNKNOWN;

	descr = fpu_descr[fputype];

	printf("fpu: %s\n", descr);

	if (fputype == FPU_NONE) {
		/* Might as well turn the enable bit back off. */
		enable_fpu(0);
	}
}

static int
fpu_probe()
{
	label_t	faultbuf;
	struct fpframe fpframe;
	u_char b;

	nofault = &faultbuf;
	if (setjmp(&faultbuf)) {
		nofault = NULL;
		return FPU_NONE;
	}

	/*
	 * Synchronize FPU or cause a fault.
	 * This should leave the 881/882 in the IDLE state,
	 * so we can determine which we have by
	 * examining the size of the FP state frame.
	 */
	__asm("fnop");

	nofault = NULL;

	/* Presumably, this will not cause a fault--the fnop should
	 * have if this will. We save the state in order to get the
	 * size of the frame.
	 */
	__asm("fsave %0@" : : "a" (&fpframe) : "memory");

	b = fpframe.fpf_fsize;

	/*
	 * Now, restore a NULL state to reset the FPU.
	 */
	fpframe.fpf_null = 0;
	fpframe.fpf_idle.fpf_ccr = 0;
	m68881_restore(&fpframe);

	/*
	 * The size of a 68881 IDLE frame is 0x18
	 *           and 68882 frame is 0x38
	 */
	if (b == 0x18)
		return FPU_68881;
	if (b == 0x38)
		return FPU_68882;

	/*
	 * If it's not one of the above, we have no clue what it is.
	 */
	return FPU_UNKNOWN;
}
