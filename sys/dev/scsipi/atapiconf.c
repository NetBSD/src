/*	$NetBSD: atapiconf.c,v 1.1.2.1 1997/07/01 16:52:07 bouyer Exp $	*/

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
 *  This product includes software developed by Manuel Bouyer.
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

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/atapiconf.h>

#define SILENT_PRINTF(flags,string) if (!(flags & A_SILENT)) printf string

struct atapibus_softc {
	struct device sc_dev;
	struct scsipi_link *adapter_link; /* proto supplied by adapter */
	struct scsipi_link **sc_link;     /* dynamically allocated */
};

#ifdef __BROKEN_INDIRECT_CONFIG
int atapibusmatch __P((struct device *, void *, void *));
int atapibussubmatch __P((struct device *, void *, void *));
#else
int atapibusmatch __P((struct device *, struct cfdata *, void *));
int atapibussubmatch __P((struct device *, struct cfdata *, void *));
#endif
void atapibusattach __P((struct device *, struct device *, void *));
int atapiprint __P((void *, const char *));
int atapi_probe_bus __P((int, int));
void atapi_probedev __P((struct atapibus_softc *, int ));
struct cfattach atapibus_ca = {
	sizeof(struct atapibus_softc), atapibusmatch, atapibusattach
};

struct cfdriver atapibus_cd = {
	NULL, "atapibus", DV_DULL
};

int atapibusprint __P((void *, const char *));

struct scsi_quirk_inquiry_pattern atapi_quirk_patterns[] = {
	{{T_CDROM, T_REMOV,
	 "GCD-R580B", "", "1.00"},							ADEV_LITTLETOC},
	{{T_CDROM, T_REMOV,
	 "SANYO CRD-256P", "", "1.02"},						ADEV_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "UJDCD8730", "", "1.14"},							ADEV_NODOORLOCK},
	{{T_CDROM, T_REMOV,
	 "ALPS ELECTRIC CO.,LTD. DC544C", "", "SW03D"},		ADEV_NOTUR},
	{{T_CDROM, T_REMOV,
	 "NEC                 CD-ROM DRIVE:273", "", "4.21"},	ADEV_NOTUR},
};

int
#ifdef __BROKEN_INDIRECT_CONFIG
atapibusmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
#else
atapibusmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
#endif
	struct scsipi_link *sc_link = aux;

	if (sc_link == NULL)
		return 0;
	if (sc_link->type != BUS_ATAPI)
		return 0;
	return 1;
}

int
#ifdef __BROKEN_INDIRECT_CONFIG
atapibussubmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct cfdata *cf = match;
#else
atapibussubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
#endif
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_link *sc_link = sa->sa_sc_link;

	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != sc_link->scsipi_atapi.drive)
		return 0;
	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}
	

#if 0
void
atapi_fixquirk(sc_link)
        struct scsipi_link *ad_link;
{
        struct atapi_identify *id = &ad_link->id;
        struct atapi_quirk_inquiry_pattern *quirk;


        /*
         * Clean up the model name, serial and
         * revision numbers.
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
	struct atapibus_softc *ab = (struct atapibus_softc *)self;
	struct scsipi_link *sc_link_proto = aux;
	int nbytes;

	printf("\n");

	sc_link_proto->scsipi_atapi.atapibus = ab->sc_dev.dv_unit;
	sc_link_proto->scsipi_cmd = atapi_scsipi_cmd;
	sc_link_proto->scsipi_interpret_sense = atapi_interpret_sense;
	sc_link_proto->sc_print_addr = atapi_print_addr;

	ab->adapter_link = sc_link_proto;

	nbytes =  2 * sizeof(struct scsipi_link **);
	ab->sc_link = (struct scsipi_link **)malloc(nbytes, M_DEVBUF,
		   M_NOWAIT);
	if (ab->sc_link == NULL)
		panic("scsibusattach: can't allocate target links");
	bzero(ab->sc_link, nbytes);
	atapi_probe_bus(ab->sc_dev.dv_unit, -1);
}

int 
atapi_probe_bus(bus, target)
int bus, target;
{
	int maxtarget, mintarget;
	struct atapibus_softc *atapi;
	if (bus < 0 || bus >= atapibus_cd.cd_ndevs)
		return ENXIO;
	atapi = atapibus_cd.cd_devs[bus];
	if (!atapi)
		return ENXIO;

	if (target == -1) {
		maxtarget = 1;
		mintarget = 0;
	} else {
		if (target < 0 || target > 1)
			return ENXIO;
		maxtarget = mintarget = target;
	}
	for (target = mintarget; target <= maxtarget; target++) {
		atapi_probedev(atapi, target);
	}
	return 0;
}

void
atapi_probedev(atapi, target)
	struct atapibus_softc *atapi;
	int target;
{
	struct scsipi_link *sc_link;
	struct scsipibus_attach_args sa;
	struct atapi_identify ids;
	struct atapi_identify *id = &ids;
	struct cfdata *cf;
	struct scsi_quirk_inquiry_pattern *finger;
	int priority;
	char serial_number[20], model[40], firmware_revision[8];
	
	/* skip if already attached */
	if (atapi->sc_link[target])
		return;

