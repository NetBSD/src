/*	$NetBSD: cd.c,v 1.123 1999/02/15 18:41:04 bouyer Exp $	*/

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
 */

#include "rnd.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/cdio.h>
#include <sys/scsiio.h>
#include <sys/proc.h>
#include <sys/conf.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_cd.h>
#include <dev/scsipi/scsipi_disk.h>	/* rw_big and start_stop come */
					/* from there */
#include <dev/scsipi/scsi_disk.h>	/* rw comes from there */
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/cdvar.h>

#include "cd.h"		/* NCD_SCSIBUS and NCD_ATAPIBUS come from here */

#define	CDOUTSTANDING	4

#define	CDUNIT(z)			DISKUNIT(z)
#define	CDPART(z)			DISKPART(z)
#define	MAKECDDEV(maj, unit, part)	MAKEDISKDEV(maj, unit, part)

#define MAXTRACK	99
#define CD_BLOCK_OFFSET	150
#define CD_FRAMES	75
#define CD_SECS		60

struct cd_toc {
	struct ioc_toc_header header;
	struct cd_toc_entry entries[MAXTRACK+1]; /* One extra for the */
						 /* leadout */
};

int	cdlock __P((struct cd_softc *));
void	cdunlock __P((struct cd_softc *));
void	cdstart __P((void *));
void	cdminphys __P((struct buf *));
void	cdgetdefaultlabel __P((struct cd_softc *, struct disklabel *));
void	cdgetdisklabel __P((struct cd_softc *));
void	cddone __P((struct scsipi_xfer *));
u_long	cd_size __P((struct cd_softc *, int));
void	lba2msf __P((u_long, u_char *, u_char *, u_char *));
u_long	msf2lba __P((u_char, u_char, u_char));
int	cd_play __P((struct cd_softc *, int, int));
int	cd_play_tracks __P((struct cd_softc *, int, int, int, int));
int	cd_play_msf __P((struct cd_softc *, int, int, int, int, int, int));
int	cd_pause __P((struct cd_softc *, int));
int	cd_reset __P((struct cd_softc *));
int	cd_read_subchannel __P((struct cd_softc *, int, int, int,
	    struct cd_sub_channel_info *, int));
int	cd_read_toc __P((struct cd_softc *, int, int, void *, int, int));
int	cd_get_parms __P((struct cd_softc *, int));
int	cd_load_toc __P((struct cd_softc *, struct cd_toc *));

extern struct cfdriver cd_cd;

struct dkdriver cddkdriver = { cdstrategy };

struct scsipi_device cd_switch = {
	NULL,			/* use default error handler */
	cdstart,		/* we have a queue, which is started by this */
	NULL,			/* we do not have an async handler */
	cddone,			/* deal with stats at interrupt time */
};

/*
 * The routine called by the low level scsi routine when it discovers
 * A device suitable for this driver
 */
void
cdattach(parent, cd, sc_link, ops)
	struct device *parent;
	struct cd_softc *cd;
	struct scsipi_link *sc_link;
	const struct cd_ops *ops;
{
	SC_DEBUG(sc_link, SDEV_DB2, ("cdattach: "));

	/*
	 * Store information needed to contact our base driver
	 */
	cd->sc_link = sc_link;
	cd->sc_ops = ops;
	sc_link->device = &cd_switch;
	sc_link->device_softc = cd;
	if (sc_link->openings > CDOUTSTANDING)
		sc_link->openings = CDOUTSTANDING;

	/*
	 * Initialize and attach the disk structure.
	 */
  	cd->sc_dk.dk_driver = &cddkdriver;
	cd->sc_dk.dk_name = cd->sc_dev.dv_xname;
	disk_attach(&cd->sc_dk);

#if !defined(i386)
	dk_establish(&cd->sc_dk, &cd->sc_dev);		/* XXX */
#endif

	printf("\n");

#if NRND > 0
	rnd_attach_source(&cd->rnd_source, cd->sc_dev.dv_xname, RND_TYPE_DISK);
#endif
}

/*
 * Wait interruptibly for an exclusive lock.
 *
 * XXX
 * Several drivers do this; it should be abstracted and made MP-safe.
 */
int
cdlock(cd)
	struct cd_softc *cd;
{
	int error;

	while ((cd->flags & CDF_LOCKED) != 0) {
		cd->flags |= CDF_WANTED;
		if ((error = tsleep(cd, PRIBIO | PCATCH, "cdlck", 0)) != 0)
			return (error);
	}
	cd->flags |= CDF_LOCKED;
	return (0);
}

/*
 * Unlock and wake up any waiters.
 */
void
cdunlock(cd)
	struct cd_softc *cd;
{

	cd->flags &= ~CDF_LOCKED;
	if ((cd->flags & CDF_WANTED) != 0) {
		cd->flags &= ~CDF_WANTED;
		wakeup(cd);
	}
}

/*
 * open the device. Make sure the partition info is a up-to-date as can be.
 */
