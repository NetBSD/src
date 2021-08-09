/*	$NetBSD: mcp3k.c,v 1.2.46.1 2021/08/09 00:30:09 thorpej Exp $ */

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank Wille.
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

/*
 * Microchip MCP3x0x SAR analog to digital converters.
 * The driver supports various ADCs with different resolutions, operation
 * modes and number of input channels.
 * The reference voltage Vref defaults to the maximum output value in mV,
 * but can be changed via sysctl(3).
 *
 * MCP3001: http://ww1.microchip.com/downloads/en/DeviceDoc/21293C.pdf
 * MCP3002: http://ww1.microchip.com/downloads/en/DeviceDoc/21294E.pdf
 * MCP3004/3008: http://ww1.microchip.com/downloads/en/DeviceDoc/21295C.pdf
 * MCP3201: http://ww1.microchip.com/downloads/en/DeviceDoc/21290D.pdf
 * MCP3204/3208: http://ww1.microchip.com/downloads/en/DeviceDoc/21298c.pdf
 * MCP3301: http://ww1.microchip.com/downloads/en/DeviceDoc/21700E.pdf
 * MPC3302/3304: http://ww1.microchip.com/downloads/en/DeviceDoc/21697F.pdf
 */

#include "opt_fdt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/types.h> 
#include <sys/sysctl.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/spi/spivar.h>

#ifdef FDT
#include <dev/fdt/fdtvar.h>
#endif /* FDT */

#define M3K_MAX_SENSORS		16		/* 8 single-ended & 8 diff. */

/* mcp3x0x model description */
struct mcp3kadc_model {
	uint32_t		name;
	uint8_t			bits;
	uint8_t			channels;
	uint8_t			lead;		/* leading bits to ignore */
	uint8_t			flags;
#define M3K_SGLDIFF		0x01		/* single-ended/differential */
#define M3K_D2D1D0		0x02		/* 3 channel select bits */
#define M3K_MSBF		0x04		/* MSBF select bit */
#define M3K_SIGNED		0x80		/* result is signed */
#define M3K_CTRL_NEEDED		(M3K_SGLDIFF | M3K_D2D1D0 | M3K_MSBF)
};

struct mcp3kadc_softc {
	device_t			sc_dev;
	struct spi_handle 		*sc_sh;
	const struct mcp3kadc_model	*sc_model;
	uint32_t			sc_adc_max;
	int32_t				sc_vref_mv;
#ifdef FDT
	struct fdtbus_regulator		*sc_vref_supply;
#endif

	struct sysmon_envsys 		*sc_sme;
	envsys_data_t 			sc_sensors[M3K_MAX_SENSORS];
};

static int	mcp3kadc_match(device_t, cfdata_t, void *);
static void	mcp3kadc_attach(device_t, device_t, void *);
static void	mcp3kadc_envsys_refresh(struct sysmon_envsys *,
		    envsys_data_t *);
static int	sysctl_mcp3kadc_vref(SYSCTLFN_ARGS);

CFATTACH_DECL_NEW(mcp3kadc, sizeof(struct mcp3kadc_softc),
    mcp3kadc_match,  mcp3kadc_attach, NULL, NULL);

static const struct mcp3kadc_model mcp3001 = {
	.name = 3001,
	.bits = 10,
	.channels = 1,
	.lead = 3,
	.flags = 0
};

static const struct mcp3kadc_model mcp3002 = {
	.name = 3002,
	.bits = 10,
	.channels = 2,
	.lead = 2,
	.flags = M3K_SGLDIFF | M3K_MSBF
};

static const struct mcp3kadc_model mcp3004 = {
	.name = 3004,
	.bits = 10,
	.channels = 4,
	.lead = 2,
	.flags = M3K_SGLDIFF | M3K_D2D1D0
};

static const struct mcp3kadc_model mcp3008 = {
	.name = 3008,
	.bits = 10,
	.channels = 8,
	.lead = 2,
	.flags = M3K_SGLDIFF | M3K_D2D1D0
};

static const struct mcp3kadc_model mcp3201 = {
	.name = 3201,
	.bits = 12,
	.channels = 1,
	.lead = 3,
	.flags = 0
};

static const struct mcp3kadc_model mcp3202 = {
	.name = 3202,
	.bits = 12,
	.channels = 2,
	.lead = 2,
	.flags = M3K_SGLDIFF | M3K_MSBF
};

static const struct mcp3kadc_model mcp3204 = {
	.name = 3204,
	.bits = 12,
	.channels = 4,
	.lead = 2,
	.flags = M3K_SGLDIFF | M3K_D2D1D0
};

static const struct mcp3kadc_model mcp3208 = {
	.name = 3208,
	.bits = 12,
	.channels = 8,
	.lead = 2,
	.flags = M3K_SGLDIFF | M3K_D2D1D0
};

static const struct mcp3kadc_model mcp3301 = {
	.name = 3301,
	.bits = 13,
	.channels = 1,
	.lead = 3,
	.flags = M3K_SIGNED
};

