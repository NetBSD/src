/*	$NetBSD: platform.c,v 1.4 2003/07/15 02:46:33 lukem Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
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

/*
 * Platform configuration support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: platform.c,v 1.4 2003/07/15 02:46:33 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ofw/openfirm.h>

#include <machine/platform.h>

static void	platform_not_configured(void);
static void	platform_not_supported(void);

struct platform platform;

struct platmap platmap[] = {
	{ "FirePower,Powerized_ES",
	  PLATID_FIREPOWER_ES },

	{ "FirePower,Powerized_MX",
	  PLATID_FIREPOWER_MX },
	{ "FirePower,Powerized_MX MP",
	  PLATID_FIREPOWER_MX },

	{ "FirePower,Powerized_LX",
	  PLATID_FIREPOWER_LX },
	{ "FirePower,Powerized_LX MP",
	  PLATID_FIREPOWER_LX },

	{ "TotalImpact,BRIQ",
	  PLATID_TOTALIMPACT_BRIQ },

	{ NULL,
	  PLATID_UNKNOWN },
};

/* This is never optional. */
extern void	ofwgen_init(void);

#include "opt_firepower_es.h"
#if defined(FIREPOWER_ES)
extern void	firepower_init(void);
#else
#define		firepower_init		platform_not_configured
#endif

#include "opt_firepower_mx.h"
#if defined(FIREPOWER_MX)
extern void	firepower_init(void);
#else
#define		firepower_init		platform_not_configured
#endif

#include "opt_firepower_lx.h"
#if defined(FIREPOWER_LX)
extern void	firepower_init(void);
#else
#define		firepower_init		platform_not_configured
#endif

#include "opt_totalimpact_briq.h"
#if defined(TOTALIMPACT_BRIQ)
extern void	totalimpact_briq_init(void);
#else
#define		totalimpact_briq_init	platform_not_configured
#endif

#define	plat_init(opt, fn)	{ opt, fn }

/*
 * This table is indexed by the PLATID_* constants.
 */
const struct platinit platinit[] = {
	plat_init(NULL, ofwgen_init),
	plat_init("FIREPOWER_ES", firepower_init),
	plat_init("FIREPOWER_MX", firepower_init),
	plat_init("FIREPOWER_LX", firepower_init),
	plat_init("TOTALIMPACT_BRIQ", totalimpact_briq_init),
};

char	platform_name[64];
int	platid;

static void
platform_not_configured(void)
{

	printf("This kernel is not configured to run on a \"%s\".\n",
	    platform_name);
	printf("Please build a kernel with \"options %s\".\n",
	    platinit[platid].option);
	printf("Using generic OpenFirmware driver support.\n");

	platid = PLATID_UNKNOWN;
	(*platinit[platid].init)();
}

static void
platform_not_supported(void)
{

	printf("NetBSD does not yet support the \"%s\".\n",
	    platform_name);
	printf("Using generic OpenFirmware driver support.\n");

	platid = PLATID_UNKNOWN;
}

void
platform_init(void)
{
	const struct platmap *pm;
	int node;

	/* Fetch the platform name from the root of the OFW tree. */
	node = OF_peer(0);
	OF_getprop(node, "name", platform_name, sizeof(platform_name));

	/* Map the platform name to a platform ID. */
	for (pm = platmap; pm->name != NULL; pm++) {
		if (strcmp(platform_name, pm->name) == 0)
			break;
	}
	if (pm->name == NULL)
		platform_not_supported();
	else
		platid = pm->platid;

	/* Now initialize the platform structure. */
	(*platinit[platid].init)();
}
