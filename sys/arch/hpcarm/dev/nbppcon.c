/*	$NetBSD: nbppcon.c,v 1.2.38.1 2018/06/25 07:25:41 pgoyette Exp $ */
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nbppcon.c,v 1.2.38.1 2018/06/25 07:25:41 pgoyette Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/xscale/pxa2x0_i2c.h>

#include <hpcarm/dev/nbppconvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/at24cxxvar.h>

#include "locators.h"

#define NBPPCON_ADDR	0x13


struct nbppcon_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	int sc_address;

	struct callback {
		int cb_tag;
		int (*cb_func)(void *, int, char *);
		void *cb_arg;
	} sc_cbs[2];
};

static int nbppcon_match(device_t, cfdata_t, void *);
static void nbppcon_attach(device_t, device_t, void *);

static int nbppcon_search(device_t, cfdata_t, const int *, void *);
static int nbppcon_print(void *aux, const char *pnp);

CFATTACH_DECL_NEW(nbppcon, sizeof(struct nbppcon_softc),
    nbppcon_match, nbppcon_attach, NULL, NULL);


/* ARGSUSED */
static int
nbppcon_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_addr != NBPPCON_ADDR ||
	    !platid_match(&platid, &platid_mask_MACH_PSIONTEKLOGIX_NETBOOK_PRO))
		 return 0;

	return I2C_MATCH_ADDRESS_AND_PROBE;
}

/* ARGSUSED */
static void
nbppcon_attach(device_t parent, device_t self, void *aux)
{
	struct nbppcon_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	int rv;
	char buf[128];

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;

	aprint_naive("\n");
	aprint_normal(": NETBOOK PRO PCon\n");

#define NVRAM_ADDR	0x53
#define NVRAM_DATA_REV	0x14
	rv = seeprom_bootstrap_read(sc->sc_tag, NVRAM_ADDR, 0x00, 128,
	    buf, sizeof(buf));
	if (rv == 0)
		aprint_normal_dev(self, "NETBOOK PRO Rev.%c\n",
		    buf[NVRAM_DATA_REV] - '0' + '@');
	else
		aprint_error_dev(self, "NVRAM read failed\n");

	config_search_ia(nbppcon_search, self, "nbppcon", NULL);
}

/* ARGSUSED */
static int
nbppcon_search(device_t self, cfdata_t cf, const int *ldesc, void *aux)
{
	struct nbppcon_attach_args pcon;

	pcon.aa_name = cf->cf_name;
	pcon.aa_tag = cf->cf_loc[NBPPCONCF_TAG];

	if (config_match(self, cf, &pcon))
		config_attach(self, cf, &pcon, nbppcon_print);

	return 0;
}

/* ARGSUSED */
static int
nbppcon_print(void *aux, const char *pnp)
{

	if (pnp != NULL)
		aprint_normal("%s", pnp);
	return UNCONF;
}


void *
nbppcon_regist_callback(device_t self,
			int tag, int (*func)(void *, int, char *), void *arg)
{
	struct nbppcon_softc *sc = device_private(self);
	int i;

	for (i = 0; i < __arraycount(sc->sc_cbs); i++)
		if (sc->sc_cbs[i].cb_func == NULL) {
			sc->sc_cbs[i].cb_tag = tag;
			sc->sc_cbs[i].cb_func = func;
			sc->sc_cbs[i].cb_arg = arg;
			return func;
		}

	return NULL;
}

int
nbppcon_intr(device_t self, int buflen, char *buf)
{
	struct nbppcon_softc *sc = device_private(self);
	struct callback *cb;
	int i;

	for (i = 0; i < __arraycount(sc->sc_cbs); i++) {
		cb = &sc->sc_cbs[i];
		if (cb->cb_func != NULL &&
		    cb->cb_tag == buf[0])
			return cb->cb_func(cb->cb_arg, buflen, buf);
	}
	aprint_error_dev(sc->sc_dev, "unknown tag received: 0x%02x\n", buf[0]);
	return 0;
}

int
nbppcon_poll(device_t self, int tag, int buflen, char *buf)
{
	struct nbppcon_softc *sc = device_private(self);
	int rv;

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		aprint_error_dev(sc->sc_dev,
		    "%s: acquire bus failed\n", __func__);
		return -1;
	}

	do {
		extern int nbpiic_poll(void *, int, char *);

		rv = nbpiic_poll(sc->sc_tag->ic_cookie, buflen, buf);
		if (rv > 0 && buf[0] == tag)
			break;
	} while (rv == 0 || buf[0] != tag);

	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	return rv;
}

int
nbppcon_kbd_ready(device_t self)
{
	struct nbppcon_softc *sc = device_private(self);
	int rv;
	char cmd[1], data[1];

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		aprint_error_dev(sc->sc_dev,
		    "%s: acquire bus failed\n", __func__);
		return -1;
	}

	cmd[0] = 0x00;		/* Keyboard */
	data[0] = 0x00;		/* 0x00:Ready, 0x01:Busy? */
	rv = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_address,
	    cmd, sizeof(cmd), data, sizeof(data), I2C_F_POLL);

	cpu_dcache_wbinv_all();		/* XXXX: Why? */

	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	return rv;
}

int
nbppcon_pwr_hwreset(device_t self)
{
	struct nbppcon_softc *sc = device_private(self);
	int rv;
	char cmd[1], data[1];

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		aprint_error_dev(sc->sc_dev,
		    "%s: acquire bus failed\n", __func__);
		return -1;
	}

	cmd[0] = 0x05;		/* POWER */
	data[0] = 0x00;		/* HWReset */
#if 0
	rv = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_address,
	    cmd, sizeof(cmd), data, sizeof(data), I2C_F_POLL);
#else	/* XXXX: Oops, ensure HWReset, ignore cmd and data. */
	rv = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_address + 1,
	    cmd, sizeof(cmd), data, sizeof(data), I2C_F_POLL);
#endif

	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	return rv;
}
