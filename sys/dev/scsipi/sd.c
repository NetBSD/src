/*	$NetBSD: sd.c,v 1.56 1995/01/26 12:05:54 mycroft Exp $	*/

/*
 * Copyright (c) 1994, 1995 Charles Hannum.  All rights reserved.
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
#include <sys/conf.h>
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

#include <scsi/scsi_all.h>
#include <scsi/scsi_disk.h>
#include <scsi/scsiconf.h>

#ifdef	DDB
int     Debugger();
#else	/* DDB */
#define Debugger()
#endif	/* DDB */

#define	SDOUTSTANDING	2
#define	SDRETRIES	4

#define	SDUNIT(dev)			DISKUNIT(dev)
#define	SDPART(dev)			DISKPART(dev)
#define	MAKESDDEV(maj, unit, part)	MAKEDISKDEV(maj, unit, part)

#define	SDLABELDEV(dev)	(MAKESDDEV(major(dev), SDUNIT(dev), RAW_PART))

struct sd_softc {
	struct device sc_dev;
	struct dkdevice sc_dk;

	int flags;
#define	SDF_LOCKED	0x01
#define	SDF_WANTED	0x02
#define	SDF_WLABEL	0x04		/* label is writable */
	struct scsi_link *sc_link;	/* contains our targ, lun etc. */
	struct disk_parms {
		u_char heads;		/* Number of heads */
		u_short cyls;		/* Number of cylinders */
		u_char sectors;		/* Number of sectors/track */
		int blksize;		/* Number of bytes/sector */
		u_long disksize;	/* total number sectors */
	} params;
	struct buf buf_queue;
};

int sdmatch __P((struct device *, void *, void *));
void sdattach __P((struct device *, struct device *, void *));

struct cfdriver sdcd = {
	NULL, "sd", sdmatch, sdattach, DV_DISK, sizeof(struct sd_softc)
};

void sdgetdisklabel __P((struct sd_softc *));
int sd_get_parms __P((struct sd_softc *, int));
void sdstrategy __P((struct buf *));
void sdstart __P((struct sd_softc *));

struct dkdriver sddkdriver = { sdstrategy };

struct scsi_device sd_switch = {
	NULL,			/* Use default error handler */
	sdstart,		/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};

struct scsi_inquiry_pattern sd_patterns[] = {
	{T_DIRECT, T_FIXED,
	 "",         "",                 ""},
	{T_DIRECT, T_REMOV,
	 "",         "",                 ""},
};

int
sdmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct cfdata *cf = match;
	struct scsibus_attach_args *sa = aux;
	int priority;

	(void)scsi_inqmatch(sa->sa_inqbuf,
	    (caddr_t)sd_patterns, sizeof(sd_patterns)/sizeof(sd_patterns[0]),
	    sizeof(sd_patterns[0]), &priority);
	return (priority);
}

/*
 * The routine called by the low level scsi routine when it discovers
 * a device suitable for this driver.
 */
void
sdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sd_softc *sd = (void *)self;
	struct disk_parms *dp = &sd->params;
	struct scsibus_attach_args *sa = aux;
	struct scsi_link *sc_link = sa->sa_sc_link;

	SC_DEBUG(sc_link, SDEV_DB2, ("sdattach: "));

	/*
	 * Store information needed to contact our base driver
	 */
	sd->sc_link = sc_link;
	sc_link->device = &sd_switch;
	sc_link->device_softc = sd;
	if (sc_link->openings > SDOUTSTANDING)
		sc_link->openings = SDOUTSTANDING;

	sd->sc_dk.dk_driver = &sddkdriver;
#if !defined(i386) || defined(NEWCONFIG)
	dk_establish(&sd->sc_dk, &sd->sc_dev);
#endif

	/*
	 * Use the subdriver to request information regarding
	 * the drive. We cannot use interrupts yet, so the
	 * request must specify this.
	 */
	if (scsi_start(sd->sc_link, SSS_START,
	    SCSI_AUTOCONF | SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE | SCSI_SILENT) ||
	    sd_get_parms(sd, SCSI_AUTOCONF) != 0)
		printf(": drive offline\n");
	else
	        printf(": %dMB, %d cyl, %d head, %d sec, %d bytes/sec\n",
		    dp->disksize / (1048576 / dp->blksize), dp->cyls,
		    dp->heads, dp->sectors, dp->blksize);
}

