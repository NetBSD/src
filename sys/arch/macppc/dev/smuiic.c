/*-
 * Copyright (c) 2013 Phileas Fogg
 * All rights reserved.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/sysctl.h>

#include <machine/autoconf.h>

#include <dev/ofw/openfirm.h>
#include <dev/i2c/i2cvar.h>

#include <macppc/dev/smuvar.h>
#include <macppc/dev/smuiicvar.h>

struct smuiic_softc {
	device_t sc_dev;
	int sc_node;
	struct i2c_controller *sc_i2c;
};

static int smuiic_match(device_t, struct cfdata *, void *);
static void smuiic_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(smuiic, sizeof(struct smuiic_softc),
    smuiic_match, smuiic_attach, NULL, NULL);

static int
smuiic_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct smu_iicbus_confargs *ca = aux;

	if (strcmp(ca->ca_name, "i2c-bus") == 0)
		return 5;
	
	return 0;
}

static void
smuiic_attach(device_t parent, device_t self, void *aux)
{
	struct smu_iicbus_confargs *ca = aux;
	struct smuiic_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;
	prop_dictionary_t dict = device_properties(self);
	int devs;
	uint32_t addr;
	char compat[256];
	prop_array_t cfg;
	prop_dictionary_t dev;
	prop_data_t data;
	char name[32];

	sc->sc_dev = self;
	sc->sc_node = ca->ca_node;
	sc->sc_i2c = ca->ca_tag;
	printf("\n");

	cfg = prop_array_create();
	prop_dictionary_set(dict, "i2c-child-devices", cfg);
	prop_object_release(cfg);

	/* look for i2c devices */
	devs = OF_child(sc->sc_node);
	while (devs != 0) {
		if (OF_getprop(devs, "name", name, 256) <= 0)
			goto skip;
		if (OF_getprop(devs, "compatible",
		    compat, 256) <= 0)
			goto skip;
		if (OF_getprop(devs, "reg", &addr, 4) <= 0)
			goto skip;
		addr = (addr & 0xff) >> 1;
		dev = prop_dictionary_create();
		prop_dictionary_set_cstring(dev, "name", name);
		data = prop_data_create_data(compat, strlen(compat)+1);
		prop_dictionary_set(dev, "compatible", data);
		prop_object_release(data);
		prop_dictionary_set_uint32(dev, "addr", addr);
		prop_dictionary_set_uint64(dev, "cookie", devs);
		prop_array_add(cfg, dev);
		prop_object_release(dev);
	skip:
		devs = OF_peer(devs);
	}

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = sc->sc_i2c;

	config_found_ia(sc->sc_dev, "i2cbus", &iba, iicbus_print);
}
