/*	$NetBSD: scsiconf.c,v 1.147.2.1 2000/08/03 17:17:48 bouyer Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/scsiio.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include "locators.h"

#if 0
#if NCALS > 0
	{ T_PROCESSOR, T_FIXED, 1,
	  0, 0, 0 },
#endif	/* NCALS */
#if NBLL > 0
	{ T_PROCESSOR, T_FIXED, 1,
	  "AEG     ", "READER          ", "V1.0" },
#endif	/* NBLL */
#if NKIL > 0
	{ T_SCANNER, T_FIXED, 0,
	  "KODAK   ", "IL Scanner 900  ", 0 },
#endif	/* NKIL */
#endif

/*
 * Declarations
 */
int scsi_probedev __P((struct scsibus_softc *, int, int));
int scsi_probe_bus __P((int bus, int target, int lun));

struct scsipi_device probe_switch = {
	NULL,
	NULL,
	NULL,
	NULL,
};

int scsibusmatch __P((struct device *, struct cfdata *, void *));
void scsibusattach __P((struct device *, struct device *, void *));
int scsibusactivate __P((struct device *, enum devact));
int scsibusdetach __P((struct device *, int flags));

int scsibussubmatch __P((struct device *, struct cfdata *, void *));

struct cfattach scsibus_ca = {
	sizeof(struct scsibus_softc), scsibusmatch, scsibusattach,
	    scsibusdetach, scsibusactivate,
};

extern struct cfdriver scsibus_cd;

int scsibusprint __P((void *, const char *));
void scsibus_config_interrupts __P((struct device *));

cdev_decl(scsibus);

int
scsibusmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct scsipi_link *l = aux;
	int channel;

	if (l->type != BUS_SCSI)
		return (0);

	/*
	 * Allow single-channel controllers to specify their channel
	 * in a special way, so that it's not printed.
	 */
	channel = (l->scsipi_scsi.channel != SCSI_CHANNEL_ONLY_ONE) ?
	    l->scsipi_scsi.channel : 0;

	if (cf->cf_loc[SCSICF_CHANNEL] != channel &&
	    cf->cf_loc[SCSICF_CHANNEL] != SCSICF_CHANNEL_DEFAULT)
		return (0);

	return (1);
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
	struct scsibus_softc *sb = (struct scsibus_softc *)self;
	struct scsipi_link *sc_link_proto = aux;
	size_t nbytes;
	int i;

	sc_link_proto->scsipi_scsi.scsibus = sb->sc_dev.dv_unit;
	sc_link_proto->scsipi_cmd = scsi_scsipi_cmd;
	sc_link_proto->scsipi_interpret_sense = scsipi_interpret_sense;
	sc_link_proto->sc_print_addr = scsi_print_addr;
	sc_link_proto->scsipi_kill_pending = scsi_kill_pending;

	sb->adapter_link = sc_link_proto;
	sb->sc_maxtarget = sc_link_proto->scsipi_scsi.max_target;
	sb->sc_maxlun = sc_link_proto->scsipi_scsi.max_lun;
	printf(": %d targets, %d luns per target\n",
	    sb->sc_maxtarget + 1, sb->sc_maxlun + 1);

	/* Initialize shared data. */
	scsipi_init();

	nbytes = (sb->sc_maxtarget + 1) * sizeof(struct scsipi_link **);
	sb->sc_link = (struct scsipi_link ***)malloc(nbytes, M_DEVBUF,
	    M_NOWAIT);
	if (sb->sc_link == NULL)
		panic("scsibusattach: can't allocate target links");

	nbytes = (((int) sb->sc_maxlun) + 1) * sizeof(struct scsipi_link *);
	for (i = 0; i <= sb->sc_maxtarget; i++) {
		sb->sc_link[i] = (struct scsipi_link **)malloc(nbytes,
		    M_DEVBUF, M_NOWAIT);
		if (sb->sc_link[i] == NULL)
			panic("scsibusattach: can't allocate lun links");
		bzero(sb->sc_link[i], nbytes);
	}

	/*
	 * Defer configuration of the children until interrupts
	 * are enabled.
	 */
	config_interrupts(self, scsibus_config_interrupts);
}