/*
 * open the device. Make sure the partition info is a up-to-date as can be.
 */
int
sdopen(dev, flag, fmt)
	dev_t dev;
	int flag, fmt;
{
	int error;
	int unit, part;
	struct sd_softc *sd;
	struct scsi_link *sc_link;

	unit = SDUNIT(dev);
	if (unit >= sdcd.cd_ndevs)
		return ENXIO;
	sd = sdcd.cd_devs[unit];
	if (!sd)
		return ENXIO;

	part = SDPART(dev);
	sc_link = sd->sc_link;

	SC_DEBUG(sc_link, SDEV_DB1,
	    ("sdopen: dev=0x%x (unit %d (of %d), partition %d)\n", dev, unit,
	    sdcd.cd_ndevs, part));

	while ((sd->flags & SDF_LOCKED) != 0) {
		sd->flags |= SDF_WANTED;
		if ((error = tsleep(sd, PRIBIO | PCATCH, "sdopn", 0)) != 0)
			return error;
	}

	if (sd->sc_dk.dk_openmask != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens.
		 */
		if ((sc_link->flags & SDEV_MEDIA_LOADED) == 0)
			return ENXIO;
	} else {
		sd->flags |= SDF_LOCKED;
		sc_link->flags |= SDEV_OPEN;

		/* Check that it is still responding and ok. */
		if (error = scsi_test_unit_ready(sc_link,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE | SCSI_IGNORE_NOT_READY))
			goto bad;

		/* Lock the pack in. */
		if (error = scsi_prevent(sc_link, PR_PREVENT,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE))
			goto bad;

		/* Start the pack spinning if necessary. */
		if (error = scsi_start(sc_link, SSS_START,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE | SCSI_SILENT))
			goto bad;

		if ((sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
			sc_link->flags |= SDEV_MEDIA_LOADED;

			/* Load the physical device parameters. */
			if (sd_get_parms(sd, 0) != 0) {
				error = ENXIO;
				goto bad2;
			}
			SC_DEBUG(sc_link, SDEV_DB3, ("Params loaded "));

			/* Load the partition info if not already loaded. */
			sdgetdisklabel(sd);
			SC_DEBUG(sc_link, SDEV_DB3, ("Disklabel loaded "));
		}

		sd->flags &= ~SDF_LOCKED;
		if ((sd->flags & SDF_WANTED) != 0) {
			sd->flags &= ~SDF_WANTED;
			wakeup(sd);
		}
	}

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= sd->sc_dk.dk_label.d_npartitions ||
	     sd->sc_dk.dk_label.d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}

	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		sd->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		sd->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	sd->sc_dk.dk_openmask = sd->sc_dk.dk_copenmask | sd->sc_dk.dk_bopenmask;

	SC_DEBUG(sc_link, SDEV_DB3, ("open complete\n"));
	return 0;

bad2:
	sc_link->flags &= ~SDEV_MEDIA_LOADED;

bad:
	if (sd->sc_dk.dk_openmask == 0) {
		scsi_prevent(sc_link, PR_ALLOW,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE);
		sc_link->flags &= ~SDEV_OPEN;

		sd->flags &= ~SDF_LOCKED;
		if ((sd->flags & SDF_WANTED) != 0) {
			sd->flags &= ~SDF_WANTED;
			wakeup(sd);
		}
	}

	return error;
}

/*
 * close the device.. only called if we are the LAST occurence of an open
 * device.  Convenient now but usually a pain.
 */