static const struct mcp3kadc_model mcp3302 = {
	.name = 3302,
	.bits = 13,
	.channels = 4,
	.lead = 2,
	.flags = M3K_SIGNED | M3K_SGLDIFF | M3K_D2D1D0
};

static const struct mcp3kadc_model mcp3304 = {
	.name = 3304,
	.bits = 13,
	.channels = 8,
	.lead = 2,
	.flags = M3K_SIGNED | M3K_SGLDIFF | M3K_D2D1D0
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "microchip,mcp3001",	.data = &mcp3001 },
	{ .compat = "microchip,mcp3002",	.data = &mcp3002 },
	{ .compat = "microchip,mcp3004",	.data = &mcp3004 },
	{ .compat = "microchip,mcp3008",	.data = &mcp3008 },
	{ .compat = "microchip,mcp3201",	.data = &mcp3201 },
	{ .compat = "microchip,mcp3202",	.data = &mcp3202 },
	{ .compat = "microchip,mcp3204",	.data = &mcp3204 },
	{ .compat = "microchip,mcp3208",	.data = &mcp3208 },
	{ .compat = "microchip,mcp3301",	.data = &mcp3301 },
	{ .compat = "microchip,mcp3302",	.data = &mcp3302 },
	{ .compat = "microchip,mcp3304",	.data = &mcp3304 },

#if 0	/* We should also add support for these: */
	{ .compat = "microchip,mcp3550-50" },
	{ .compat = "microchip,mcp3550-60" },
	{ .compat = "microchip,mcp3551" },
	{ .compat = "microchip,mcp3553" },
#endif

	DEVICE_COMPAT_EOL
};

static const struct mcp3kadc_model *
mcp3kadc_lookup(const struct spi_attach_args *sa, const cfdata_t cf)
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
		const struct mcp3kadc_model *model;

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
mcp3kadc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct spi_attach_args *sa = aux;
	int rv;

	rv = spi_compatible_match(sa, cf, compat_data);
	if (rv != 0) {
		/*
		 * If we're doing indirect config, the user must
		 * have specified the correct model.
		 */
		if (sa->sa_clist == NULL && mcp3kadc_lookup(sa, cf) == NULL) {
			return 0;
		}
	}

	return rv;
}

#ifdef FDT
static bool
mcp3kadc_vref_fdt(struct mcp3kadc_softc *sc)
{
	devhandle_t devhandle = device_handle(sc->sc_dev);
	int phandle = devhandle_to_of(devhandle);
	int error;
	u_int uvolts;

	sc->sc_vref_supply = fdtbus_regulator_acquire(phandle, "vref-supply");
	if (sc->sc_vref_supply == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to acquire \"vref-supply\"\n");
		return false;
	}

	error = fdtbus_regulator_enable(sc->sc_vref_supply);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to enable \"vref-supply\" (error = %d)\n",
		    error);
		return false;
	}

	error = fdtbus_regulator_get_voltage(sc->sc_vref_supply, &uvolts);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to get \"vref-supply\" voltage (error = %d)\n",
		    error);
		(void) fdtbus_regulator_disable(sc->sc_vref_supply);
		return false;
	}

	/*
	 * Device tree property is uV, convert to mV that we use
	 * internally.
	 */
	sc->sc_vref_mv = uvolts / 1000;
	return true;
}
#endif /* FDT */

