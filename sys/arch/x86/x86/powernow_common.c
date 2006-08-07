/*	$NetBSD: powernow_common.c,v 1.2 2006/08/07 20:58:23 xtraeme Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juan Romero Pardines.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Common functions for PowerNow drivers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: powernow_common.c,v 1.2 2006/08/07 20:58:23 xtraeme Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <x86/include/powernow.h>

/*
 * CPUID Extended function 8000_0007h.
 * 
 * Reference documentation:
 *
 * - AMD Processor Recognition Application Note. December 2005.
 *   Section 2.4. Advanced Power Management Features.
 *
 */

struct powernow_extflags {
	int mask;
	const char *name;
} pnow_extflag[] = {
	{ 0x01, "TS" },		/* Thermal Sensor */
	{ 0x02, "FID" },	/* Frequency ID Control */
	{ 0x04, "VID" },	/* Voltage ID Control */
	{ 0x08, "TTP" },	/* Thermal Trip */
	{ 0x10, "TM" },		/* Thermal Control */
	{ 0x20, "STC" },	/* Software Thermal Control */
	{ 0, NULL }
};

uint32_t
powernow_probe(struct cpu_info *ci, uint32_t val)
{
	uint32_t descs[4];

	CPUID(0x80000000, descs[0], descs[1], descs[2], descs[3]);

	if (descs[0] >= 0x80000007) {
		CPUID(0x80000007, descs[0], descs[1], descs[2], descs[3]);
		if (descs[3] & 0x06) {
			if ((ci->ci_signature & 0xF00) == val)
				return descs[3];
		}
	}

	return 0;
}

int
powernow_extflags(struct cpu_info *ci, uint32_t feature_flag)
{
	int family, j, rval;
	char *cpuname;

	rval = 0;

	family = (ci->ci_signature >> 8) & 15;
	cpuname = ci->ci_dev->dv_xname;

	/* Print out available cpuid extended features. */
	aprint_normal("%s: AMD Power Management features:", cpuname);
	for (j = 0; pnow_extflag[j].name != NULL; j++) {
		if (feature_flag & pnow_extflag[j].mask)
			aprint_normal(" %s", pnow_extflag[j].name);
	}
	aprint_normal("\n");

	/*
	 * Check for FID and VID cpuid extended feature flag, if they
	 * are available, it's ok to continue enabling powernow.
	 */
	if ((feature_flag & pnow_extflag[1].mask) &&
	    (feature_flag & pnow_extflag[2].mask)) {
		switch (family) {
		case 6:
			rval = 6;
			break;
		case 15:
			rval = 15;
			break;
		default:
			rval = 0;
			break;
		}
	}

	return rval;
}
