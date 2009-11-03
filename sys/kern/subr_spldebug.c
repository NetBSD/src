/*	$NetBSD: subr_spldebug.c,v 1.1 2009/11/03 05:23:28 dyoung Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Young.
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
 * Interrupt priority level debugging code.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_spldebug.c,v 1.1 2009/11/03 05:23:28 dyoung Exp $");

#include <sys/param.h>
#include <sys/spldebug.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/cpu.h>
#include <machine/return.h>

__strong_alias(spldebug_start, _spldebug_start);
__strong_alias(spldebug_stop, _spldebug_stop);

#define SPLRAISE_STACKLEN 32

void *splraise_retaddrs[MAXCPUS][SPLRAISE_STACKLEN][4];
int splraise_depth[MAXCPUS] = {0};
int spllowered_to[MAXCPUS] = {0};
void *spllowered_from[MAXCPUS][2] = {{0}};
bool spldebug = false;

void	_spldebug_start(void);
void	_spldebug_stop(void);

void
_spldebug_start(void)
{
	spldebug = true;
}

void
_spldebug_stop(void)
{
	spldebug = false;
}

void
spldebug_lower(int ipl)
{
	struct cpu_info *ci;

	if (!spldebug)
		return;

	if (ipl >= IPL_HIGH)
		return;

	if (cold)
		ci = &cpu_info_primary;
	else
		ci = curcpu();

	if (cpu_index(ci) > MAXCPUS)
		return;

	splraise_depth[cpu_index(ci)] = 0;
	spllowered_to[cpu_index(ci)] = ipl;
#if 0
	spllowered_from[cpu_index(ci)][0] = return_address(0);
	spllowered_from[cpu_index(ci)][1] = return_address(1);
#endif
}

void
spldebug_raise(int ipl)
{
	int i;
	void **retaddrs;
	struct cpu_info *ci;

	if (!spldebug)
		return;

	if (ipl < IPL_HIGH)
		return;

	if (cold)
		ci = &cpu_info_primary;
	else
		ci = curcpu();

	if (cpu_index(ci) >= MAXCPUS)
		return;

	if (splraise_depth[cpu_index(ci)] >= SPLRAISE_STACKLEN)
		return;

	retaddrs = &splraise_retaddrs[cpu_index(ci)]
	    [splraise_depth[cpu_index(ci)]++][0];

	retaddrs[0] = retaddrs[1] = retaddrs[2] = retaddrs[3] = NULL;

	for (i = 0; i < 4; i++) {
		retaddrs[i] = return_address(i);

		if (retaddrs[i] == NULL)
			break;
	}
}