int 
sdclose(dev, flag, fmt)
	dev_t dev;
	int flag, fmt;
{
	struct sd_softc *sd = sdcd.cd_devs[SDUNIT(dev)];
	int part = SDPART(dev);
	int s;

	switch (fmt) {
	case S_IFCHR:
		sd->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		sd->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	sd->sc_dk.dk_openmask = sd->sc_dk.dk_copenmask | sd->sc_dk.dk_bopenmask;

	if (sd->sc_dk.dk_openmask == 0) {
		sd->flags |= SDF_LOCKED;

#if 0
		s = splbio();
		while (...) {
			sd->flags |= SDF_WAITING;
			if ((error = tsleep(sd, PRIBIO | PCATCH, "sdcls", 0)) != 0)
				return error;
		}
		splx(s);
#endif

		scsi_prevent(sd->sc_link, PR_ALLOW,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_NOT_READY);
		sd->sc_link->flags &= ~SDEV_OPEN;

		sd->flags &= ~SDF_LOCKED;
		if ((sd->flags & SDF_WANTED) != 0) {
			sd->flags &= ~SDF_WANTED;
			wakeup(sd);
		}
	}

	return 0;
}

/*
 * trim the size of the transfer if needed, called by physio
 * basically the smaller of our max and the scsi driver's
 * minphys (note we have no max)
 *
 * Trim buffer length if buffer-size is bigger than page size
 */
void 
sdminphys(bp)
	struct buf *bp;
{
	register struct sd_softc *sd = sdcd.cd_devs[SDUNIT(bp->b_dev)];

	(sd->sc_link->adapter->scsi_minphys) (bp);
}

/*
 * Actually translate the requested transfer into one the physical driver
 * can understand.  The transfer is described by a buf and will include
 * only one physical transfer.
 */
void 
sdstrategy(bp)
	struct buf *bp;
{
	struct sd_softc *sd = sdcd.cd_devs[SDUNIT(bp->b_dev)];
	int opri;

	SC_DEBUG(sd->sc_link, SDEV_DB2, ("sdstrategy "));
	SC_DEBUG(sd->sc_link, SDEV_DB1,
	    ("%d bytes @ blk %d\n", bp->b_bcount, bp->b_blkno));
	sdminphys(bp);
	/*
	 * If the device has been made invalid, error out
	 */
	if (!(sd->sc_link->flags & SDEV_MEDIA_LOADED)) {
		bp->b_error = EIO;
		goto bad;
	}
#if 0
	/*
	 * "soft" write protect check
	 */
	if ((sd->flags & SDF_WRITEPROT) && (bp->b_flags & B_READ) == 0) {
		bp->b_error = EROFS;
		goto bad;
	}
#endif
	/*
	 * If it's a null transfer, return immediatly
	 */
	if (bp->b_bcount == 0)
		goto done;

	/*
	 * Do bounds checking, adjust transfer. if error, process.
	 * If end of partition, just return.
	 */
	if (bounds_check_with_label(bp, &sd->sc_dk.dk_label,
	    (sd->flags & SDF_WLABEL) != 0) <= 0)
		goto done;

	opri = splbio();

	/*
	 * Place it in the queue of disk activities for this disk
	 */
	disksort(&sd->buf_queue, bp);

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion
	 */
	sdstart(sd);

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
 * sdstart looks to see if there is a buf waiting for the device
 * and that the device is not already busy. If both are true,
 * It dequeues the buf and creates a scsi command to perform the
 * transfer in the buf. The transfer request will call scsi_done
 * on completion, which will in turn call this routine again
 * so that the next queued transfer is performed.
 * The bufs are queued by the strategy routine (sdstrategy)
 *
 * This routine is also called after other non-queued requests
 * have been made of the scsi driver, to ensure that the queue
 * continues to be drained.
 *
 * must be called at the correct (highish) spl level
 * sdstart() is called at splbio from sdstrategy and scsi_done
 */
void 
sdstart(sd)
	register struct sd_softc *sd;
{
	register struct	scsi_link *sc_link = sd->sc_link;
	struct buf *bp = 0;
	struct buf *dp;
	struct scsi_rw_big cmd;
	int blkno, nblks;
	struct partition *p;

	SC_DEBUG(sc_link, SDEV_DB2, ("sdstart "));
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
		dp = &sd->buf_queue;
		if ((bp = dp->b_actf) == NULL)	/* yes, an assign */
			return;
		dp->b_actf = bp->b_actf;

		/*
		 * If the device has become invalid, abort all the
		 * reads and writes until all files have been closed and
		 * re-opened
		 */
		if (!(sc_link->flags & SDEV_MEDIA_LOADED)) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			continue;
		}

		/*
		 * We have a buf, now we know we are going to go through
		 * With this thing..
		 *
		 *  First, translate the block to absolute
		 */
		blkno =
		    bp->b_blkno / (sd->sc_dk.dk_label.d_secsize / DEV_BSIZE);
		if (SDPART(bp->b_dev) != RAW_PART) {
			p = &sd->sc_dk.dk_label.d_partitions[SDPART(bp->b_dev)];
			blkno += p->p_offset;
		}
		nblks = howmany(bp->b_bcount, sd->sc_dk.dk_label.d_secsize);

		/*
		 *  Fill out the scsi command
		 */
		bzero(&cmd, sizeof(cmd));
		cmd.opcode = (bp->b_flags & B_READ) ? READ_BIG : WRITE_BIG;
		cmd.addr_3 = (blkno & 0xff000000) >> 24;
		cmd.addr_2 = (blkno & 0xff0000) >> 16;
		cmd.addr_1 = (blkno & 0xff00) >> 8;
		cmd.addr_0 = blkno & 0xff;
		cmd.length2 = (nblks & 0xff00) >> 8;
		cmd.length1 = (nblks & 0xff);

		/*
		 * Call the routine that chats with the adapter.
		 * Note: we cannot sleep as we may be an interrupt
		 */
		if (scsi_scsi_cmd(sc_link, (struct scsi_generic *)&cmd,
		    sizeof(cmd), (u_char *)bp->b_data, bp->b_bcount,
		    SDRETRIES, 10000, bp, SCSI_NOSLEEP |
		    ((bp->b_flags & B_READ) ? SCSI_DATA_IN : SCSI_DATA_OUT)))
			printf("%s: not queued", sd->sc_dev.dv_xname);
	}
}

