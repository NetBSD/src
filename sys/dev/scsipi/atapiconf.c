/*	$NetBSD: atapiconf.c,v 1.28.4.1 1999/11/15 00:41:23 fvdl Exp $	*/

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

#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/atapi_all.h>
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/atapiconf.h>

#include "locators.h"

#define SILENT_PRINTF(flags,string) if (!(flags & A_SILENT)) printf string
#define MAX_TARGET 1

struct atapibus_softc {
	struct device sc_dev;
	struct scsipi_link *adapter_link;	/* proto supplied by adapter */
	struct scsipi_link **sc_link;		/* dynamically allocated */
	struct ata_drive_datas *sc_drvs;	/* array supplied by adapter */
};

int	atapibusmatch __P((struct device *, struct cfdata *, void *));
int	atapibussubmatch __P((struct device *, struct cfdata *, void *));
void	atapibusattach __P((struct device *, struct device *, void *));
int	atapibusactivate __P((struct device *, enum devact));
int	atapibusdetach __P((struct device *, int flags));

int	atapiprint __P((void *, const char *));

int	atapi_probe_bus __P((int, int));
void	atapi_probedev __P((struct atapibus_softc *, int ));

struct cfattach atapibus_ca = {
	sizeof(struct atapibus_softc), atapibusmatch, atapibusattach,
	atapibusdetach, atapibusactivate,
};

extern struct cfdriver atapibus_cd;

int atapibusprint __P((void *, const char *));

struct scsi_quirk_inquiry_pattern atapi_quirk_patterns[] = {
	{{T_CDROM, T_REMOV,
	 "ALPS ELECTRIC CO.,LTD. DC544C", "", "SW03D"},	ADEV_NOTUR},
	{{T_CDROM, T_REMOV,
	 "BCD-16X 1997-04-25", "", "VER 2.2"},	SDEV_NOSTARTUNIT},
	{{T_CDROM, T_REMOV,
	 "BCD-24X 1997-06-27", "", "VER 2.0"},	SDEV_NOSTARTUNIT},
	{{T_CDROM, T_REMOV,
	 "CR-2801TE", "", "1.07"},		ADEV_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "CREATIVECD3630E", "", "AC101"},	ADEV_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "FX320S", "", "q01"},			ADEV_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "GCD-R580B", "", "1.00"},		ADEV_LITTLETOC},
	{{T_CDROM, T_REMOV,
	 "MATSHITA CR-574", "", "1.02"},	ADEV_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "MATSHITA CR-574", "", "1.06"},	ADEV_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "Memorex CRW-2642", "", "1.0g"},	ADEV_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "NEC                 CD-ROM DRIVE:273", "", "4.21"}, ADEV_NOTUR},
	{{T_CDROM, T_REMOV,
	 "SANYO CRD-256P", "", "1.02"},		ADEV_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "SANYO CRD-254P", "", "1.02"},		ADEV_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "SANYO CRD-S54P", "", "1.08"},		ADEV_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "CD-ROM  CDR-S1", "", "1.70"},		ADEV_NOCAPACITY}, /* Sanyo */
	{{T_CDROM, T_REMOV,
	 "CD-ROM  CDR-N16", "", "1.25"},	ADEV_NOCAPACITY}, /* Sanyo */
	{{T_CDROM, T_REMOV,
	 "UJDCD8730", "", "1.14"},		ADEV_NODOORLOCK}, /* Acer */
};

int
atapibusmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ata_atapi_attach *aa_link = aux;

	if (aa_link == NULL)
		return (0);
	if (aa_link->aa_type != T_ATAPI)
		return (0);
	if (cf->cf_loc[ATAPICF_CHANNEL] != aa_link->aa_channel &&
	    cf->cf_loc[ATAPICF_CHANNEL] != ATAPICF_CHANNEL_DEFAULT)
	    return 0;
	return (1);
}

