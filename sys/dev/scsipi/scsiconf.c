/*	$NetBSD: scsiconf.c,v 1.159 2001/07/18 20:19:24 bouyer Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
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
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/scsiio.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include "locators.h"

const struct scsipi_periphsw scsi_probe_dev = {
	NULL,
	NULL,
	NULL,
	NULL,
};

int	scsi_probe_device __P((struct scsibus_softc *, int, int));

int	scsibusmatch __P((struct device *, struct cfdata *, void *));
void	scsibusattach __P((struct device *, struct device *, void *));
int	scsibusactivate __P((struct device *, enum devact));
int	scsibusdetach __P((struct device *, int flags));

int	scsibussubmatch __P((struct device *, struct cfdata *, void *));

struct cfattach scsibus_ca = {
	sizeof(struct scsibus_softc), scsibusmatch, scsibusattach,
	    scsibusdetach, scsibusactivate,
};

extern struct cfdriver scsibus_cd;

int	scsibusprint __P((void *, const char *));
void	scsibus_config_interrupts __P((struct device *));

cdev_decl(scsibus);

const struct scsipi_bustype scsi_bustype = {
	SCSIPI_BUSTYPE_SCSI,
	scsi_scsipi_cmd,
	scsipi_interpret_sense,
	scsi_print_addr,
	scsi_kill_pending,
};

int
scsiprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct scsipi_channel *chan = aux;
	struct scsipi_adapter *adapt = chan->chan_adapter;

	/* only "scsibus"es can attach to "scsi"s; easy. */
	if (pnp)
		printf("scsibus at %s", pnp);

	/* don't print channel if the controller says there can be only one. */
	if (adapt->adapt_nchannels != 1)
		printf(" channel %d", chan->chan_channel);

	return (UNCONF);
}

int
scsibusmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct scsipi_channel *chan = aux;

	if (chan->type != BUS_SCSI)
		return 0;

	if (cf->cf_loc[SCSICF_CHANNEL] != chan->chan_channel &&
	    cf->cf_loc[SCSICF_CHANNEL] != SCSICF_CHANNEL_DEFAULT)
		return (0);

	return (1);
}

void
scsibusattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct scsibus_softc *sc = (void *) self;
	struct scsipi_channel *chan = aux;

	sc->sc_channel = chan;

	/* Initialize the channel structure first */
	if (scsipi_channel_init(chan)) {
		printf(": failed to init channel\n");
		return;
	}

	printf(": %d targets, %d luns per target\n",
	    chan->chan_ntargets, chan->chan_nluns);


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
	struct scsibus_softc *sc = (void *) self;

#ifndef SCSI_DELAY
#define SCSI_DELAY 2
#endif

	if ((sc->sc_channel->chan_flags & SCSIPI_CHAN_NOSETTLE) == 0 &&
	    SCSI_DELAY > 0) {
		printf("%s: waiting %d seconds for devices to settle...\n",
		    self->dv_xname, SCSI_DELAY);
		/* ...an identifier we know no one will use... */
		(void) tsleep(scsibus_config_interrupts, PRIBIO,
		    "scsidly", SCSI_DELAY * hz);
	}

	scsi_probe_bus(sc, -1, -1);
}

int
scsibussubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;

	if (cf->cf_loc[SCSIBUSCF_TARGET] != SCSIBUSCF_TARGET_DEFAULT &&
	    cf->cf_loc[SCSIBUSCF_TARGET] != periph->periph_target)
		return (0);
	if (cf->cf_loc[SCSIBUSCF_LUN] != SCSIBUSCF_LUN_DEFAULT &&
	    cf->cf_loc[SCSIBUSCF_LUN] != periph->periph_lun)
		return (0);
	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
scsibusactivate(self, act)
	struct device *self;
	enum devact act;
{
	struct scsibus_softc *sc = (void *) self;
	struct scsipi_channel *chan = sc->sc_channel;
	struct scsipi_periph *periph;
	int target, lun, error = 0, s;

