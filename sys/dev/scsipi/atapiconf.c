/*	$NetBSD: atapiconf.c,v 1.28.2.4 1999/11/01 22:54:18 thorpej Exp $	*/

/*
 * Copyright (c) 1996 Manuel Bouyer.  All rights reserved.
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
 *	This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/kthread.h>

#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/atapi_all.h>
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

struct atapibus_softc {
	struct device sc_dev;
	struct scsipi_channel *sc_channel;	/* our scsipi_channel */
	struct ata_drive_datas *sc_drvs;	/* array supplied by adapter */
};

int	atapibusmatch __P((struct device *, struct cfdata *, void *));
void	atapibusattach __P((struct device *, struct device *, void *));
int	atapibusactivate __P((struct device *, enum devact));
int	atapibusdetach __P((struct device *, int flags));

int	atapibussubmatch __P((struct device *, struct cfdata *, void *));

int	atapiprint __P((void *, const char *));

int	atapi_probe_bus __P((struct atapibus_softc *, int));
void	atapi_probe_device __P((struct atapibus_softc *, int ));

struct cfattach atapibus_ca = {
	sizeof(struct atapibus_softc), atapibusmatch, atapibusattach,
	atapibusdetach, atapibusactivate,
};

extern struct cfdriver atapibus_cd;

int atapibusprint __P((void *, const char *));

const struct scsipi_bustype atapi_bustype = {
	SCSIPI_BUSTYPE_ATAPI,
	atapi_scsipi_cmd,
	atapi_interpret_sense,
	atapi_print_addr,
	atapi_kill_pending,
};

struct scsi_quirk_inquiry_pattern atapi_quirk_patterns[] = {
	{{T_CDROM, T_REMOV,
	 "ALPS ELECTRIC CO.,LTD. DC544C", "", "SW03D"},	PQUIRK_NOTUR},
	{{T_CDROM, T_REMOV,
	 "BCD-16X 1997-04-25", "", "VER 2.2"},	PQUIRK_NOSTARTUNIT},
	{{T_CDROM, T_REMOV,
	 "BCD-24X 1997-06-27", "", "VER 2.0"},	PQUIRK_NOSTARTUNIT},
	{{T_CDROM, T_REMOV,
	 "CR-2801TE", "", "1.07"},		PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "CREATIVECD3630E", "", "AC101"},	PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "FX320S", "", "q01"},			PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "GCD-R580B", "", "1.00"},		PQUIRK_LITTLETOC},
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
	{{T_CDROM, T_REMOV,
	 "UJDCD8730", "", "1.14"},		PQUIRK_NODOORLOCK}, /* Acer */
};

int
atapibusmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ata_atapi_attach *aa = aux;

	if (aa == NULL)
		return (0);

	if (aa->aa_type != T_ATAPI)
		return (0);

	if (cf->cf_loc[ATAPICF_CHANNEL] != aa->aa_channel &&
	    cf->cf_loc[ATAPICF_CHANNEL] != ATAPICF_CHANNEL_DEFAULT)
		return (0);

	return (1);
}

int
atapibussubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;

	if (cf->cf_loc[ATAPIBUSCF_DRIVE] != ATAPIBUSCF_DRIVE_DEFAULT &&
	    cf->cf_loc[ATAPIBUSCF_DRIVE] != periph->periph_target)
		return (0);
	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

#if 0
void
atapi_fixquirk(periph)
	struct scsipi_link *ad_link;
{
	struct ataparams *id = &ad_link->id;
	struct atapi_quirk_inquiry_pattern *quirk;

	/*
	 * Clean up the model name, serial and revision numbers.
	 */
	btrim(id->model, sizeof(id->model));
	btrim(id->serial_number, sizeof(id->serial_number));
	btrim(id->firmware_revision, sizeof(id->firmware_revision));
}
#endif

