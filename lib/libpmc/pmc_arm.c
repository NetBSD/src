/*	$NetBSD: pmc_arm.c,v 1.3 2003/03/09 00:42:57 lukem Exp $	*/

/*
 * Copyright (c 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pmc_arm.c,v 1.3 2003/03/09 00:42:57 lukem Exp $");

#include <sys/param.h>
#include <pmc.h>

#include "pmc_private.h"

static const struct pmc_event i80200_pmc_evids[] = {
	{ "insfetch-miss",		0x00 },
	{ "insfetch-stall",		0x01 },
	{ "datadep-stall",		0x02 },  
	{ "itlb-miss",			0x03 },
	{ "dtlb-miss",			0x04 },
	{ "branch-taken",		0x05 },
	{ "branch-mispredicted",	0x06 },
	{ "instruction-executed",	0x07 },
	{ "dcachebufffull-stall-time",	0x08 },
	{ "dcachebufffull-stall-count",	0x09 },
	{ "dcache-access",		0x0A },
	{ "dcache-miss",		0x0B },
	{ "dcache-writeback",		0x0C },
	{ "swchange-pc",		0x0D },
	{ "bcu-mem-request",		0x10 },
	{ "bcu-queue-full",		0x11 },
	{ "bcu-queue-drain",		0x12 },
	{ "bcu-ecc-no-elog",		0x14 },
	{ "bcu-1bit-error",		0x15 },
	{ "narrow-ecc-caused-rmw",	0x16 },
	{ "clock",			0x100 },
	{ "clock-div-64",		0x101 },

	{ NULL,				0 },
};

static const struct pmc_class2evid arm_pmc_classes[] = {
	{ PMC_CLASS_I80200,		"i80200",
	  i80200_pmc_evids },
	{ PMC_TYPE_I80200_CCNT,		"i80200 cycle counter",
	  NULL },
	{ PMC_TYPE_I80200_PMCx,		"i80200 performance counter",
	  NULL },

	{ 0,				NULL,
	  NULL },
};

const struct pmc_class2evid *_pmc_md_classes = arm_pmc_classes;