int 
cdopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct cd_softc *cd;
	struct scsipi_link *sc_link;
	int unit, part;
	int error;

	unit = CDUNIT(dev);
	if (unit >= cd_cd.cd_ndevs)
		return (ENXIO);
	cd = cd_cd.cd_devs[unit];
	if (cd == NULL)
		return (ENXIO);

	sc_link = cd->sc_link;
	part = CDPART(dev);

	SC_DEBUG(sc_link, SDEV_DB1,
	    ("cdopen: dev=0x%x (unit %d (of %d), partition %d)\n", dev, unit,
	    cd_cd.cd_ndevs, CDPART(dev)));

	/*
	 * If this is the first open of this device, add a reference
	 * to the adapter.
	 */
	if (cd->sc_dk.dk_openmask == 0 &&
	    (error = scsipi_adapter_addref(sc_link)) != 0)
		return (error);

	if ((error = cdlock(cd)) != 0)
		goto bad4;

	if ((sc_link->flags & SDEV_OPEN) != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens.
		 */
		if ((sc_link->flags & SDEV_MEDIA_LOADED) == 0 &&
			(part != RAW_PART || fmt != S_IFCHR )) {
			error = EIO;
			goto bad3;
		}
	} else {
		/* Check that it is still responding and ok. */
		error = scsipi_test_unit_ready(sc_link,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE |
		    SCSI_IGNORE_NOT_READY);
		SC_DEBUG(sc_link, SDEV_DB1,
		    ("cdopen: scsipi_test_unit_ready, error=%d\n", error));
		if (error)
			goto bad3;

		/*
		 * Start the pack spinning if necessary. Always allow the
		 * raw parition to be opened, for raw IOCTLs. Data transfers
		 * will check for SDEV_MEDIA_LOADED.
		 */
		error = scsipi_start(sc_link, SSS_START,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE |
		    SCSI_SILENT);
		SC_DEBUG(sc_link, SDEV_DB1,
		    ("cdopen: scsipi_start, error=%d\n", error));
		if (error) {
			if (part != RAW_PART || fmt != S_IFCHR) 
				goto bad3;
			else
				goto out;
		}

		sc_link->flags |= SDEV_OPEN;

		/* Lock the pack in. */
		error = scsipi_prevent(sc_link, PR_PREVENT,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE);
		SC_DEBUG(sc_link, SDEV_DB1,
		    ("cdopen: scsipi_prevent, error=%d\n", error));
		if (error)
			goto bad;

		if ((sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
			sc_link->flags |= SDEV_MEDIA_LOADED;

			/* Load the physical device parameters. */
			if (cd_get_parms(cd, 0) != 0) {
				error = ENXIO;
				goto bad2;
			}
			SC_DEBUG(sc_link, SDEV_DB3, ("Params loaded "));

			/* Fabricate a disk label. */
			cdgetdisklabel(cd);
			SC_DEBUG(sc_link, SDEV_DB3, ("Disklabel fabricated "));
		}
	}

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= cd->sc_dk.dk_label->d_npartitions ||
	    cd->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}

out:	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		cd->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		cd->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	cd->sc_dk.dk_openmask =
	    cd->sc_dk.dk_copenmask | cd->sc_dk.dk_bopenmask;

	SC_DEBUG(sc_link, SDEV_DB3, ("open complete\n"));
	cdunlock(cd);
	return (0);

bad2:
	sc_link->flags &= ~SDEV_MEDIA_LOADED;

bad:
	if (cd->sc_dk.dk_openmask == 0) {
		scsipi_prevent(sc_link, PR_ALLOW,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE);
		sc_link->flags &= ~SDEV_OPEN;
	}

bad3:
	cdunlock(cd);
bad4:
	if (cd->sc_dk.dk_openmask == 0)
		scsipi_adapter_delref(sc_link);
	return (error);
}

/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
 */
int 
cdclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(dev)];
	int part = CDPART(dev);
	int error;

	if ((error = cdlock(cd)) != 0)
		return (error);

	switch (fmt) {
	case S_IFCHR:
		cd->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		cd->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	cd->sc_dk.dk_openmask =
	    cd->sc_dk.dk_copenmask | cd->sc_dk.dk_bopenmask;

	if (cd->sc_dk.dk_openmask == 0) {
		scsipi_wait_drain(cd->sc_link);

		scsipi_prevent(cd->sc_link, PR_ALLOW,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_NOT_READY);
		cd->sc_link->flags &= ~SDEV_OPEN;

		scsipi_wait_drain(cd->sc_link);

		scsipi_adapter_delref(cd->sc_link);
	}

	cdunlock(cd);
	return (0);
}

/*
 * Actually translate the requested transfer into one the physical driver can
 * understand.  The transfer is described by a buf and will include only one
 * physical transfer.
 */
