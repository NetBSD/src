/*	$NetBSD: mvsocpmu.c,v 1.1.18.2 2017/12/03 11:35:54 jdolecek Exp $	*/
/*
 * Copyright (c) 2016 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: mvsocpmu.c,v 1.1.18.2 2017/12/03 11:35:54 jdolecek Exp $");

#include "opt_mvsoc.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <dev/sysmon/sysmonvar.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/mvsocpmuvar.h>

#include <dev/marvell/marvellreg.h>
#include <dev/marvell/marvellvar.h>

#define UC2UK(uc)	((uc) + 273150000)
#define UK2UC(uk)	((uk) - 273150000)

#define MVSOCPMU_TM_CSR		0x0	/* Control and Status Register */
#define   TM_CSR_TMDIS		  (1 << 0)	/* Thermal Manager Disable */
#define   TM_CSR_THERMTEMPOUT(v)  (((v) >>  1) & 0x1ff)/* Current Temperature */
#define   TM_CSR_THR(oh, c)	  ((((oh) & 0x1ff) << 19)|(((c) & 0x1ff) << 10))
#define   TM_CSR_COOLTHR_MASK	  (0x1ff << 10)
#define   TM_CSR_OVERHEATTHR_MASK (0x1ff << 19)
#define   TM_CSR_COOLTHR(v)	  (((v) >> 10) & 0x1ff) /* Cooling Threshold */
#define   TM_CSR_OVERHEATTHR(v)   (((v) >> 19) & 0x1ff)/* Over Heat Threshold */
#define MVSOCPMU_TM_CDR		0x4	/* Cooling Delay Register */
#define MVSOCPMU_TM_ODR		0x8	/* Overheat Delay Register */

#define MVSOCPMU_TM_READ(sc, r)		\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_tmh, _TM_REG(r))
#define MVSOCPMU_TM_WRITE(sc, r, v)	\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_tmh, _TM_REG(r), (v))
#define _TM_REG(r)	MVSOCPMU_TM_ ## r

static void mvsocpmu_tm_init(struct mvsocpmu_softc *);
static void mvsocpmu_tm_refresh(struct sysmon_envsys *, envsys_data_t *);
static void mvsocpmu_tm_get_limits(struct sysmon_envsys *, envsys_data_t *,
				   sysmon_envsys_lim_t *, uint32_t *);
static void mvsocpmu_tm_set_limits(struct sysmon_envsys *, envsys_data_t *,
				   sysmon_envsys_lim_t *, uint32_t *);

/* ARGSUSED */
int
mvsocpmu_match(device_t parent, struct cfdata *match, void *aux)
{
	struct marvell_attach_args *mva = aux;

	if (strcmp(mva->mva_name, match->cf_name) != 0)
		return 0;
	return 1;
}

/* ARGSUSED */
void
mvsocpmu_attach(device_t parent, device_t self, void *aux)
{
	struct mvsocpmu_softc *sc = device_private(self);

	aprint_naive("\n");
	aprint_normal(": Marvell SoC Power Management Unit\n");

	sc->sc_dev = self;

	if (sc->sc_val2uc != NULL && sc->sc_uc2val != NULL)
		mvsocpmu_tm_init(sc);
}

static void
mvsocpmu_tm_init(struct mvsocpmu_softc *sc)
{
	uint32_t csr;

	/* set default thresholds */
	csr = MVSOCPMU_TM_READ(sc, CSR);
	sc->sc_deflims.sel_warnmin = UC2UK(sc->sc_val2uc(TM_CSR_COOLTHR(csr)));
	sc->sc_deflims.sel_warnmax =
	    UC2UK(sc->sc_val2uc(TM_CSR_OVERHEATTHR(csr)));
	sc->sc_defprops = PROP_WARNMIN | PROP_WARNMAX;

	sc->sc_sme = sysmon_envsys_create();
	/* Initialize sensor data. */
	sc->sc_sensor.units = ENVSYS_STEMP;
	sc->sc_sensor.state = ENVSYS_SINVALID;
	sc->sc_sensor.flags = ENVSYS_FMONLIMITS;
	strlcpy(sc->sc_sensor.desc, device_xname(sc->sc_dev),
	    sizeof(sc->sc_sensor.desc));
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor)) {
		aprint_error_dev(sc->sc_dev, "Unable to attach sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	/* Hook into system monitor. */
	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = mvsocpmu_tm_refresh;
	sc->sc_sme->sme_get_limits = mvsocpmu_tm_get_limits;
	sc->sc_sme->sme_set_limits = mvsocpmu_tm_set_limits;
	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(sc->sc_dev,
		    "Unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

static void
mvsocpmu_tm_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct mvsocpmu_softc *sc = sme->sme_cookie;
	uint32_t csr, uc, uk;

	csr = MVSOCPMU_TM_READ(sc, CSR);
	if (csr & TM_CSR_TMDIS) {
		sc->sc_sensor.state = ENVSYS_SINVALID;
		return;
	}
	uc = sc->sc_val2uc(TM_CSR_THERMTEMPOUT(csr));	/* uC */
	uk = UC2UK(uc);					/* convert to uKelvin */
	sc->sc_sensor.value_cur = uk;
	sc->sc_sensor.state = ENVSYS_SVALID;
}

static void
mvsocpmu_tm_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
		       sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct mvsocpmu_softc *sc = sme->sme_cookie;
	uint32_t csr;

	csr = MVSOCPMU_TM_READ(sc, CSR);
	limits->sel_warnmin = UC2UK(sc->sc_val2uc(TM_CSR_COOLTHR(csr)));
	limits->sel_warnmax = UC2UK(sc->sc_val2uc(TM_CSR_OVERHEATTHR(csr)));
	*props = (PROP_WARNMIN | PROP_WARNMAX | PROP_DRIVER_LIMITS);
}

static void
mvsocpmu_tm_set_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
		       sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct mvsocpmu_softc *sc = sme->sme_cookie;
	uint32_t csr, mask;
	int oh, c;

	if (limits == NULL) {
		limits = &sc->sc_deflims;
		props  = &sc->sc_defprops;
	}
	oh = c = 0;
	mask = 0x0;
	if (*props & PROP_WARNMIN) {
		c = sc->sc_uc2val(UK2UC(limits->sel_warnmin));
		mask |= TM_CSR_COOLTHR_MASK;
	}
	if (*props & PROP_WARNMAX) {
		oh = sc->sc_uc2val(UK2UC(limits->sel_warnmax));
		mask |= TM_CSR_OVERHEATTHR_MASK;
	}
	if (mask != 0) {
		csr = MVSOCPMU_TM_READ(sc, CSR);
		csr &= ~mask;
		MVSOCPMU_TM_WRITE(sc, CSR, csr | TM_CSR_THR(oh, c));
	}

	/*
	 * If at least one limit is set that we can handle, and no
	 * limits are set that we cannot handle, tell sysmon that
	 * the driver will take care of monitoring the limits!
	 */
	if (*props & (PROP_WARNMIN | PROP_WARNMAX))
		*props |= PROP_DRIVER_LIMITS;
	else
		*props &= ~PROP_DRIVER_LIMITS;
}
