/*	$NetBSD: ralink_mainbus.c,v 1.2.12.2 2017/12/03 11:36:28 jdolecek Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ralink_mainbus.c,v 1.2.12.2 2017/12/03 11:36:28 jdolecek Exp $");

#include "locators.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>

#include <mips/ralink/ralink_reg.h>
#include <mips/ralink/ralink_var.h>

typedef struct mainbus_softc {
	device_t	sc_dev;
	bus_space_tag_t	sc_memt;
	bus_dma_tag_t	sc_dmat;
} mainbus_softc_t;

static int  mainbus_match(device_t, cfdata_t, void *);
static void mainbus_attach(device_t, device_t, void *);
static int  mainbus_search(device_t, cfdata_t, const int *, void *);
static int  mainbus_print(void *, const char *);
static int  mainbus_find(device_t, cfdata_t, const int *, void *);
static void mainbus_attach_critical(struct mainbus_softc *);


CFATTACH_DECL_NEW(mainbus, sizeof(struct mainbus_softc),
	mainbus_match, mainbus_attach, NULL, NULL);

static bool mainbus_found;

/*
 * critical devices, if found, will be attached in the order listed here
 */
static const struct {
	const char *name;
	bool required;
} critical_devs[] = {
	{ .name = "cpu0",	.required = true  },
	{ .name = "rwdog0",	.required = false },
	{ .name = "rgpio0",	.required = true  },
	{ .name = "ri2c0",	.required = false },
	{ .name = "com0",	.required = false },
};


static int
mainbus_match(device_t parent, cfdata_t match, void *aux)
{
	if (mainbus_found)
		return 0;
	return 1;
}

static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	mainbus_softc_t * const sc = device_private(self);
	struct mainbus_attach_args ma;
	union {
		char buf[9];
		uint32_t id[2];
	} xid;

	mainbus_found = true;

	sc->sc_dev = self;
	sc->sc_memt = &ra_bus_memt;
	sc->sc_dmat = &ra_bus_dmat;

	xid.id[0] = bus_space_read_4(sc->sc_memt, ra_sysctl_bsh, RA_SYSCTL_ID0);
	xid.id[1] = bus_space_read_4(sc->sc_memt, ra_sysctl_bsh, RA_SYSCTL_ID1);
	xid.buf[8] = '\0';
	if (xid.buf[6] == ' ') {
		xid.buf[6] = '\0';
	} else if (xid.buf[7] == ' ') {
		xid.buf[7] = '\0';
	}
	const char * const manuf = xid.buf[0] == 'M' ? "Mediatek" : "Ralink";

	aprint_naive(": %s %s System Bus\n", manuf, xid.buf);
	aprint_normal(": %s %s System Bus\n", manuf, xid.buf);

	ma.ma_name = NULL;
	ma.ma_memt = sc->sc_memt;
	ma.ma_dmat = sc->sc_dmat;

	/* attach critical devices */
	mainbus_attach_critical(sc);

	/* attach other devices */
	config_search_ia(mainbus_search, self, "mainbus", &ma);
}

static int
mainbus_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct mainbus_attach_args *ma;

	ma = aux;
	ma->ma_addr = cf->cf_loc[MAINBUSCF_ADDR];

	if (config_match(parent, cf, aux) > 0)
		config_attach(parent, cf, aux, mainbus_print);
	else
		mainbus_print(aux, cf->cf_name);
	return 0;
}

int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *ma;

	if (pnp)
		aprint_normal("%s unconfigured\n", pnp);

	ma = aux;
	if (ma->ma_addr != MAINBUSCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%llx", ma->ma_addr);

	return UNCONF;
}

static int
mainbus_find(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct mainbus_attach_args * const ma = aux;
	char devname[16];

	snprintf(devname, sizeof(devname), "%s%d", cf->cf_name, cf->cf_unit);
	if (strcmp(ma->ma_name, devname) != 0)
		return 0;

	return config_match(parent, cf, ma);
}

static void
mainbus_attach_critical(struct mainbus_softc *sc)
{
	struct mainbus_attach_args ma;
	cfdata_t cf;
	size_t i;

	for (i = 0; i < __arraycount(critical_devs); i++) {
		ma.ma_name = critical_devs[i].name;
		ma.ma_memt = sc->sc_memt;
		ma.ma_dmat = sc->sc_dmat;

		cf = config_search_ia(mainbus_find, sc->sc_dev, "mainbus", &ma);
		if (cf == NULL && critical_devs[i].required)
			panic("%s: failed to find %s",
			    __func__, critical_devs[i].name);

		ma.ma_addr = cf->cf_loc[MAINBUSCF_ADDR];
		config_attach(sc->sc_dev, cf, &ma, mainbus_print);
	}
}