void
scsibus_config_interrupts(self)
	struct device *self;
{
#ifndef SCSI_DELAY
#define SCSI_DELAY 2
#endif
	if (SCSI_DELAY > 0) {
		printf("%s: waiting %d seconds for devices to settle...\n",
		    self->dv_xname, SCSI_DELAY);
		/* ...an identifier we know no one will use... */
		(void) tsleep(scsibus_config_interrupts, PRIBIO,
		    "scsidly", SCSI_DELAY * hz);
	}

	scsi_probe_bus(self->dv_unit, -1, -1);
}

int
scsibussubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_link *sc_link = sa->sa_sc_link;

	if (cf->cf_loc[SCSIBUSCF_TARGET] != SCSIBUSCF_TARGET_DEFAULT &&
	    cf->cf_loc[SCSIBUSCF_TARGET] != sc_link->scsipi_scsi.target)
		return (0);
	if (cf->cf_loc[SCSIBUSCF_LUN] != SCSIBUSCF_LUN_DEFAULT &&
	    cf->cf_loc[SCSIBUSCF_LUN] != sc_link->scsipi_scsi.lun)
		return (0);
	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
scsibusactivate(self, act)
	struct device *self;
	enum devact act;
{
	struct scsibus_softc *sc = (struct scsibus_softc *) self;
	struct scsipi_link *sc_link;
	int target, lun, error = 0, s;

	s = splbio();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		for (target = 0; target <= sc->sc_maxtarget; target++) {
			if (target ==
			    sc->adapter_link->scsipi_scsi.adapter_target)
				continue;
			for (lun = 0; lun <= sc->sc_maxlun; lun++) {
				sc_link = sc->sc_link[target][lun];
				if (sc_link == NULL)
					continue;
				error =
				    config_deactivate(sc_link->device_softc);
				if (error)
					goto out;
			}
		}
		break;
	}
 out:
	splx(s);
	return (error);
}

int
scsibusdetach(self, flags)
	struct device *self;
	int flags;
{
	struct scsibus_softc *sc = (struct scsibus_softc *) self;
	struct scsipi_link *sc_link;
	int target, lun, error;

	for (target = 0; target <= sc->sc_maxtarget; target++) {
		if (target == sc->adapter_link->scsipi_scsi.adapter_target)
			continue;
		for (lun = 0; lun <= sc->sc_maxlun; lun++) {
			sc_link = sc->sc_link[target][lun];
			if (sc_link == NULL)
				continue;
			error = config_detach(sc_link->device_softc, flags);
			if (error)
				return (error);
			free(sc_link, M_DEVBUF);
			sc->sc_link[target][lun] = NULL;
		}
	}
	return (0);
}

/*
 * Probe the requested scsi bus. It must be already set up.
 * -1 requests all set up scsi busses.
 * target and lun optionally narrow the search if not -1
 */
int
scsi_probe_busses(bus, target, lun)
	int bus, target, lun;
{

	if (bus == -1) {
		for (bus = 0; bus < scsibus_cd.cd_ndevs; bus++)
			if (scsibus_cd.cd_devs[bus])
				scsi_probe_bus(bus, target, lun);
		return (0);
	} else
		return (scsi_probe_bus(bus, target, lun));
}

/*
 * Probe the requested scsi bus. It must be already set up.
 * target and lun optionally narrow the search if not -1
 */
int
scsi_probe_bus(bus, target, lun)
	int bus, target, lun;
{
	struct scsibus_softc *scsi;
	int maxtarget, mintarget, maxlun, minlun;
	u_int8_t scsi_addr;
	int error;

	if (bus < 0 || bus >= scsibus_cd.cd_ndevs)
		return (ENXIO);
	scsi = scsibus_cd.cd_devs[bus];
	if (scsi == NULL)
		return (ENXIO);

	scsi_addr = scsi->adapter_link->scsipi_scsi.adapter_target;

	if (target == -1) {
		maxtarget = scsi->sc_maxtarget;
		mintarget = 0;
	} else {
		if (target < 0 || target > scsi->sc_maxtarget)
			return (EINVAL);
		maxtarget = mintarget = target;
	}

	if (lun == -1) {
		maxlun = scsi->sc_maxlun;
		minlun = 0;
	} else {
		if (lun < 0 || lun > scsi->sc_maxlun)
			return (EINVAL);
		maxlun = minlun = lun;
	}

	if ((error = scsipi_adapter_addref(scsi->adapter_link)) != 0)
		return (error);
	for (target = mintarget; target <= maxtarget; target++) {
		if (target == scsi_addr)
			continue;
		for (lun = minlun; lun <= maxlun; lun++) {
			/*
			 * See if there's a device present, and configure it.
			 */
			if (scsi_probedev(scsi, target, lun) == 0) {
				break;
			}
			/* otherwise something says we should look further */
		}
	}
	scsipi_adapter_delref(scsi->adapter_link);
	return (0);
}

