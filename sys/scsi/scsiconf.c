/*	$NetBSD: scsiconf.c,v 1.14 1994/06/29 06:43:10 cgd Exp $	*/

/*
 * Copyright (c) 1994 Charles Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles Hannum.
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

/*
 * Originally written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#if defined(i386) && !defined(NEWCONFIG)
#include <i386/isa/isa_device.h>
#endif

#include "st.h"
#include "sd.h"
#include "ch.h"
#include "cd.h"
#include "uk.h"
#include "su.h"

#ifdef TFS
#include "bll.h"
#include "cals.h"
#include "kil.h"
#include "scan.h"
#else /* TFS */
#define	NBLL 0
#define	NCALS 0
#define	NKIL 0
#define	NSCAN 0
#endif /* TFS */

/*
 * The structure of known drivers for autoconfiguration
 */
struct scsidevs {
	char *devname;
	u_int32 type;
	boolean removable;
	int flags;		/* 1 show my comparisons during boot(debug) */
#define SC_SHOWME	0x01
#define	SC_ONE_LU	0x00
#define	SC_MORE_LUS	0x02
	char *manufacturer;
	char *model;
	char *version;
};

#if	NUK > 0
static struct scsidevs unknowndev = {
	"uk", -1, 0, SC_MORE_LUS,
	"standard", "any", "any"
};
#endif 	/*NUK*/

static struct scsidevs knowndevs[] = {
#if NSD > 0
	{ "sd", T_DIRECT, T_FIXED, SC_ONE_LU,
	  "standard", "any", "any" },
	{ "sd", T_DIRECT, T_FIXED, SC_ONE_LU,
	  "MAXTOR  ", "XT-4170S        ", "B5A " },
#endif	/* NSD */
#if NST > 0
	{ "st", T_SEQUENTIAL, T_REMOV, SC_ONE_LU,
	  "standard", "any", "any" },
#endif	/* NST */
#if NCALS > 0
	{ "cals", T_PROCESSOR, T_FIXED, SC_MORE_LUS,
	  "standard", "any", "any" },
#endif	/* NCALS */
#if NCH > 0
	{ "ch", T_CHANGER, T_REMOV, SC_ONE_LU,
	  "standard", "any", "any" },
#endif	/* NCH */
#if NCD > 0
#ifndef UKTEST	/* make cdroms unrecognised to test the uk driver */
	{ "cd", T_READONLY, T_REMOV, SC_ONE_LU,
	  "SONY    ", "CD-ROM CDU-8012 ", "3.1a" },
	{ "cd", T_READONLY, T_REMOV, SC_MORE_LUS,
	  "PIONEER ", "CD-ROM DRM-600  ", "any" },
#endif
#endif	/* NCD */
#if NBLL > 0
	{ "bll", T_PROCESSOR, T_FIXED, SC_MORE_LUS,
	  "AEG     ", "READER          ", "V1.0" },
#endif	/* NBLL */
#if NKIL > 0
	{ "kil", T_SCANNER, T_FIXED, SC_ONE_LU,
	  "KODAK   ", "IL Scanner 900  ", "any" },
#endif	/* NKIL */
	{ 0 }
};

/*
 * Declarations
 */
struct scsidevs *scsi_probedev();
struct scsidevs *selectdev();
int scsi_probe_bus __P((int bus, int targ, int lun));

struct scsi_device probe_switch = {
	NULL,
	NULL,
	NULL,
	NULL,
	"probe",
	0,
};

int scsibusmatch __P((struct device *, struct cfdata *, void *));
void scsibusattach __P((struct device *, struct device *, void *));

struct cfdriver scsibuscd = {
	NULL, "scsibus", scsibusmatch, scsibusattach, DV_DULL,
	sizeof(struct scsibus_data)
};

int
scsibusmatch(parent, cf, aux)
        struct device *parent;
        struct cfdata *cf;
        void *aux;
{

	return 1;
}

/*
 * The routine called by the adapter boards to get all their
 * devices configured in.
 */
void
scsibusattach(parent, self, aux)
        struct device *parent, *self;
        void *aux;
{
	struct scsibus_data *sb = (struct scsibus_data *)self;
	struct scsi_link *sc_link_proto = aux;

	sc_link_proto->scsibus = sb->sc_dev.dv_unit;
	sb->adapter_link = sc_link_proto;
	printf("\n");

#if defined(SCSI_DELAY) && SCSI_DELAY > 2
	printf("%s: waiting for scsi devices to settle\n",
		sb->sc_dev.dv_xname);
#else	/* SCSI_DELAY > 2 */
#undef	SCSI_DELAY
#define SCSI_DELAY 2
#endif	/* SCSI_DELAY */
	delay(1000000 * SCSI_DELAY);

	scsi_probe_bus(sb->sc_dev.dv_unit, -1, -1);
}