	s = splbio();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		for (target = 0; target < chan->chan_ntargets;
		     target++) {
			if (target == chan->chan_id)
				continue;
			for (lun = 0; lun < chan->chan_nluns; lun++) {
				periph = scsipi_lookup_periph(chan,
				    target, lun);
				if (periph == NULL)
					continue;
				error = config_deactivate(periph->periph_dev);
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
	struct scsibus_softc *sc = (void *) self;
	struct scsipi_channel *chan = sc->sc_channel;

	/*
	 * Shut down the channel.
	 */
	scsipi_channel_shutdown(chan);

	/*
	 * Now detach all of the periphs.
	 */
	return scsipi_target_detach(chan, -1, -1, flags);
}

/*
 * Probe the requested scsi bus. It must be already set up.
 * target and lun optionally narrow the search if not -1
 */
int
scsi_probe_bus(sc, target, lun)
	struct scsibus_softc *sc;
	int target, lun;
{
	struct scsipi_channel *chan = sc->sc_channel;
	int maxtarget, mintarget, maxlun, minlun;
	int error;

	if (target == -1) {
		maxtarget = chan->chan_ntargets - 1;
		mintarget = 0;
	} else {
		if (target < 0 || target >= chan->chan_ntargets)
			return (EINVAL);
		maxtarget = mintarget = target;
	}

	if (lun == -1) {
		maxlun = chan->chan_nluns - 1;
		minlun = 0;
	} else {
		if (lun < 0 || lun >= chan->chan_nluns)
			return (EINVAL);
		maxlun = minlun = lun;
	}

	/*
	 * Some HBAs provide an abstracted view of the bus; give them an
	 * oppertunity to re-scan it before we do.
	 */
	if (chan->chan_adapter->adapt_ioctl != NULL)
		(*chan->chan_adapter->adapt_ioctl)(chan, SCBUSIOLLSCAN, NULL,
		    0, curproc);

	if ((error = scsipi_adapter_addref(chan->chan_adapter)) != 0)
		return (error);
	for (target = mintarget; target <= maxtarget; target++) {
		if (target == chan->chan_id)
			continue;
		for (lun = minlun; lun <= maxlun; lun++) {
			/*
			 * See if there's a device present, and configure it.
			 */
			if (scsi_probe_device(sc, target, lun) == 0)
				break;
			/* otherwise something says we should look further */
		}
		
		/*
		 * Now that we've discovered all of the LUNs on this
		 * I_T Nexus, update the xfer mode for all of them
		 * that we know about.
		 */
		scsipi_set_xfer_mode(chan, target, 1);
	}
	scsipi_adapter_delref(chan->chan_adapter);
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
	const char *dtype, *qtype;
	char vendor[33], product[65], revision[17];
	int target, lun;

	if (pnp != NULL)
		printf("%s", pnp);

	inqbuf = &sa->sa_inqbuf;

	target = sa->sa_periph->periph_target;
	lun = sa->sa_periph->periph_lun;
	type = inqbuf->type & SID_TYPE;

	/*
	 * Figure out basic device type and qualifier.
	 */
	dtype = 0;
	switch (inqbuf->type & SID_QUAL) {
	case SID_QUAL_LU_PRESENT:
		qtype = "";
		break;

	case SID_QUAL_LU_NOTPRESENT:
		qtype = " offline";
		break;

	case SID_QUAL_reserved:
	case SID_QUAL_LU_NOT_SUPP:
		panic("scsibusprint: impossible qualifier");

	default:
		qtype = "";
		dtype = "vendor-unique";
		break;
	}
	if (dtype == NULL)
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

const struct scsi_quirk_inquiry_pattern scsi_quirk_patterns[] = {
	{{T_CDROM, T_REMOV,
	 "CHINON  ", "CD-ROM CDS-431  ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "Chinon  ", "CD-ROM CDS-525  ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "CHINON  ", "CD-ROM CDS-535  ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "DEC     ", "RRD42   (C) DEC ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "DENON   ", "DRD-25X         ", "V"},    PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "GENERIC ", "CRD-BP2         ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "HP      ", "C4324/C4325     ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "IMS     ", "CDD521/10       ", "2.06"}, PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "MATSHITA", "CD-ROM CR-5XX   ", "1.0b"}, PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "MEDAVIS ", "RENO CD-ROMX2A  ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "MEDIAVIS", "CDR-H93MV       ", "1.3"},  PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "NEC     ", "CD-ROM DRIVE:55 ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "NEC     ", "CD-ROM DRIVE:83 ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "NEC     ", "CD-ROM DRIVE:84 ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "NEC     ", "CD-ROM DRIVE:841", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "PIONEER ", "CD-ROM DR-124X  ", "1.01"}, PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "SONY    ", "CD-ROM CDU-541  ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "SONY    ", "CD-ROM CDU-55S  ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "SONY    ", "CD-ROM CDU-561  ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "SONY    ", "CD-ROM CDU-8003A", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "SONY    ", "CD-ROM CDU-8012 ", ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "TEAC    ", "CD-ROM          ", "1.06"}, PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "TEAC    ", "CD-ROM CD-56S   ", "1.0B"}, PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "TEXEL   ", "CD-ROM          ", "1.06"}, PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "TEXEL   ", "CD-ROM DM-XX24 K", "1.09"}, PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "TEXEL   ", "CD-ROM DM-XX24 K", "1.10"}, PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "TOSHIBA ", "XM-4101TASUNSLCD", "1755"}, PQUIRK_NOLUNS|PQUIRK_NOSYNC},
	/* "IBM CDRM00201     !F" 0724 is an IBM OEM Toshiba XM-4101BME */
	{{T_CDROM, T_REMOV,
	 "IBM     ", "CDRM00201     !F", "0724"}, PQUIRK_NOLUNS|PQUIRK_NOSYNC},
	{{T_CDROM, T_REMOV,
	 "ShinaKen", "CD-ROM DM-3x1S", "1.04"},   PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "JVC     ", "R2626",            ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "YAMAHA", "CRW8424S",           ""},     PQUIRK_NOLUNS},
	{{T_CDROM, T_REMOV,
	 "VMware", "Virtual",           "1.0"},
				PQUIRK_NOSTARTUNIT|PQUIRK_NODOORLOCK},