void
cdstrategy(bp)
	struct buf *bp;
{
	struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(bp->b_dev)];
	int opri;

	SC_DEBUG(cd->sc_link, SDEV_DB2, ("cdstrategy "));
	SC_DEBUG(cd->sc_link, SDEV_DB1,
	    ("%ld bytes @ blk %d\n", bp->b_bcount, bp->b_blkno));
	/*
	 * If the device has been made invalid, error out
	 * maybe the media changed
	 */
	if ((cd->sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
		if (cd->sc_link->flags & SDEV_OPEN)
			bp->b_error = EIO;
		else
			bp->b_error = ENODEV;
		goto bad;
	}
	/*
	 * The transfer must be a whole number of blocks, offset must not
	 * be negative.
	 */
	if ((bp->b_bcount % cd->sc_dk.dk_label->d_secsize) != 0 ||
	    bp->b_blkno < 0 ) {
		bp->b_error = EINVAL;
		goto bad;
	}
	/*
	 * If it's a null transfer, return immediately
	 */
	if (bp->b_bcount == 0)
		goto done;

	/*
	 * Do bounds checking, adjust transfer. if error, process.
	 * If end of partition, just return.
	 */
	if (CDPART(bp->b_dev) != RAW_PART &&
	    bounds_check_with_label(bp, cd->sc_dk.dk_label,
	    (cd->flags & (CDF_WLABEL|CDF_LABELLING)) != 0) <= 0)
		goto done;

	opri = splbio();

	/*
	 * Place it in the queue of disk activities for this disk
	 */
	disksort(&cd->buf_queue, bp);

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion
	 */
	cdstart(cd);

	splx(opri);
	return;

bad:
	bp->b_flags |= B_ERROR;
done:
	/*
	 * Correctly set the buf to indicate a completed xfer
	 */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

/*
 * cdstart looks to see if there is a buf waiting for the device
 * and that the device is not already busy. If both are true,
 * It deques the buf and creates a scsi command to perform the
 * transfer in the buf. The transfer request will call scsipi_done
 * on completion, which will in turn call this routine again
 * so that the next queued transfer is performed.
 * The bufs are queued by the strategy routine (cdstrategy)
 *
 * This routine is also called after other non-queued requests
 * have been made of the scsi driver, to ensure that the queue
 * continues to be drained.
 *
 * must be called at the correct (highish) spl level
 * cdstart() is called at splbio from cdstrategy and scsipi_done
 */
void 
cdstart(v)
	register void *v;
{
	register struct cd_softc *cd = v;
	register struct scsipi_link *sc_link = cd->sc_link;
	struct disklabel *lp = cd->sc_dk.dk_label;
	struct buf *bp = 0;
	struct buf *dp;
	struct scsipi_rw_big cmd_big;
#if NCD_SCSIBUS > 0 
	struct scsi_rw cmd_small;
#endif
	struct scsipi_generic *cmdp;
	int blkno, nblks, cmdlen;
	struct partition *p;

	SC_DEBUG(sc_link, SDEV_DB2, ("cdstart "));
	/*
	 * Check if the device has room for another command
	 */
	while (sc_link->openings > 0) {
		/*
		 * there is excess capacity, but a special waits
		 * It'll need the adapter as soon as we clear out of the
		 * way and let it run (user level wait).
		 */
		if (sc_link->flags & SDEV_WAITING) {
			sc_link->flags &= ~SDEV_WAITING;
			wakeup((caddr_t)sc_link);
			return;
		}

		/*
		 * See if there is a buf with work for us to do..
		 */
		dp = &cd->buf_queue;
		if ((bp = dp->b_actf) == NULL)	/* yes, an assign */
			return;
		dp->b_actf = bp->b_actf;

		/*
		 * If the device has become invalid, abort all the
		 * reads and writes until all files have been closed and
		 * re-opened
		 */
		if ((sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			bp->b_resid = bp->b_bcount;
			biodone(bp);
			continue;
		}

		/*
		 * We have a buf, now we should make a command
		 *
		 * First, translate the block to absolute and put it in terms
		 * of the logical blocksize of the device.
		 */
		blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
		if (CDPART(bp->b_dev) != RAW_PART) {
			p = &lp->d_partitions[CDPART(bp->b_dev)];
			blkno += p->p_offset;
		}
		nblks = howmany(bp->b_bcount, lp->d_secsize);

#if NCD_SCSIBUS > 0
		/*
		 *  Fill out the scsi command.  If the transfer will
		 *  fit in a "small" cdb, use it.
		 */
		if (((blkno & 0x1fffff) == blkno) &&
		    ((nblks & 0xff) == nblks) && sc_link->type == BUS_SCSI) {
			/*
			 * We can fit in a small cdb.
			 */
			bzero(&cmd_small, sizeof(cmd_small));
			cmd_small.opcode = (bp->b_flags & B_READ) ?
			    SCSI_READ_COMMAND : SCSI_WRITE_COMMAND;
			_lto3b(blkno, cmd_small.addr);
			cmd_small.length = nblks & 0xff;
			cmdlen = sizeof(cmd_small);
			cmdp = (struct scsipi_generic *)&cmd_small;
		} else
#endif
		{
			/*
			 * Need a large cdb.
			 */
			bzero(&cmd_big, sizeof(cmd_big));
			cmd_big.opcode = (bp->b_flags & B_READ) ?
			    READ_BIG : WRITE_BIG;
			_lto4b(blkno, cmd_big.addr);
			_lto2b(nblks, cmd_big.length);
			cmdlen = sizeof(cmd_big);
			cmdp = (struct scsipi_generic *)&cmd_big;
		}

		/* Instrumentation. */
		disk_busy(&cd->sc_dk);

		/*
		 * Call the routine that chats with the adapter.
		 * Note: we cannot sleep as we may be an interrupt
		 */
		if (scsipi_command(sc_link, cmdp, cmdlen, (u_char *)bp->b_data,
		    bp->b_bcount, CDRETRIES, 30000, bp, SCSI_NOSLEEP |
		    ((bp->b_flags & B_READ) ? SCSI_DATA_IN : SCSI_DATA_OUT))) {
			disk_unbusy(&cd->sc_dk, 0); 
			printf("%s: not queued", cd->sc_dev.dv_xname);
		}
	}
}

void
cddone(xs)
	struct scsipi_xfer *xs;
{
	struct cd_softc *cd = xs->sc_link->device_softc;

	if (xs->bp != NULL) {
		disk_unbusy(&cd->sc_dk, xs->bp->b_bcount - xs->bp->b_resid);
#if NRND > 0
		rnd_add_uint32(&cd->rnd_source, xs->bp->b_blkno);
#endif
	}
}

void
cdminphys(bp)
	struct buf *bp;
{
	struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(bp->b_dev)];
	long max;

	/*
	 * If the device is ancient, we want to make sure that
	 * the transfer fits into a 6-byte cdb.
	 *
	 * XXX Note that the SCSI-I spec says that 256-block transfers
	 * are allowed in a 6-byte read/write, and are specified
	 * by settng the "length" to 0.  However, we're conservative
	 * here, allowing only 255-block transfers in case an
	 * ancient device gets confused by length == 0.  A length of 0
	 * in a 10-byte read/write actually means 0 blocks.
	 */
	if (cd->flags & CDF_ANCIENT) {
		max = cd->sc_dk.dk_label->d_secsize * 0xff;

		if (bp->b_bcount > max)
			bp->b_bcount = max;
	}

	(*cd->sc_link->adapter->scsipi_minphys)(bp);
}

int
cdread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(cdstrategy, NULL, dev, B_READ, cdminphys, uio));
}

