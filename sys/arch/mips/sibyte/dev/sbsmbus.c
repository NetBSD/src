/* $NetBSD: sbsmbus.c,v 1.1 2002/06/04 08:32:41 simonb Exp $ */

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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/swarm.h>
#include <mips/sibyte/dev/sbsmbusvar.h>

#include "locators.h"

static int smbus_match(struct device *, struct cfdata *, void *);
static void smbus_attach(struct device *, struct device *, void *);
static int smbus_print(void *, const char *);
static int smbus_submatch(struct device *, struct cfdata *, void *);

struct cfattach smbus_ca = {
	sizeof(struct device), smbus_match, smbus_attach
};

/* autoconfiguration match information for zbbus children */
struct smbus_attach_locs {
	int	sa_interface;
	int	sa_device;
};

/* XXX XXX this table should be imported from machine-specific code XXX XXX */
static const struct smbus_attach_locs smbus_devs[] = {
	{ X1240_SMBUS_CHAN,	X1240_RTC_SMBUS_DEV },
};
static const int smbus_dev_count = sizeof smbus_devs / sizeof smbus_devs[0];

static int found = 0;

static int
smbus_match(struct device *parent, struct cfdata *match, void *aux)
{

	/* 2 SMBus's on the BCM112x and BCM1250 */
	return (found < 2);
}

static void
smbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct smbus_attach_args sa;
	int i;

	found++;
	printf("\n");

	for (i = 0; i < smbus_dev_count; i++) {
		if (self->dv_unit != smbus_devs[i].sa_interface)
			continue;

		memset(&sa, 0, sizeof sa);
		sa.sa_interface = smbus_devs[i].sa_interface;
		sa.sa_device = smbus_devs[i].sa_device;
		config_found_sm(self, &sa, smbus_print, smbus_submatch);
	}
}


static int
smbus_print(void *aux, const char *pnp)
{
	struct smbus_attach_args *sa = aux;

	if (pnp)
		printf("rtc0 at %s", pnp);	/* XXX! */
	printf(" device 0x%x", sa->sa_device);

	return (UNCONF);
}

static int
smbus_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct smbus_attach_args *sa = aux;

	if (cf->cf_loc[SMBUSCF_DEV] != SMBUSCF_DEV_DEFAULT &&
	    cf->cf_loc[SMBUSCF_DEV] != sa->sa_device)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}
