/* $NetBSD: iic_eumb.c,v 1.10.2.1 2011/06/06 09:06:34 jruoho Exp $ */

/*-
 * Copyright (c) 2010,2011 Frank Wille.
 * All rights reserved.
 *
 * Written by Frank Wille for The NetBSD Project.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iic_eumb.c,v 1.10.2.1 2011/06/06 09:06:34 jruoho Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <dev/i2c/motoi2cvar.h>
#include <sandpoint/sandpoint/eumbvar.h>
#include <machine/bootinfo.h>

struct iic_eumb_softc {
	device_t		sc_dev;
	struct motoi2c_softc	sc_motoi2c;
};

static int  iic_eumb_match(struct device *, struct cfdata *, void *);
static void iic_eumb_attach(struct device *, struct device *, void *);

CFATTACH_DECL_NEW(iic_eumb, sizeof(struct iic_eumb_softc),
    iic_eumb_match, iic_eumb_attach, NULL, NULL);

static int found;

struct i2cdev {
	const char *family;
	const char *name;
	int addr;
};

static struct i2cdev rtcmodel[] = {
    { "dlink",    "strtc",      0x68 },
    { "iomega",   "dsrtc",      0x68 },
    { "kurobox",  "rs5c372rtc", 0x32 },
    { "qnap",     "s390rtc",    0x30 },
    { "synology", "rs5c372rtc", 0x32 },
};

static void add_i2c_child_devices(device_t, const char *);

static int
iic_eumb_match(device_t parent, cfdata_t cf, void *aux)
{

	return found == 0;
}

static void
iic_eumb_attach(device_t parent, device_t self, void *aux)
{
	struct iic_eumb_softc *sc;
	struct eumb_attach_args *eaa;
	bus_space_handle_t ioh;
	struct btinfo_prodfamily *pfam;

	sc = device_private(self);
	sc->sc_dev = self;
	eaa = aux;
	found = 1;

	aprint_naive("\n");
	aprint_normal("\n");

	if ((pfam = lookup_bootinfo(BTINFO_PRODFAMILY)) != NULL)
		add_i2c_child_devices(self, pfam->name);

	/*
	 * map EUMB registers and attach MI motoi2c with default settings
	 */
	bus_space_map(eaa->eumb_bt, 0x3000, 0x20, 0, &ioh);
	sc->sc_motoi2c.sc_iot = eaa->eumb_bt;
	sc->sc_motoi2c.sc_ioh = ioh;
	motoi2c_attach_common(self, &sc->sc_motoi2c, NULL);
}

static void
add_i2c_child_devices(device_t self, const char *family)
{
	struct i2cdev *rtc;
	prop_dictionary_t pd;
	prop_array_t pa;
	int i;

	rtc = NULL;
	for (i = 0; i < (int)(sizeof(rtcmodel)/sizeof(rtcmodel[0])); i++) {
		if (strcmp(family, rtcmodel[i].family) == 0) {
			rtc = &rtcmodel[i];
			goto found;
		}
	}
	return;

 found:
	pd = prop_dictionary_create();
	pa = prop_array_create();
	prop_dictionary_set_cstring_nocopy(pd, "name", rtc->name);
	prop_dictionary_set_uint32(pd, "addr", rtc->addr);
	prop_array_add(pa, pd);
	prop_dictionary_set(device_properties(self), "i2c-child-devices", pa);
	prop_object_release(pd);
	prop_object_release(pa);
}