void
atapibusattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct atapibus_softc *sc = (void *) self;
	struct ata_atapi_attach *aa = aux;
	struct scsipi_channel *chan = aa->aa_bus_private;

	sc->sc_channel = chan;
	sc->sc_drvs = aa->aa_drv_data;

	/* ATAPI has no LUNs. */
	chan->chan_nluns = 1;
	printf(": %d targets\n", chan->chan_ntargets);

	/* Initialize the channel. */
	scsipi_channel_init(chan);

	/* Probe the bus for devices. */
	atapi_probe_bus(sc, -1);
}

int
atapibusactivate(self, act)
	struct device *self;
	enum devact act;
{
	struct atapibus_softc *sc = (void *) self;
	struct scsipi_channel *chan = sc->sc_channel;
	struct scsipi_periph *periph;
	int target, error = 0, s;

	s = splbio();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		for (target = 0; target < chan->chan_ntargets; target++) {
			periph = chan->chan_periphs[target][0];
			if (periph == NULL)
				continue;
			error = config_deactivate(periph->periph_dev);
			if (error)
				goto out;
		}
		break;
	}
 out:
	splx(s);
	return (error);
}

int
atapibusdetach(self, flags)
	struct device *self;
	int flags;
{
	struct atapibus_softc *sc = (void *)self;
	struct scsipi_channel *chan = sc->sc_channel;
	struct scsipi_periph *periph;
	int target, error;

	for (target = 0; target < chan->chan_ntargets; target++) {
		periph = chan->chan_periphs[target][0];
		if (periph == NULL)
			continue;
		error = config_detach(periph->periph_dev, flags);
		if (error)
			return (error);

		/*
		 * We have successfully detached the child.  Drop the
		 * direct reference for the child so that wdcdetach
		 * won't call detach routine twice.
		 */
#ifdef DIAGNOSTIC
		if (periph->periph_dev != sc->sc_drvs[target].drv_softc)
			panic("softc mismatch");
#endif
		sc->sc_drvs[target].drv_softc = NULL;

		free(periph, M_DEVBUF);
		chan->chan_periphs[target][0] = NULL;
	}
	return (0);
}

int
atapi_probe_bus(sc, target)
	struct atapibus_softc *sc;
	int target;
{
	struct scsipi_channel *chan = sc->sc_channel;
	int maxtarget, mintarget;
	int error;

	if (target == -1) {
		maxtarget = 1;
		mintarget = 0;
	} else {
		if (target < 0 || target >= chan->chan_ntargets)
			return (ENXIO);
		maxtarget = mintarget = target;
	}

	if ((error = scsipi_adapter_addref(chan->chan_adapter)) != 0)
		return (error);
	for (target = mintarget; target <= maxtarget; target++)
		atapi_probe_device(sc, target);
	scsipi_adapter_delref(chan->chan_adapter);
	return (0);
}

