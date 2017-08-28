/*	$NetBSD: atapiconf.c,v 1.87.6.3 2017/08/28 17:52:26 skrll Exp $	*/

/*
 * Copyright (c) 1996, 2001 Manuel Bouyer.  All rights reserved.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: atapiconf.c,v 1.87.6.3 2017/08/28 17:52:26 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/atomic.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/atapiconf.h>

#include "locators.h"

#define SILENT_PRINTF(flags,string) if (!(flags & A_SILENT)) printf string
#define MAX_TARGET 1

const struct scsipi_periphsw atapi_probe_periphsw = {
	NULL,
	NULL,
	NULL,
	NULL,
};

static int	atapibusmatch(device_t, cfdata_t, void *);
static void	atapibusattach(device_t, device_t, void *);
static int	atapibusdetach(device_t, int flags);
static void	atapibuschilddet(device_t, device_t);

static int	atapibussubmatch(device_t, cfdata_t, const int *, void *);

static int	atapi_probe_bus(struct atapibus_softc *, int);

static int	atapibusprint(void *, const char *);

CFATTACH_DECL3_NEW(atapibus, sizeof(struct atapibus_softc),
    atapibusmatch, atapibusattach, atapibusdetach, NULL, NULL,
    atapibuschilddet, DVF_DETACH_SHUTDOWN);

extern struct cfdriver atapibus_cd;

static const struct scsi_quirk_inquiry_pattern atapi_quirk_patterns[] = {
	{{T_CDROM, T_REMOV,
	 "ALPS ELECTRIC CO.,LTD. DC544C", "", "SW03D"},	PQUIRK_NOTUR},
	{{T_CDROM, T_REMOV,
	 "CR-2801TE", "", "1.07"},		PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "CREATIVECD3630E", "", "AC101"},	PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "FX320S", "", "q01"},			PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "GCD-R580B", "", "1.00"},		PQUIRK_LITTLETOC},
	{{T_CDROM, T_REMOV,
	 "HITACHI CDR-7730", "", "0008a"},      PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "MATSHITA CR-574", "", "1.02"},	PQUIRK_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "MATSHITA CR-574", "", "1.06"},	PQUIRK_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "Memorex CRW-2642", "", "1.0g"},	PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "NEC                 CD-ROM DRIVE:273", "", "4.21"}, PQUIRK_NOTUR},
	{{T_CDROM, T_REMOV,
	 "SANYO CRD-256P", "", "1.02"},		PQUIRK_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "SANYO CRD-254P", "", "1.02"},		PQUIRK_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "SANYO CRD-S54P", "", "1.08"},		PQUIRK_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "CD-ROM  CDR-S1", "", "1.70"},		PQUIRK_NOCAPACITY}, /* Sanyo */
	{{T_CDROM, T_REMOV,
	 "CD-ROM  CDR-N16", "", "1.25"},	PQUIRK_NOCAPACITY}, /* Sanyo */
};

int
atapiprint(void *aux, const char *pnp)
{
	if (pnp)
		aprint_normal("atapibus at %s", pnp);
	return (UNCONF);
}

static int
atapibusmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct scsipi_channel *chan = aux;

	if (chan == NULL)
		return (0);

	if (SCSIPI_BUSTYPE_TYPE(chan->chan_bustype->bustype_type) !=
	    SCSIPI_BUSTYPE_ATAPI)
		return (0);

	return (1);
}

static int
atapibussubmatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;

	if (cf->cf_loc[ATAPIBUSCF_DRIVE] != ATAPIBUSCF_DRIVE_DEFAULT &&
	    cf->cf_loc[ATAPIBUSCF_DRIVE] != periph->periph_target)
		return (0);
	return (config_match(parent, cf, aux));
}

static void
atapibusattach(device_t parent, device_t self, void *aux)
{
	struct atapibus_softc *sc = device_private(self);
	struct scsipi_channel *chan = aux;

	sc->sc_channel = chan;
	sc->sc_dev = self;

	chan->chan_name = device_xname(sc->sc_dev);
	chan->chan_id = -1;

	/* ATAPI has no LUNs. */
	chan->chan_nluns = 1;
	aprint_naive("\n");
	aprint_normal(": %d targets\n", chan->chan_ntargets);

	if (atomic_inc_uint_nv(&chan_running(chan)) == 1)
		mutex_init(chan_mtx(chan), MUTEX_DEFAULT, IPL_BIO);

	cv_init(&chan->chan_cv_thr, "scshut");
	cv_init(&chan->chan_cv_comp, "sccomp");
	cv_init(&chan->chan_cv_xs, "xscmd");

	/* Initialize the channel. */
	chan->chan_init_cb = NULL;
	chan->chan_init_cb_arg = NULL;
	scsipi_channel_init(chan);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/* Probe the bus for devices. */
	atapi_probe_bus(sc, -1);
}