/*
 * Print out autoconfiguration information for a subdevice.
 *
 * This is a slight abuse of 'standard' autoconfiguration semantics,
 * because 'print' functions don't normally print the colon and
 * device information.  However, in this case that's better than
 * either printing redundant information before the attach message,
 * or having the device driver call a special function to print out
 * the standard device information.
 */
int
scsibusprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_inquiry_pattern *inqbuf;
	u_int8_t type;
	char *dtype, *qtype;
	char vendor[33], product[65], revision[17];
	int target, lun;

	if (pnp != NULL)
		printf("%s", pnp);

	inqbuf = &sa->sa_inqbuf;

	target = sa->sa_sc_link->scsipi_scsi.target;
	lun = sa->sa_sc_link->scsipi_scsi.lun;

	type = inqbuf->type & SID_TYPE;

	/*
	 * Figure out basic device type and qualifier.
	 */
	dtype = 0;
	switch (inqbuf->type & SID_QUAL) {
	case SID_QUAL_LU_OK:
		qtype = "";
		break;

	case SID_QUAL_LU_OFFLINE:
		qtype = " offline";
		break;

	case SID_QUAL_RSVD:
	case SID_QUAL_BAD_LU:
		panic("scsibusprint: impossible qualifier");

	default:
		qtype = "";
		dtype = "vendor-unique";
		break;
	}
	if (dtype == 0)
		dtype = scsipi_dtype(type);

	scsipi_strvis(vendor, 33, inqbuf->vendor, 8);
	scsipi_strvis(product, 65, inqbuf->product, 16);
	scsipi_strvis(revision, 17, inqbuf->revision, 4);

	printf(" target %d lun %d: <%s, %s, %s> SCSI%d %d/%s %s%s",
	    target, lun, vendor, product, revision,
	    sa->scsipi_info.scsi_version & SID_ANSII, type, dtype,
	    inqbuf->removable ? "removable" : "fixed", qtype);

	return (UNCONF);
}