int
cdwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(cdstrategy, NULL, dev, B_WRITE, cdminphys, uio));
}

/*
 * conversion between minute-seconde-frame and logical block adress
 * adresses format
 */
void
lba2msf (lba, m, s, f)
	u_long lba;
	u_char *m, *s, *f;
{   
	u_long tmp;

	tmp = lba + CD_BLOCK_OFFSET;	/* offset of first logical frame */
	tmp &= 0xffffff;		/* negative lbas use only 24 bits */
	*m = tmp / (CD_SECS * CD_FRAMES);
	tmp %= (CD_SECS * CD_FRAMES);
	*s = tmp / CD_FRAMES;
	*f = tmp % CD_FRAMES;
}

u_long
msf2lba (m, s, f)
	u_char m, s, f;
{

	return ((((m * CD_SECS) + s) * CD_FRAMES + f) - CD_BLOCK_OFFSET);
}


/*
 * Perform special action on behalf of the user.
 * Knows about the internals of this device
 */
int
cdioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(dev)];
	int part = CDPART(dev);
	int error;

	SC_DEBUG(cd->sc_link, SDEV_DB2, ("cdioctl 0x%lx ", cmd));

	/*
	 * If the device is not valid, some IOCTLs can still be
	 * handled on the raw partition. Check this here.
	 */
	if ((cd->sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
		switch (cmd) {
		case DIOCWLABEL:
		case DIOCLOCK:
		case ODIOCEJECT:
		case DIOCEJECT:
		case SCIOCIDENTIFY:
		case OSCIOCIDENTIFY:
		case SCIOCCOMMAND:
		case SCIOCDEBUG:
		case CDIOCGETVOL:
		case CDIOCSETVOL:
		case CDIOCSETMONO:
		case CDIOCSETSTEREO:
		case CDIOCSETMUTE:
		case CDIOCSETLEFT:
		case CDIOCSETRIGHT:
		case CDIOCCLOSE:
		case CDIOCEJECT:
		case CDIOCALLOW:
		case CDIOCPREVENT:
		case CDIOCSETDEBUG:
		case CDIOCCLRDEBUG:
		case CDIOCRESET:
		case SCIOCRESET:
		case CDIOCLOADUNLOAD:
			if (part == RAW_PART)
				break;
		/* FALLTHROUGH */
		default:
			if ((cd->sc_link->flags & SDEV_OPEN) == 0)
				return (ENODEV);
			else
				return (EIO);
		}
	}

	switch (cmd) {
	case DIOCGDINFO:
		*(struct disklabel *)addr = *(cd->sc_dk.dk_label);
		return (0);

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = cd->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &cd->sc_dk.dk_label->d_partitions[part];
		return (0);

	case DIOCWDINFO:
	case DIOCSDINFO:
		if ((flag & FWRITE) == 0)
			return (EBADF);

		if ((error = cdlock(cd)) != 0)
			return (error);
		cd->flags |= CDF_LABELLING;

		error = setdisklabel(cd->sc_dk.dk_label,
		    (struct disklabel *)addr, /*cd->sc_dk.dk_openmask : */0,
		    cd->sc_dk.dk_cpulabel);
		if (error == 0) {
			/* XXX ? */
		}

		cd->flags &= ~CDF_LABELLING;
		cdunlock(cd);
		return (error);

	case DIOCWLABEL:
		return (EBADF);

	case DIOCGDEFLABEL:
		cdgetdefaultlabel(cd, (struct disklabel *)addr);
		return (0);

	case CDIOCPLAYTRACKS: {
		struct ioc_play_track *args = (struct ioc_play_track *)addr;

		if ((error = (*cd->sc_ops->cdo_set_pa_immed)(cd, 0)) != 0)
			return (error);
		return (cd_play_tracks(cd, args->start_track,
		    args->start_index, args->end_track, args->end_index));
	}
	case CDIOCPLAYMSF: {
		struct ioc_play_msf *args = (struct ioc_play_msf *)addr;

		if ((error = (*cd->sc_ops->cdo_set_pa_immed)(cd, 0)) != 0)
			return (error);
		return (cd_play_msf(cd, args->start_m, args->start_s,
		    args->start_f, args->end_m, args->end_s, args->end_f));
	}
	case CDIOCPLAYBLOCKS: {
		struct ioc_play_blocks *args = (struct ioc_play_blocks *)addr;

		if ((error = (*cd->sc_ops->cdo_set_pa_immed)(cd, 0)) != 0)
			return (error);
		return (cd_play(cd, args->blk, args->len));
	}
	case CDIOCREADSUBCHANNEL: {
		struct ioc_read_subchannel *args =
		    (struct ioc_read_subchannel *)addr;
		struct cd_sub_channel_info data;
		int len = args->data_len;

		if (len > sizeof(data) ||
		    len < sizeof(struct cd_sub_channel_header))
			return (EINVAL);
		error = cd_read_subchannel(cd, args->address_format,
		    args->data_format, args->track, &data, len);
		if (error)
			return (error);
		len = min(len, _2btol(data.header.data_len) +
		    sizeof(struct cd_sub_channel_header));
		return (copyout(&data, args->data, len));
	}
	case CDIOREADTOCHEADER: {
		struct ioc_toc_header th;

		if ((error = cd_read_toc(cd, 0, 0, &th, sizeof(th), 0)) != 0)
			return (error);
		if (cd->sc_link->quirks & ADEV_LITTLETOC) {
#if BYTE_ORDER == BIG_ENDIAN
			bswap((u_int8_t *)&th.len, sizeof(th.len));
#endif
		} else
			th.len = ntohs(th.len);
		bcopy(&th, addr, sizeof(th));
		return (0);
	}
	case CDIOREADTOCENTRYS: {
		struct cd_toc toc;
		struct ioc_read_toc_entry *te =
		    (struct ioc_read_toc_entry *)addr;
		struct ioc_toc_header *th;
		struct cd_toc_entry *cte;
		int len = te->data_len;
		int ntracks;

		th = &toc.header;

		if (len > sizeof(toc.entries) ||
		    len < sizeof(struct cd_toc_entry))
			return (EINVAL);
		error = cd_read_toc(cd, te->address_format, te->starting_track,
		    &toc, len + sizeof(struct ioc_toc_header), 0);
		if (error)
			return (error);
		if (te->address_format == CD_LBA_FORMAT)
			for (ntracks =
			    th->ending_track - th->starting_track + 1;
			    ntracks >= 0; ntracks--) {
				cte = &toc.entries[ntracks];
				cte->addr_type = CD_LBA_FORMAT;
				if (cd->sc_link->quirks & ADEV_LITTLETOC) {
#if BYTE_ORDER == BIG_ENDIAN
					bswap((u_int8_t*)&cte->addr,
					    sizeof(cte->addr));
#endif
				} else
					cte->addr.lba = ntohl(cte->addr.lba);
			}
		if (cd->sc_link->quirks & ADEV_LITTLETOC) {
#if BYTE_ORDER == BIG_ENDIAN
			bswap((u_int8_t*)&th->len, sizeof(th->len));
#endif
		} else
			th->len = ntohs(th->len);
		len = min(len, th->len - (sizeof(th->starting_track) +
		    sizeof(th->ending_track)));
		return (copyout(toc.entries, te->data, len));
	}
	case CDIOREADMSADDR: {
		struct cd_toc toc;
		int sessno = *(int*)addr;
		struct cd_toc_entry *cte;

		if (sessno != 0)
			return (EINVAL);

		error = cd_read_toc(cd, 0, 0, &toc,
		  sizeof(struct ioc_toc_header) + sizeof(struct cd_toc_entry),
		  0x40 /* control word for "get MS info" */);

		if (error)
			return (error);

		cte = &toc.entries[0];
		if (cd->sc_link->quirks & ADEV_LITTLETOC) {
#if BYTE_ORDER == BIG_ENDIAN
			bswap((u_int8_t*)&cte->addr, sizeof(cte->addr));
#endif
		} else
			cte->addr.lba = ntohl(cte->addr.lba);
		if (cd->sc_link->quirks & ADEV_LITTLETOC) {
#if BYTE_ORDER == BIG_ENDIAN
			bswap((u_int8_t*)&toc.header.len, sizeof(toc.header.len));
#endif
		} else
			toc.header.len = ntohs(toc.header.len);

		*(int*)addr = (toc.header.len >= 10 && cte->track > 1) ?
			cte->addr.lba : 0;
		return 0;
	}
	case CDIOCSETPATCH: {
		struct ioc_patch *arg = (struct ioc_patch *)addr;

		return ((*cd->sc_ops->cdo_setchan)(cd, arg->patch[0],
		    arg->patch[1], arg->patch[2], arg->patch[3], 0));
	}
	case CDIOCGETVOL: {
		struct ioc_vol *arg = (struct ioc_vol *)addr;

		return ((*cd->sc_ops->cdo_getvol)(cd, arg, 0));
	}
	case CDIOCSETVOL: {
		struct ioc_vol *arg = (struct ioc_vol *)addr;

		return ((*cd->sc_ops->cdo_setvol)(cd, arg, 0));
	}

	case CDIOCSETMONO:
		return ((*cd->sc_ops->cdo_setchan)(cd, BOTH_CHANNEL,
		    BOTH_CHANNEL, MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCSETSTEREO:
		return ((*cd->sc_ops->cdo_setchan)(cd, LEFT_CHANNEL,
		    RIGHT_CHANNEL, MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCSETMUTE:
		return ((*cd->sc_ops->cdo_setchan)(cd, MUTE_CHANNEL,
		    MUTE_CHANNEL, MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCSETLEFT:
		return ((*cd->sc_ops->cdo_setchan)(cd, LEFT_CHANNEL,
		    LEFT_CHANNEL, MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCSETRIGHT:
		return ((*cd->sc_ops->cdo_setchan)(cd, RIGHT_CHANNEL,
		    RIGHT_CHANNEL, MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCRESUME:
		return (cd_pause(cd, PA_RESUME));
	case CDIOCPAUSE:
		return (cd_pause(cd, PA_PAUSE));
	case CDIOCSTART:
		return (scsipi_start(cd->sc_link, SSS_START, 0));
	case CDIOCSTOP:
		return (scsipi_start(cd->sc_link, SSS_STOP, 0));
	case CDIOCCLOSE:
		return (scsipi_start(cd->sc_link, SSS_START|SSS_LOEJ, 
		    SCSI_IGNORE_MEDIA_CHANGE));
	case DIOCEJECT:
		if (*(int *)addr == 0) {
			/*
			 * Don't force eject: check that we are the only
			 * partition open. If so, unlock it.
			 */
			if ((cd->sc_dk.dk_openmask & ~(1 << part)) == 0 &&
			    cd->sc_dk.dk_bopenmask + cd->sc_dk.dk_copenmask ==
			    cd->sc_dk.dk_openmask) {
				error =  scsipi_prevent(cd->sc_link, PR_ALLOW,
				    SCSI_IGNORE_NOT_READY);
				if (error)
					return (error);
			} else {
				return (EBUSY); 
			}
		}
		/* FALLTHROUGH */
	case CDIOCEJECT: /* FALLTHROUGH */
	case ODIOCEJECT:
		return (scsipi_start(cd->sc_link, SSS_STOP|SSS_LOEJ, 0));
	case CDIOCALLOW:
		return (scsipi_prevent(cd->sc_link, PR_ALLOW, 0));
	case CDIOCPREVENT:
		return (scsipi_prevent(cd->sc_link, PR_PREVENT, 0));
	case DIOCLOCK:
		return (scsipi_prevent(cd->sc_link,
		    (*(int *)addr) ? PR_PREVENT : PR_ALLOW, 0));
	case CDIOCSETDEBUG:
		cd->sc_link->flags |= (SDEV_DB1 | SDEV_DB2);
		return (0);
	case CDIOCCLRDEBUG:
		cd->sc_link->flags &= ~(SDEV_DB1 | SDEV_DB2);
		return (0);
	case CDIOCRESET:
	case SCIOCRESET:
		return (cd_reset(cd));
	case CDIOCLOADUNLOAD: {
		struct ioc_load_unload *args = (struct ioc_load_unload *)addr;

		return ((*cd->sc_ops->cdo_load_unload)(cd, args->options,
			args->slot));
	}

	default:
		if (part != RAW_PART)
			return (ENOTTY);
		return (scsipi_do_ioctl(cd->sc_link, dev, cmd, addr, flag, p));
	}

#ifdef DIAGNOSTIC
	panic("cdioctl: impossible");
#endif
}

void
cdgetdefaultlabel(cd, lp)
	struct cd_softc *cd;
	struct disklabel *lp;
{

	bzero(lp, sizeof(struct disklabel));

	lp->d_secsize = cd->params.blksize;
	lp->d_ntracks = 1;
	lp->d_nsectors = 100;
	lp->d_ncylinders = (cd->params.disksize / 100) + 1;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	switch (cd->sc_link->type) {
#if NCD_SCSIBUS > 0
	    case BUS_SCSI:
		lp->d_type = DTYPE_SCSI;
		break;
#endif
#if NCD_ATAPIBUS > 0
	    case BUS_ATAPI:
		lp->d_type = DTYPE_ATAPI;
		break;
#endif
	}
	strncpy(lp->d_typename, cd->name, 16);
	strncpy(lp->d_packname, "fictitious", 16);
	lp->d_secperunit = cd->params.disksize;
	lp->d_rpm = 300;
	lp->d_interleave = 1;
	lp->d_flags = D_REMOVABLE;

	lp->d_partitions[0].p_offset = 0;
	lp->d_partitions[0].p_size =
	    lp->d_secperunit * (lp->d_secsize / DEV_BSIZE);
	lp->d_partitions[0].p_fstype = FS_ISO9660;
	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size =
	    lp->d_secperunit * (lp->d_secsize / DEV_BSIZE);
	lp->d_partitions[RAW_PART].p_fstype = FS_ISO9660;
	lp->d_npartitions = RAW_PART + 1;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);
}

/*
 * Load the label information on the named device
 * Actually fabricate a disklabel
 *
 * EVENTUALLY take information about different
 * data tracks from the TOC and put it in the disklabel
 */
void
cdgetdisklabel(cd)
	struct cd_softc *cd;
{
	struct disklabel *lp = cd->sc_dk.dk_label;

	bzero(cd->sc_dk.dk_cpulabel, sizeof(struct cpu_disklabel));

	cdgetdefaultlabel(cd, lp);
}

/*
 * Find out from the device what it's capacity is
 */
u_long
cd_size(cd, flags)
	struct cd_softc *cd;
	int flags;
{
	struct scsipi_read_cd_cap_data rdcap;
	struct scsipi_read_cd_capacity scsipi_cmd;
	int blksize;
	u_long size;

	if (cd->sc_link->quirks & ADEV_NOCAPACITY) {
		/*
		 * the drive doesn't support the READ_CD_CAPACITY command
		 * use a fake size
		 */
		cd->params.blksize = 2048;
		cd->params.disksize = 400000;
		return (400000);
	}

	/*
	 * make up a scsi command and ask the scsi driver to do
	 * it for you.
	 */
	bzero(&scsipi_cmd, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = READ_CD_CAPACITY;

	/*
	 * If the command works, interpret the result as a 4 byte
	 * number of blocks and a blocksize
	 */
	if (scsipi_command(cd->sc_link,
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),
	    (u_char *)&rdcap, sizeof(rdcap), CDRETRIES, 20000, NULL,
	    flags | SCSI_DATA_IN) != 0)
		return (0);

	blksize = _4btol(rdcap.length);
	if ((blksize < 512) || ((blksize & 511) != 0))
		blksize = 2048;	/* some drives lie ! */
	cd->params.blksize = blksize;

	size = _4btol(rdcap.addr) + 1;
	if (size < 100)
		size = 400000;	/* ditto */
	cd->params.disksize = size;

	SC_DEBUG(cd->sc_link, SDEV_DB2, ("cd_size: %d %ld\n", blksize, size));
	return (size);
}

/*
 * Get scsi driver to send a "start playing" command
 */
int
cd_play(cd, blkno, nblks)
	struct cd_softc *cd;
	int blkno, nblks;
{
	struct scsipi_play scsipi_cmd;

	bzero(&scsipi_cmd, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = PLAY;
	_lto4b(blkno, scsipi_cmd.blk_addr);
	_lto2b(nblks, scsipi_cmd.xfer_len);
	return (scsipi_command(cd->sc_link,
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),
	    0, 0, CDRETRIES, 200000, NULL, 0));
}

/*
 * Get scsi driver to send a "start playing" command
 */
int
cd_play_tracks(cd, strack, sindex, etrack, eindex)
	struct cd_softc *cd;
	int strack, sindex, etrack, eindex;
{
	struct cd_toc toc;
	int error;

	if (!etrack)
		return (EIO);
	if (strack > etrack)
		return (EINVAL);

	if ((error = cd_load_toc(cd, &toc)) != 0)
		return (error);

	if (++etrack > (toc.header.ending_track+1))
		etrack = toc.header.ending_track+1;

	strack -= toc.header.starting_track;
	etrack -= toc.header.starting_track;
	if (strack < 0)
		return (EINVAL);

	return (cd_play_msf(cd, toc.entries[strack].addr.msf.minute,
	    toc.entries[strack].addr.msf.second,
	    toc.entries[strack].addr.msf.frame,
	    toc.entries[etrack].addr.msf.minute,
	    toc.entries[etrack].addr.msf.second,
	    toc.entries[etrack].addr.msf.frame));
}

/*
 * Get scsi driver to send a "play msf" command
 */
int
cd_play_msf(cd, startm, starts, startf, endm, ends, endf)
	struct cd_softc *cd;
	int startm, starts, startf, endm, ends, endf;
{
	struct scsipi_play_msf scsipi_cmd;

	bzero(&scsipi_cmd, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = PLAY_MSF;
	scsipi_cmd.start_m = startm;
	scsipi_cmd.start_s = starts;
	scsipi_cmd.start_f = startf;
	scsipi_cmd.end_m = endm;
	scsipi_cmd.end_s = ends;
	scsipi_cmd.end_f = endf;
	return (scsipi_command(cd->sc_link,
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),
	    0, 0, CDRETRIES, 2000, NULL, 0));
}

/*
 * Get scsi driver to send a "start up" command
 */
int
cd_pause(cd, go)
	struct cd_softc *cd;
	int go;
{
	struct scsipi_pause scsipi_cmd;

	bzero(&scsipi_cmd, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = PAUSE;
	scsipi_cmd.resume = go & 0xff;
	return (scsipi_command(cd->sc_link,
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),
	    0, 0, CDRETRIES, 2000, NULL, 0));
}

/*
 * Get scsi driver to send a "RESET" command
 */
int
cd_reset(cd)
	struct cd_softc *cd;
{

	return (scsipi_command(cd->sc_link, 0, 0, 0, 0,
	    CDRETRIES, 2000, NULL, SCSI_RESET));
}

/*
 * Read subchannel
 */
int
cd_read_subchannel(cd, mode, format, track, data, len)
	struct cd_softc *cd;
	int mode, format, track, len;
	struct cd_sub_channel_info *data;
{
	struct scsipi_read_subchannel scsipi_cmd;

	bzero(&scsipi_cmd, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = READ_SUBCHANNEL;
	if (mode == CD_MSF_FORMAT)
		scsipi_cmd.byte2 |= CD_MSF;
	scsipi_cmd.byte3 = SRS_SUBQ;
	scsipi_cmd.subchan_format = format;
	scsipi_cmd.track = track;
	_lto2b(len, scsipi_cmd.data_len);
	return (scsipi_command(cd->sc_link,
	    (struct scsipi_generic *)&scsipi_cmd,
	    sizeof(struct scsipi_read_subchannel), (u_char *)data, len,
	    CDRETRIES, 5000, NULL, SCSI_DATA_IN|SCSI_SILENT));
}

/*
 * Read table of contents
 */
int
cd_read_toc(cd, mode, start, data, len, control)
	struct cd_softc *cd;
	int mode, start, len, control;
	void *data;
{
	struct scsipi_read_toc scsipi_cmd;
	int ntoc;

	bzero(&scsipi_cmd, sizeof(scsipi_cmd));
#if 0
	if (len != sizeof(struct ioc_toc_header))
		ntoc = ((len) - sizeof(struct ioc_toc_header)) /
		    sizeof(struct cd_toc_entry);
	else
#endif
	ntoc = len;
	scsipi_cmd.opcode = READ_TOC;
	if (mode == CD_MSF_FORMAT)
		scsipi_cmd.byte2 |= CD_MSF;
	scsipi_cmd.from_track = start;
	_lto2b(ntoc, scsipi_cmd.data_len);
	scsipi_cmd.control = control;
	return (scsipi_command(cd->sc_link,
	    (struct scsipi_generic *)&scsipi_cmd,
	    sizeof(struct scsipi_read_toc), (u_char *)data, len, CDRETRIES,
	    5000, NULL, SCSI_DATA_IN));
}

int
cd_load_toc(cd, toc)
	struct cd_softc *cd;
	struct cd_toc *toc;
{
	int ntracks, len, error;

	if ((error = cd_read_toc(cd, 0, 0, toc, sizeof(toc->header), 0)) != 0)
		return (error);

	ntracks = toc->header.ending_track - toc->header.starting_track + 1;
	len = (ntracks + 1) * sizeof(struct cd_toc_entry) +
	    sizeof(toc->header);
	if ((error = cd_read_toc(cd, CD_MSF_FORMAT, 0, toc, len, 0)) != 0)
		return (error);
	return (0);
}

/*
 * Get the scsi driver to send a full inquiry to the device and use the
 * results to fill out the disk parameter structure.
 */
int
cd_get_parms(cd, flags)
	struct cd_softc *cd;
	int flags;
{

	/*
	 * give a number of sectors so that sec * trks * cyls
	 * is <= disk_size
	 */
	if (cd_size(cd, flags) == 0)
		return (ENXIO);
	return (0);
}

int
cdsize(dev)
	dev_t dev;
{

	/* CD-ROMs are read-only. */
	return (-1);
}

int
cddump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{

	/* Not implemented. */
	return (ENXIO);
}