static void
atapibuschilddet(device_t self, device_t child)
{
	struct atapibus_softc *sc = device_private(self);
	struct scsipi_channel *chan = sc->sc_channel;
	struct scsipi_periph *periph;
	int target;

	mutex_enter(chan_mtx(chan));
	for (target = 0; target < chan->chan_ntargets; target++) {
		periph = scsipi_lookup_periph_locked(chan, target, 0);
		if (periph == NULL || periph->periph_dev != child)
			continue;
		scsipi_remove_periph(chan, periph);
		scsipi_free_periph(periph);
		break;
	}
	mutex_exit(chan_mtx(chan));
}

/* same as scsibusdetach */
static int
atapibusdetach(device_t self, int flags)
{
	struct atapibus_softc *sc = device_private(self);
	struct scsipi_channel *chan = sc->sc_channel;
	int error = 0;

	/*
	 * Detach all of the periphs.
	 */
	error = scsipi_target_detach(chan, -1, -1, flags);
	if (error)
		return error;

	pmf_device_deregister(self);

	/*
	 * Shut down the channel.
	 */
	scsipi_channel_shutdown(chan);

	cv_destroy(&chan->chan_cv_xs);
	cv_destroy(&chan->chan_cv_comp);
	cv_destroy(&chan->chan_cv_thr);

	if (atomic_dec_uint_nv(&chan_running(chan)) == 0)
		mutex_destroy(chan_mtx(chan));

	return 0;
}

static int
atapi_probe_bus(struct atapibus_softc *sc, int target)
{
	struct scsipi_channel *chan = sc->sc_channel;
	int maxtarget, mintarget;
	int error;
	struct atapi_adapter *atapi_adapter;

	KASSERT(chan->chan_ntargets >= 1);

	if (target == -1) {
		maxtarget = chan->chan_ntargets - 1;
		mintarget = 0;
	} else {
		if (target < 0 || target >= chan->chan_ntargets)
			return (ENXIO);
		maxtarget = mintarget = target;
	}

	if ((error = scsipi_adapter_addref(chan->chan_adapter)) != 0)
		return (error);
	atapi_adapter = (struct atapi_adapter*)chan->chan_adapter;
	for (target = mintarget; target <= maxtarget; target++)
		atapi_adapter->atapi_probe_device(sc, target);
	scsipi_adapter_delref(chan->chan_adapter);
	return (0);
}

void *
atapi_probe_device(struct atapibus_softc *sc, int target,
    struct scsipi_periph *periph, struct scsipibus_attach_args *sa)
{
	struct scsipi_channel *chan = sc->sc_channel;
	const struct scsi_quirk_inquiry_pattern *finger;
	cfdata_t cf;
	int priority, quirks;

	finger = scsipi_inqmatch(
	    &sa->sa_inqbuf, (const void *)atapi_quirk_patterns,
	    sizeof(atapi_quirk_patterns) /
	        sizeof(atapi_quirk_patterns[0]),
	    sizeof(atapi_quirk_patterns[0]), &priority);

	if (finger != NULL)
		quirks = finger->quirks;
	else
		quirks = 0;

	/*
	 * Now apply any quirks from the table.
	 */
	periph->periph_quirks |= quirks;

	if ((cf = config_search_ia(atapibussubmatch, sc->sc_dev,
	    "atapibus", sa)) != 0) {
		scsipi_insert_periph(chan, periph);
		/*
		 * XXX Can't assign periph_dev here, because we'll
		 * XXX need it before config_attach() returns.  Must
		 * XXX assign it in periph driver.
		 */
		return config_attach(sc->sc_dev, cf, sa,
		    atapibusprint);
	} else {
		atapibusprint(sa, device_xname(sc->sc_dev));
		aprint_normal(" not configured\n");
		scsipi_free_periph(periph);
		return NULL;
	}
}

static int
atapibusprint(void *aux, const char *pnp)
{
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_inquiry_pattern *inqbuf;
	const char *dtype;

	if (pnp != NULL)
		aprint_normal("%s", pnp);

	inqbuf = &sa->sa_inqbuf;

	dtype = scsipi_dtype(inqbuf->type & SID_TYPE);
	aprint_normal(" drive %d: <%s, %s, %s> %s %s",
	    sa->sa_periph->periph_target, inqbuf->vendor,
	    inqbuf->product, inqbuf->revision, dtype,
	    inqbuf->removable ? "removable" : "fixed");
	return (UNCONF);
}
