/*      $NetBSD: mcp48x1.c,v 1.1.64.1 2021/08/09 00:30:09 thorpej Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
__KERNEL_RCSID(0, "$NetBSD: mcp48x1.c,v 1.1.64.1 2021/08/09 00:30:09 thorpej Exp $");

/* 
 * Driver for Microchip MCP4801/MCP4811/MCP4821 DAC. 
 *
 * XXX: needs more testing.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#include <dev/sysmon/sysmonvar.h> 

#include <dev/spi/spivar.h>

#define MCP48X1DAC_DEBUG 0

#define MCP48X1DAC_WRITE	__BIT(15)	/* active low */
#define MCP48X1DAC_GAIN		__BIT(13)	/* active low */
#define MCP48X1DAC_SHDN		__BIT(12)	/* active low */
#define MCP48X1DAC_DATA		__BITS(11,0)	/* data */

struct mcp48x1dac_model {
	u_int name;
	uint8_t	resolution;
	uint8_t	shift;			/* data left shift during write */
};

struct mcp48x1dac_softc {
	device_t sc_dev;
	struct spi_handle *sc_sh;

	const struct mcp48x1dac_model *sc_dm;

	uint16_t sc_dac_data;
	bool sc_dac_gain;
	bool sc_dac_shutdown;
	
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sm_vo;		/* envsys "sensor" (Vo) */
};

static int	mcp48x1dac_match(device_t, cfdata_t, void *);
static void	mcp48x1dac_attach(device_t, device_t, void *);

static bool	mcp48x1dac_envsys_attach(struct mcp48x1dac_softc *sc);
static void	mcp48x1dac_envsys_refresh(struct sysmon_envsys *,
		    envsys_data_t *);

static void	mcp48x1dac_write(struct mcp48x1dac_softc *);
static uint16_t mcp48x1dac_regval_to_mv(struct mcp48x1dac_softc *);

static void	mcp48x1dac_setup_sysctl(struct mcp48x1dac_softc *sc);
static int	sysctl_mcp48x1dac_data(SYSCTLFN_ARGS);
static int	sysctl_mcp48x1dac_gain(SYSCTLFN_ARGS);

CFATTACH_DECL_NEW(mcp48x1dac, sizeof(struct mcp48x1dac_softc),
    mcp48x1dac_match, mcp48x1dac_attach, NULL, NULL);

static const struct mcp48x1dac_model mcp4801 = {
	.name = 4801,
	.resolution = 8,
	.shift = 4
};

static const struct mcp48x1dac_model mcp4811 = {
	.name = 4811,
	.resolution = 10,
	.shift = 2
};

static const struct mcp48x1dac_model mcp4821 = {
	.name = 4821,
	.resolution = 12,
	.shift = 0
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "microchip,mcp4801",	.data = &mcp4801 },
	{ .compat = "microchip,mcp4811",	.data = &mcp4811 },
	{ .compat = "microchip,mcp4821",	.data = &mcp4821 },
	DEVICE_COMPAT_EOL
};

static const struct mcp48x1dac_model *
mcp48x1dac_lookup(const struct spi_attach_args *sa, const cfdata_t cf)
{
	const struct device_compatible_entry *dce;

	if (sa->sa_clist != NULL) {
		dce = device_compatible_lookup_strlist(sa->sa_clist,
		    sa->sa_clist_size, compat_data);
		if (dce == NULL) {
			return NULL;
		}
		return dce->data;
	} else {
		const struct mcp48x1dac_model *model;

		for (dce = compat_data; dce->compat != NULL; dce++) {
			model = dce->data;
			if (model->name == cf->cf_flags) {
				return model;
			}
		}
		return NULL;
	}
}

static int
mcp48x1dac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct spi_attach_args *sa = aux;
	int rv;

	rv = spi_compatible_match(sa, cf, compat_data);
	if (rv != 0) {
		/*
		 * If we're doing indorect config, the user must
		 * have specified the correct model.
		 */
		if (sa->sa_clist == NULL && mcp48x1dac_lookup(sa, cf) == NULL) {
			return 0;
		}
	}

	return rv;
}

static void
mcp48x1dac_attach(device_t parent, device_t self, void *aux)
{
	struct mcp48x1dac_softc *sc = device_private(self);
	struct spi_attach_args *sa = aux;
	const struct mcp48x1dac_model *model;
	int error;

	sc->sc_dev = self;
	sc->sc_sh = sa->sa_handle;

	model = mcp48x1dac_lookup(sa, device_cfdata(self));
	KASSERT(model != NULL);

	sc->sc_dm = model;

	aprint_naive(": Digital to Analog converter\n");	
	aprint_normal(": MCP%u DAC\n", model->name);

	/* configure for 20MHz */
	error = spi_configure(sa->sa_handle, SPI_MODE_0, 20000000);
	if (error) {
		aprint_error_dev(self, "spi_configure failed (error = %d)\n",
		    error);
		return;
	}

	if(!mcp48x1dac_envsys_attach(sc)) {
		aprint_error_dev(sc->sc_dev, "failed to attach envsys\n");
		return;
	};

	sc->sc_dac_data = 0;
	sc->sc_dac_gain = false;
	sc->sc_dac_shutdown = false;
	mcp48x1dac_write(sc);

	mcp48x1dac_setup_sysctl(sc);
}

