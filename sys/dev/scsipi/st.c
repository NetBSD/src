/*	$NetBSD: st.c,v 1.142.2.3 2002/02/11 20:10:13 jdolecek Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 * major changes by Julian Elischer (julian@jules.dialix.oz.au) May 1993
 *
 * A lot of rewhacking done by mjacob (mjacob@nas.nasa.gov).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: st.c,v 1.142.2.3 2002/02/11 20:10:13 jdolecek Exp $");

#include "opt_scsi.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/mtio.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/vnode.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsi_tape.h>
#include <dev/scsipi/stvar.h>

/* Defines for device specific stuff */
#define DEF_FIXED_BSIZE  512

#define STMODE(z)	( minor(z)       & 0x03)
#define STDSTY(z)	((minor(z) >> 2) & 0x03)
#define STUNIT(z)	((minor(z) >> 4)       )
#define STNMINOR	16

#define NORMAL_MODE	0
#define NOREW_MODE	1
#define EJECT_MODE	2
#define CTRL_MODE	3

#define	FALSE		0
#define	TRUE		1

#ifndef		ST_MOUNT_DELAY
#define		ST_MOUNT_DELAY		0
#endif

/*
 * Define various devices that we know mis-behave in some way,
 * and note how they are bad, so we can correct for them
 */