int
atapibussubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_link *sc_link = sa->sa_sc_link;

	if (cf->cf_loc[ATAPIBUSCF_DRIVE] != ATAPIBUSCF_DRIVE_DEFAULT &&
	    cf->cf_loc[ATAPIBUSCF_DRIVE] != sc_link->scsipi_atapi.drive)
		return (0);
	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

#if 0
void
atapi_fixquirk(sc_link)
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
	struct atapibus_softc *sc_ab = (struct atapibus_softc *)self;
	struct ata_atapi_attach *aa_link = aux;
	struct scsipi_link *sc_link_proto;
	int nbytes;

	printf("\n");

	/* Initialize shared data. */
	scsipi_init();

	sc_link_proto = malloc(sizeof(struct scsipi_link),
	    M_DEVBUF, M_NOWAIT);
	if (sc_link_proto == NULL)
	    printf("atapibusattach : can't allocate scsipi link proto\n");
	memset(sc_link_proto, 0, sizeof(struct scsipi_link));

	sc_link_proto->type = BUS_ATAPI;
	sc_link_proto->openings = aa_link->aa_openings;
	sc_link_proto->scsipi_atapi.channel = aa_link->aa_channel;
	sc_link_proto->adapter_softc = parent;
	sc_link_proto->adapter = aa_link->aa_bus_private;
	sc_link_proto->scsipi_atapi.atapibus = sc_ab->sc_dev.dv_unit;
	sc_link_proto->scsipi_cmd = atapi_scsipi_cmd;
	sc_link_proto->scsipi_interpret_sense = atapi_interpret_sense;
	sc_link_proto->sc_print_addr = atapi_print_addr;
	sc_link_proto->scsipi_kill_pending = atapi_kill_pending;


	sc_ab->adapter_link = sc_link_proto;
	sc_ab->sc_drvs = aa_link->aa_drv_data;

	nbytes = 2 * sizeof(struct scsipi_link **);
	sc_ab->sc_link = (struct scsipi_link **)malloc(nbytes, M_DEVBUF,
	    M_NOWAIT);
	if (sc_ab->sc_link == NULL)
		panic("scsibusattach: can't allocate target links");
	memset(sc_ab->sc_link, 0, nbytes);
	atapi_probe_bus(sc_ab->sc_dev.dv_unit, -1);
}

int
atapibusactivate(self, act)
	struct device *self;
	enum devact act;
{
	struct atapibus_softc *sc = (struct atapibus_softc *)self;
	struct scsipi_link *sc_link;
	int target, error = 0, s;