/*
 * Perform special action on behalf of the user
 * Knows about the internals of this device
 */
int 
sdioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct sd_softc *sd = sdcd.cd_devs[SDUNIT(dev)];
	int error;

	/*
	 * If the device is not valid.. abandon ship
	 */
	if (!(sd->sc_link->flags & SDEV_MEDIA_LOADED))
		return EIO;

	switch (cmd) {
	case DIOCGDINFO:
		*(struct disklabel *)addr = sd->sc_dk.dk_label;
		return 0;

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = &sd->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &sd->sc_dk.dk_label.d_partitions[SDPART(dev)];
		return 0;

	case DIOCSDINFO:
		if ((flag & FWRITE) == 0)
			return EBADF;
		error = setdisklabel(&sd->sc_dk.dk_label,
		    (struct disklabel *)addr, /*sd->sc_dk.dk_openmask : */0,
		    &sd->sc_dk.dk_cpulabel);
		return error;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return EBADF;
		if (*(int *)addr)
			sd->flags |= SDF_WLABEL;
		else
			sd->flags &= ~SDF_WLABEL;
		return 0;

	case DIOCWDINFO:
		if ((flag & FWRITE) == 0)
			return EBADF;
		error = setdisklabel(&sd->sc_dk.dk_label,
		    (struct disklabel *)addr, /*sd->sc_dk.dk_openmask : */0,
		    &sd->sc_dk.dk_cpulabel);
		if (error == 0) {
			/* Simulate opening partition 0 so write succeeds. */
			sd->sc_dk.dk_openmask |= (1 << 0);	/* XXX */
			error = writedisklabel(SDLABELDEV(dev), sdstrategy,
			    &sd->sc_dk.dk_label, &sd->sc_dk.dk_cpulabel);
			sd->sc_dk.dk_openmask =
			    sd->sc_dk.dk_copenmask | sd->sc_dk.dk_bopenmask;
		}
		return error;

	default:
		if (SDPART(dev) != RAW_PART)
			return ENOTTY;
		return scsi_do_ioctl(sd->sc_link, dev, cmd, addr, flag, p);
	}

#ifdef DIAGNOSTIC
	panic("sdioctl: impossible");
#endif
}

/*
 * Load the label information on the named device
 */
void 
sdgetdisklabel(sd)
	struct sd_softc *sd;
{
	char *errstring;

	bzero(&sd->sc_dk.dk_label, sizeof(struct disklabel));
	bzero(&sd->sc_dk.dk_cpulabel, sizeof(struct cpu_disklabel));