const struct st_quirk_inquiry_pattern st_quirk_patterns[] = {
	{{T_SEQUENTIAL, T_REMOV,
	 "        ", "                ", "    "}, {0, 0, {
		{ST_Q_FORCE_BLKSIZE, 512, 0},		/* minor 0-3 */
		{ST_Q_FORCE_BLKSIZE, 512, QIC_24},	/* minor 4-7 */
		{ST_Q_FORCE_BLKSIZE, 0, HALFINCH_1600},	/* minor 8-11 */
		{ST_Q_FORCE_BLKSIZE, 0, HALFINCH_6250}	/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "TANDBERG", " TDC 3600       ", ""},     {0, 12, {
		{0, 0, 0},				/* minor 0-3 */
		{ST_Q_FORCE_BLKSIZE, 0, QIC_525},	/* minor 4-7 */
		{0, 0, QIC_150},			/* minor 8-11 */
		{0, 0, QIC_120}				/* minor 12-15 */
	}}},
 	{{T_SEQUENTIAL, T_REMOV,
 	 "TANDBERG", " TDC 3800       ", ""},     {0, 0, {
		{ST_Q_FORCE_BLKSIZE, 512, 0},		/* minor 0-3 */
		{0, 0, QIC_525},			/* minor 4-7 */
		{0, 0, QIC_150},			/* minor 8-11 */
		{0, 0, QIC_120}				/* minor 12-15 */
	}}},
	/*
	 * lacking a manual for the 4200, it's not clear what the
	 * specific density codes should be- the device is a 2.5GB
	 * capable QIC drive, those density codes aren't readily
	 * availabel. The 'default' will just have to do.
	 */
 	{{T_SEQUENTIAL, T_REMOV,
 	 "TANDBERG", " TDC 4200       ", ""},     {0, 0, {
		{ST_Q_FORCE_BLKSIZE, 512, 0},		/* minor 0-3 */
		{0, 0, QIC_525},			/* minor 4-7 */
		{0, 0, QIC_150},			/* minor 8-11 */
		{0, 0, QIC_120}				/* minor 12-15 */
	}}},
	/*
	 * At least -005 and -007 need this.  I'll assume they all do unless I
	 * hear otherwise.  - mycroft, 31MAR1994
	 */
	{{T_SEQUENTIAL, T_REMOV,
	 "ARCHIVE ", "VIPER 2525 25462", ""},     {0, 0, {
		{ST_Q_SENSE_HELP, 0, 0},		/* minor 0-3 */
		{ST_Q_SENSE_HELP, 0, QIC_525},		/* minor 4-7 */
		{0, 0, QIC_150},			/* minor 8-11 */
		{0, 0, QIC_120}				/* minor 12-15 */
	}}},
	/*
	 * One user reports that this works for his tape drive.  It probably
	 * needs more work.  - mycroft, 09APR1994
	 */
	{{T_SEQUENTIAL, T_REMOV,
	 "SANKYO  ", "CP525           ", ""},    {0, 0, {
		{ST_Q_FORCE_BLKSIZE, 512, 0},		/* minor 0-3 */
		{ST_Q_FORCE_BLKSIZE, 512, QIC_525},	/* minor 4-7 */
		{0, 0, QIC_150},			/* minor 8-11 */
		{0, 0, QIC_120}				/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "ANRITSU ", "DMT780          ", ""},     {0, 0, {
		{ST_Q_FORCE_BLKSIZE, 512, 0},		/* minor 0-3 */
		{ST_Q_FORCE_BLKSIZE, 512, QIC_525},	/* minor 4-7 */
		{0, 0, QIC_150},			/* minor 8-11 */
		{0, 0, QIC_120}				/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "ARCHIVE ", "VIPER 150  21247", ""},     {ST_Q_ERASE_NOIMM, 12, {
		{ST_Q_SENSE_HELP, 0, 0},		/* minor 0-3 */
		{0, 0, QIC_150},			/* minor 4-7 */
		{0, 0, QIC_120},			/* minor 8-11 */
		{0, 0, QIC_24}				/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "ARCHIVE ", "VIPER 150  21531", ""},     {ST_Q_ERASE_NOIMM, 12, {
		{ST_Q_SENSE_HELP, 0, 0},		/* minor 0-3 */
		{0, 0, QIC_150},			/* minor 4-7 */
		{0, 0, QIC_120},			/* minor 8-11 */
		{0, 0, QIC_24}				/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "WANGTEK ", "5099ES SCSI", ""},          {0, 0, {
		{ST_Q_FORCE_BLKSIZE, 512, 0},		/* minor 0-3 */
		{0, 0, QIC_11},				/* minor 4-7 */
		{0, 0, QIC_24},				/* minor 8-11 */
		{0, 0, QIC_24}				/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "WANGTEK ", "5150ES SCSI", ""},          {0, 0, {
		{ST_Q_FORCE_BLKSIZE, 512, 0},		/* minor 0-3 */
		{0, 0, QIC_24},				/* minor 4-7 */
		{0, 0, QIC_120},			/* minor 8-11 */
		{0, 0, QIC_150}				/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "WANGTEK ", "5525ES SCSI REV7", ""},     {0, 0, {
		{0, 0, 0},				/* minor 0-3 */
		{ST_Q_BLKSIZE, 0, QIC_525},		/* minor 4-7 */
		{0, 0, QIC_150},			/* minor 8-11 */
		{0, 0, QIC_120}				/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "WangDAT ", "Model 1300      ", ""},     {0, 0, {
		{0, 0, 0},				/* minor 0-3 */
		{ST_Q_FORCE_BLKSIZE, 512, DDS},		/* minor 4-7 */
		{ST_Q_FORCE_BLKSIZE, 1024, DDS},	/* minor 8-11 */
		{ST_Q_FORCE_BLKSIZE, 0, DDS}		/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "EXABYTE ", "EXB-8200        ", "263H"}, {0, 5, {
		{0, 0, 0},				/* minor 0-3 */
		{0, 0, 0},				/* minor 4-7 */
		{0, 0, 0},				/* minor 8-11 */
		{0, 0, 0}				/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "STK",      "9490",             ""},
				{ST_Q_FORCE_BLKSIZE, 0, {
		{0, 0, 0},				/* minor 0-3 */
		{0, 0, 0},				/* minor 4-7 */
		{0, 0, 0},				/* minor 8-11 */
		{0, 0, 0}				/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "STK",      "SD-3",             ""},
				{ST_Q_FORCE_BLKSIZE, 0, {
		{0, 0, 0},				/* minor 0-3 */
		{0, 0, 0},				/* minor 4-7 */
		{0, 0, 0},				/* minor 8-11 */
		{0, 0, 0}				/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "IBM",      "03590",            ""},     {ST_Q_IGNORE_LOADS, 0, {
		{0, 0, 0},				/* minor 0-3 */
		{0, 0, 0},				/* minor 4-7 */
		{0, 0, 0},				/* minor 8-11 */
		{0, 0, 0}				/* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "HP      ", "T4000s          ", ""},     {ST_Q_UNIMODAL, 0, {
		{0, 0, QIC_3095},			/* minor 0-3 */
		{0, 0, QIC_3095},			/* minor 4-7 */
		{0, 0, QIC_3095},			/* minor 8-11 */
		{0, 0, QIC_3095},			/* minor 12-15 */
	}}},
#if 0
	{{T_SEQUENTIAL, T_REMOV,
	 "EXABYTE ", "EXB-8200        ", ""},     {0, 12, {
		{0, 0, 0},				/* minor 0-3 */
		{0, 0, 0},				/* minor 4-7 */
		{0, 0, 0},				/* minor 8-11 */
		{0, 0, 0}				/* minor 12-15 */
	}}},
#endif
	{{T_SEQUENTIAL, T_REMOV,
	 "TEAC    ", "MT-2ST/N50      ", ""},     {ST_Q_IGNORE_LOADS, 0, {
		{0, 0, 0},			        /* minor 0-3 */
		{0, 0, 0},			        /* minor 4-7 */
		{0, 0, 0},			        /* minor 8-11 */
		{0, 0, 0}			        /* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "OnStream", "ADR50 Drive", ""},	  {ST_Q_UNIMODAL, 0, {
		{ST_Q_FORCE_BLKSIZE, 512, 0},	        /* minor 0-3 */
		{ST_Q_FORCE_BLKSIZE, 512, 0},	        /* minor 4-7 */
		{ST_Q_FORCE_BLKSIZE, 512, 0},	        /* minor 8-11 */
		{ST_Q_FORCE_BLKSIZE, 512, 0},	        /* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "OnStream DI-30",      "",   "1.0"},  {ST_Q_NOFILEMARKS, 0, {
		{0, 0, 0},                              /* minor 0-3 */
		{0, 0, 0},                              /* minor 4-7 */
		{0, 0, 0},                              /* minor 8-11 */
		{0, 0, 0}                               /* minor 12-15 */
	}}},
	{{T_SEQUENTIAL, T_REMOV,
	 "NCR H621", "0-STD-03-46F880 ", ""},     {ST_Q_NOPREVENT, 0, {
		{0, 0, 0},			       /* minor 0-3 */
		{0, 0, 0},			       /* minor 4-7 */
		{0, 0, 0},			       /* minor 8-11 */
		{0, 0, 0}			       /* minor 12-15 */
	}}},
};

#define NOEJECT 0
#define EJECT 1

void	st_identify_drive __P((struct st_softc *,
	    struct scsipi_inquiry_pattern *));
void	st_loadquirks __P((struct st_softc *));
int	st_mount_tape __P((dev_t, int));
void	st_unmount __P((struct st_softc *, boolean));
int	st_decide_mode __P((struct st_softc *, boolean));
void	ststart __P((struct scsipi_periph *));
void	stdone __P((struct scsipi_xfer *));
int	st_read __P((struct st_softc *, char *, int, int));
int	st_space __P((struct st_softc *, int, u_int, int));
int	st_write_filemarks __P((struct st_softc *, int, int));
int	st_check_eod __P((struct st_softc *, boolean, int *, int));
int	st_load __P((struct st_softc *, u_int, int));
int	st_rewind __P((struct st_softc *, u_int, int));
int	st_interpret_sense __P((struct scsipi_xfer *));
int	st_touch_tape __P((struct st_softc *));
int	st_erase __P((struct st_softc *, int full, int flags));
int	st_rdpos __P((struct st_softc *, int, u_int32_t *));
int	st_setpos __P((struct st_softc *, int, u_int32_t *));

const struct scsipi_periphsw st_switch = {
	st_interpret_sense,
	ststart,
	NULL,
	stdone
};

#if	defined(ST_ENABLE_EARLYWARN)
#define	ST_INIT_FLAGS	ST_EARLYWARN
#else
#define	ST_INIT_FLAGS	0
#endif

/*
 * The routine called by the low level scsi routine when it discovers
 * A device suitable for this driver
 */
void
stattach(parent, st, aux)
	struct device *parent;
	struct st_softc *st;
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;

	SC_DEBUG(periph, SCSIPI_DB2, ("stattach: "));

	/*
	 * Store information needed to contact our base driver
	 */
	st->sc_periph = periph;
	periph->periph_dev = &st->sc_dev;
	periph->periph_switch = &st_switch;

	/*
	 * Set initial flags
	 */

	st->flags = ST_INIT_FLAGS;

	/*
	 * Check if the drive is a known criminal and take
	 * Any steps needed to bring it into line
	 */
	st_identify_drive(st, &sa->sa_inqbuf);
	/*
	 * Use the subdriver to request information regarding the drive.
	 */
	printf("\n");
	printf("%s: %s", st->sc_dev.dv_xname, st->quirkdata ? "rogue, " : "");
	if (scsipi_test_unit_ready(periph,
	    XS_CTL_DISCOVERY | XS_CTL_SILENT | XS_CTL_IGNORE_MEDIA_CHANGE) ||
	    st->ops(st, ST_OPS_MODESENSE,
	    XS_CTL_DISCOVERY | XS_CTL_SILENT | XS_CTL_IGNORE_MEDIA_CHANGE))
		printf("drive empty\n");
	else {
		printf("density code %d, ", st->media_density);
		if (st->media_blksize > 0)
			printf("%d-byte", st->media_blksize);
		else
			printf("variable");
		printf(" blocks, write-%s\n",
		    (st->flags & ST_READONLY) ? "protected" : "enabled");
	}

	/*
	 * Set up the buf queue for this device
	 */
	BUFQ_INIT(&st->buf_queue);

#if NRND > 0
	rnd_attach_source(&st->rnd_source, st->sc_dev.dv_xname,
			  RND_TYPE_TAPE, 0);
#endif
}

int
stactivate(self, act)
	struct device *self;
	enum devact act;
{
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		/*
		 * Nothing to do; we key off the device's DVF_ACTIVE.
		 */
		break;
	}
	return (rv);
}

int
stdetach(self, flags)
	struct device *self;
	int flags;
{
	struct st_softc *st = (struct st_softc *)self;
	struct buf *bp;
	int s, bmaj, cmaj, mn;

	/* locate the major number */
	for (bmaj = 0; bmaj <= nblkdev; bmaj++)
		if (bdevsw[bmaj].d_open == stopen)
			break;
	for (cmaj = 0; cmaj <= nchrdev; cmaj++)
		if (cdevsw[cmaj].d_open == stopen)
			break;

	s = splbio();

	/* Kill off any queued buffers. */
	while ((bp = BUFQ_FIRST(&st->buf_queue)) != NULL) {
		BUFQ_REMOVE(&st->buf_queue, bp);
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}

	/* Kill off any pending commands. */
	scsipi_kill_pending(st->sc_periph);

	splx(s);

	/* Nuke the vnodes for any open instances */
	mn = STUNIT(self->dv_unit);
	vdevgone(bmaj, mn, mn+STNMINOR-1, VBLK);
	vdevgone(cmaj, mn, mn+STNMINOR-1, VCHR);


#if NRND > 0
	/* Unhook the entropy source. */
	rnd_detach_source(&st->rnd_source);
#endif

	return (0);
}

/*
 * Use the inquiry routine in 'scsi_base' to get drive info so we can
 * Further tailor our behaviour.
 */
void
st_identify_drive(st, inqbuf)
	struct st_softc *st;
	struct scsipi_inquiry_pattern *inqbuf;
{
	struct st_quirk_inquiry_pattern *finger;
	int priority;

	finger = (struct st_quirk_inquiry_pattern *)scsipi_inqmatch(inqbuf,
	    (caddr_t)st_quirk_patterns,
	    sizeof(st_quirk_patterns) / sizeof(st_quirk_patterns[0]),
	    sizeof(st_quirk_patterns[0]), &priority);
	if (priority != 0) {
		st->quirkdata = &finger->quirkdata;
		st->drive_quirks = finger->quirkdata.quirks;
		st->quirks = finger->quirkdata.quirks;	/* start value */
		st->page_0_size = finger->quirkdata.page_0_size;
		st_loadquirks(st);
	}
}

/*
 * initialise the subdevices to the default (QUIRK) state.
 * this will remove any setting made by the system operator or previous
 * operations.
 */
void
st_loadquirks(st)
	struct st_softc *st;
{
	int i;
	struct	modes *mode;
	struct	modes *mode2;

	mode = st->quirkdata->modes;
	mode2 = st->modes;
	for (i = 0; i < 4; i++) {
		memset(mode2, 0, sizeof(struct modes));
		st->modeflags[i] &= ~(BLKSIZE_SET_BY_QUIRK |
		    DENSITY_SET_BY_QUIRK | BLKSIZE_SET_BY_USER |
		    DENSITY_SET_BY_USER);
		if ((mode->quirks | st->drive_quirks) & ST_Q_FORCE_BLKSIZE) {
			mode2->blksize = mode->blksize;
			st->modeflags[i] |= BLKSIZE_SET_BY_QUIRK;
		}
		if (mode->density) {
			mode2->density = mode->density;
			st->modeflags[i] |= DENSITY_SET_BY_QUIRK;
		}
		mode2->quirks |= mode->quirks;
		mode++;
		mode2++;
	}
}

/*
 * open the device.
 */
int
stopen(dev, flags, mode, p)
	dev_t dev;
	int flags;
	int mode;
	struct proc *p;
{
	u_int stmode, dsty;
	int error, sflags, unit, tries, ntries;
	struct st_softc *st;
	struct scsipi_periph *periph;
	struct scsipi_adapter *adapt;

	unit = STUNIT(dev);
	if (unit >= st_cd.cd_ndevs)
		return (ENXIO);
	st = st_cd.cd_devs[unit];
	if (st == NULL)
		return (ENXIO);

	stmode = STMODE(dev);
	dsty = STDSTY(dev);

	periph = st->sc_periph;
	adapt = periph->periph_channel->chan_adapter;

	SC_DEBUG(periph, SCSIPI_DB1, ("open: dev=0x%x (unit %d (of %d))\n", dev,
	    unit, st_cd.cd_ndevs));


	/*
	 * Only allow one at a time
	 */
	if (periph->periph_flags & PERIPH_OPEN) {
		printf("%s: already open\n", st->sc_dev.dv_xname);
		return (EBUSY);
	}

	if ((error = scsipi_adapter_addref(adapt)) != 0)
		return (error);

	/*
	 * clear any latched errors.
	 */
	st->mt_resid = 0;
	st->mt_erreg = 0;
	st->asc = 0;
	st->ascq = 0;

	/*
	 * Catch any unit attention errors. Be silent about this
	 * unless we're already mounted. We ignore media change
	 * if we're in control mode or not mounted yet.
	 */
	if ((st->flags & ST_MOUNTED) == 0 || stmode == CTRL_MODE) {
#ifdef SCSIDEBUG
		sflags = XS_CTL_IGNORE_MEDIA_CHANGE;
#else
		sflags = XS_CTL_SILENT|XS_CTL_IGNORE_MEDIA_CHANGE;
#endif
	} else
		sflags = 0;
 
	/*
	 * If we're already mounted or we aren't configured for
	 * a mount delay, only try a test unit ready once. Otherwise,
	 * try up to ST_MOUNT_DELAY times with a rest interval of
	 * one second between each try.
	 */
 
	if ((st->flags & ST_MOUNTED) || ST_MOUNT_DELAY == 0) {
		ntries = 1;
	} else {
		ntries = ST_MOUNT_DELAY;
	}

	for (error = tries = 0; tries < ntries; tries++) {
		int slpintr, oflags;

		/*
		 * If we had no error, or we're opening the control mode
		 * device, we jump out right away.
		 */

		error = scsipi_test_unit_ready(periph, sflags);
		if (error == 0 || stmode == CTRL_MODE) {
			break;
		}

		/*
		 * We had an error.
		 *
		 * If we're already mounted or we aren't configured for
		 * a mount delay, or the error isn't a NOT READY error,
		 * skip to the error exit now.
		 */
		if ((st->flags & ST_MOUNTED) || ST_MOUNT_DELAY == 0 ||
		    (st->mt_key != SKEY_NOT_READY)) {
			goto bad;
		}

		/*
		 * clear any latched errors.
		 */
		st->mt_resid = 0;
		st->mt_erreg = 0;
		st->asc = 0;
		st->ascq = 0;

		/*
		 * Fake that we have the device open so
		 * we block other apps from getting in.
		 */

		oflags = periph->periph_flags;
		periph->periph_flags |= PERIPH_OPEN;

		slpintr = tsleep(&lbolt, PUSER|PCATCH, "stload", 0);

		periph->periph_flags = oflags;	/* restore flags */
		if (slpintr) {
			goto bad;
		}
	} 


	/*
	 * If the mode is 3 (e.g. minor = 3,7,11,15) then the device has
	 * been opened to set defaults and perform other, usually non-I/O
	 * related, operations. In this case, do a quick check to see
	 * whether the unit actually had a tape loaded (this will be known
	 * as to whether or not we got a NOT READY for the above
	 * unit attention). If a tape is there, go do a mount sequence.
	 */
	if (stmode == CTRL_MODE && st->mt_key == SKEY_NOT_READY) {
		periph->periph_flags |= PERIPH_OPEN;
		return (0);
	}

	/*
	 * If we get this far and had an error set, that means we failed
	 * to pass the 'test unit ready' test for the non-controlmode device,
	 * so we bounce the open.
	 */
 
	if (error)
		return (error);
 
	/*
	 * Else, we're now committed to saying we're open.
	 */
 
	periph->periph_flags |= PERIPH_OPEN; /* unit attn are now errors */
 
	/*
	 * If it's a different mode, or if the media has been
	 * invalidated, unmount the tape from the previous
	 * session but continue with open processing
	 */
	if (st->last_dsty != dsty ||
	    (periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)
		st_unmount(st, NOEJECT);

	/*
	 * If we are not mounted, then we should start a new
	 * mount session.
	 */
	if (!(st->flags & ST_MOUNTED)) {
		if ((error = st_mount_tape(dev, flags)) != 0)
			goto bad;
		st->last_dsty = dsty;
	}

	SC_DEBUG(periph, SCSIPI_DB2, ("open complete\n"));
	return (0);

bad:
	st_unmount(st, NOEJECT);
	scsipi_adapter_delref(adapt);
	periph->periph_flags &= ~PERIPH_OPEN;
	return (error);
}

/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
 */
int
stclose(dev, flags, mode, p)
	dev_t dev;
	int flags;
	int mode;
	struct proc *p;
{
	int stxx, error = 0;
	struct st_softc *st = st_cd.cd_devs[STUNIT(dev)];
	struct scsipi_periph *periph = st->sc_periph;
	struct scsipi_adapter *adapt = periph->periph_channel->chan_adapter;

	SC_DEBUG(st->sc_periph, SCSIPI_DB1, ("closing\n"));

	/*
	 * Make sure that a tape opened in write-only mode will have
	 * file marks written on it when closed, even if not written to.
	 *
	 * This is for SUN compatibility. Actually, the Sun way of
	 * things is to:
	 *
	 *	only write filemarks if there are fmks to be written and
	 *   		- open for write (possibly read/write)
	 *		- the last operation was a write
	 * 	or:
	 *		- opened for wronly
	 *		- no data was written (including filemarks)
	 */

	stxx = st->flags & (ST_WRITTEN | ST_FM_WRITTEN);
	if (((flags & FWRITE) && stxx == ST_WRITTEN) ||
	    ((flags & O_ACCMODE) == FWRITE && stxx == 0)) {
		int nm;
		error = st_check_eod(st, FALSE, &nm, 0);
	}

	switch (STMODE(dev)) {
	case NORMAL_MODE:
		st_unmount(st, NOEJECT);
		break;
	case NOREW_MODE:
	case CTRL_MODE:
		/*
		 * Leave mounted unless media seems to have been removed.
		 *
		 * Otherwise, if we're to terminate a tape with more than one
		 * filemark [ and because we're not rewinding here ], backspace
		 * one filemark so that later appends will see an unbroken
		 * sequence of:
		 *
		 *	file - FMK - file - FMK ... file - FMK FMK (EOM)
		 */
		if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
			st_unmount(st, NOEJECT);
		} else if (error == 0) {
			/*
			 * ST_WRITTEN was preserved from above.
			 *
			 * All we need to know here is:
			 *
			 *	Were we writing this tape and was the last
			 *	operation a write?
			 *
			 *	Are there supposed to be 2FM at EOD?
			 *
			 * If both statements are true, then we backspace
			 * one filemark.
			 */
			stxx |= (st->flags & ST_2FM_AT_EOD);
			if ((flags & FWRITE) != 0 &&
			    (stxx == (ST_2FM_AT_EOD|ST_WRITTEN))) {
				error = st_space(st, -1, SP_FILEMARKS, 0);
			}
		}
		break;
	case EJECT_MODE:
		st_unmount(st, EJECT);
		break;
	}

	scsipi_wait_drain(periph);

	scsipi_adapter_delref(adapt);
	periph->periph_flags &= ~PERIPH_OPEN;

	return (error);
}

/*
 * Start a new mount session.
 * Copy in all the default parameters from the selected device mode.
 * and try guess any that seem to be defaulted.
 */
int
st_mount_tape(dev, flags)
	dev_t dev;
	int flags;
{
	int unit;
	u_int dsty;
	struct st_softc *st;
	struct scsipi_periph *periph;
	int error = 0;

	unit = STUNIT(dev);
	dsty = STDSTY(dev);
	st = st_cd.cd_devs[unit];
	periph = st->sc_periph;

	if (st->flags & ST_MOUNTED)
		return (0);

	SC_DEBUG(periph, SCSIPI_DB1, ("mounting\n "));
	st->flags |= ST_NEW_MOUNT;
	st->quirks = st->drive_quirks | st->modes[dsty].quirks;
	/*
	 * If the media is new, then make sure we give it a chance to
	 * to do a 'load' instruction.  (We assume it is new.)
	 */
	if ((error = st_load(st, LD_LOAD, XS_CTL_SILENT)) != 0)
		return (error);
	/*
	 * Throw another dummy instruction to catch
	 * 'Unit attention' errors. Many drives give
	 * these after doing a Load instruction (with
	 * the MEDIUM MAY HAVE CHANGED asc/ascq).
	 */
	scsipi_test_unit_ready(periph, XS_CTL_SILENT);	/* XXX */

	/*
	 * Some devices can't tell you much until they have been
	 * asked to look at the media. This quirk does this.
	 */
	if (st->quirks & ST_Q_SENSE_HELP)
		if ((error = st_touch_tape(st)) != 0)
			return (error);
	/*
	 * Load the physical device parameters
	 * loads: blkmin, blkmax
	 */
	if ((error = st->ops(st, ST_OPS_RBL, 0)) != 0)
		return (error);
	/*
	 * Load the media dependent parameters
	 * includes: media_blksize,media_density,numblks
	 * As we have a tape in, it should be reflected here.
	 * If not you may need the "quirk" above.
	 */
	if ((error = st->ops(st, ST_OPS_MODESENSE, 0)) != 0)
		return (error);
	/*
	 * If we have gained a permanent density from somewhere,
	 * then use it in preference to the one supplied by
	 * default by the driver.
	 */
	if (st->modeflags[dsty] & (DENSITY_SET_BY_QUIRK | DENSITY_SET_BY_USER))
		st->density = st->modes[dsty].density;
	else
		st->density = st->media_density;
	/*
	 * If we have gained a permanent blocksize
	 * then use it in preference to the one supplied by
	 * default by the driver.
	 */
	st->flags &= ~ST_FIXEDBLOCKS;
	if (st->modeflags[dsty] &
	    (BLKSIZE_SET_BY_QUIRK | BLKSIZE_SET_BY_USER)) {
		st->blksize = st->modes[dsty].blksize;
		if (st->blksize)
			st->flags |= ST_FIXEDBLOCKS;
	} else {
		if ((error = st_decide_mode(st, FALSE)) != 0)
			return (error);
	}
	if ((error = st->ops(st, ST_OPS_MODESELECT, 0)) != 0) {
		/* ATAPI will return ENODEV for this, and this may be OK */
		if (error != ENODEV) {
			printf("%s: cannot set selected mode\n",
			    st->sc_dev.dv_xname);
			return (error);
		}
	}
	if (!(st->quirks & ST_Q_NOPREVENT)) {
		scsipi_prevent(periph, PR_PREVENT,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_NOT_READY);
	}
	st->flags &= ~ST_NEW_MOUNT;
	st->flags |= ST_MOUNTED;
	periph->periph_flags |= PERIPH_MEDIA_LOADED;	/* move earlier? */

	return (0);
}

/*
 * End the present mount session.
 * Rewind, and optionally eject the tape.
 * Reset various flags to indicate that all new
 * operations require another mount operation
 */
void
st_unmount(st, eject)
	struct st_softc *st;
	boolean eject;
{
	struct scsipi_periph *periph = st->sc_periph;
	int nmarks;

	if ((st->flags & ST_MOUNTED) == 0)
		return;
	SC_DEBUG(periph, SCSIPI_DB1, ("unmounting\n"));
	st_check_eod(st, FALSE, &nmarks, XS_CTL_IGNORE_NOT_READY);
	st_rewind(st, 0, XS_CTL_IGNORE_NOT_READY);

	/*
	 * Section 9.3.3 of the SCSI specs states that a device shall return
	 * the density value specified in the last succesfull MODE SELECT
	 * after an unload operation, in case it is not able to
	 * automatically determine the density of the new medium.
	 *
	 * So we instruct the device to use the default density, which will
	 * prevent the use of stale density values (in particular,
	 * in st_touch_tape().
	 */
	st->density = 0;
	if (st->ops(st, ST_OPS_MODESELECT, 0) != 0) {
		printf("%s: WARNING: cannot revert to default density\n",
			st->sc_dev.dv_xname);
	}

	scsipi_prevent(periph, PR_ALLOW,
	    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_NOT_READY);
	if (eject)
		st_load(st, LD_UNLOAD, XS_CTL_IGNORE_NOT_READY);
	st->flags &= ~(ST_MOUNTED | ST_NEW_MOUNT);
	periph->periph_flags &= ~PERIPH_MEDIA_LOADED;
}

/*
 * Given all we know about the device, media, mode, 'quirks' and
 * initial operation, make a decision as to how we should be set
 * to run (regarding blocking and EOD marks)
 */
int
st_decide_mode(st, first_read)
	struct st_softc *st;
	boolean	first_read;
{

	SC_DEBUG(st->sc_periph, SCSIPI_DB2, ("starting block mode decision\n"));

	/*
	 * If the drive can only handle fixed-length blocks and only at
	 * one size, perhaps we should just do that.
	 */
	if (st->blkmin && (st->blkmin == st->blkmax)) {
		st->flags |= ST_FIXEDBLOCKS;
		st->blksize = st->blkmin;
		SC_DEBUG(st->sc_periph, SCSIPI_DB3,
		    ("blkmin == blkmax of %d\n", st->blkmin));
		goto done;
	}
	/*
	 * If the tape density mandates (or even suggests) use of fixed
	 * or variable-length blocks, comply.
	 */
	switch (st->density) {
	case HALFINCH_800:
	case HALFINCH_1600:
	case HALFINCH_6250:
	case DDS:
		st->flags &= ~ST_FIXEDBLOCKS;
		st->blksize = 0;
		SC_DEBUG(st->sc_periph, SCSIPI_DB3,
		    ("density specified variable\n"));
		goto done;
	case QIC_11:
	case QIC_24:
	case QIC_120:
	case QIC_150:
	case QIC_525:
	case QIC_1320:
	case QIC_3095:
	case QIC_3220:
		st->flags |= ST_FIXEDBLOCKS;
		if (st->media_blksize > 0)
			st->blksize = st->media_blksize;
		else
			st->blksize = DEF_FIXED_BSIZE;
		SC_DEBUG(st->sc_periph, SCSIPI_DB3,
		    ("density specified fixed\n"));
		goto done;
	}
	/*
	 * If we're about to read the tape, perhaps we should choose
	 * fixed or variable-length blocks and block size according to
	 * what the drive found on the tape.
	 */
	if (first_read &&
	    (!(st->quirks & ST_Q_BLKSIZE) || (st->media_blksize == 0) ||
	    (st->media_blksize == DEF_FIXED_BSIZE) ||
	    (st->media_blksize == 1024))) {
		if (st->media_blksize > 0)
			st->flags |= ST_FIXEDBLOCKS;
		else
			st->flags &= ~ST_FIXEDBLOCKS;
		st->blksize = st->media_blksize;
		SC_DEBUG(st->sc_periph, SCSIPI_DB3,
		    ("Used media_blksize of %d\n", st->media_blksize));
		goto done;
	}
	/*
	 * We're getting no hints from any direction.  Choose variable-
	 * length blocks arbitrarily.
	 */
	st->flags &= ~ST_FIXEDBLOCKS;
	st->blksize = 0;
	SC_DEBUG(st->sc_periph, SCSIPI_DB3,
	    ("Give up and default to variable mode\n"));

done:
	/*
	 * Decide whether or not to write two file marks to signify end-
	 * of-data.  Make the decision as a function of density.  If
	 * the decision is not to use a second file mark, the SCSI BLANK
	 * CHECK condition code will be recognized as end-of-data when
	 * first read.
	 * (I think this should be a by-product of fixed/variable..julian)
	 */
	switch (st->density) {
/*      case 8 mm:   What is the SCSI density code for 8 mm, anyway? */
	case QIC_11:
	case QIC_24:
	case QIC_120:
	case QIC_150:
	case QIC_525:
	case QIC_1320:
	case QIC_3095:
	case QIC_3220:
		st->flags &= ~ST_2FM_AT_EOD;
		break;
	default:
		st->flags |= ST_2FM_AT_EOD;
	}
	return (0);
}

/*
 * Actually translate the requested transfer into
 * one the physical driver can understand
 * The transfer is described by a buf and will include
 * only one physical transfer.
 */
void
ststrategy(bp)
	struct buf *bp;
{
	struct st_softc *st = st_cd.cd_devs[STUNIT(bp->b_dev)];
	int s;

	SC_DEBUG(st->sc_periph, SCSIPI_DB1,
	    ("ststrategy %ld bytes @ blk %d\n", bp->b_bcount, bp->b_blkno));
	/*
	 * If it's a null transfer, return immediatly
	 */
	if (bp->b_bcount == 0)
		goto done;

	/* If offset is negative, error */
	if (bp->b_blkno < 0) {
		bp->b_error = EINVAL;
		goto bad;
	}

	/*
	 * Odd sized request on fixed drives are verboten
	 */
	if (st->flags & ST_FIXEDBLOCKS) {
		if (bp->b_bcount % st->blksize) {
			printf("%s: bad request, must be multiple of %d\n",
			    st->sc_dev.dv_xname, st->blksize);
			bp->b_error = EIO;
			goto bad;
		}
	}
	/*
	 * as are out-of-range requests on variable drives.
	 */
	else if (bp->b_bcount < st->blkmin ||
	    (st->blkmax && bp->b_bcount > st->blkmax)) {
		printf("%s: bad request, must be between %d and %d\n",
		    st->sc_dev.dv_xname, st->blkmin, st->blkmax);
		bp->b_error = EIO;
		goto bad;
	}
	s = splbio();

	/*
	 * Place it in the queue of activities for this tape
	 * at the end (a bit silly because we only have on user..
	 * (but it could fork()))
	 */
	BUFQ_INSERT_TAIL(&st->buf_queue, bp);

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion
	 * (All a bit silly if we're only allowing 1 open but..)
	 */
	ststart(st->sc_periph);

	splx(s);
	return;
bad:
	bp->b_flags |= B_ERROR;
done:
	/*
	 * Correctly set the buf to indicate a completed xfer
	 */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
	return;
}

/*
 * ststart looks to see if there is a buf waiting for the device
 * and that the device is not already busy. If both are true,
 * It dequeues the buf and creates a scsi command to perform the
 * transfer required. The transfer request will call scsipi_done
 * on completion, which will in turn call this routine again
 * so that the next queued transfer is performed.
 * The bufs are queued by the strategy routine (ststrategy)
 *
 * This routine is also called after other non-queued requests
 * have been made of the scsi driver, to ensure that the queue
 * continues to be drained.
 * ststart() is called at splbio
 */
void
ststart(periph)
	struct scsipi_periph *periph;
{
	struct st_softc *st = (void *)periph->periph_dev;
	struct buf *bp;
	struct scsi_rw_tape cmd;
	int flags, error;

	SC_DEBUG(periph, SCSIPI_DB2, ("ststart "));
	/*
	 * See if there is a buf to do and we are not already
	 * doing one
	 */
	while (periph->periph_active < periph->periph_openings) {
		/* if a special awaits, let it proceed first */
		if (periph->periph_flags & PERIPH_WAITING) {
			periph->periph_flags &= ~PERIPH_WAITING;
			wakeup((caddr_t)periph);
			return;
		}

		if ((bp = BUFQ_FIRST(&st->buf_queue)) == NULL)
			return;
		BUFQ_REMOVE(&st->buf_queue, bp);

		/*
		 * If the device has been unmounted by the user
		 * then throw away all requests until done.
		 */
		if ((st->flags & ST_MOUNTED) == 0 ||
		    (periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
			/* make sure that one implies the other.. */
			periph->periph_flags &= ~PERIPH_MEDIA_LOADED;
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			bp->b_resid = bp->b_bcount;
			biodone(bp);
			continue;
		}
		/*
		 * only FIXEDBLOCK devices have pending I/O or space operations.
		 */
		if (st->flags & ST_FIXEDBLOCKS) {
			/*
			 * If we are at a filemark but have not reported it yet
			 * then we should report it now
			 */
			if (st->flags & ST_AT_FILEMARK) {
				if ((bp->b_flags & B_READ) == B_WRITE) {
					/*
					 * Handling of ST_AT_FILEMARK in
					 * st_space will fill in the right file
					 * mark count.
					 * Back up over filemark
					 */
					if (st_space(st, 0, SP_FILEMARKS, 0)) {
						bp->b_flags |= B_ERROR;
						bp->b_error = EIO;
						biodone(bp);
						continue;
					}
				} else {
					bp->b_resid = bp->b_bcount;
					bp->b_error = 0;
					bp->b_flags &= ~B_ERROR;
					st->flags &= ~ST_AT_FILEMARK;
					biodone(bp);
					continue;	/* seek more work */
				}
			}
		}
		/*
		 * If we are at EOM but have not reported it
		 * yet then we should report it now.
		 */
		if (st->flags & (ST_EOM_PENDING|ST_EIO_PENDING)) {
			bp->b_resid = bp->b_bcount;
			if (st->flags & ST_EIO_PENDING) {
				bp->b_error = EIO;
				bp->b_flags |= B_ERROR;
			}
			st->flags &= ~(ST_EOM_PENDING|ST_EIO_PENDING);
			biodone(bp);
			continue;	/* seek more work */
		}

		/*
		 * Fill out the scsi command
		 */
		memset(&cmd, 0, sizeof(cmd));
		flags = XS_CTL_NOSLEEP | XS_CTL_ASYNC;
		if ((bp->b_flags & B_READ) == B_WRITE) {
			cmd.opcode = WRITE;
			st->flags &= ~ST_FM_WRITTEN;
			flags |= XS_CTL_DATA_OUT;
		} else {
			cmd.opcode = READ;
			flags |= XS_CTL_DATA_IN;
		}

		/*
		 * Handle "fixed-block-mode" tape drives by using the
		 * block count instead of the length.
		 */
		if (st->flags & ST_FIXEDBLOCKS) {
			cmd.byte2 |= SRW_FIXED;
			_lto3b(bp->b_bcount / st->blksize, cmd.len);
		} else
			_lto3b(bp->b_bcount, cmd.len);

		/*
		 * go ask the adapter to do all this for us
		 */
		error = scsipi_command(periph,
		    (struct scsipi_generic *)&cmd, sizeof(cmd),
		    (u_char *)bp->b_data, bp->b_bcount,
		    0, ST_IO_TIME, bp, flags);
		if (error) {
			printf("%s: not queued, error %d\n",
			    st->sc_dev.dv_xname, error);
		}
	} /* go back and see if we can cram more work in.. */
}

void
stdone(xs)
	struct scsipi_xfer *xs;
{
	struct st_softc *st = (void *)xs->xs_periph->periph_dev;

	if (xs->bp != NULL) {
		if ((xs->bp->b_flags & B_READ) == B_WRITE) {
			st->flags |= ST_WRITTEN;
		} else {
			st->flags &= ~ST_WRITTEN;
		}
#if NRND > 0
		rnd_add_uint32(&st->rnd_source, xs->bp->b_blkno);
#endif
	}
}

int
stread(dev, uio, iomode)
	dev_t dev;
	struct uio *uio;
	int iomode;
{
	struct st_softc *st = st_cd.cd_devs[STUNIT(dev)];

	return (physio(ststrategy, NULL, dev, B_READ,
	    st->sc_periph->periph_channel->chan_adapter->adapt_minphys, uio));
}

int
stwrite(dev, uio, iomode)
	dev_t dev;
	struct uio *uio;
	int iomode;
{
	struct st_softc *st = st_cd.cd_devs[STUNIT(dev)];

	return (physio(ststrategy, NULL, dev, B_WRITE,
	    st->sc_periph->periph_channel->chan_adapter->adapt_minphys, uio));
}

/*
 * Perform special action on behalf of the user;
 * knows about the internals of this device
 */
int
stioctl(dev, cmd, arg, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t arg;
	int flag;
	struct proc *p;
{
	int error = 0;
	int unit;
	int number, nmarks, dsty;
	int flags;
	struct st_softc *st;
	int hold_blksize;
	u_int8_t hold_density;
	struct mtop *mt = (struct mtop *) arg;

	/*
	 * Find the device that the user is talking about
	 */
	flags = 0;		/* give error messages, act on errors etc. */
	unit = STUNIT(dev);
	dsty = STDSTY(dev);
	st = st_cd.cd_devs[unit];
	hold_blksize = st->blksize;
	hold_density = st->density;

	switch ((u_int) cmd) {

	case MTIOCGET: {
		struct mtget *g = (struct mtget *) arg;
		/*
		 * (to get the current state of READONLY)
		 */
		error = st->ops(st, ST_OPS_MODESENSE, XS_CTL_SILENT);
		if (error) {
			/*
			 * Ignore the error if in control mode;
			 * this is mandated by st(4).
			 */
			if (STMODE(dev) != CTRL_MODE)
				break;
			error = 0;
		}
		SC_DEBUG(st->sc_periph, SCSIPI_DB1, ("[ioctl: get status]\n"));
		memset(g, 0, sizeof(struct mtget));
		g->mt_type = 0x7;	/* Ultrix compat *//*? */
		g->mt_blksiz = st->blksize;
		g->mt_density = st->density;
		g->mt_mblksiz[0] = st->modes[0].blksize;
		g->mt_mblksiz[1] = st->modes[1].blksize;
		g->mt_mblksiz[2] = st->modes[2].blksize;
		g->mt_mblksiz[3] = st->modes[3].blksize;
		g->mt_mdensity[0] = st->modes[0].density;
		g->mt_mdensity[1] = st->modes[1].density;
		g->mt_mdensity[2] = st->modes[2].density;
		g->mt_mdensity[3] = st->modes[3].density;
		if (st->flags & ST_READONLY)
			g->mt_dsreg |= MT_DS_RDONLY;
		if (st->flags & ST_MOUNTED)
			g->mt_dsreg |= MT_DS_MOUNTED;
		g->mt_resid = st->mt_resid;
		g->mt_erreg = st->mt_erreg;
		/*
		 * clear latched errors.
		 */
		st->mt_resid = 0;
		st->mt_erreg = 0;
		st->asc = 0;
		st->ascq = 0;
		break;
	}
	case MTIOCTOP: {

		SC_DEBUG(st->sc_periph, SCSIPI_DB1,
		    ("[ioctl: op=0x%x count=0x%x]\n", mt->mt_op,
			mt->mt_count));

		/* compat: in U*x it is a short */
		number = mt->mt_count;
		switch ((short) (mt->mt_op)) {
		case MTWEOF:	/* write an end-of-file record */
			error = st_write_filemarks(st, number, flags);
			break;
		case MTBSF:	/* backward space file */
			number = -number;
		case MTFSF:	/* forward space file */
			error = st_check_eod(st, FALSE, &nmarks, flags);
			if (!error)
				error = st_space(st, number - nmarks,
				    SP_FILEMARKS, flags);
			break;
		case MTBSR:	/* backward space record */
			number = -number;
		case MTFSR:	/* forward space record */
			error = st_check_eod(st, TRUE, &nmarks, flags);
			if (!error)
				error = st_space(st, number, SP_BLKS, flags);
			break;
		case MTREW:	/* rewind */
			error = st_rewind(st, 0, flags);
			break;
		case MTOFFL:	/* rewind and put the drive offline */
			st_unmount(st, EJECT);
			break;
		case MTNOP:	/* no operation, sets status only */
			break;
		case MTRETEN:	/* retension the tape */
			error = st_load(st, LD_RETENSION, flags);
			if (!error)
				error = st_load(st, LD_LOAD, flags);
			break;
		case MTEOM:	/* forward space to end of media */
			error = st_check_eod(st, FALSE, &nmarks, flags);
			if (!error)
				error = st_space(st, 1, SP_EOM, flags);
			break;
		case MTCACHE:	/* enable controller cache */
			st->flags &= ~ST_DONTBUFFER;
			goto try_new_value;
		case MTNOCACHE:	/* disable controller cache */
			st->flags |= ST_DONTBUFFER;
			goto try_new_value;
		case MTERASE:	/* erase volume */
			error = st_erase(st, number, flags);
			break;
		case MTSETBSIZ:	/* Set block size for device */
#ifdef	NOTYET
			if (!(st->flags & ST_NEW_MOUNT)) {
				uprintf("re-mount tape before changing blocksize");
				error = EINVAL;
				break;
			}
#endif
			if (number == 0)
				st->flags &= ~ST_FIXEDBLOCKS;
			else {
				if ((st->blkmin || st->blkmax) &&
				    (number < st->blkmin ||
				    number > st->blkmax)) {
					error = EINVAL;
					break;
				}
				st->flags |= ST_FIXEDBLOCKS;
			}
			st->blksize = number;
			st->flags |= ST_BLOCK_SET;	/*XXX */
			goto try_new_value;

		case MTSETDNSTY:	/* Set density for device and mode */
			/*
			 * Any number >= 0 and <= 0xff is legal. Numbers
			 * above 0x80 are 'vendor unique'.
			 */
			if (number < 0 || number > 255) {
				error = EINVAL;
				break;
			} else
				st->density = number;
			goto try_new_value;

		case MTCMPRESS:
			error = st->ops(st, (number == 0) ?
			    ST_OPS_CMPRSS_OFF : ST_OPS_CMPRSS_ON,
			    XS_CTL_SILENT);
			break;

		case MTEWARN:
			if (number)
				st->flags |= ST_EARLYWARN;
			else
				st->flags &= ~ST_EARLYWARN;
			break;

		default:
			error = EINVAL;
		}
		break;
	}
	case MTIOCIEOT:
	case MTIOCEEOT:
		break;

	case MTIOCRDSPOS:
		error = st_rdpos(st, 0, (u_int32_t *) arg);
		break;

	case MTIOCRDHPOS:
		error = st_rdpos(st, 1, (u_int32_t *) arg);
		break;

	case MTIOCSLOCATE:
		error = st_setpos(st, 0, (u_int32_t *) arg);
		break;

	case MTIOCHLOCATE:
		error = st_setpos(st, 1, (u_int32_t *) arg);
		break;


	default:
		error = scsipi_do_ioctl(st->sc_periph, dev, cmd, arg,
					flag, p);
		break;
	}
	return (error);
/*-----------------------------*/
try_new_value:
	/*
	 * Check that the mode being asked for is aggreeable to the
	 * drive. If not, put it back the way it was.
	 *
	 * If in control mode, we can make (persistent) mode changes
	 * even if no medium is loaded (see st(4)).
	 */
	if ((STMODE(dev) != CTRL_MODE || (st->flags & ST_MOUNTED) != 0) &&
	    (error = st->ops(st, ST_OPS_MODESELECT, 0)) != 0) {
		/* put it back as it was */
		printf("%s: cannot set selected mode\n", st->sc_dev.dv_xname);
		st->density = hold_density;
		st->blksize = hold_blksize;
		if (st->blksize)
			st->flags |= ST_FIXEDBLOCKS;
		else
			st->flags &= ~ST_FIXEDBLOCKS;
		return (error);
	}
	/*
	 * As the drive liked it, if we are setting a new default,
	 * set it into the structures as such.
	 *
	 * The means for deciding this are not finalised yet- but
	 * if the device was opened in Control Mode, the values
	 * are persistent now across mounts.
	 */
	if (STMODE(dev) == CTRL_MODE) {
		switch ((short) (mt->mt_op)) {
		case MTSETBSIZ:
			st->modes[dsty].blksize = st->blksize;
			st->modeflags[dsty] |= BLKSIZE_SET_BY_USER;
			break;
		case MTSETDNSTY:
			st->modes[dsty].density = st->density;
			st->modeflags[dsty] |= DENSITY_SET_BY_USER;
			break;
		}
	}
	return (0);
}

/*
 * Do a synchronous read.
 */
int
st_read(st, buf, size, flags)
	struct st_softc *st;
	int size;
	int flags;
	char *buf;
{
	struct scsi_rw_tape cmd;

	/*
	 * If it's a null transfer, return immediatly
	 */
	if (size == 0)
		return (0);
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = READ;
	if (st->flags & ST_FIXEDBLOCKS) {
		cmd.byte2 |= SRW_FIXED;
		_lto3b(size / (st->blksize ? st->blksize : DEF_FIXED_BSIZE),
		    cmd.len);
	} else
		_lto3b(size, cmd.len);
	return (scsipi_command(st->sc_periph,
	    (struct scsipi_generic *)&cmd, sizeof(cmd),
	    (u_char *)buf, size, 0, ST_IO_TIME, NULL, flags | XS_CTL_DATA_IN));
}

/*
 * issue an erase command
 */
int
st_erase(st, full, flags)
	struct st_softc *st;
	int full, flags;
{
	int tmo;
	struct scsi_erase cmd;

	/*
	 * Full erase means set LONG bit in erase command, which asks
	 * the drive to erase the entire unit.  Without this bit, we're
	 * asking the drive to write an erase gap.
	 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = ERASE;
	if (full) {
		cmd.byte2 = SE_LONG;
		tmo = ST_SPC_TIME;
	} else {
		tmo = ST_IO_TIME;
	}

	/*
	 * XXX We always do this asynchronously, for now, unless the device
	 * has the ST_Q_ERASE_NOIMM quirk.  How long should we wait if we
	 * want to (eventually) to it synchronously?
	 */
	if ((st->quirks & ST_Q_ERASE_NOIMM) == 0)
		cmd.byte2 |= SE_IMMED;

	return (scsipi_command(st->sc_periph,
	    (struct scsipi_generic *)&cmd, sizeof(cmd),
	    0, 0, ST_RETRIES, tmo, NULL, flags));
}

/*
 * skip N blocks/filemarks/seq filemarks/eom
 */
int
st_space(st, number, what, flags)
	struct st_softc *st;
	u_int what;
	int flags;
	int number;
{
	struct scsi_space cmd;
	int error;

	switch (what) {
	case SP_BLKS:
		if (st->flags & ST_PER_ACTION) {
			if (number > 0) {
				st->flags &= ~ST_PER_ACTION;
				return (EIO);
			} else if (number < 0) {
				if (st->flags & ST_AT_FILEMARK) {
					/*
					 * Handling of ST_AT_FILEMARK
					 * in st_space will fill in the
					 * right file mark count.
					 */
					error = st_space(st, 0, SP_FILEMARKS,
					    flags);
					if (error)
						return (error);
				}
				if (st->flags & ST_BLANK_READ) {
					st->flags &= ~ST_BLANK_READ;
					return (EIO);
				}
				st->flags &= ~(ST_EIO_PENDING|ST_EOM_PENDING);
			}
		}
		break;
	case SP_FILEMARKS:
		if (st->flags & ST_EIO_PENDING) {
			if (number > 0) {
				/* pretend we just discovered the error */
				st->flags &= ~ST_EIO_PENDING;
				return (EIO);
			} else if (number < 0) {
				/* back away from the error */
				st->flags &= ~ST_EIO_PENDING;
			}
		}
		if (st->flags & ST_AT_FILEMARK) {
			st->flags &= ~ST_AT_FILEMARK;
			number--;
		}
		if ((st->flags & ST_BLANK_READ) && (number < 0)) {
			/* back away from unwritten tape */
			st->flags &= ~ST_BLANK_READ;
			number++;	/* XXX dubious */
		}
		break;
	case SP_EOM:
		if (st->flags & ST_EOM_PENDING) {
			/* we're already there */
			st->flags &= ~ST_EOM_PENDING;
			return (0);
		}
		if (st->flags & ST_EIO_PENDING) {
			/* pretend we just discovered the error */
			st->flags &= ~ST_EIO_PENDING;
			return (EIO);
		}
		if (st->flags & ST_AT_FILEMARK)
			st->flags &= ~ST_AT_FILEMARK;
		break;
	}
	if (number == 0)
		return (0);

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SPACE;
	cmd.byte2 = what;
	_lto3b(number, cmd.number);

	return (scsipi_command(st->sc_periph,
	    (struct scsipi_generic *)&cmd, sizeof(cmd),
	    0, 0, 0, ST_SPC_TIME, NULL, flags));
}

/*
 * write N filemarks
 */
int
st_write_filemarks(st, number, flags)
	struct st_softc *st;
	int flags;
	int number;
{
	struct scsi_write_filemarks cmd;

	/*
	 * It's hard to write a negative number of file marks.
	 * Don't try.
	 */
	if (number < 0)
		return (EINVAL);
	switch (number) {
	case 0:		/* really a command to sync the drive's buffers */
		break;
	case 1:
		if (st->flags & ST_FM_WRITTEN)	/* already have one down */
			st->flags &= ~ST_WRITTEN;
		else
			st->flags |= ST_FM_WRITTEN;
		st->flags &= ~ST_PER_ACTION;
		break;
	default:
		st->flags &= ~(ST_PER_ACTION | ST_WRITTEN);
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = WRITE_FILEMARKS;
	if (scsipi_periph_bustype(st->sc_periph) == SCSIPI_BUSTYPE_ATAPI)
		cmd.byte2 = SR_IMMED;
	/*
	 * The ATAPI Onstream DI-30 doesn't support writing filemarks, but
	 * WRITE_FILEMARKS is still used to flush the buffer
	 */
	if ((st->quirks & ST_Q_NOFILEMARKS) == 0)
		_lto3b(number, cmd.number);

	return (scsipi_command(st->sc_periph,
	    (struct scsipi_generic *)&cmd, sizeof(cmd),
	    0, 0, 0, ST_IO_TIME * 4, NULL, flags));
}

/*
 * Make sure the right number of file marks is on tape if the
 * tape has been written.  If the position argument is true,
 * leave the tape positioned where it was originally.
 *
 * nmarks returns the number of marks to skip (or, if position
 * true, which were skipped) to get back original position.
 */
int
st_check_eod(st, position, nmarks, flags)
	struct st_softc *st;
	boolean position;
	int *nmarks;
	int flags;
{
	int error;

	switch (st->flags & (ST_WRITTEN | ST_FM_WRITTEN | ST_2FM_AT_EOD)) {
	default:
		*nmarks = 0;
		return (0);
	case ST_WRITTEN:
	case ST_WRITTEN | ST_FM_WRITTEN | ST_2FM_AT_EOD:
		*nmarks = 1;
		break;
	case ST_WRITTEN | ST_2FM_AT_EOD:
		*nmarks = 2;
	}
	error = st_write_filemarks(st, *nmarks, flags);
	if (position && !error)
		error = st_space(st, -*nmarks, SP_FILEMARKS, flags);
	return (error);
}

/*
 * load/unload/retension
 */
int
st_load(st, type, flags)
	struct st_softc *st;
	u_int type;
	int flags;
{
	int error;
	struct scsi_load cmd;

	if (type != LD_LOAD) {
		int nmarks;

		error = st_check_eod(st, FALSE, &nmarks, flags);
		if (error) {
			printf("%s: failed to write closing filemarks at "
			    "unload, errno=%d\n", st->sc_dev.dv_xname, error);
			return (error);
		}
	}
	if (st->quirks & ST_Q_IGNORE_LOADS) {
		if (type == LD_LOAD) {
			/*
			 * If we ignore loads, at least we should try a rewind.
			 */
			return st_rewind(st, 0, flags);
		}
		/* otherwise, we should do what's asked of us */
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = LOAD;
	if (scsipi_periph_bustype(st->sc_periph) == SCSIPI_BUSTYPE_ATAPI)
		cmd.byte2 = SR_IMMED;
	cmd.how = type;

	error = scsipi_command(st->sc_periph,
	    (struct scsipi_generic *)&cmd, sizeof(cmd),
	    0, 0, ST_RETRIES, ST_SPC_TIME, NULL, flags);
	if (error) {
		printf("%s: error %d in st_load (op %d)\n",
		    st->sc_dev.dv_xname, error, type);
	}
	return (error);
}

/*
 *  Rewind the device
 */
int
st_rewind(st, immediate, flags)
	struct st_softc *st;
	u_int immediate;
	int flags;
{
	struct scsi_rewind cmd;
	int error;
	int nmarks;

	error = st_check_eod(st, FALSE, &nmarks, flags);
	if (error) {
		printf("%s: failed to write closing filemarks at "
		    "rewind, errno=%d\n", st->sc_dev.dv_xname, error);
		return (error);
	}
	st->flags &= ~ST_PER_ACTION;

	/*
	 * ATAPI tapes always need immediate to be set
	 */
	if (scsipi_periph_bustype(st->sc_periph) == SCSIPI_BUSTYPE_ATAPI)
		immediate = SR_IMMED;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = REWIND;
	cmd.byte2 = immediate;

	error = scsipi_command(st->sc_periph,
	    (struct scsipi_generic *)&cmd, sizeof(cmd), 0, 0, ST_RETRIES,
	    immediate ? ST_CTL_TIME: ST_SPC_TIME, NULL, flags);
	if (error) {
		printf("%s: error %d trying to rewind\n",
		    st->sc_dev.dv_xname, error);
	}
	return (error);
}

int
st_rdpos(st, hard, blkptr)
	struct st_softc *st;
	int hard;
	u_int32_t *blkptr;
{
	int error;
	u_int8_t posdata[20];
	struct scsi_tape_read_position cmd;

	/*
	 * First flush any pending writes...
	 */
	error = st_write_filemarks(st, 0, XS_CTL_SILENT);

	/*
	 * The latter case is for 'write protected' tapes
	 * which are too stupid to recognize a zero count
	 * for writing filemarks as a no-op.
	 */
	if (error != 0 && error != EACCES && error != EROFS)
		return (error);

	memset(&cmd, 0, sizeof(cmd));
	memset(&posdata, 0, sizeof(posdata));
	cmd.opcode = READ_POSITION;
	if (hard)
		cmd.byte1 = 1;

	error = scsipi_command(st->sc_periph,
	    (struct scsipi_generic *)&cmd, sizeof(cmd), (u_char *)&posdata,
	    sizeof(posdata), ST_RETRIES, ST_CTL_TIME, NULL,
	    XS_CTL_SILENT | XS_CTL_DATA_IN | XS_CTL_DATA_ONSTACK);

	if (error == 0) {
#if	0
		printf("posdata:");
		for (hard = 0; hard < sizeof(posdata); hard++)
			printf("%02x ", posdata[hard] & 0xff);
		printf("\n");
#endif
		if (posdata[0] & 0x4)	/* Block Position Unknown */
			error = EINVAL;
		else
			*blkptr = _4btol(&posdata[4]);
	}
	return (error);
}

int
st_setpos(st, hard, blkptr)
	struct st_softc *st;
	int hard;
	u_int32_t *blkptr;
{
	int error;
	struct scsi_tape_locate cmd;

	/*
	 * First flush any pending writes. Strictly speaking,
	 * we're not supposed to have to worry about this,
	 * but let's be untrusting.
	 */
	error = st_write_filemarks(st, 0, XS_CTL_SILENT);

	/*
	 * The latter case is for 'write protected' tapes
	 * which are too stupid to recognize a zero count
	 * for writing filemarks as a no-op.
	 */
	if (error != 0 && error != EACCES && error != EROFS)
		return (error);

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = LOCATE;
	if (hard)
		cmd.byte2 = 1 << 2;
	_lto4b(*blkptr, cmd.blkaddr);
	error = scsipi_command(st->sc_periph,
		(struct scsipi_generic *)&cmd, sizeof(cmd),
		NULL, 0, ST_RETRIES, ST_SPC_TIME, NULL, 0);
	/*
	 * XXX: Note file && block number position now unknown (if
	 * XXX: these things ever start being maintained in this driver)
	 */
	return (error);
}


/*
 * Look at the returned sense and act on the error and determine
 * the unix error number to pass back..., 0 (== report no error),
 * -1 = retry the operation, -2 continue error processing.
 */
int
st_interpret_sense(xs)
	struct scsipi_xfer *xs;
{
	struct scsipi_periph *periph = xs->xs_periph;
	struct scsipi_sense_data *sense = &xs->sense.scsi_sense;
	struct buf *bp = xs->bp;
	struct st_softc *st = (void *)periph->periph_dev;
	int retval = EJUSTRETURN;
	int doprint = ((xs->xs_control & XS_CTL_SILENT) == 0);
	u_int8_t key;
	int32_t info;

	/*
	 * If it isn't a extended or extended/deferred error, let
	 * the generic code handle it.
	 */
	if ((sense->error_code & SSD_ERRCODE) != 0x70 &&
	    (sense->error_code & SSD_ERRCODE) != 0x71) {	/* DEFFERRED */
		return (retval);
	}

	if (sense->error_code & SSD_ERRCODE_VALID)
		info = _4btol(sense->info);
	else
		info = (st->flags & ST_FIXEDBLOCKS) ?
		    xs->datalen / st->blksize : xs->datalen;
	key = sense->flags & SSD_KEY;
	st->mt_erreg = key;
	st->asc = sense->add_sense_code;
	st->ascq = sense->add_sense_code_qual;
	st->mt_resid = (short) info;

	if (key == SKEY_NOT_READY && st->asc == 0x4 && st->ascq == 0x1) {
		/* Not Ready, Logical Unit Is in Process Of Becoming Ready */
		scsipi_periph_freeze(periph, 1);
		callout_reset(&periph->periph_callout,
		    hz, scsipi_periph_timed_thaw, periph);
		return (ERESTART);
	}

	/*
	 * If the device is not open yet, let generic handle
	 */
	if ((periph->periph_flags & PERIPH_OPEN) == 0) {
		return (retval);
	}


	if (st->flags & ST_FIXEDBLOCKS) {
		xs->resid = info * st->blksize;
		if (sense->flags & SSD_EOM) {
			if ((st->flags & ST_EARLYWARN) == 0)
				st->flags |= ST_EIO_PENDING;
			st->flags |= ST_EOM_PENDING;
			if (bp)
				bp->b_resid = xs->resid;
		}
		if (sense->flags & SSD_FILEMARK) {
			st->flags |= ST_AT_FILEMARK;
			if (bp)
				bp->b_resid = xs->resid;
		}
		if (sense->flags & SSD_ILI) {
			st->flags |= ST_EIO_PENDING;
			if (bp)
				bp->b_resid = xs->resid;
			if (sense->error_code & SSD_ERRCODE_VALID &&
			    (xs->xs_control & XS_CTL_SILENT) == 0)
				printf("%s: block wrong size, %d blocks "
				    "residual\n", st->sc_dev.dv_xname, info);

			/*
			 * This quirk code helps the drive read
			 * the first tape block, regardless of
			 * format.  That is required for these
			 * drives to return proper MODE SENSE
			 * information.
			 */
			if ((st->quirks & ST_Q_SENSE_HELP) &&
			    (periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)
				st->blksize -= 512;
		}
		/*
		 * If data wanted and no data was transferred, do it immediately
		 */
		if (xs->datalen && xs->resid >= xs->datalen) {
			if (st->flags & ST_EIO_PENDING)
				return (EIO);
			if (st->flags & ST_AT_FILEMARK) {
				if (bp)
					bp->b_resid = xs->resid;
				return (0);
			}
		}
	} else {		/* must be variable mode */
		if (sense->flags & SSD_EOM) {
			/*
			 * The current semantics of this
			 * driver requires EOM detection
			 * to return EIO unless early
			 * warning detection is enabled
			 * for variable mode (this is always
			 * on for fixed block mode).
			 */
			if (st->flags & ST_EARLYWARN) {
				st->flags |= ST_EOM_PENDING;
				retval = 0;
			} else {
				retval = EIO;
			}

			/*
			 * If it's an unadorned EOM detection,
			 * suppress printing an error.
			 */
			if (key == SKEY_NO_SENSE) {
				doprint = 0;
			}
		} else if (sense->flags & SSD_FILEMARK) {
			retval = 0;
		} else if (sense->flags & SSD_ILI) {
			if (info < 0) {
				/*
				 * The tape record was bigger than the read
				 * we issued.
				 */
				if ((xs->xs_control & XS_CTL_SILENT) == 0) {
					printf("%s: %d-byte tape record too big"
					    " for %d-byte user buffer\n",
					    st->sc_dev.dv_xname,
					    xs->datalen - info, xs->datalen);
				}
				retval = EIO;
			} else {
				retval = 0;
			}
		}
		xs->resid = info;
		if (bp)
			bp->b_resid = info;
	}

#ifndef SCSIPI_DEBUG
	if (retval == 0 && key == SKEY_NO_SENSE)
		doprint = 0;
#endif
	if (key == SKEY_BLANK_CHECK) {
		/*
		 * This quirk code helps the drive read the
		 * first tape block, regardless of format.  That
		 * is required for these drives to return proper
		 * MODE SENSE information.
		 */
		if ((st->quirks & ST_Q_SENSE_HELP) &&
		    (periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
			/* still starting */
			st->blksize -= 512;
		} else if (!(st->flags & (ST_2FM_AT_EOD | ST_BLANK_READ))) {
			st->flags |= ST_BLANK_READ;
			xs->resid = xs->datalen;
			if (bp) {
				bp->b_resid = xs->resid;
				/* return an EOF */
			}
			retval = 0;
		}
	}

	/*
	 * If generic sense processing will continue, we should not
	 * print sense info here.
	 */
	if (retval == EJUSTRETURN)
		doprint = 0;

	if (doprint) {
#ifdef	SCSIVERBOSE
		scsipi_print_sense(xs, 0);
#else
		scsipi_printaddr(periph);
		printf("Sense Key 0x%02x", key);
		if ((sense->error_code & SSD_ERRCODE_VALID) != 0) {
			switch (key) {
			case SKEY_NOT_READY:
			case SKEY_ILLEGAL_REQUEST:
			case SKEY_UNIT_ATTENTION:
			case SKEY_WRITE_PROTECT:
				break;
			case SKEY_BLANK_CHECK:
				printf(", requested size: %d (decimal)", info);
				break;
			case SKEY_ABORTED_COMMAND:
				if (xs->xs_retries)
					printf(", retrying");
				printf(", cmd 0x%x, info 0x%x",
				    xs->cmd->opcode, info);
				break;
			default:
				printf(", info = %d (decimal)", info);
			}
		}
		if (sense->extra_len != 0) {
			int n;
			printf(", data =");
			for (n = 0; n < sense->extra_len; n++)
				printf(" %02x", sense->cmd_spec_info[n]);
		}
		printf("\n");
#endif
	}
	return (retval);
}

/*
 * The quirk here is that the drive returns some value to st_mode_sense
 * incorrectly until the tape has actually passed by the head.
 *
 * The method is to set the drive to large fixed-block state (user-specified
 * density and 1024-byte blocks), then read and rewind to get it to sense the
 * tape.  If that doesn't work, try 512-byte fixed blocks.  If that doesn't
 * work, as a last resort, try variable- length blocks.  The result will be
 * the ability to do an accurate st_mode_sense.
 *
 * We know we can do a rewind because we just did a load, which implies rewind.
 * Rewind seems preferable to space backward if we have a virgin tape.
 *
 * The rest of the code for this quirk is in ILI processing and BLANK CHECK
 * error processing, both part of st_interpret_sense.
 */
int
st_touch_tape(st)
	struct st_softc *st;
{
	char *buf;
	int readsize;
	int error;

	buf = malloc(1024, M_TEMP, M_NOWAIT);
	if (buf == NULL)
		return (ENOMEM);

	if ((error = st->ops(st, ST_OPS_MODESENSE, 0)) != 0)
		goto bad;

	/*
	 * If the block size is already known from the
	 * sense data, use it. Else start probing at 1024.
	 */
	if (st->media_blksize > 0)
		st->blksize = st->media_blksize;
	else
		st->blksize = 1024;

	do {
		switch (st->blksize) {
		case 512:
		case 1024:
			readsize = st->blksize;
			st->flags |= ST_FIXEDBLOCKS;
			break;
		default:
			readsize = 1;
			st->flags &= ~ST_FIXEDBLOCKS;
		}
		if ((error = st->ops(st, ST_OPS_MODESELECT, XS_CTL_SILENT))
		    != 0) {
			/*
			 * The device did not agree with the proposed
			 * block size. If we exhausted our options,
			 * return failure, else try another.
			 */
			if (readsize == 1)
				goto bad;
			st->blksize -= 512;
			continue;
		}
		st_read(st, buf, readsize, XS_CTL_SILENT);	/* XXX */
		if ((error = st_rewind(st, 0, 0)) != 0) {
bad:			free(buf, M_TEMP);
			return (error);
		}
	} while (readsize != 1 && readsize > st->blksize);

	free(buf, M_TEMP);
	return (0);
}

int
stdump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{

	/* Not implemented. */
	return (ENXIO);
}
