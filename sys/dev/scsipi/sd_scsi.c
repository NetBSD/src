/*	$NetBSD: sd_scsi.c,v 1.1 1998/01/15 02:21:40 cgd Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1997 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
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
 * Originally written by Julian Elischer (julian@dialix.oz.au)
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
 * Ported to run under 386BSD by Julian Elischer (julian@dialix.oz.au) Sept 1992
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/conf.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsi_disk.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/sdvar.h>

#ifdef __BROKEN_INDIRECT_CONFIG
int	sd_scsibus_match __P((struct device *, void *, void *));
#else
int	sd_scsibus_match __P((struct device *, struct cfdata *, void *));
#endif
void	sd_scsibus_attach __P((struct device *, struct device *, void *));

struct cfattach sd_scsibus_ca = {
	sizeof(struct sd_softc), sd_scsibus_match, sd_scsibus_attach
};

struct scsipi_inquiry_pattern sd_scsibus_patterns[] = {
	{T_DIRECT, T_FIXED,
	 "",         "",                 ""},
	{T_DIRECT, T_REMOV,
	 "",         "",                 ""},
	{T_OPTICAL, T_FIXED,
	 "",         "",                 ""},
	{T_OPTICAL, T_REMOV,
	 "",         "",                 ""},
};

struct sd_scsibus_mode_sense_data {
	struct scsi_mode_header header;
	struct scsi_blk_desc blk_desc;
	union scsi_disk_pages pages;
};

static int	sd_scsibus_mode_sense __P((struct sd_softc *,
		    struct sd_scsibus_mode_sense_data *, int, int));
static int	sd_scsibus_get_parms __P((struct sd_softc *,
		    struct disk_parms *, int));
static int	sd_scsibus_get_optparms __P((struct sd_softc *,
		    struct disk_parms *, int));

const struct sd_ops sd_scsibus_ops = {
	sd_scsibus_get_parms,
};

int
sd_scsibus_match(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	int priority;

	if (sa->sa_sc_link->type != BUS_SCSI)
		return (0);

	(void)scsipi_inqmatch(&sa->sa_inqbuf,
	    (caddr_t)sd_scsibus_patterns,
	    sizeof(sd_scsibus_patterns) / sizeof(sd_scsibus_patterns[0]),
	    sizeof(sd_scsibus_patterns[0]), &priority);
	return (priority);
}

/*
 * The routine called by the low level scsi routine when it discovers
 * a device suitable for this driver.
 */
void
sd_scsibus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sd_softc *sd = (void *)self;
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_link *sc_link = sa->sa_sc_link;

	SC_DEBUG(sc_link, SDEV_DB2, ("cd_scsibus_attach: "));

	sd->type = (sa->sa_inqbuf.type & SID_TYPE);

	/*
	 * Note if this device is ancient.  This is used in sdminphys().
	 */
	if ((sa->scsipi_info.scsi_version & SID_ANSII) == 0)
		sd->flags |= SDF_ANCIENT;

	sdattach(parent, sd, sc_link, &sd_scsibus_ops);
}

static int
sd_scsibus_mode_sense(sd, scsipi_sense, page, flags)
	struct sd_softc *sd;
	struct sd_scsibus_mode_sense_data *scsipi_sense;
	int page, flags;
{
	struct scsi_mode_sense scsipi_cmd;

	/*
	 * Make sure the sense buffer is clean before we do
	 * the mode sense, so that checks for bogus values of
	 * 0 will work in case the mode sense fails.
	 */
	bzero(scsipi_sense, sizeof(*scsipi_sense));

	bzero(&scsipi_cmd, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = SCSI_MODE_SENSE;
	scsipi_cmd.page = page;
	scsipi_cmd.length = 0x20;
	/*
	 * If the command worked, use the results to fill out
	 * the parameter structure
	 */
	return (scsipi_command(sd->sc_link,
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),
	    (u_char *)scsipi_sense, sizeof(*scsipi_sense),
	    SDRETRIES, 6000, NULL, flags | SCSI_DATA_IN | SCSI_SILENT));
}