	sd->sc_dk.dk_label.d_secsize = sd->params.blksize;
	sd->sc_dk.dk_label.d_ntracks = sd->params.heads;
	sd->sc_dk.dk_label.d_nsectors = sd->params.sectors;
	sd->sc_dk.dk_label.d_ncylinders = sd->params.cyls;
	sd->sc_dk.dk_label.d_secpercyl =
	    sd->sc_dk.dk_label.d_ntracks * sd->sc_dk.dk_label.d_nsectors;
	if (sd->sc_dk.dk_label.d_secpercyl == 0) {
		sd->sc_dk.dk_label.d_secpercyl = 100;
		/* as long as it's not 0 - readdisklabel divides by it (?) */
	}

	strncpy(sd->sc_dk.dk_label.d_typename, "SCSI disk", 16);
	sd->sc_dk.dk_label.d_type = DTYPE_SCSI;
	strncpy(sd->sc_dk.dk_label.d_packname, "ficticious", 16);
	sd->sc_dk.dk_label.d_secperunit = sd->params.disksize;
	sd->sc_dk.dk_label.d_rpm = 3600;
	sd->sc_dk.dk_label.d_interleave = 1;
	sd->sc_dk.dk_label.d_flags = 0;

	sd->sc_dk.dk_label.d_partitions[RAW_PART].p_offset = 0;
	sd->sc_dk.dk_label.d_partitions[RAW_PART].p_size =
	    sd->sc_dk.dk_label.d_secperunit *
	    (sd->sc_dk.dk_label.d_secsize / DEV_BSIZE);
	sd->sc_dk.dk_label.d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	sd->sc_dk.dk_label.d_npartitions = RAW_PART + 1;

	sd->sc_dk.dk_label.d_magic = DISKMAGIC;
	sd->sc_dk.dk_label.d_magic2 = DISKMAGIC;
	sd->sc_dk.dk_label.d_checksum = dkcksum(&sd->sc_dk.dk_label);

	/*
	 * Call the generic disklabel extraction routine
	 */
	if (errstring = readdisklabel(MAKESDDEV(0, sd->sc_dev.dv_unit,
	    RAW_PART), sdstrategy, &sd->sc_dk.dk_label,
	    &sd->sc_dk.dk_cpulabel)) {
		printf("%s: %s\n", sd->sc_dev.dv_xname, errstring);
		return;
	}
}

/*
 * Find out from the device what it's capacity is
 */
u_long 
sd_size(sd, flags)
	struct sd_softc *sd;
	int flags;
{
	struct scsi_read_cap_data rdcap;
	struct scsi_read_capacity scsi_cmd;
	u_long size;

	/*
	 * make up a scsi command and ask the scsi driver to do
	 * it for you.
	 */
	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.opcode = READ_CAPACITY;

	/*
	 * If the command works, interpret the result as a 4 byte
	 * number of blocks
	 */
	if (scsi_scsi_cmd(sd->sc_link, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)&rdcap, sizeof(rdcap), SDRETRIES,
	    2000, NULL, flags | SCSI_DATA_IN) != 0)
		return 0;

	size = (rdcap.addr_3 << 24) + (rdcap.addr_2 << 16) +
	    (rdcap.addr_1 << 8) + rdcap.addr_0 + 1;

	return size;
}

/*
 * Tell the device to map out a defective block
 */
int 
sd_reassign_blocks(sd, block)
	struct sd_softc *sd;
	u_long block;
{
	struct scsi_reassign_blocks scsi_cmd;
	struct scsi_reassign_blocks_data rbdata;

	bzero(&scsi_cmd, sizeof(scsi_cmd));
	bzero(&rbdata, sizeof(rbdata));
	scsi_cmd.opcode = REASSIGN_BLOCKS;

	rbdata.length_msb = 0;
	rbdata.length_lsb = sizeof(rbdata.defect_descriptor[0]);
	rbdata.defect_descriptor[0].dlbaddr_3 = ((block >> 24) & 0xff);
	rbdata.defect_descriptor[0].dlbaddr_2 = ((block >> 16) & 0xff);
	rbdata.defect_descriptor[0].dlbaddr_1 = ((block >> 8) & 0xff);
	rbdata.defect_descriptor[0].dlbaddr_0 = ((block) & 0xff);

	return scsi_scsi_cmd(sd->sc_link, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)&rbdata, sizeof(rbdata), SDRETRIES,
	    5000, NULL, SCSI_DATA_OUT);
}