	{{T_DIRECT, T_FIXED,
	 "MICROP  ", "1588-15MBSUN0669", ""},     PQUIRK_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "MICROP  ", "2217-15MQ1091501", ""},     PQUIRK_NOSYNCCACHE},
	{{T_OPTICAL, T_REMOV,
	 "EPSON   ", "OMD-5010        ", "3.08"}, PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "TOSHIBA ","CD-ROM XM-3401TA", "0283"},  PQUIRK_CDROM|PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "TOSHIBA ", "CD-ROM DRIVE:XM", "1971"}, PQUIRK_CDROM|PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "ADAPTEC ", "AEC-4412BD",       "1.2A"}, PQUIRK_NOMODESENSE},
	{{T_DIRECT, T_FIXED,
	 "DEC     ", "RZ55     (C) DEC", ""},     PQUIRK_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "EMULEX  ", "MD21/S2     ESDI", "A00"},  PQUIRK_FORCELUNS|PQUIRK_AUTOSAVE},
	/* Gives non-media hardware failure in response to start-unit command */
	{{T_DIRECT, T_FIXED,
	 "HITACHI", "DK515C",		"CP16"},  PQUIRK_NOSTARTUNIT},
	{{T_DIRECT, T_FIXED,
	 "HITACHI", "DK515C",		"CP15"},  PQUIRK_NOSTARTUNIT},
	{{T_DIRECT, T_FIXED,
	 "HP      ", "C372",             ""},     PQUIRK_NOTAG},
	{{T_DIRECT, T_FIXED,
	 "IBMRAID ", "0662S",		 ""},     PQUIRK_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "IBM     ", "0663H",		 ""},     PQUIRK_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "IBM",	     "0664",		 ""},     PQUIRK_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "IBM     ", "H3171-S2",	 ""},	  PQUIRK_NOLUNS|PQUIRK_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "IBM     ", "KZ-C",		 ""},	  PQUIRK_AUTOSAVE},
	/* Broken IBM disk */
	{{T_DIRECT, T_FIXED,
	 ""	   , "DFRSS2F",		 ""},	  PQUIRK_AUTOSAVE},
	{{T_DIRECT, T_REMOV,
	 "MPL     ", "MC-DISK-        ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "XT-3280         ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "XT-4380S        ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "MXT-1240S       ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "XT-4170S        ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "XT-8760S",         ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "LXT-213S        ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "LXT-213S SUN0207", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MAXTOR  ", "LXT-200S        ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "MEGADRV ", "EV1000",           ""},     PQUIRK_NOMODESENSE},
	{{T_DIRECT, T_FIXED,
	 "MST     ", "SnapLink        ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "NEC     ", "D3847           ", "0307"}, PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "QUANTUM ", "ELS85S          ", ""},     PQUIRK_AUTOSAVE},
	{{T_DIRECT, T_FIXED,
	 "QUANTUM ", "LPS525S         ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "QUANTUM ", "P105S 910-10-94x", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "QUANTUM ", "PD1225S         ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "QUANTUM ", "PD210S   SUN0207", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "RODIME  ", "RO3000S         ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "SEAGATE ", "ST125N          ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "SEAGATE ", "ST157N          ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "SEAGATE ", "ST296           ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "SEAGATE ", "ST296N          ", ""},     PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "SEAGATE ", "ST19171",          ""},     PQUIRK_NOMODESENSE},
	{{T_DIRECT, T_FIXED,
	 "SEAGATE ", "ST34501FC       ", ""},     PQUIRK_NOMODESENSE},
	{{T_DIRECT, T_FIXED,
	 "TOSHIBA ", "MK538FB         ", "6027"}, PQUIRK_NOLUNS},
	{{T_DIRECT, T_FIXED,
	 "VMware", "Virtual",           "1.0"},
				PQUIRK_NOSTARTUNIT|PQUIRK_NODOORLOCK},

	{{T_DIRECT, T_REMOV,
	 "iomega", "jaz 1GB", 		 ""},	  PQUIRK_NOMODESENSE},
	{{T_DIRECT, T_REMOV,
	 "IOMEGA", "ZIP 100",		 ""},	  PQUIRK_NOMODESENSE},
	{{T_DIRECT, T_REMOV,
	 "IOMEGA", "ZIP 100",		 "J.03"}, PQUIRK_NOMODESENSE|PQUIRK_NOLUNS},
	/* Letting the motor run kills floppy drives and disks quite fast. */
	{{T_DIRECT, T_REMOV,
	 "TEAC", "FC-1",		 ""},	  PQUIRK_NOSTARTUNIT},

	{{T_DIRECT, T_REMOV,
	 "Y-E DATA", "USB-FDU",          "3.04"}, PQUIRK_NOMODESENSE},
	{{T_DIRECT, T_REMOV,
	 "TEAC", "FD-05PUB",             "1026"}, PQUIRK_NOMODESENSE},

	/* XXX: QIC-36 tape behind Emulex adapter.  Very broken. */
	{{T_SEQUENTIAL, T_REMOV,
	 "        ", "                ", "    "}, PQUIRK_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "CALIPER ", "CP150           ", ""},     PQUIRK_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "EXABYTE ", "EXB-8200        ", ""},     PQUIRK_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "SONY    ", "GY-10C          ", ""},     PQUIRK_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "SONY    ", "SDT-2000        ", "2.09"}, PQUIRK_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "SONY    ", "SDT-5000        ", "3."},   PQUIRK_NOSYNC|PQUIRK_NOWIDE},
	{{T_SEQUENTIAL, T_REMOV,
	 "SONY    ", "SDT-5200        ", "3."},   PQUIRK_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "TANDBERG", " TDC 3600       ", ""},     PQUIRK_NOLUNS},
	/* Following entry reported as a Tandberg 3600; ref. PR1933 */
	{{T_SEQUENTIAL, T_REMOV,
	 "ARCHIVE ", "VIPER 150  21247", ""},     PQUIRK_NOLUNS},
	/* Following entry for a Cipher ST150S; ref. PR4171 */
	{{T_SEQUENTIAL, T_REMOV,
	 "ARCHIVE ", "VIPER 1500 21247", "2.2G"}, PQUIRK_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "ARCHIVE ", "Python 28454-XXX", ""},     PQUIRK_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "WANGTEK ", "5099ES SCSI",      ""},     PQUIRK_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "WANGTEK ", "5150ES SCSI",      ""},     PQUIRK_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "WANGTEK ", "SCSI-36",		 ""},     PQUIRK_NOLUNS},
	{{T_SEQUENTIAL, T_REMOV,
	 "WangDAT ", "Model 1300      ", "02.4"}, PQUIRK_NOSYNC|PQUIRK_NOWIDE},
	{{T_SEQUENTIAL, T_REMOV,
	 "WangDAT ", "Model 2600      ", "01.7"}, PQUIRK_NOSYNC|PQUIRK_NOWIDE},
	{{T_SEQUENTIAL, T_REMOV,
	 "WangDAT ", "Model 3200      ", "02.2"}, PQUIRK_NOSYNC|PQUIRK_NOWIDE},
	{{T_SEQUENTIAL, T_REMOV,
	 "TEAC    ", "MT-2ST/N50      ", ""},     PQUIRK_NOLUNS},

	{{T_SCANNER, T_FIXED,
	 "RICOH   ", "IS60            ", "1R08"}, PQUIRK_NOLUNS},
	{{T_SCANNER, T_FIXED,
	 "UMAX    ", "Astra 1200S     ", "V2.9"}, PQUIRK_NOLUNS},
	{{T_SCANNER, T_FIXED,
	 "UMAX    ", "Astra 1220S     ", ""},     PQUIRK_NOLUNS},
	{{T_SCANNER, T_FIXED,
	 "UMAX    ", "UMAX S-6E       ", "V2.0"}, PQUIRK_NOLUNS},
	{{T_SCANNER, T_FIXED,
	 "UMAX    ", "UMAX S-12       ", "V2.1"}, PQUIRK_NOLUNS},
	{{T_SCANNER, T_FIXED,
	 "ULTIMA  ", "A6000C          ", ""}, PQUIRK_NOLUNS},
	{{T_PROCESSOR, T_FIXED,
	 "SYMBIOS", "", ""},     PQUIRK_NOLUNS},
	{{T_PROCESSOR, T_FIXED,
	 "LITRONIC", "PCMCIA          ", ""},     PQUIRK_NOLUNS},
	{{T_CHANGER, T_REMOV,
	 "SONY    ", "CDL1100         ", ""},     PQUIRK_NOLUNS},
	{{T_ENCLOSURE, T_FIXED,
	 "SUN     ", "SENA            ", ""}, PQUIRK_NOLUNS},
};