struct scsi_quirk_inquiry_pattern scsi_quirk_patterns[] = {
	{{T_CDROM, T_REMOV,
	 "CHINON  ", "CD-ROM CDS-431  ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "Chinon  ", "CD-ROM CDS-525  ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "CHINON  ", "CD-ROM CDS-535  ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "DEC     ", "RRD42   (C) DEC ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "DENON   ", "DRD-25X         ", "V"},    SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "GENERIC ", "CRD-BP2         ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "HP      ", "C4324/C4325     ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "IMS     ", "CDD521/10       ", "2.06"}, SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "MATSHITA", "CD-ROM CR-5XX   ", "1.0b"}, SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "MEDAVIS ", "RENO CD-ROMX2A  ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "MEDIAVIS", "CDR-H93MV       ", "1.3"},  SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "NEC     ", "CD-ROM DRIVE:55 ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "NEC     ", "CD-ROM DRIVE:83 ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "NEC     ", "CD-ROM DRIVE:84 ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "NEC     ", "CD-ROM DRIVE:841", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "PIONEER ", "CD-ROM DR-124X  ", "1.01"}, SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "SONY    ", "CD-ROM CDU-541  ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "SONY    ", "CD-ROM CDU-55S  ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "SONY    ", "CD-ROM CDU-561  ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "SONY    ", "CD-ROM CDU-8003A", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "SONY    ", "CD-ROM CDU-8012 ", ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "TEAC    ", "CD-ROM          ", "1.06"}, SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "TEAC    ", "CD-ROM CD-56S   ", "1.0B"}, SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "TEXEL   ", "CD-ROM          ", "1.06"}, SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "TEXEL   ", "CD-ROM DM-XX24 K", "1.09"}, SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "TEXEL   ", "CD-ROM DM-XX24 K", "1.10"}, SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "TOSHIBA ", "XM-4101TASUNSLCD", "1755"}, SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "ShinaKen", "CD-ROM DM-3x1S", "1.04"}, SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "JVC     ", "R2626",            ""},     SDEV_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "YAMAHA", "CRW8424S",           ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MICROP  ", "1588-15MBSUN0669", ""},     SDEV_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "MICROP  ", "2217-15MQ1091501", ""},     SDEV_NOSYNCCACHE},
	{{T_OPTICAL, T_REMOV,
	 "EPSON   ", "OMD-5010        ", "3.08"}, SDEV_NOLUNS},

	{{T_DIRECT, T_FIXED,
	"TOSHIBA ", "CD-ROM XM-3401TA", "0283"}, SDEV_CDROM|SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	"TOSHIBA ", "CD-ROM DRIVE:XM", "1971"}, SDEV_CDROM|SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "ADAPTEC ", "AEC-4412BD",       "1.2A"}, SDEV_NOMODESENSE},
	{{T_DIRECT, T_FIXED,
	 "DEC     ", "RZ55     (C) DEC", ""},     SDEV_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "EMULEX  ", "MD21/S2     ESDI", "A00"},  SDEV_FORCELUNS|SDEV_AUTOSAVE},
	/* Gives non-media hardware failure in response to start-unit command */
	{{T_DIRECT, T_FIXED,
	 "HITACHI", "DK515C",		"CP16"},  SDEV_NOSTARTUNIT},
	{{T_DIRECT, T_FIXED,
	 "HITACHI", "DK515C",		"CP15"},  SDEV_NOSTARTUNIT},
	{{T_DIRECT, T_FIXED,
	 "HP      ", "C372",             ""},     SDEV_NOTAG},
	{{T_DIRECT, T_FIXED,
	 "IBMRAID ", "0662S",		 ""},     SDEV_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "IBM     ", "0663H",		 ""},     SDEV_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "IBM",	     "0664",		 ""},     SDEV_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "IBM     ", "H3171-S2",	 ""},	  SDEV_NOLUNS|SDEV_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "IBM     ", "KZ-C",		 ""},	  SDEV_AUTOSAVE},
	/* Broken IBM disk */
	{{T_DIRECT, T_FIXED,
	 ""	   , "DFRSS2F",		 ""},	  SDEV_AUTOSAVE},
	{{T_DIRECT, T_REMOV,
	 "MPL     ", "MC-DISK-        ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "XT-3280         ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "XT-4380S        ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "MXT-1240S       ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "XT-4170S        ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "XT-8760S",         ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "LXT-213S        ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "LXT-213S SUN0207", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "LXT-200S        ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MEGADRV ", "EV1000",           ""},     SDEV_NOMODESENSE},
	{{T_DIRECT, T_FIXED,
	 "MST     ", "SnapLink        ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "NEC     ", "D3847           ", "0307"}, SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "QUANTUM ", "ELS85S          ", ""},     SDEV_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "QUANTUM ", "LPS525S         ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "QUANTUM ", "P105S 910-10-94x", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "QUANTUM ", "PD1225S         ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "QUANTUM ", "PD210S   SUN0207", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "RODIME  ", "RO3000S         ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "SEAGATE ", "ST125N          ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "SEAGATE ", "ST157N          ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "SEAGATE ", "ST296           ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "SEAGATE ", "ST296N          ", ""},     SDEV_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "SEAGATE ", "ST19171",          ""},     SDEV_NOMODESENSE},
	{{T_DIRECT, T_FIXED,
	 "SEAGATE ", "ST34501FC       ", ""},     SDEV_NOMODESENSE},
	{{T_DIRECT, T_FIXED,
	 "TOSHIBA ", "MK538FB         ", "6027"}, SDEV_NOLUNS},
	{{T_DIRECT, T_REMOV,
	 "iomega", "jaz 1GB", 		 ""},	  SDEV_NOMODESENSE},
	{{T_DIRECT, T_REMOV,
	 "IOMEGA", "ZIP 100",		 ""},	  SDEV_NOMODESENSE},
	{{T_DIRECT, T_REMOV,
	 "IOMEGA", "ZIP 100",		 "J.03"}, SDEV_NOMODESENSE|SDEV_NOLUNS},
	/* Letting the motor run kills floppy drives and disks quite fast. */
	{{T_DIRECT, T_REMOV,
	 "TEAC", "FC-1",		 ""},	  SDEV_NOSTARTUNIT},

	{{T_DIRECT, T_REMOV,
	 "Y-E DATA", "USB-FDU",		 "3.04"}, SDEV_NOMODESENSE},
	{{T_DIRECT, T_REMOV,
	 "TEAC", "FD-05PUB",		 "1026"}, SDEV_NOMODESENSE},

	/* XXX: QIC-36 tape behind Emulex adapter.  Very broken. */
	{{T_SEQUENTIAL, T_REMOV,
	 "        ", "                ", "    "}, SDEV_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "CALIPER ", "CP150           ", ""},     SDEV_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "EXABYTE ", "EXB-8200        ", ""},     SDEV_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "SONY    ", "GY-10C          ", ""},     SDEV_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "SONY    ", "SDT-2000        ", "2.09"}, SDEV_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "SONY    ", "SDT-5000        ", "3."},   SDEV_NOSYNC|SDEV_NOWIDE},
	{{T_SEQUENTIAL, T_REMOV,
	 "SONY    ", "SDT-5200        ", "3."},   SDEV_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "TANDBERG", " TDC 3600       ", ""},     SDEV_NOLUNS},
	/* Following entry reported as a Tandberg 3600; ref. PR1933 */
	{{T_SEQUENTIAL, T_REMOV,
	 "ARCHIVE ", "VIPER 150  21247", ""},     SDEV_NOLUNS},
	/* Following entry for a Cipher ST150S; ref. PR4171 */
	{{T_SEQUENTIAL, T_REMOV,
	 "ARCHIVE ", "VIPER 1500 21247", "2.2G"}, SDEV_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "ARCHIVE ", "Python 28454-XXX", ""},     SDEV_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "WANGTEK ", "5099ES SCSI",      ""},     SDEV_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "WANGTEK ", "5150ES SCSI",      ""},     SDEV_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "WANGTEK ", "SCSI-36",		 ""},     SDEV_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "WangDAT ", "Model 1300      ", "02.4"}, SDEV_NOSYNC|SDEV_NOWIDE},
	{{T_SEQUENTIAL, T_REMOV,
	 "WangDAT ", "Model 2600      ", "01.7"}, SDEV_NOSYNC|SDEV_NOWIDE},
	{{T_SEQUENTIAL, T_REMOV,
	 "WangDAT ", "Model 3200      ", "02.2"}, SDEV_NOSYNC|SDEV_NOWIDE},
	{{T_SEQUENTIAL, T_REMOV,
	 "TEAC    ", "MT-2ST/N50      ", ""},     SDEV_NOLUNS},

	{{T_SCANNER, T_FIXED,
	 "RICOH   ", "IS60            ", "1R08"}, SDEV_NOLUNS},
	{{T_SCANNER, T_FIXED,
	 "UMAX    ", "Astra 1200S     ", "V2.9"}, SDEV_NOLUNS},
	{{T_SCANNER, T_FIXED,
	 "UMAX    ", "Astra 1220S     ", ""}, SDEV_NOLUNS},
	{{T_SCANNER, T_FIXED,
	 "UMAX    ", "UMAX S-6E       ", "V2.0"}, SDEV_NOLUNS},
	{{T_SCANNER, T_FIXED,
	 "UMAX    ", "UMAX S-12       ", "V2.1"}, SDEV_NOLUNS},
	{{T_SCANNER, T_FIXED,
	 "ULTIMA  ", "A6000C          ", ""}, SDEV_NOLUNS},

	{{T_PROCESSOR, T_FIXED,
	 "LITRONIC", "PCMCIA          ", ""},     SDEV_NOLUNS},

	{{T_CHANGER, T_REMOV,
	 "SONY    ", "CDL1100         ", ""},     SDEV_NOLUNS},

	{{T_ENCLOSURE, T_FIXED,
	 "SUN     ", "SENA            ", ""},     SDEV_NOLUNS},
};