#define b2tol(a)	(((unsigned)(a##_1) << 8) + (unsigned)a##_0 )

/*
 * Get the scsi driver to send a full inquiry to the * device and use the
 * results to fill out the disk parameter structure.
 */
int 
sd_get_parms(sd, flags)
	struct sd_softc *sd;
	int flags;
{
	struct disk_parms *dp = &sd->params;
	struct scsi_mode_sense scsi_cmd;
	struct scsi_mode_sense_data {
		struct scsi_mode_header header;
		struct scsi_blk_desc blk_desc;
		union disk_pages pages;
	} scsi_sense;
	u_long sectors;

	/*
	 * do a "mode sense page 4"
	 */
	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.opcode = MODE_SENSE;
	scsi_cmd.page = 4;
	scsi_cmd.length = 0x20;
	/*
	 * If the command worked, use the results to fill out
	 * the parameter structure
	 */
	if (scsi_scsi_cmd(sd->sc_link, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)&scsi_sense, sizeof(scsi_sense),
	    SDRETRIES, 6000, NULL, flags | SCSI_DATA_IN) != 0) {
		printf("%s: could not mode sense (4)", sd->sc_dev.dv_xname);
	fake_it:
		printf("; using ficticious geometry\n");
		/*
		 * use adaptec standard ficticious geometry
		 * this depends on which controller (e.g. 1542C is
		 * different. but we have to put SOMETHING here..)
		 */
		sectors = sd_size(sd, flags);
		dp->heads = 64;
		dp->sectors = 32;
		dp->cyls = sectors / (64 * 32);
		dp->blksize = 512;
		dp->disksize = sectors;
	} else {
		SC_DEBUG(sd->sc_link, SDEV_DB3,
		    ("%d cyls, %d heads, %d precomp, %d red_write, %d land_zone\n",
		    _3btol(&scsi_sense.pages.rigid_geometry.ncyl_2),
		    scsi_sense.pages.rigid_geometry.nheads,
		    b2tol(scsi_sense.pages.rigid_geometry.st_cyl_wp),
		    b2tol(scsi_sense.pages.rigid_geometry.st_cyl_rwc),
		    b2tol(scsi_sense.pages.rigid_geometry.land_zone)));

		/*
		 * KLUDGE!! (for zone recorded disks)
		 * give a number of sectors so that sec * trks * cyls
		 * is <= disk_size 
		 * can lead to wasted space! THINK ABOUT THIS !
		 */
		dp->heads = scsi_sense.pages.rigid_geometry.nheads;
		dp->cyls =
		    _3btol(&scsi_sense.pages.rigid_geometry.ncyl_2);
		dp->blksize = _3btol(scsi_sense.blk_desc.blklen);

		if (dp->heads == 0 || dp->cyls == 0) {
			printf("%s: mode sense (4) returned nonsense",
			    sd->sc_dev.dv_xname);
			goto fake_it;
		}

		if (dp->blksize == 0)
			dp->blksize = 512;

		sectors = sd_size(sd, flags);
		dp->disksize = sectors;
		sectors /= (dp->heads * dp->cyls);
		dp->sectors = sectors;	/* XXX dubious on SCSI */
	}

	return 0;
}