static void
mcp48x1dac_write(struct mcp48x1dac_softc *sc) 
{
	int rv;
	uint16_t reg, regbe;

	reg = 0;

	if (!(sc->sc_dac_gain))
		reg |= MCP48X1DAC_GAIN;

	if (!(sc->sc_dac_shutdown))
		reg |= MCP48X1DAC_SHDN;

	reg |= sc->sc_dac_data << sc->sc_dm->shift;

	regbe = htobe16(reg);

#ifdef MCP48X1DAC_DEBUG
	aprint_normal_dev(sc->sc_dev, "sending %x over SPI\n", regbe);
#endif /* MCP48X1DAC_DEBUG */

	rv = spi_send(sc->sc_sh, 2, (uint8_t*) &regbe); /* XXX: ugly cast */
 
	if (rv != 0) 
		aprint_error_dev(sc->sc_dev, "error sending data over SPI\n");
}

static bool
mcp48x1dac_envsys_attach(struct mcp48x1dac_softc *sc)
{

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sm_vo.units = ENVSYS_SVOLTS_DC;
	sc->sc_sm_vo.state = ENVSYS_SINVALID;
	strlcpy(sc->sc_sm_vo.desc, device_xname(sc->sc_dev),
	    sizeof(sc->sc_sm_vo.desc));
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sm_vo)) {
		sysmon_envsys_destroy(sc->sc_sme);
		return false;
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_refresh = mcp48x1dac_envsys_refresh;
	sc->sc_sme->sme_cookie = sc;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(sc->sc_dev, "unable to register in sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
	}

	return true;
}

static uint16_t 
mcp48x1dac_regval_to_mv(struct mcp48x1dac_softc *sc)
{
	uint16_t mv;

	mv = (2048 * sc->sc_dac_data / (1 << sc->sc_dm->resolution));

	if (sc->sc_dac_gain)
		mv *= 2;

	return mv;
}

static void
mcp48x1dac_envsys_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct mcp48x1dac_softc *sc;

	sc = sme->sme_cookie;

	edata->value_cur = mcp48x1dac_regval_to_mv(sc); 
	edata->state = ENVSYS_SVALID;
}

static void
mcp48x1dac_setup_sysctl(struct mcp48x1dac_softc *sc)
{
	const struct sysctlnode *me = NULL, *node = NULL;
 
	sysctl_createv(NULL, 0, NULL, &me,
	    CTLFLAG_READWRITE,
	    CTLTYPE_NODE, device_xname(sc->sc_dev), NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "data", "Digital value to convert to analog",
	    sysctl_mcp48x1dac_data, 1, (void *)sc, 0,
	    CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL);
	
	sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "gain", "Gain 2x enable",
	    sysctl_mcp48x1dac_gain, 1, (void *)sc, 0,
	    CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL);

}


SYSCTL_SETUP(sysctl_mcp48x1dac_setup, "sysctl mcp48x1dac subtree setup")
{
	sysctl_createv(NULL, 0, NULL, NULL, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "machdep", NULL, NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_EOL);
}


static int
sysctl_mcp48x1dac_data(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct mcp48x1dac_softc *sc = node.sysctl_data;
	int newdata, err;

	node.sysctl_data = &sc->sc_dac_data;
	if ((err = (sysctl_lookup(SYSCTLFN_CALL(&node)))) != 0) 
		return err;

	if (newp) {
		newdata = *(int *)node.sysctl_data;
		if (newdata > (1 << sc->sc_dm->resolution))
			return EINVAL;
		sc->sc_dac_data = (uint16_t) newdata;
		mcp48x1dac_write(sc);
		return 0;
	} else {
		/* nothing to do, since we can't read from DAC */
		node.sysctl_size = 4;
	}

	return err;
}

static int
sysctl_mcp48x1dac_gain(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct mcp48x1dac_softc *sc = node.sysctl_data;
	int newgain, err;

	node.sysctl_data = &sc->sc_dac_gain;
	if ((err = (sysctl_lookup(SYSCTLFN_CALL(&node)))) != 0) 
		return err;

	if (newp) {
		newgain = *(int *)node.sysctl_data;
		sc->sc_dac_gain = (bool) newgain;
		mcp48x1dac_write(sc);
		return 0;
	} else {
		/* nothing to do, since we can't read from DAC */
		node.sysctl_size = 4;
	}

	return err;
}