void
atapi_probe_device(sc, target)
	struct atapibus_softc *sc;
	int target;
{
	struct scsipi_channel *chan = sc->sc_channel;
	struct scsipi_periph *periph;
	struct ataparams ids;
	struct ataparams *id = &ids;
	struct ata_drive_datas *drvp = &sc->sc_drvs[target];
	struct scsi_quirk_inquiry_pattern *finger;
	struct scsipibus_attach_args sa;
	struct cfdata *cf;
	int priority, quirks;
	char serial_number[21], model[41], firmware_revision[9];

	/* skip if already attached */
	if (chan->chan_periphs[target][0] != NULL)
		return;

	if (wdc_atapi_get_params(chan, target,
	    XS_CTL_POLL|XS_CTL_NOSLEEP, id) == 0) {
#ifdef ATAPI_DEBUG_PROBE
		printf("%s drive %d: cmdsz 0x%x drqtype 0x%x\n",
		    sc->sc_dev.dv_xname, target,
		    id->atap_config & ATAPI_CFG_CMD_MASK,
		    id->atap_config & ATAPI_CFG_DRQ_MASK);
#endif
		periph = malloc(sizeof(*periph), M_DEVBUF, M_NOWAIT);
		if (periph == NULL) {
			printf("%s: unable to allocate periph for drive %d\n",
			    sc->sc_dev.dv_xname, target);
			return;
		}
		memset(periph, 0, sizeof(*periph));

		periph->periph_dev = NULL;
		periph->periph_channel = chan;
		periph->periph_switch = &atapi_probe_periphsw;

		/*
		 * Start with one command opening.  The periph
		 * driver will grow this if it knows it can
		 * take advantage of it.
		 */
		periph->periph_openings = 1;
		periph->periph_active = 0;

		periph->periph_target = target;
		periph->periph_lun = 0;

		TAILQ_INIT(&periph->periph_xferq);

#ifdef SCSIPI_DEBUG
		if (SCSIPI_DEBUG_TYPE == SCSIPI_BUSTYPE_ATAPI &&
		    SCSIPI_DEBUG_TARGET == target)
			periph->periph_dbflags |= SCSIPI_DEBUG_FLAGS;
#endif

		periph->periph_type = ATAPI_CFG_TYPE(id->atap_config);
		if (id->atap_config & ATAPI_CFG_REMOV)
			periph->periph_flags |= PERIPH_REMOVABLE;

		sa.sa_periph = periph;
		sa.sa_inqbuf.type =  ATAPI_CFG_TYPE(id->atap_config);
		sa.sa_inqbuf.removable = id->atap_config & ATAPI_CFG_REMOV ?
		    T_REMOV : T_FIXED;
		scsipi_strvis(model, 40, id->atap_model, 40);
		scsipi_strvis(serial_number, 20, id->atap_serial, 20);
		scsipi_strvis(firmware_revision, 8, id->atap_revision, 8);
		sa.sa_inqbuf.vendor = model;
		sa.sa_inqbuf.product = serial_number;
		sa.sa_inqbuf.revision = firmware_revision;

		finger = (struct scsi_quirk_inquiry_pattern *)scsipi_inqmatch(
		    &sa.sa_inqbuf, (caddr_t)atapi_quirk_patterns,
		    sizeof(atapi_quirk_patterns) /
		        sizeof(atapi_quirk_patterns[0]),
		    sizeof(atapi_quirk_patterns[0]), &priority);

		if (finger != NULL)
			quirks = finger->quirks;
		else
			quirks = 0;

		/*
		 * Determine the operating mode capabilities of the device.
		 */
		if ((id->atap_config & ATAPI_CFG_CMD_MASK) == ATAPI_CFG_CMD_16)
			periph->periph_cap |= PERIPH_CAP_CMD16;
		/* XXX This is gross. */
		periph->periph_cap |= (id->atap_config & ATAPI_CFG_DRQ_MASK);

		/*
		 * Now apply any quirks from the table.
		 */
		periph->periph_quirks |= quirks;

		if ((cf = config_search(atapibussubmatch, &sc->sc_dev,
		    &sa)) != 0) {
			chan->chan_periphs[target][0] = periph;
			/*
			 * XXX Can't assign periph_dev here, because we'll
			 * XXX need it before config_attach() returns.  Must
			 * XXX assign it in periph driver.
			 */
			drvp->drv_softc = config_attach(&sc->sc_dev, cf, &sa,
			    atapibusprint);
			wdc_probe_caps(drvp);
			return;
		} else {
			atapibusprint(&sa, sc->sc_dev.dv_xname);
			printf(" not configured\n");
			free(periph, M_DEVBUF);
			return;
		}
	}
}

int
atapibusprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_inquiry_pattern *inqbuf;
	char *dtype;

	if (pnp != NULL)
		printf("%s", pnp);

	inqbuf = &sa->sa_inqbuf;

	dtype = scsipi_dtype(inqbuf->type & SID_TYPE);
	printf(" drive %d: <%s, %s, %s> type %d %s %s",
	    sa->sa_periph->periph_target ,inqbuf->vendor,
	    inqbuf->product, inqbuf->revision, inqbuf->type, dtype,
	    inqbuf->removable ? "removable" : "fixed");
	return (UNCONF);
}