/*
 * Probe the requested scsi bus. It must be already set up.
 * -1 requests all set up scsi busses.
 * targ and lun optionally narrow the search if not -1
 */
int
scsi_probe_busses(bus, targ, lun)
	int bus, targ, lun;
{

	if (bus == -1) {
		for (bus = 0; bus < scsibuscd.cd_ndevs; bus++)
			if (scsibuscd.cd_devs[bus])
				scsi_probe_bus(bus, targ, lun);
		return 0;
	} else {
		return scsi_probe_bus(bus, targ, lun);
	}
}

/*
 * Probe the requested scsi bus. It must be already set up.
 * targ and lun optionally narrow the search if not -1
 */
int
scsi_probe_bus(bus, targ, lun)
	int bus, targ, lun;
{
	struct scsibus_data *scsi;
	int	maxtarg, mintarg, maxlun, minlun;
	struct scsi_link *sc_link_proto;
	u_int8  scsi_addr ;
	struct scsidevs *bestmatch = NULL;
	struct scsi_link *sc_link = NULL;
	boolean maybe_more;

	if (bus < 0 || bus >= scsibuscd.cd_ndevs)
		return ENXIO;
	scsi = scsibuscd.cd_devs[bus];
	if (!scsi)
		return ENXIO;

	sc_link_proto = scsi->adapter_link;
	scsi_addr = sc_link_proto->adapter_targ;

	if (targ == -1) {
		maxtarg = 7;
		mintarg = 0;
	} else {
		if (targ < 0 || targ > 7)
			return EINVAL;
		maxtarg = mintarg = targ;
	}

	if (lun == -1) {
		maxlun = 7;
		minlun = 0;
	} else {
		if (lun < 0 || lun > 7)
			return EINVAL;
		maxlun = minlun = lun;
	}

	for (targ = mintarg; targ <= maxtarg; targ++) {
		maybe_more = 0;	/* by default only check 1 lun */
#if 0 /* XXXX */
		if (targ == scsi_addr)
			continue;
#endif
		for (lun = minlun; lun <= maxlun; lun++) {
			/*
			 * The spot appears to already have something
			 * linked in, skip past it. Must be doing a 'reprobe'
			 */
			if (scsi->sc_link[targ][lun]) {
				/* don't do this one, but check other luns */
				maybe_more = 1;
				continue;
			}
			/*
			 * If we presently don't have a link block
			 * then allocate one to use while probing
			 */
			if (!sc_link) {
				sc_link = malloc(sizeof(*sc_link), M_TEMP, M_NOWAIT);
				*sc_link = *sc_link_proto;	/* struct copy */
				sc_link->opennings = 1;
				sc_link->device = &probe_switch;
			}
			sc_link->target = targ;
			sc_link->lun = lun;
			bestmatch = scsi_probedev(sc_link, &maybe_more);
			/*
			 * We already know what the device is.  We use a
			 * special matching routine which insists that the
			 * cfdata is of the right type rather than putting
			 * more intelligence in individual match routines for
			 * each high-level driver.  We must have
			 * scsi_targmatch() do all of the comparisons, or we
			 * could get stuck in an infinite loop trying the same
			 * device repeatedly.  We use the `fordriver' field of
			 * the scsi_link for now, rather than inventing a new
			 * structure just for the config_search().
			 */
			if (bestmatch) {
				sc_link->fordriver = bestmatch->devname;
				if (config_found((struct device *)scsi,
						 sc_link, NULL)) {
					scsi->sc_link[targ][lun] = sc_link;
					sc_link = NULL;	/* it's been used */
				} else
					printf("No matching config entry.\n");
			}
			if (!maybe_more)/* nothing suggests we'll find more */
				break;	/* nothing here, skip to next targ */
			/* otherwise something says we should look further */
		}
	}
	if (sc_link)
		free(sc_link, M_TEMP);
	return 0;
}