/*
 * given a target and lun, ask the device what
 * it is, and find the correct driver table
 * entry.
 */
int
scsi_probedev(scsi, target, lun)
	struct scsibus_softc *scsi;
	int target, lun;
{
	struct scsipi_link *sc_link;
	struct scsipi_inquiry_data inqbuf;
	struct scsi_quirk_inquiry_pattern *finger;
	int checkdtype, priority, docontinue;
	struct scsipibus_attach_args sa;
	struct cfdata *cf;

	/*
	 * Assume no more luns to search after this one.
	 * If we successfully get Inquiry data and after
	 * merging quirks we find we can probe for more
	 * luns, we will.
	 */
	docontinue = 0;

	/* Skip this slot if it is already attached. */
	if (scsi->sc_link[target][lun] != NULL)
		return (docontinue);

	sc_link = malloc(sizeof(*sc_link), M_DEVBUF, M_NOWAIT);
	*sc_link = *scsi->adapter_link;
	sc_link->active = 0;
	sc_link->scsipi_scsi.target = target;
	sc_link->scsipi_scsi.lun = lun;
	sc_link->device = &probe_switch;
	TAILQ_INIT(&sc_link->pending_xfers);

	/*
	 * Ask the device what it is
	 */
#if defined(SCSIDEBUG) && DEBUGTYPE == BUS_SCSI
	if (target == DEBUGTARGET && lun == DEBUGLUN)
		sc_link->flags |= DEBUGLEVEL;
#endif /* SCSIDEBUG */

	(void) scsipi_test_unit_ready(sc_link,
	    XS_CTL_DISCOVERY | XS_CTL_IGNORE_ILLEGAL_REQUEST |
	    XS_CTL_IGNORE_NOT_READY | XS_CTL_IGNORE_MEDIA_CHANGE);

#ifdef SCSI_2_DEF
	/* some devices need to be told to go to SCSI2 */
	/* However some just explode if you tell them this.. leave it out */
	scsi_change_def(sc_link, XS_CTL_DISCOVERY | XS_CTL_SILENT);
#endif /* SCSI_2_DEF */

	/* Now go ask the device all about itself. */
	bzero(&inqbuf, sizeof(inqbuf));
	if (scsipi_inquire(sc_link, &inqbuf,
	    XS_CTL_DISCOVERY | XS_CTL_DATA_ONSTACK) != 0)
		goto bad;

	{
		u_int8_t *extension = &inqbuf.flags1;
		int len = inqbuf.additional_length;
		while (len < 3)
			extension[len++] = '\0';
		while (len < 3 + 28)
			extension[len++] = ' ';
		while (len < 3 + 28 + 20)
			extension[len++] = '\0';
		while (len < 3 + 28 + 20 + 1)
			extension[len++] = '\0';
		while (len < 3 + 28 + 20 + 1 + 1)
			extension[len++] = '\0';
		while (len < 3 + 28 + 20 + 1 + 1 + (8*2))
			extension[len++] = ' ';
	}

	sa.sa_sc_link = sc_link;
	sa.sa_inqbuf.type = inqbuf.device;
	sa.sa_inqbuf.removable = inqbuf.dev_qual2 & SID_REMOVABLE ?
	    T_REMOV : T_FIXED;
	sa.sa_inqbuf.vendor = inqbuf.vendor;
	sa.sa_inqbuf.product = inqbuf.product;
	sa.sa_inqbuf.revision = inqbuf.revision;
	sa.scsipi_info.scsi_version = inqbuf.version;
	sa.sa_inqptr = &inqbuf;

	finger = (struct scsi_quirk_inquiry_pattern *)scsipi_inqmatch(
	    &sa.sa_inqbuf, (caddr_t)scsi_quirk_patterns,
	    sizeof(scsi_quirk_patterns)/sizeof(scsi_quirk_patterns[0]),
	    sizeof(scsi_quirk_patterns[0]), &priority);

	/*
	 * Based upon the inquiry flags we got back, and if we're
	 * at SCSI-2 or better, set some limiting quirks.
	 */
	if ((inqbuf.version & SID_ANSII) >= 2) {
		if ((inqbuf.flags3 & SID_CmdQue) == 0)
			sc_link->quirks |= SDEV_NOTAG;
		if ((inqbuf.flags3 & SID_Sync) == 0)
			sc_link->quirks |= SDEV_NOSYNC;
		if ((inqbuf.flags3 & SID_WBus16) == 0)
			sc_link->quirks |= SDEV_NOWIDE;
	}
	/*
	 * Now apply any quirks from the table.
	 */
	if (priority != 0)
		sc_link->quirks |= finger->quirks;
	if ((inqbuf.version & SID_ANSII) == 0 &&
	    (sc_link->quirks & SDEV_FORCELUNS) == 0)
		sc_link->quirks |= SDEV_NOLUNS;
	sc_link->scsipi_scsi.scsi_version = inqbuf.version;

	if (sc_link->quirks & SDEV_CDROM) {
		sc_link->quirks ^= SDEV_CDROM;
		inqbuf.dev_qual2 |= SID_REMOVABLE;
		sa.sa_inqbuf.type = inqbuf.device = ((inqbuf.device & ~SID_REMOVABLE) | T_CDROM);
		sa.sa_inqbuf.removable = T_REMOV;
	
	}

	if ((sc_link->quirks & SDEV_NOLUNS) == 0)
		docontinue = 1;

	/*
	 * note what BASIC type of device it is
	 */
	if ((inqbuf.dev_qual2 & SID_REMOVABLE) != 0)
		sc_link->flags |= SDEV_REMOVABLE;

	/*
	 * Any device qualifier that has the top bit set (qualifier&4 != 0)
	 * is vendor specific and won't match in this switch.
	 * All we do here is throw out bad/negative responses.
	 */
	checkdtype = 0;
	switch (inqbuf.device & SID_QUAL) {
	case SID_QUAL_LU_OK:
	case SID_QUAL_LU_OFFLINE:
		checkdtype = 1;
		break;

	case SID_QUAL_RSVD:
	case SID_QUAL_BAD_LU:
		goto bad;

	default:
		break;
	}
	if (checkdtype)
		switch (inqbuf.device & SID_TYPE) {
		case T_DIRECT:
		case T_SEQUENTIAL:
		case T_PRINTER:
		case T_PROCESSOR:
		case T_WORM:
		case T_CDROM:
		case T_SCANNER:
		case T_OPTICAL:
		case T_CHANGER:
		case T_COMM:
		case T_IT8_1:
		case T_IT8_2:
		case T_STORARRAY:
		case T_ENCLOSURE:
		case T_SIMPLE_DIRECT:
		case T_OPTIC_CARD_RW:
		case T_OBJECT_STORED:
		default:
			break;
		case T_NODEVICE:
			goto bad;
		}

	if ((cf = config_search(scsibussubmatch, (struct device *)scsi,
	    &sa)) != NULL) {
		scsi->sc_link[target][lun] = sc_link;
		config_attach((struct device *)scsi, cf, &sa, scsibusprint);
	} else {
		scsibusprint(&sa, scsi->sc_dev.dv_xname);
		printf(" not configured\n");
		goto bad;
	}

	return (docontinue);

bad:
	free(sc_link, M_DEVBUF);
	return (docontinue);
}