static int
sd_scsibus_get_optparms(sd, dp, flags)
	struct sd_softc *sd;
	struct disk_parms *dp;
	int flags;
{
	struct scsi_mode_sense scsipi_cmd;
	struct sd_scsibus_mode_sense_data scsipi_sense;
	u_long sectors;
	int error;

	dp->blksize = 512;
	if ((sectors = scsipi_size(sd->sc_link, flags)) == 0)
		return (SDGP_RESULT_OFFLINE);		/* XXX? */

	/* XXX
	 * It is better to get the following params from the
	 * mode sense page 6 only (optical device parameter page).
	 * However, there are stupid optical devices which does NOT
	 * support the page 6. Ghaa....
	 */
	bzero(&scsipi_cmd, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = SCSI_MODE_SENSE;
	scsipi_cmd.page = 0x3f;	/* all pages */
	scsipi_cmd.length = sizeof(struct scsi_mode_header) +
	    sizeof(struct scsi_blk_desc);

	if ((error = scsipi_command(sd->sc_link,  
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),  
	    (u_char *)&scsipi_sense, sizeof(scsipi_sense), SDRETRIES,
	    6000, NULL, flags | SCSI_DATA_IN)) != 0)
		return (SDGP_RESULT_OFFLINE);		/* XXX? */

	dp->blksize = _3btol(scsipi_sense.blk_desc.blklen);
	if (dp->blksize == 0) 
		dp->blksize = 512;

	/*
	 * Create a pseudo-geometry.
	 */
	dp->heads = 64;
	dp->sectors = 32;
	dp->cyls = sectors / (dp->heads * dp->sectors);
	dp->disksize = sectors;

	return (SDGP_RESULT_OK);
}

/*
 * Get the scsi driver to send a full inquiry to the * device and use the
 * results to fill out the disk parameter structure.
 */
static int
sd_scsibus_get_parms(sd, dp, flags)
	struct sd_softc *sd;
	struct disk_parms *dp;
	int flags;
{
	struct sd_scsibus_mode_sense_data scsipi_sense;
	u_long sectors;
	int page;
	int error;

	dp->rot_rate = 3600;		/* XXX any way of getting this? */

	/*
	 * If offline, the SDEV_MEDIA_LOADED flag will be
	 * cleared by the caller if necessary.
	 */
	if (sd->type == T_OPTICAL)
		return (sd_scsibus_get_optparms(sd, dp, flags));

	if ((error = sd_scsibus_mode_sense(sd, &scsipi_sense, page = 4,
	    flags)) == 0) {
		SC_DEBUG(sd->sc_link, SDEV_DB3,
		    ("%d cyls, %d heads, %d precomp, %d red_write, %d land_zone\n",
		    _3btol(scsipi_sense.pages.rigid_geometry.ncyl),
		    scsipi_sense.pages.rigid_geometry.nheads,
		    _2btol(scsipi_sense.pages.rigid_geometry.st_cyl_wp),
		    _2btol(scsipi_sense.pages.rigid_geometry.st_cyl_rwc),
		    _2btol(scsipi_sense.pages.rigid_geometry.land_zone)));

		/*
		 * KLUDGE!! (for zone recorded disks)
		 * give a number of sectors so that sec * trks * cyls
		 * is <= disk_size
		 * can lead to wasted space! THINK ABOUT THIS !
		 */
		dp->heads = scsipi_sense.pages.rigid_geometry.nheads;
		dp->cyls = _3btol(scsipi_sense.pages.rigid_geometry.ncyl);
		dp->blksize = _3btol(scsipi_sense.blk_desc.blklen);

		if (dp->heads == 0 || dp->cyls == 0)
			goto fake_it;

		if (dp->blksize == 0)
			dp->blksize = 512;

		sectors = scsipi_size(sd->sc_link, flags);
		dp->disksize = sectors;
		sectors /= (dp->heads * dp->cyls);
		dp->sectors = sectors;	/* XXX dubious on SCSI */

		return (SDGP_RESULT_OK);
	}

	if ((error = sd_scsibus_mode_sense(sd, &scsipi_sense, page = 5,
	    flags)) == 0) {
		dp->heads = scsipi_sense.pages.flex_geometry.nheads;
		dp->cyls = _2btol(scsipi_sense.pages.flex_geometry.ncyl);
		dp->blksize = _3btol(scsipi_sense.blk_desc.blklen);
		dp->sectors = scsipi_sense.pages.flex_geometry.ph_sec_tr;
		dp->disksize = dp->heads * dp->cyls * dp->sectors;
		if (dp->disksize == 0)
			goto fake_it;

		if (dp->blksize == 0)
			dp->blksize = 512;

		return (SDGP_RESULT_OK);
	}

fake_it:
	if ((sd->sc_link->quirks & SDEV_NOMODESENSE) == 0) {
		if (error == 0)
			printf("%s: mode sense (%d) returned nonsense",
			    sd->sc_dev.dv_xname, page);
		else
			printf("%s: could not mode sense (4/5)",
			    sd->sc_dev.dv_xname);
		printf("; using fictitious geometry\n");
	}
	/*
	 * use adaptec standard fictitious geometry
	 * this depends on which controller (e.g. 1542C is
	 * different. but we have to put SOMETHING here..)
	 */
	sectors = scsipi_size(sd->sc_link, flags);
	dp->heads = 64;
	dp->sectors = 32;
	dp->cyls = sectors / (64 * 32);
	dp->blksize = 512;
	dp->disksize = sectors;
	return (SDGP_RESULT_OK);
}
