/* $NetBSD: sbsmbus.c,v 1.16.12.1 2017/12/03 11:36:29 jdolecek Exp $ */

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbsmbus.c,v 1.16.12.1 2017/12/03 11:36:29 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <evbmips/sbmips/swarm.h>
#include <mips/sibyte/dev/sbsmbusvar.h>

#include <dev/smbus/x1241reg.h>
#include <dev/smbus/m41t81reg.h>

#include "locators.h"

static int smbus_match(device_t, cfdata_t, void *);
static void smbus_attach(device_t, device_t, void *);
static int smbus_print(void *, const char *);

CFATTACH_DECL_NEW(smbus, 0,
    smbus_match, smbus_attach, NULL, NULL);

/* autoconfiguration match information for zbbus children */
struct smbus_attach_locs {
	int	sa_interface;
	int	sa_device;
};

/* XXX XXX this table should be imported from machine-specific code XXX XXX */
static const struct smbus_attach_locs smbus_devs[] = {
	{ X1241_SMBUS_CHAN,	X1241_RTC_SLAVEADDR },
	{ M41T81_SMBUS_CHAN,	M41T81_SLAVEADDR },
};

static int found = 0;

static int
smbus_match(device_t parent, cfdata_t match, void *aux)
{

	/* 2 SMBus's on the BCM112x and BCM1250 */
	return (found < 2);
}

static void
smbus_attach(device_t parent, device_t self, void *aux)
{
	struct smbus_attach_args sa;
	int i;
	int locs[SMBUSCF_NLOCS];

	found++;
	aprint_normal("\n");

	for (i = 0; i < __arraycount(smbus_devs); i++) {
		if (device_unit(self) != smbus_devs[i].sa_interface)
			continue;

		memset(&sa, 0, sizeof sa);
		sa.sa_interface = smbus_devs[i].sa_interface;
		sa.sa_device = smbus_devs[i].sa_device;

		locs[SMBUSCF_CHAN] = 0; /* XXX */
		locs[SMBUSCF_DEV] = smbus_devs[i].sa_device;

		config_found_sm_loc(self, "smbus", locs, &sa,
				    smbus_print, config_stdsubmatch);
	}
}


static int
smbus_print(void *aux, const char *pnp)
{
	struct smbus_attach_args *sa = aux;

	if (pnp)
		aprint_normal("rtc0 at %s", pnp);	/* XXX! */
	aprint_normal(" device 0x%x", sa->sa_device);

	return (UNCONF);
}
