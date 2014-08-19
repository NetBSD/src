/*	$NetBSD: fpu_machdep.c,v 1.2.2.1 2014/08/20 00:03:26 tls Exp $	*/

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
 * Floating Point Unit (MC68881/882) initialization.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu_machdep.c,v 1.2.2.1 2014/08/20 00:03:26 tls Exp $");

#include "opt_fpu_emulate.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/frame.h>

#include <sun3/sun3/machdep.h>

static const char *fpu_descr[] = {
#ifdef	FPU_EMULATE
	"emulator",		/* 0 */
#else
	"no math support",	/* 0 */
#endif
	"mc68881",		/* 1 */
	"mc68882",		/* 2 */
	"mc68040 internal",	/* 3 */
	"mc68060 internal",	/* 4 */
	"unknown type" };	/* 5 */

void
initfpu(void)
{
	const char *descr;
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
	} else
		m68k_make_fpu_idle_frame();
}
