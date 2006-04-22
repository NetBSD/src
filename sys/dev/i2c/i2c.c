/*	$NetBSD: i2c.c,v 1.5.6.1 2006/04/22 11:38:52 simonb Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
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
#include <sys/device.h>
#include <sys/event.h>
#include <sys/conf.h>

#include <dev/i2c/i2cvar.h>

#include "locators.h"

struct iic_softc {
	struct device sc_dev;
	i2c_tag_t sc_tag;
	int sc_type;
};

int
iicbus_print(void *aux, const char *pnp)
{
	struct i2cbus_attach_args *iba = aux;

	if (pnp != NULL)
		aprint_normal("%s at %s", iba->iba_name, pnp);

	return (UNCONF);
}

static int
iic_print(void *aux, const char *pnp)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_addr != (i2c_addr_t)-1)
		aprint_normal(" addr 0x%x", ia->ia_addr);

	return (UNCONF);
}

static int
iic_search(struct device *parent, struct cfdata *cf,
	   const int *ldesc, void *aux)
{
	struct iic_softc *sc = (void *) parent;
	struct i2c_attach_args ia;

	ia.ia_tag = sc->sc_tag;
	ia.ia_addr = cf->cf_loc[IICCF_ADDR];
	ia.ia_size = cf->cf_loc[IICCF_SIZE];
	ia.ia_type = sc->sc_type;

	if (config_match(parent, cf, &ia) > 0)
		config_attach(parent, cf, &ia, iic_print);

	return (0);
}

static int
iic_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct i2cbus_attach_args *iba = aux;

	/* Just make sure we're looking for i2c. */
	return (strcmp(iba->iba_name, cf->cf_name) == 0);
}

static void
iic_attach(struct device *parent, struct device *self, void *aux)
{
	struct iic_softc *sc = device_private(self);
	struct i2cbus_attach_args *iba = aux;

	aprint_naive(": I2C bus\n");
	aprint_normal(": I2C bus\n");

	sc->sc_tag = iba->iba_tag;
	sc->sc_type = iba->iba_type;

	/*
	 * Attach all i2c devices described in the kernel
	 * configuration file.
	 */
	config_search_ia(iic_search, self, "iic", NULL);
}

CFATTACH_DECL(iic, sizeof(struct iic_softc),
    iic_match, iic_attach, NULL, NULL);