int
#ifndef i386
scsi_targmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
#else
scsi_targmatch(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cfdata *cf = self->dv_cfdata;
#endif
	struct scsi_link *sc_link = aux;
	char *devname = sc_link->fordriver;
#if !defined(i386) || defined(NEWCONFIG)

#define	cf_target cf_loc[0]
#define	cf_lun cf_loc[1]
	if (cf->cf_target != -1 && cf->cf_target != sc_link->target)
		return 0;
	if (cf->cf_lun != -1 && cf->cf_lun != sc_link->lun)
		return 0;
#undef cf_target
#undef cf_lun
#else
	struct isa_device *id = (void *)cf->cf_loc;

	if (id->id_physid != -1 &&
	    id->id_physid != ((sc_link->target << 3) | sc_link->lun))
		return 0;
#endif
	if (strcmp(cf->cf_driver->cd_name, devname))
		return 0;

	return 1;
}

/*
 * given a target and lu, ask the device what
 * it is, and find the correct driver table
 * entry.
 */
struct scsidevs *
scsi_probedev(sc_link, maybe_more)
	boolean *maybe_more;
	struct scsi_link *sc_link;
{
	u_int8  target = sc_link->target;
	u_int8  lun = sc_link->lun;
	struct scsi_adapter *scsi_adapter = sc_link->adapter;
	struct scsidevs *bestmatch = NULL;
	char   *dtype = NULL, *desc;
	char   *qtype;
	static struct scsi_inquiry_data inqbuf;
	u_int32 len, qualifier, type;
	boolean remov;
	char    manu[32];
	char    model[32];
	char    version[32];

	bzero(&inqbuf, sizeof(inqbuf));
	/*
	 * Ask the device what it is
	 */
#ifdef	SCSIDEBUG
	if (target == DEBUGTARG && lun == DEBUGLUN)
		sc_link->flags |= DEBUGLEVEL;
	else
		sc_link->flags &= ~(SDEV_DB1 | SDEV_DB2 | SDEV_DB3 | SDEV_DB4);
#endif	/* SCSIDEBUG */
	/* catch unit attn */
	scsi_test_unit_ready(sc_link, SCSI_NOSLEEP | SCSI_NOMASK | SCSI_SILENT);
#ifdef	DOUBTFULL
	switch (scsi_test_unit_ready(sc_link, SCSI_NOSLEEP | SCSI_NOMASK | SCSI_SILENT)) {
	case 0:		/* said it WAS ready */
	case EBUSY:		/* replied 'NOT READY' but WAS present, continue */
	case ENXIO:
		break;
	case EIO:		/* device timed out */
	case EINVAL:		/* Lun not supported */
	default:
		return NULL;

	}
#endif	/*DOUBTFULL*/
#ifdef	SCSI_2_DEF
	/* some devices need to be told to go to SCSI2 */
	/* However some just explode if you tell them this.. leave it out */
	scsi_change_def(sc_link, SCSI_NOSLEEP | SCSI_NOMASK | SCSI_SILENT);
#endif /*SCSI_2_DEF */

	/* Now go ask the device all about itself */
	if (scsi_inquire(sc_link, &inqbuf, SCSI_NOSLEEP | SCSI_NOMASK) != 0)
		return NULL;

	/*
	 * note what BASIC type of device it is
	 */
	type = inqbuf.device & SID_TYPE;
	qualifier = inqbuf.device & SID_QUAL;
	remov = inqbuf.dev_qual2 & SID_REMOVABLE;

	/*
	 * Any device qualifier that has the top bit set (qualifier&4 != 0)
	 * is vendor specific and won't match in this switch.
	 */
	switch (qualifier) {
	case SID_QUAL_LU_OK:
		qtype = "";
		break;

	case SID_QUAL_LU_OFFLINE:
		qtype = ", Unit not Connected!";
		break;

	case SID_QUAL_RSVD:
		qtype = ", Reserved Peripheral Qualifier!";
		*maybe_more = 1;
		return NULL;
		break;

	case SID_QUAL_BAD_LU:
		/*
		 * Check for a non-existent unit.  If the device is returning
		 * this much, then we must set the flag that has
		 * the searchers keep looking on other luns.
		 */
		qtype = ", The Target can't support this Unit!";
		*maybe_more = 1;
		return NULL;

	default:
		dtype = "vendor specific";
		qtype = "";
		*maybe_more = 1;
		break;
	}
	if (dtype == 0) {
		switch (type) {
		case T_DIRECT:
			dtype = "direct";
			break;
		case T_SEQUENTIAL:
			dtype = "sequential";
			break;
		case T_PRINTER:
			dtype = "printer";
			break;
		case T_PROCESSOR:
			dtype = "processor";
			break;
		case T_READONLY:
			dtype = "readonly";
			break;
		case T_WORM:
			dtype = "worm";
			break;
		case T_SCANNER:
			dtype = "scanner";
			break;
		case T_OPTICAL:
			dtype = "optical";
			break;
		case T_CHANGER:
			dtype = "changer";
			break;
		case T_COMM:
			dtype = "communication";
			break;
		case T_NODEVICE:
			*maybe_more = 1;
			return NULL;
		default:
			dtype = NULL;
			break;
		}
	}