static void
mcp3kadc_attach(device_t parent, device_t self, void *aux)
{
	const struct sysctlnode *rnode, *node;
	struct spi_attach_args *sa = aux;
	struct mcp3kadc_softc *sc = device_private(self);
	devhandle_t devhandle = device_handle(self);
	const struct mcp3kadc_model *model;
	int ch, i, error;
	bool vref_read_only;

	sc->sc_dev = self;
	sc->sc_sh = sa->sa_handle;

	model = mcp3kadc_lookup(sa, device_cfdata(self));
	KASSERT(model != NULL);

	sc->sc_model = model;

	aprint_naive(": Analog to Digital converter\n");
	aprint_normal(": MCP%u %u-channel %u-bit ADC\n",
	    (unsigned)model->name, (unsigned)model->channels,
	    (unsigned)model->bits);

	/* configure for 1MHz */
	error = spi_configure(sa->sa_handle, SPI_MODE_0, 1000000);
	if (error) {
		aprint_error_dev(self, "spi_configure failed (error = %d)\n",
		    error);
		return;
	}

	vref_read_only = false;
	switch (devhandle_type(devhandle)) {
#ifdef FDT
	case DEVHANDLE_TYPE_OF:
		vref_read_only = mcp3kadc_vref_fdt(sc);
		if (! vref_read_only) {
			/* Error already displayed. */
			return;
		}
		break;
#endif /* FDT */
	default:
		/*
		 * set a default Vref in mV according to the chip's ADC
		 * resolution
		 */
		sc->sc_vref_mv = 1 << ((model->flags & M3K_SIGNED) ?
		    model->bits - 1 : model->bits);
		break;
	}


	/* remember maximum value for this ADC - also used for masking */
	sc->sc_adc_max = (1 << model->bits) - 1;

	/* attach voltage sensors to envsys */
	sc->sc_sme = sysmon_envsys_create();

	/* adc difference from two neighbouring channels */
	for (ch = 0; ch < model->channels; ch++) {
		KASSERT(ch < M3K_MAX_SENSORS);
		sc->sc_sensors[ch].units = ENVSYS_SVOLTS_DC;
		sc->sc_sensors[ch].state = ENVSYS_SINVALID;
		if (model->channels == 1)
			strlcpy(sc->sc_sensors[ch].desc, "adc diff ch0",
			    sizeof(sc->sc_sensors[ch].desc));
		else
			snprintf(sc->sc_sensors[ch].desc,
			    sizeof(sc->sc_sensors[ch].desc),
			    "adc diff ch%d-ch%d", ch, ch ^ 1);
		sc->sc_sensors[ch].private = ch;
		sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensors[ch]);
	}

	if (model->flags & M3K_SGLDIFF) {
		/* adc from single ended channels */
		for (i = 0; i < model->channels; i++, ch++) {
			KASSERT(ch < M3K_MAX_SENSORS);
			sc->sc_sensors[ch].units = ENVSYS_SVOLTS_DC;
			sc->sc_sensors[ch].state = ENVSYS_SINVALID;
			snprintf(sc->sc_sensors[ch].desc,
			    sizeof(sc->sc_sensors[ch].desc),
			    "adc single ch%d", i);
			sc->sc_sensors[ch].private = ch;
			sysmon_envsys_sensor_attach(sc->sc_sme,
			    &sc->sc_sensors[ch]);
		}
	}

	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_refresh = mcp3kadc_envsys_refresh;
	sc->sc_sme->sme_cookie = sc;
	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
	}

	/* create a sysctl node for adjusting the ADC's reference voltage */
	rnode = node = NULL;
	sysctl_createv(NULL, 0, NULL, &rnode,
	    CTLFLAG_READWRITE,
	    CTLTYPE_NODE, device_xname(sc->sc_dev), NULL,
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL);

	const int ctlflag = vref_read_only ? CTLFLAG_READONLY
					   : CTLFLAG_READWRITE;
	if (rnode != NULL)
		sysctl_createv(NULL, 0, NULL, &node,
		    ctlflag | CTLFLAG_OWNDESC,
		    CTLTYPE_INT, "vref",
		    SYSCTL_DESCR("ADC reference voltage"),
		    sysctl_mcp3kadc_vref, 0, (void *)sc, 0,
		    CTL_HW, rnode->sysctl_num, CTL_CREATE, CTL_EOL);
}

static void
mcp3kadc_envsys_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct mcp3kadc_softc *sc;
	const struct mcp3kadc_model *model;
	uint8_t buf[2], ctrl;
	int32_t val, scale;

	sc = sme->sme_cookie;
	model = sc->sc_model;
	scale = sc->sc_adc_max + 1;

	if (model->flags & M3K_CTRL_NEEDED) {
		/* we need to send some control bits first */
		ctrl = 1;	/* start bit */

		if (model->flags & M3K_SGLDIFF) {
			/* bit set to select single-ended mode */
			ctrl <<= 1;
			ctrl |= edata->private >= model->channels;
		}

		if (model->flags & M3K_D2D1D0) {
			/* 3 bits select the channel */
			ctrl <<= 3;
			ctrl |= edata->private & (model->channels - 1);
		} else {
			/* 1 bit selects between two channels */
			ctrl <<= 1;
			ctrl |= edata->private & 1;
		}

		if (model->flags & M3K_MSBF) {
			/* bit select MSB first format */
			ctrl <<= 1;
			ctrl |= 1;
		}

		/* send control bits, receive ADC data */
		if (spi_send_recv(sc->sc_sh, 1, &ctrl, 2, buf) != 0) {
			edata->state = ENVSYS_SINVALID;
			return;
		}
	} else {

		/* just read data from the ADC */
		if (spi_recv(sc->sc_sh, 2, buf) != 0) {
			edata->state = ENVSYS_SINVALID;
			return;
		}
	}

	/* extract big-endian ADC data from buffer */
	val = (buf[0] << 8) | buf[1];
	val = (val >> (16 - (model->bits + model->lead))) & sc->sc_adc_max;

	/* sign-extend the result, when needed */
	if (model->flags & M3K_SIGNED) {
		if (val & (1 << (model->bits - 1)))
			val -= sc->sc_adc_max + 1;
		scale >>= 1;	/* MSB is the sign */
	}

	/* scale the value for Vref and convert to mV */
	edata->value_cur = (sc->sc_vref_mv * val / scale) * 1000;
	edata->state = ENVSYS_SVALID;
}

static int
sysctl_mcp3kadc_vref(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct mcp3kadc_softc *sc;
	int32_t t;
	int error;

	node = *rnode;
	sc = node.sysctl_data;

	t = sc->sc_vref_mv;
	node.sysctl_data = &t;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;
	if (t <= 0)
		return EINVAL;

	sc->sc_vref_mv = t;
	return 0;
}