	s = splbio();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		for (target = 0; target <= MAX_TARGET; target++) {
			sc_link = sc->sc_link[target];
			if (sc_link == NULL)
				continue;
			error = config_deactivate(sc_link->device_softc);
			if (error != 0)
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
	struct atapibus_softc *sc = (struct atapibus_softc *)self;
	struct scsipi_link *sc_link;
	int target, error;

	for (target = 0; target <= MAX_TARGET; target++) {
		sc_link = sc->sc_link[target];
		if (sc_link == NULL)
			continue;
		error = config_detach(sc_link->device_softc, flags);
		if (error != 0)
			return (error);

		/*
		 * We have successfully detached the child.  Drop the
		 * direct reference for the child so that wdcdetach
		 * won't call detach routine twice.
		 */
#ifdef DIAGNOSTIC
		if (sc_link->device_softc != sc->sc_drvs[target].drv_softc)
			panic("softc mismatch");
#endif
		sc->sc_drvs[target].drv_softc = NULL;

		free(sc_link, M_DEVBUF);
		sc->sc_link[target] = NULL;
	}
	return (0);
}

int
atapi_probe_bus(bus, target)
	int bus, target;
{
	int maxtarget, mintarget;
	struct atapibus_softc *atapi;
	int error;

	if (bus < 0 || bus >= atapibus_cd.cd_ndevs)
		return (ENXIO);
	atapi = atapibus_cd.cd_devs[bus];
	if (atapi == NULL)
		return (ENXIO);

	if (target == -1) {
		maxtarget = 1;
		mintarget = 0;
	} else {
		if (target < 0 || target > 1)
			return (ENXIO);
		maxtarget = mintarget = target;
	}
	if ((error = scsipi_adapter_addref(atapi->adapter_link)) != 0)
		return (error);
	for (target = mintarget; target <= maxtarget; target++)
		atapi_probedev(atapi, target);
	scsipi_adapter_delref(atapi->adapter_link);
	return (0);
}

void
atapi_probedev(atapi, target)
	struct atapibus_softc *atapi;
	int target;
{
	struct scsipi_link *sc_link;
	struct scsipibus_attach_args sa;
	struct ataparams ids;
	struct ataparams *id = &ids;
	struct ata_drive_datas *drvp = &atapi->sc_drvs[target];
	struct cfdata *cf;
	struct scsi_quirk_inquiry_pattern *finger;
	int priority;
	char serial_number[21], model[41], firmware_revision[9];

	/* skip if already attached */
	if (atapi->sc_link[target])
		return;

	if (wdc_atapi_get_params(atapi->adapter_link, target,
	    XS_CTL_POLL|XS_CTL_NOSLEEP, id) == COMPLETE) {
#ifdef ATAPI_DEBUG_PROBE
		printf("%s drive %d: cmdsz 0x%x drqtype 0x%x\n",
		    atapi->sc_dev.dv_xname, target,
		    id->atap_config & ATAPI_CFG_CMD_MASK,
		    id->atap_config & ATAPI_CFG_DRQ_MASK);
#endif
		/*
		 * Allocate a device link and try and attach
		 * a driver to this device.  If we fail, free
		 * the link.
		 */
		sc_link = malloc(sizeof(*sc_link), M_DEVBUF, M_NOWAIT);
		if (sc_link == NULL) {
			printf("%s: can't allocate link for drive %d\n",
			    atapi->sc_dev.dv_xname, target);
			return;
		}
		/* Fill in link. */
		*sc_link = *atapi->adapter_link;
		sc_link->active = 0;
		sc_link->scsipi_atapi.drive = target;
		sc_link->device = NULL;
		TAILQ_INIT(&sc_link->pending_xfers);
#if defined(SCSIDEBUG) && DEBUGTYPE == BUS_ATAPI
		if (DEBUGTARGET == -1 || target == DEBUGTARGET)
			sc_link->flags |= DEBUGLEVEL;
#endif /* SCSIDEBUG */
		if ((id->atap_config & ATAPI_CFG_CMD_MASK) == ATAPI_CFG_CMD_16)
			sc_link->scsipi_atapi.cap |= ACAP_LEN;
		sc_link->scsipi_atapi.cap |=
		    (id->atap_config & ATAPI_CFG_DRQ_MASK);
		sa.sa_sc_link = sc_link;
		sa.sa_inqbuf.type =  ATAPI_CFG_TYPE(id->atap_config);
		sa.sa_inqbuf.removable =
		    id->atap_config & ATAPI_CFG_REMOV ? T_REMOV : T_FIXED;
		if (sa.sa_inqbuf.removable)
			sc_link->flags |= SDEV_REMOVABLE;
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
		if (priority != 0)
			sc_link->quirks |= finger->quirks;

		if ((cf = config_search(atapibussubmatch, &atapi->sc_dev,
		    &sa)) != 0) {
			atapi->sc_link[target] = sc_link;
			drvp->drv_softc = config_attach(&atapi->sc_dev, cf,
			    &sa, atapibusprint);
			wdc_probe_caps(drvp);
			return;
		} else {
			atapibusprint(&sa, atapi->sc_dev.dv_xname);
			printf(" not configured\n");
			free(sc_link, M_DEVBUF);
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
	    sa->sa_sc_link->scsipi_atapi.drive,inqbuf->vendor,
	    inqbuf->product, inqbuf->revision, inqbuf->type, dtype,
	    inqbuf->removable ? "removable" : "fixed");
	return (UNCONF);
}