	/*
	 * Then if it's advanced enough, more detailed
	 * information
	 */
	if ((inqbuf.version & SID_ANSII) > 0) {
		if ((len = inqbuf.additional_length
			+ ((char *) inqbuf.unused
			    - (char *) &inqbuf))
		    > (sizeof(struct scsi_inquiry_data) - 1))
			        len = sizeof(struct scsi_inquiry_data) - 1;
		desc = inqbuf.vendor;
		desc[len - (desc - (char *) &inqbuf)] = 0;
		strncpy(manu, inqbuf.vendor, 8);
		manu[8] = 0;
		strncpy(model, inqbuf.product, 16);
		model[16] = 0;
		strncpy(version, inqbuf.revision, 4);
		version[4] = 0;
	} else
		/*
		 * If not advanced enough, use default values
		 */
	{
		desc = "early protocol device";
		strncpy(manu, "unknown", 8);
		strncpy(model, "unknown", 16);
		strncpy(version, "????", 4);
	}
	printf("%s targ %d lun %d: <%s%s%s> SCSI%d ",
		((struct device *)sc_link->adapter_softc)->dv_xname,
		target, lun, manu, model, version,
		inqbuf.version & SID_ANSII);
	if (dtype)
		printf("%s", dtype);
	else
		printf("type %d", type);
	printf(" %s\n", remov ? "removable" : "fixed");
	if (qtype[0])
		printf("%s targ %d lun %d: qualifier %d(%s)\n",
			((struct device *)sc_link->adapter_softc)->dv_xname,
			target, lun, qualifier, qtype);

	/*
	 * Try make as good a match as possible with
	 * available sub drivers       
	 */
	bestmatch = selectdev(qualifier, type, remov ? T_REMOV : T_FIXED,
			manu, model, version);
	if (bestmatch && bestmatch->flags & SC_MORE_LUS)
		*maybe_more = 1;
	return bestmatch;
}

/*
 * Try make as good a match as possible with
 * available sub drivers       
 */
struct scsidevs *
selectdev(qualifier, type, remov, manu, model, rev)
	u_int32 qualifier, type;
	boolean remov;
	char   *manu, *model, *rev;
{
	u_int32 numents = (sizeof(knowndevs) / sizeof(struct scsidevs)) - 1;
	u_int32 count = 0;
	u_int32 bestmatches = 0;
	struct scsidevs *bestmatch = (struct scsidevs *) 0;
	struct scsidevs *thisentry = knowndevs;

	type |= qualifier;	/* why? */

	thisentry--;
	while (count++ < numents) {
		thisentry++;
		if (type != thisentry->type)
			continue;
		if (bestmatches < 1) {
			bestmatches = 1;
			bestmatch = thisentry;
		}
		if (remov != thisentry->removable)
			continue;
		if (bestmatches < 2) {
			bestmatches = 2;
			bestmatch = thisentry;
		}
		if (thisentry->flags & SC_SHOWME)
			printf("\n%s-\n%s-", thisentry->manufacturer, manu);
		if (strcmp(thisentry->manufacturer, manu))
			continue;
		if (bestmatches < 3) {
			bestmatches = 3;
			bestmatch = thisentry;
		}
		if (thisentry->flags & SC_SHOWME)
			printf("\n%s-\n%s-", thisentry->model, model);
		if (strcmp(thisentry->model, model))
			continue;
		if (bestmatches < 4) {
			bestmatches = 4;
			bestmatch = thisentry;
		}
		if (thisentry->flags & SC_SHOWME)
			printf("\n%s-\n%s-", thisentry->version, rev);
		if (strcmp(thisentry->version, rev))
			continue;
		if (bestmatches < 5) {
			bestmatches = 5;
			bestmatch = thisentry;
			break;
		}
	}
#if NUK > 0
	if (!bestmatch)
		bestmatch = &unknowndev;
#endif
	if (!bestmatch)
		printf("No matching driver.\n");
	return bestmatch;
}