/*
 * given a target and lun, ask the device what
 * it is, and find the correct driver table
 * entry.
 */
int
scsi_probe_device(sc, target, lun)
	struct scsibus_softc *sc;
	int target, lun;
{
	struct scsipi_channel *chan = sc->sc_channel;
	struct scsipi_periph *periph;
	struct scsipi_inquiry_data inqbuf;
	struct scsi_quirk_inquiry_pattern *finger;
	int checkdtype, priority, docontinue, quirks;
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
	if (scsipi_lookup_periph(chan, target, lun) != NULL)
		return (docontinue);

	periph = scsipi_alloc_periph(M_NOWAIT);
	if (periph == NULL) {
#ifdef	DIAGNOSTIC
		printf("%s: cannot allocate periph for target %d lun %d\n",
		    sc->sc_dev.dv_xname, target, lun);
#endif
		return (ENOMEM);
	}
	periph->periph_channel = chan;
	periph->periph_switch = &scsi_probe_dev;

	periph->periph_target = target;
	periph->periph_lun = lun;
	periph->periph_quirks = chan->chan_defquirks;

#ifdef SCSIPI_DEBUG
	if (SCSIPI_DEBUG_TYPE == SCSIPI_BUSTYPE_SCSI &&
	    SCSIPI_DEBUG_TARGET == target &&
	    SCSIPI_DEBUG_LUN == lun)
		periph->periph_dbflags |= SCSIPI_DEBUG_FLAGS;
#endif

	/*
	 * Ask the device what it is
	 */
	(void) scsipi_test_unit_ready(periph,
	    XS_CTL_DISCOVERY | XS_CTL_IGNORE_ILLEGAL_REQUEST |
	    XS_CTL_IGNORE_NOT_READY | XS_CTL_IGNORE_MEDIA_CHANGE);

#ifdef SCSI_2_DEF
	/* some devices need to be told to go to SCSI2 */
	/* However some just explode if you tell them this.. leave it out */
	scsi_change_def(periph, XS_CTL_DISCOVERY | XS_CTL_SILENT);
#endif /* SCSI_2_DEF */

	/* Now go ask the device all about itself. */
	memset(&inqbuf, 0, sizeof(inqbuf));
	if (scsipi_inquire(periph, &inqbuf,
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

	periph->periph_type = inqbuf.device & SID_TYPE;
	if (inqbuf.dev_qual2 & SID_REMOVABLE)
		periph->periph_flags |= PERIPH_REMOVABLE;
	periph->periph_version = inqbuf.version & SID_ANSII;

	/*
	 * Any device qualifier that has the top bit set (qualifier&4 != 0)
	 * is vendor specific and won't match in this switch.
	 * All we do here is throw out bad/negative responses.
	 */
	checkdtype = 0;
	switch (inqbuf.device & SID_QUAL) {
	case SID_QUAL_LU_PRESENT:
	case SID_QUAL_LU_NOTPRESENT:
		checkdtype = 1;
		break;

	case SID_QUAL_reserved:
	case SID_QUAL_LU_NOT_SUPP:
		goto bad;

	default:
		break;
	}

	/* Let the adapter driver handle the device separatley if it wants. */
	if (chan->chan_adapter->adapt_accesschk != NULL &&
	    (*chan->chan_adapter->adapt_accesschk)(periph, &sa.sa_inqbuf))
		goto bad;

	if (checkdtype) {
		switch (periph->periph_type) {
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
	}

	sa.sa_periph = periph;
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

	if (finger != NULL)
		quirks = finger->quirks;
	else
		quirks = 0;

	/*
	 * Determine the operating mode capabilities of the device.
	 */
	if (periph->periph_version >= 2) {
		if ((inqbuf.flags3 & SID_CmdQue) != 0 &&
		    (quirks & PQUIRK_NOTAG) == 0)
			periph->periph_cap |= PERIPH_CAP_TQING;
		if ((inqbuf.flags3 & SID_Linked) != 0)
			periph->periph_cap |= PERIPH_CAP_LINKCMDS;
		if ((inqbuf.flags3 & SID_Sync) != 0 &&
		    (quirks & PQUIRK_NOSYNC) == 0)
			periph->periph_cap |= PERIPH_CAP_SYNC;
		if ((inqbuf.flags3 & SID_WBus16) != 0 &&
		    (quirks & PQUIRK_NOWIDE) == 0)
			periph->periph_cap |= PERIPH_CAP_WIDE16;
		if ((inqbuf.flags3 & SID_WBus32) != 0 &&
		    (quirks & PQUIRK_NOWIDE) == 0)
			periph->periph_cap |= PERIPH_CAP_WIDE32;
		if ((inqbuf.flags3 & SID_SftRe) != 0)
			periph->periph_cap |= PERIPH_CAP_SFTRESET;
		if ((inqbuf.flags3 & SID_RelAdr) != 0)
			periph->periph_cap |= PERIPH_CAP_RELADR;
	}

	/*
	 * Now apply any quirks from the table.
	 */
	periph->periph_quirks |= quirks;
	if (periph->periph_version == 0 &&
	    (periph->periph_quirks & PQUIRK_FORCELUNS) == 0)
		periph->periph_quirks |= PQUIRK_NOLUNS;

	if (periph->periph_quirks & PQUIRK_CDROM) {
		periph->periph_quirks ^= PQUIRK_CDROM;
		inqbuf.dev_qual2 |= SID_REMOVABLE;
		sa.sa_inqbuf.type = inqbuf.device = ((inqbuf.device & ~SID_REMOVABLE) | T_CDROM);
		sa.sa_inqbuf.removable = T_REMOV;
	}

	if ((periph->periph_quirks & PQUIRK_NOLUNS) == 0)
		docontinue = 1;

	if ((cf = config_search(scsibussubmatch, &sc->sc_dev, &sa)) != NULL) {
		scsipi_insert_periph(chan, periph);
		/*
		 * XXX Can't assign periph_dev here, because we'll
		 * XXX need it before config_attach() returns.  Must
		 * XXX assign it in periph driver.
		 */
		(void) config_attach(&sc->sc_dev, cf, &sa, scsibusprint);
	} else {
		scsibusprint(&sa, sc->sc_dev.dv_xname);
		printf(" not configured\n");
		goto bad;
	}

	return (docontinue);

bad:
	free(periph, M_DEVBUF);
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

	if ((error = scsipi_adapter_addref(sc->sc_channel->chan_adapter)) != 0)
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

	scsipi_adapter_delref(sc->sc_channel->chan_adapter);

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
	struct scsipi_channel *chan = sc->sc_channel;
	int error;

	/*
	 * Enforce write permission for ioctls that change the
	 * state of the bus.  Host adapter specific ioctls must
	 * be checked by the adapter driver.
	 */
	switch (cmd) {
	case SCBUSIOSCAN:
	case SCBUSIODETACH:
	case SCBUSIORESET:
		if ((flag & FWRITE) == 0)
			return (EBADF);
	}

	switch (cmd) {
	case SCBUSIOSCAN:
	    {
		struct scbusioscan_args *a =
		    (struct scbusioscan_args *)addr;

		error = scsi_probe_bus(sc, a->sa_target, a->sa_lun);
		break;
	    }

	case SCBUSIODETACH:
	    {
		struct scbusiodetach_args *a =
		    (struct scbusiodetach_args *)addr;

		error = scsipi_target_detach(chan, a->sa_target, a->sa_lun, 0);
		break;
	    }


	case SCBUSIORESET:
		/* FALLTHROUGH */
	default:
		if (chan->chan_adapter->adapt_ioctl == NULL)
			error = ENOTTY;
		else
			error = (*chan->chan_adapter->adapt_ioctl)(chan,
			    cmd, addr, flag, p);
		break;
	}

	return (error);
}
