/*	$NetBSD: atapiconf.c,v 1.36.2.3 2001/09/21 22:36:11 nathanw Exp $	*/

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

#include <dev/ata/atavar.h>
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

int	atapibusmatch __P((struct device *, struct cfdata *, void *));
void	atapibusattach __P((struct device *, struct device *, void *));
int	atapibusactivate __P((struct device *, enum devact));
int	atapibusdetach __P((struct device *, int flags));

int	atapibussubmatch __P((struct device *, struct cfdata *, void *));

int	atapi_probe_bus __P((struct atapibus_softc *, int));

struct cfattach atapibus_ca = {
	sizeof(struct atapibus_softc), atapibusmatch, atapibusattach,
	atapibusdetach, atapibusactivate,
};

extern struct cfdriver atapibus_cd;

int atapibusprint __P((void *, const char *));

const struct scsi_quirk_inquiry_pattern atapi_quirk_patterns[] = {
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
	{{T_CDROM, T_REMOV,
	 "UJDCD8730", "", "1.14"},		PQUIRK_NODOORLOCK}, /* Acer */
	{{T_DIRECT, T_REMOV,		/* Panasonic MultiMediaCard */
	  "04DA", "1B00", "0010"},		PQUIRK_BYTE5_ZERO |
	 					PQUIRK_NO_FLEX_PAGE },
	{{T_DIRECT, T_REMOV,		/* ZiO! MultiMediaCard */
	  "eUSB", "MultiMediaCard", ""},	PQUIRK_NO_FLEX_PAGE },
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
			periph = scsipi_lookup_periph(chan, target, 0);
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

	/*
	 * Shut down the channel.
	 */
	scsipi_channel_shutdown(chan);

	/*
	 * Now detach all of the periphs.
	 */
	for (target = 0; target < chan->chan_ntargets; target++) {
		periph = scsipi_lookup_periph(chan, target, 0);
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

		scsipi_remove_periph(chan, periph);
		free(periph, M_DEVBUF);
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
	struct atapi_adapter *atapi_adapter;

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
	atapi_adapter = (struct atapi_adapter*)chan->chan_adapter;
	for (target = mintarget; target <= maxtarget; target++)
		atapi_adapter->atapi_probe_device(sc, target);
	scsipi_adapter_delref(chan->chan_adapter);
	return (0);
}

void *
atapi_probe_device(sc, target, periph, sa)
	struct atapibus_softc *sc;
	int target;
	struct scsipi_periph *periph;
	struct scsipibus_attach_args *sa;
{
	struct scsipi_channel *chan = sc->sc_channel;
	struct scsi_quirk_inquiry_pattern *finger;
	struct cfdata *cf;
	int priority, quirks;

	finger = (struct scsi_quirk_inquiry_pattern *)scsipi_inqmatch(
	    &sa->sa_inqbuf, (caddr_t)atapi_quirk_patterns,
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

	if ((cf = config_search(atapibussubmatch, &sc->sc_dev,
	    sa)) != 0) {
		scsipi_insert_periph(chan, periph);
		/*
		 * XXX Can't assign periph_dev here, because we'll
		 * XXX need it before config_attach() returns.  Must
		 * XXX assign it in periph driver.
		 */
		return config_attach(&sc->sc_dev, cf, sa,
		    atapibusprint);
	} else {
		atapibusprint(sa, sc->sc_dev.dv_xname);
		printf(" not configured\n");
		free(periph, M_DEVBUF);
		return NULL;
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