int
sdsize(dev)
	dev_t dev;
{
	struct sd_softc *sd;
	int part;
	int size;

	if (sdopen(dev, 0, S_IFBLK) != 0)
		return -1;
	sd = sdcd.cd_devs[SDUNIT(dev)];
	part = SDPART(dev);
	if (sd->sc_dk.dk_label.d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = sd->sc_dk.dk_label.d_partitions[part].p_size;
	if (sdclose(dev, 0, S_IFBLK) != 0)
		return -1;
	return size;
}


#define SCSIDUMP 1
#undef	SCSIDUMP
#define NOT_TRUSTED 1

#ifdef SCSIDUMP
#include <vm/vm.h>

static struct scsi_xfer sx;
#define	MAXTRANSFER 8		/* 1 page at a time */

/*
 * dump all of physical memory into the partition specified, starting
 * at offset 'dumplo' into the partition.
 */
int
sddump(dev_t dev)
{				/* dump core after a system crash */
	register struct sd_softc *sd;	/* disk unit to do the IO */
	int32	num;		/* number of sectors to write */
	u_int32	unit, part;
	int32	blkoff, blknum, blkcnt = MAXTRANSFER;
	int32	nblocks;
	char	*addr;
	struct	scsi_rw_big cmd;
	extern	int Maxmem;
	static	int sddoingadump = 0;
#define MAPTO CADDR1
	extern	caddr_t MAPTO;	/* map the page we are about to write, here */
	struct	scsi_xfer *xs = &sx;
	int	retval;
	int	c;

	addr = (char *) 0;	/* starting address */

	/* toss any characters present prior to dump */
	while ((c = sgetc(1)) && (c != 0x100)); /*syscons and pccons differ */

	/* size of memory to dump */
	num = Maxmem;
	unit = SDUNIT(dev);	/* eventually support floppies? */
	part = SDPART(dev);	/* file system */
	/* check for acceptable drive number */
	if (unit >= sdcd.cd_ndevs)
		return ENXIO;

	sd = sd_softc[unit];
	if (!sd)
		return ENXIO;
	if (sd->sc_link->flags & SDEV_MEDIA_LOADED != SDEV_MEDIA_LOADED)
		return ENXIO;

	/* Convert to disk sectors */
	num = (u_int32)num * NBPG / sd->sc_dk.dk_label.d_secsize;

	/* check if controller active */
	if (sddoingadump)
		return EFAULT;

	nblocks = sd->sc_dk.dk_label.d_partitions[part].p_size;
	blkoff = sd->sc_dk.dk_label.d_partitions[part].p_offset;

	/* check transfer bounds against partition size */
	if ((dumplo < 0) || ((dumplo + num) > nblocks))
		return EINVAL;

	sddoingadump = 1;

	blknum = dumplo + blkoff;
	while (num > 0) {
		pmap_enter(kernel_pmap,
		    MAPTO,
		    trunc_page(addr),
		    VM_PROT_READ,
		    TRUE);
#ifndef	NOT_TRUSTED
		/*
		 *  Fill out the scsi command
		 */
		bzero(&cmd, sizeof(cmd));
		cmd.opcode = WRITE_BIG;
		cmd.addr_3 = (blknum & 0xff000000) >> 24;
		cmd.addr_2 = (blknum & 0xff0000) >> 16;
		cmd.addr_1 = (blknum & 0xff00) >> 8;
		cmd.addr_0 = blknum & 0xff;
		cmd.length2 = (blkcnt & 0xff00) >> 8;
		cmd.length1 = (blkcnt & 0xff);
		/*
		 * Fill out the scsi_xfer structure
		 *    Note: we cannot sleep as we may be an interrupt
		 * don't use scsi_scsi_cmd() as it may want 
		 * to wait for an xs.
		 */
		bzero(xs, sizeof(sx));
		xs->flags |= SCSI_AUTOCONF | INUSE;
		xs->sc_link = sd->sc_link;
		xs->retries = SDRETRIES;
		xs->timeout = 10000;	/* 10000 millisecs for a disk ! */
		xs->cmd = (struct scsi_generic *)&cmd;
		xs->cmdlen = sizeof(cmd);
		xs->resid = blkcnt * 512;
		xs->error = XS_NOERROR;
		xs->bp = 0;
		xs->data = (u_char *) MAPTO;
		xs->datalen = blkcnt * 512;

		/*
		 * Pass all this info to the scsi driver.
		 */
		retval = (*(sd->sc_link->adapter->scsi_cmd)) (xs);
		if (retval != COMPLETE)
			return ENXIO;
#else	/* NOT_TRUSTED */
		/* lets just talk about this first... */
		printf("sd%d: dump addr 0x%x, blk %d\n", unit, addr, blknum);
#endif	/* NOT_TRUSTED */

		if ((unsigned)addr % (1024 * 1024) == 0)
			printf("%d ", num / 2048);
		/* update block count */
		num -= blkcnt;
		blknum += blkcnt;
		(int)addr += 512 * blkcnt;

		/* operator aborting dump? */
		if ((c = sgetc(1)) && (c != 0x100))
			return EINTR;
	}
	return 0;
}
#else	/* SCSIDUMP */
int
sddump()
{
	printf("\nsddump()        -- not implemented\n");
	delay(6000000);		/* 6 seconds */
	return -1;
}
#endif	/* SCSIDUMP */