	if (wdc_atapi_get_params(atapi->adapter_link, target, id)) {
#ifdef ATAPI_DEBUG_PROBE
		printf("%s drive %d: cmdsz 0x%x drqtype 0x%x\n",
		    atapi->sc_dev.dv_xname, target,
		    id->config.cmd_drq_rem & ATAPI_PACKET_SIZE_MASK,
		    id->config.cmd_drq_rem & ATAPI_DRQ_MASK);
#endif
        /*
         * Shuffle string byte order.
         * Mitsumi and NEC drives don't need this.
         */
        if (((id->model[0] == 'N' && id->model[1] == 'E') ||
            (id->model[0] == 'F' && id->model[1] == 'X')) == 0)
                bswap(id->model, sizeof(id->model));
        bswap(id->serial_number, sizeof(id->serial_number));
        bswap(id->firmware_revision, sizeof(id->firmware_revision));

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
		sc_link->scsipi_atapi.drive = target;
		sc_link->device = NULL;
#if defined(SCSIDEBUG) && DEBUGTYPE == BUS_ATAPI
    if (DEBUGTARGET == -1 || target == DEBUGTARGET)
		sc_link->flags |= DEBUGLEVEL;
#endif /* SCSIDEBUG */
		if (id->config.cmd_drq_rem & ATAPI_PACKET_SIZE_16)
			sc_link->scsipi_atapi.cap |= ACAP_LEN;
		sc_link->scsipi_atapi.cap |=
		    (id->config.cmd_drq_rem & ATAPI_DRQ_MASK) << 3;
#if 0
		bcopy(id, &ad_link->id, sizeof(*id));
           /* Fix strings and look through the quirk table. */
           atapi_fixquirk(ad_link, id);
#endif
		sa.sa_sc_link = sc_link;
		sa.sa_inqbuf.type =  id->config.device_type & SID_TYPE;
		sa.sa_inqbuf.removable = id->config.cmd_drq_rem & ATAPI_REMOVABLE ?
			T_REMOV : T_FIXED;
		if (sa.sa_inqbuf.removable)
			sc_link->flags |= SDEV_REMOVABLE;
		scsipi_strvis(model, id->model, 40);
		scsipi_strvis(serial_number, id->serial_number, 20);
		scsipi_strvis(firmware_revision, id->firmware_revision, 8);
		sa.sa_inqbuf.vendor = model;
		sa.sa_inqbuf.product = serial_number;
		sa.sa_inqbuf.revision = firmware_revision;

		finger = (struct scsi_quirk_inquiry_pattern *)scsipi_inqmatch(
			&sa.sa_inqbuf,
			(caddr_t)atapi_quirk_patterns,
			sizeof(atapi_quirk_patterns)/sizeof(atapi_quirk_patterns[0]),
			sizeof(atapi_quirk_patterns[0]), &priority);
		if (priority != 0)
			sc_link->quirks |= finger->quirks;

		if ((cf = config_search(atapibussubmatch, (struct device *)atapi,
			&sa)) != 0) {
			atapi->sc_link[target] = sc_link;
			config_attach((struct device *)atapi, cf, &sa, atapibusprint);
			return;
		} else {
			atapibusprint(&sa, atapi->sc_dev.dv_xname);
			printf(" not configured\n");
			free(sc_link, M_DEVBUF);
			return;
		}

#if 0 /* WAS: */
		/* Try to find a match. */
		if (config_found(self, ad_link, atapiprint) == NULL)
			free(ad_link, M_DEVBUF);
#endif
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
		sa->sa_sc_link->scsipi_atapi.drive,inqbuf->vendor, inqbuf->product,
		inqbuf->revision, inqbuf->type, dtype,
		inqbuf->removable ? "removable" : "fixed");
	return (UNCONF);
}