/****** Entry points for user control of the SCSI bus. ******/

int
scsibusopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct scsibus_softc *sc;
	int error, unit = minor(dev);

	if (unit >= scsibus_cd.cd_ndevs ||
	    (sc = scsibus_cd.cd_devs[unit]) == NULL)
		return (ENXIO);

	if (sc->sc_flags & SCSIBUSF_OPEN)
		return (EBUSY);

	if ((error = scsipi_adapter_addref(sc->adapter_link)) != 0)
		return (error);

	sc->sc_flags |= SCSIBUSF_OPEN;

	return (0);
}

int
scsibusclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct scsibus_softc *sc = scsibus_cd.cd_devs[minor(dev)];

	scsipi_adapter_delref(sc->adapter_link);

	sc->sc_flags &= ~SCSIBUSF_OPEN;

	return (0);
}

int
scsibusioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct scsibus_softc *sc = scsibus_cd.cd_devs[minor(dev)];
	struct scsipi_link *sc_link = sc->adapter_link;
	int error;

	/*
	 * Enforce write permission for ioctls that change the
	 * state of the bus.  Host adapter specific ioctls must
	 * be checked by the adapter driver.
	 */
	switch (cmd) {
	case SCBUSIOSCAN:
	case SCBUSIORESET:
		if ((flag & FWRITE) == 0)
			return (EBADF);
	}

	switch (cmd) {
	case SCBUSIOSCAN:
	    {
		struct scbusioscan_args *a =
		    (struct scbusioscan_args *)addr;

		/* XXX Change interface to this function. */
		error = scsi_probe_busses(minor(dev), a->sa_target,
		    a->sa_lun);
		break;
	    }

	case SCBUSIORESET:
		/* FALLTHROUGH */
	default:
		if (sc_link->adapter->scsipi_ioctl == NULL)
			error = ENOTTY;
		else
			error = (*sc_link->adapter->scsipi_ioctl)(sc_link,
			    cmd, addr, flag, p);
		break;
	}

	return (error);
}
