/*	$NetBSD: rz.c,v 1.47.2.2 2001/03/29 09:03:02 bouyer Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson of Lawrence Berkeley Laboratory and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)rz.c	8.1 (Berkeley) 7/29/93
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: rz.c,v 1.47.2.2 2001/03/29 09:03:02 bouyer Exp $");

/*
 * SCSI CCS (Command Command Set) disk driver.
 * NOTE: The name was changed from "sd" to "rz" for DEC naming compatibility.
 * I guess I can't avoid confusion someplace.
 */

#include "rz.h"
#include "rnd.h"		/* is random device-driver configured? */

#if NRZ > 0

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#if NRND > 0
#include <sys/rnd.h>
#endif	/* NRND */

#include <ufs/ufs/dinode.h>		/* XXX for fs.h */
#include <ufs/ffs/fs.h>			/* XXX for BBSIZE & SBSIZE */

#include <dev/scsipi/scsi_all.h>	/* MI SCSI defs for mode-sense */
#include <dev/scsipi/scsi_disk.h>	/* disk-specific sense data */
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_cd.h>	/* cd-specific sense data */
#include <dev/scsipi/scsipi_disk.h>	/* disk-specific sense data */
#include <dev/scsipi/scsipiconf.h>

#include <machine/conf.h>

#include <pmax/dev/device.h>		/* 4.4BSD/pmax driver definitions */
#include <pmax/dev/scsi.h>


#define	SDRETRIES	4
#define	CDRETRIES	4

struct rz_softc;
struct scsi_mode_sense_data;
struct disk_parms;

static void	rzdone __P((int unit, int error, int resid, int status));
static void	rzgetdefaultlabel __P((struct rz_softc *, struct disklabel *lp));
static void	rzgetinfo __P((dev_t dev));
static void	rzlblkstrat __P((struct buf *bp, int bsize));
static int	rzprobe __P((void /*struct pmax_scsi_device*/ *sd));
static int	rzready __P((struct rz_softc *sc));
static void	rzstart __P((int unit));


/* Machinery for format and drive inquiry commands. */
static int	rz_command __P((struct rz_softc *sc,
			struct scsipi_generic *scsipi_cmd, int cmdlen,
			u_char *data_addr, int data_len,
			int nretries,	int timeout,
			struct buf *bp, u_int flags));

static int	rz_mode_sense __P((struct rz_softc *sd,
		    struct scsi_mode_sense_data *scsipi_sense, int page,
		    int pagelen, int flags));
static int	rz_getsize __P((struct rz_softc *sc, int flags));
static void	rzgetgeom __P((struct rz_softc *, int flags));
static void	rz_setlabelgeom __P((struct disklabel *lp, struct disk_parms *dp));
static u_long	rz_cdsize __P((struct rz_softc *cd, int flags));


struct	pmax_driver rzdriver = {
	"rz", rzprobe,
	(void (*) __P((struct ScsiCmd *cmd))) rzstart,
	rzdone,
};

struct	size {
	u_long	strtblk;
	u_long	nblocks;
#define RZ_END ((u_long) -1)
};

/*
 * Since the SCSI standard tends to hide the disk structure, we define
 * partitions in terms of DEV_BSIZE blocks.  The default partition table
 * (for an unlabeled disk) reserves 8K for a boot area, has an 8 meg
 * root and 32 meg of swap.  The rest of the space on the drive goes in
 * the G partition.  As usual, the C partition covers the entire disk
 * (including the boot area).
 */
static struct size rzdefaultpart[MAXPARTITIONS] = {
	/*   Start     Size         Partition */
	{       0,   65536 },	/* A -- 32Mbyte root */
	{   63356,  131072 },	/* B -- 64Mbyte swap */
	{       0,  RZ_END },	/* C -- entire disk */
	{  196608,   16384 },	/* D -- 8meg for var or miniroots */
	{  212992,  409600 },	/* E -- /usr */
	{  622592,  409600 },	/* F -- /home, alternate /usr */
	{ 1032912,  RZ_END },	/* G -- F to end of disk */
	{  196608,  RZ_END }	/* H -- B to end of disk */
};

struct rzstats {
	long	rzresets;
	long	rztransfers;
	long	rzpartials;
};

static	struct rz_softc {
	struct	device sc_dev;		/* new config glue */
	struct	pmax_scsi_device *sc_sd;	/* physical unit info */
	pid_t	sc_format_pid;		/* process using "format" mode */
	short	sc_flags;		/* see below */
	short	sc_type;		/* drive type from INQUIRY cmd */
	u_int	sc_blks;		/* number of blocks on device */
	int	sc_blksize;		/* device block size in bytes */
	struct	disk sc_dkdev;		/* generic disk device info */
#define	sc_label	sc_dkdev.dk_label	/* XXX compat */
#define	sc_openpart	sc_dkdev.dk_openmask	/* XXX compat */
#define	sc_bopenpart	sc_dkdev.dk_bopenmask	/* XXX compat */
#define	sc_copenpart	sc_dkdev.dk_copenmask	/* XXX compat */
#define	sc_bshift	sc_dkdev.dk_blkshift	/* XXX compat */
	struct	rzstats sc_stats;	/* statisic counts */
	struct	buf_queue sc_tab;	/* queue of pending operations */
	int	sc_active;		/* number of active requests */
	struct	buf sc_buf;		/* buf for doing I/O */
	struct	buf sc_errbuf;		/* buf for doing REQUEST_SENSE */
	struct	ScsiCmd sc_cmd;		/* command for controller */
	ScsiGroup1Cmd sc_rwcmd;		/* SCSI cmd if not in "format" mode */
	struct	scsi_fmt_cdb sc_cdb;	/* SCSI cmd if in "format" mode */
	struct	scsi_fmt_sense sc_sense;	/* sense data from last cmd */
	struct disk_parms {
		u_char heads;		/* number of heads */
		u_short cyls;		/* number of cylinders */
		u_char sectors;		/* number of sectors/track */
		int blksize;		/* number of bytes/sector */
		u_long disksize;	/* total number sectors */
	} params;
	u_char	sc_capbuf[128];		/* buffer for SCSI_READ_CAPACITY */

#if NRND > 0
	rndsource_element_t rnd_source;
#endif	/* NRND */

} rz_softc[NRZ];

/* sc_flags values */
#define	RZF_ALIVE		0x0001	/* drive found and ready */
#define	RZF_SENSEINPROGRESS	0x0002	/* REQUEST_SENSE command in progress */
#define	RZF_ALTCMD		0x0004	/* alternate command in progress */
#define	RZF_HAVELABEL		0x0008	/* valid label found on disk */
#define	RZF_WLABEL		0x0010	/* label is writeable */
#define	RZF_WAIT		0x0020	/* waiting for sc_tab to drain */
#define	RZF_REMOVEABLE		0x0040	/* disk is removable */
#define	RZF_TRYSYNC		0x0080	/* try synchronous operation */
#define	RZF_NOERR		0x0100	/* don't print error messages */
#define	RZF_FAKEGEOM		0x02000 /* couldn't get geometry */

#ifdef DEBUG
#define RZB_ERROR	0x01
#define RZB_PARTIAL	0x02
#define RZB_PRLABEL	0x04
int	rzdebug = RZB_ERROR;
#endif

#define	rzunit(x)	(minor(x) >> 3)
#define rzpart(x)	(minor(x) & 0x7)

struct scsi_mode_sense_data {
	struct scsi_mode_header header;
	struct scsi_blk_desc blk_desc;
	union scsi_disk_pages pages;
};

/*
 * Table of scsi commands users are allowed to access via "format" mode.
 *  0 means not legal.
 *  1 means legal.
 */
static char legal_cmds[256] = {
/*****  0   1   2   3   4   5   6   7     8   9   A   B   C   D   E   F */
/*00*/	0,  0,  0,  0,  1,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*10*/	0,  0,  1,  0,  0,  1,  0,  0,    0,  0,  1,  0,  0,  0,  0,  0,
/*20*/	0,  0,  0,  0,  0,  1,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*30*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*40*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*50*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*60*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*70*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*80*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*90*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*a0*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*b0*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*c0*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*d0*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*e0*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
/*f0*/	0,  0,  0,  0,  0,  0,  0,  0,    0,  0,  0,  0,  0,  0,  0,  0,
};

/*
 * Test to see if the unit is ready and if not, try to make it ready.
 * Also, find the drive capacity.
 */
static int
rzready(sc)
	struct rz_softc *sc;
{
	int tries;
	ScsiClass7Sense *sp;

	/* don't print SCSI errors */
	sc->sc_flags |= RZF_NOERR;

	/* see if the device is ready */
	for (tries = 10; ; ) {
		sc->sc_cdb.len = sizeof(ScsiGroup0Cmd);
		scsiGroup0Cmd(SCSI_TEST_UNIT_READY, sc->sc_rwcmd.unitNumber,
			0, 0, (ScsiGroup0Cmd *)sc->sc_cdb.cdb);
		sc->sc_buf.b_flags = B_BUSY | B_PHYS | B_READ;
		sc->sc_buf.b_bcount = 0;
		sc->sc_buf.b_data = (caddr_t)0;
		BUFQ_INSERT_HEAD(&sc->sc_tab, &sc->sc_buf);

		sc->sc_cmd.cmd = sc->sc_cdb.cdb;
		sc->sc_cmd.cmdlen = sc->sc_cdb.len;
		sc->sc_cmd.buf = (caddr_t)0;
		sc->sc_cmd.buflen = 0;
		/* setup synchronous data transfers if the device supports it */
		if (tries == 10 && (sc->sc_flags & RZF_TRYSYNC))
			sc->sc_cmd.flags = SCSICMD_USE_SYNC;
		else
			sc->sc_cmd.flags = 0;

		disk_busy(&sc->sc_dkdev);	/* XXX */
		(*sc->sc_sd->sd_cdriver->d_start)(&sc->sc_cmd);
		if (!biowait(&sc->sc_buf))
			break;
		if (--tries < 0)
			return (0);
		if (!(sc->sc_sense.status & SCSI_STATUS_CHECKCOND))
			goto again;
		sp = (ScsiClass7Sense *)sc->sc_sense.sense;
		if (sp->error7 != 0x70)
			goto again;
		if (sp->key == SCSI_CLASS7_UNIT_ATTN && tries != 9) {
			/* drive recalibrating, give it a while */
			DELAY(1000000);
			continue;
		}
		if (sp->key == SCSI_CLASS7_NOT_READY) {
			ScsiStartStopCmd *cp;

			/* try to spin-up disk with start/stop command */
			sc->sc_cdb.len = sizeof(ScsiGroup0Cmd);
			cp = (ScsiStartStopCmd *)sc->sc_cdb.cdb;
			cp->command = SCSI_START_STOP;
			cp->unitNumber = sc->sc_rwcmd.unitNumber;
			cp->immed = 0;
			cp->loadEject = 0;
			cp->start = 1;
			cp->pad1 = 0;
			cp->pad2 = 0;
			cp->pad3 = 0;
			cp->pad4 = 0;
			cp->control = 0;
			sc->sc_buf.b_flags = B_BUSY | B_PHYS | B_READ;
			sc->sc_buf.b_bcount = 0;
			sc->sc_buf.b_data = (caddr_t)0;
			BUFQ_INSERT_HEAD(&sc->sc_tab, &sc->sc_buf);
			rzstart(sc->sc_cmd.unit);
			if (biowait(&sc->sc_buf))
				return (0);
			/*
			 * The RZ25 seems to need a short delay here.  It
			 * appears to take the test unit ready command OK,
			 * but then fails with a "not ready" when getting
			 * thie size.
			 */
			DELAY(1000);
			continue;
		}
	again:
		DELAY(1000);
	}

	/* print SCSI errors */
	sc->sc_flags &= ~RZF_NOERR;

	/* find out how big a disk this is. Punt on error. */
	if (rz_getsize(sc, 0) == 0) {
		return 0;
	}

	/*XXX perhaps move to rzprobe? */
	rzgetgeom(sc, SCSI_SILENT);
	return (1);
}


static int
rz_getsize(sc, flags)
	struct rz_softc *sc;
	int flags;
{
	int i;

	if (sc->sc_type == SCSI_ROM_TYPE) {
		int cdsize;

		cdsize = rz_cdsize(sc, flags);
		sc->params.disksize = cdsize;
		sc->sc_bshift = 0;
		for (i = sc->sc_blksize; i > DEV_BSIZE; i >>= 1)
			++sc->sc_bshift;
		sc->sc_blks <<= sc->sc_bshift;
		return (cdsize);
	}

	sc->sc_cdb.len = sizeof(ScsiGroup1Cmd);
	scsiGroup1Cmd(SCSI_READ_CAPACITY, sc->sc_rwcmd.unitNumber, 0, 0,
		(ScsiGroup1Cmd *)sc->sc_cdb.cdb);
	sc->sc_buf.b_flags = B_BUSY | B_PHYS | B_READ;
	sc->sc_buf.b_bcount = 8; /* XXX 8 was sizeof(sc->sc_capbuf). */
	sc->sc_buf.b_data = (caddr_t)sc->sc_capbuf;
	BUFQ_INSERT_HEAD(&sc->sc_tab, &sc->sc_buf);
	sc->sc_flags |= RZF_ALTCMD;
	rzstart(sc->sc_cmd.unit);
	sc->sc_flags &= ~RZF_ALTCMD;
	if (biowait(&sc->sc_buf) || sc->sc_buf.b_resid != 0) {
		return (0);
	}
	sc->sc_blks = ((sc->sc_capbuf[0] << 24) | (sc->sc_capbuf[1] << 16) |
		(sc->sc_capbuf[2] << 8) | sc->sc_capbuf[3]) + 1;
	sc->sc_blksize = (sc->sc_capbuf[4] << 24) | (sc->sc_capbuf[5] << 16) |
		(sc->sc_capbuf[6] << 8) | sc->sc_capbuf[7];

	sc->sc_bshift = 0;
	for (i = sc->sc_blksize; i > DEV_BSIZE; i >>= 1)
		++sc->sc_bshift;
	sc->sc_blks <<= sc->sc_bshift;

	return (sc->sc_blks);
}


/*
 * Test to see if device is present.
 * Return true if found and initialized ok.
 */
static int
rzprobe(xxxsd)
	void *xxxsd;
{
	struct pmax_scsi_device *sd = xxxsd;
	struct rz_softc *sc = &rz_softc[sd->sd_unit];
	int i;
	struct disk_parms *dp = &sc->params;
	ScsiInquiryData inqbuf;

	if (sd->sd_unit >= NRZ)
		return (0);

	BUFQ_INIT(&sc->sc_tab);
	callout_init(&sc->sc_cmd.timo_ch);

	/* init some parameters that don't change */
	sc->sc_sd = sd;
	sc->sc_cmd.sd = sd;
	sc->sc_cmd.unit = sd->sd_unit;
	sc->sc_cmd.lun = 0;
	sc->sc_rwcmd.unitNumber = sd->sd_slave;

	/* XXX set up the external name */
	bzero(&sc->sc_dev, sizeof(sc->sc_dev));			/* XXX */
	sprintf(sc->sc_dev.dv_xname, "rz%d", sd->sd_unit);	/* XXX */
	sc->sc_dev.dv_unit = sd->sd_unit;			/* XXX */
	sc->sc_dev.dv_class = DV_DISK;				/* XXX */

	/* Initialize the disk structure. */
	bzero(&sc->sc_dkdev, sizeof(sc->sc_dkdev));
	sc->sc_dkdev.dk_name = sc->sc_dev.dv_xname;

	/* try to find out what type of device this is */
	sc->sc_format_pid = 1;		/* force use of sc_cdb */
	sc->sc_flags = RZF_NOERR;	/* don't print SCSI errors */
	sc->sc_cdb.len = sizeof(ScsiGroup0Cmd);
	scsiGroup0Cmd(SCSI_INQUIRY, sd->sd_slave, 0, sizeof(inqbuf),
		(ScsiGroup0Cmd *)sc->sc_cdb.cdb);
	sc->sc_buf.b_flags = B_BUSY | B_PHYS | B_READ;
	sc->sc_buf.b_bcount = sizeof(inqbuf);
	sc->sc_buf.b_data = (caddr_t)&inqbuf;
	BUFQ_INSERT_HEAD(&sc->sc_tab, &sc->sc_buf);
	rzstart(sd->sd_unit);

/*XXX*/	/*printf("probe rz%d\n", sd->sd_unit);*/

	if (biowait(&sc->sc_buf) ||
	    (i = sizeof(inqbuf) - sc->sc_buf.b_resid) < 5)
		goto bad;
	switch (inqbuf.type) {
	case SCSI_DISK_TYPE:		/* disk */
	case SCSI_WORM_TYPE:		/* WORM */
	case SCSI_ROM_TYPE:		/* CD-ROM */
	case SCSI_OPTICAL_MEM_TYPE:	/* Magneto-optical */
		break;

	case SCSI_TAPE_TYPE:		/* tape, handled by tz driver */
		goto bad;

	default:			/* not a disk (or tape) */
		printf("rz%d: unknown media code 0x%x\n",
		       sd->sd_unit, inqbuf.type);
		goto bad;
	}
	sc->sc_type = inqbuf.type;
	if (inqbuf.flags & SCSI_SYNC)
		sc->sc_flags |= RZF_TRYSYNC;

	if (!inqbuf.rmb) {
		if (!rzready(sc))
			goto bad;
	}

	printf("rz%d at %s%d drive %d slave %d", sd->sd_unit,
		sd->sd_cdriver->d_name, sd->sd_ctlr, sd->sd_drive,
		sd->sd_slave);

	if (inqbuf.version < 1 || i < 36)
		printf(" type 0x%x, qual 0x%x, ver %d",
			inqbuf.type, inqbuf.qualifier, inqbuf.version);
	else {
		char vid[9], pid[17], revl[5];

		bcopy((caddr_t)inqbuf.vendorID, (caddr_t)vid, 8);
		bcopy((caddr_t)inqbuf.productID, (caddr_t)pid, 16);
		bcopy((caddr_t)inqbuf.revLevel, (caddr_t)revl, 4);
		for (i = 8; --i > 0; )
			if (vid[i] != ' ')
				break;
		vid[i+1] = 0;
		for (i = 16; --i > 0; )
			if (pid[i] != ' ')
				break;
		pid[i+1] = 0;
		for (i = 4; --i > 0; )
			if (revl[i] != ' ')
				break;
		revl[i+1] = 0;
		printf(" %s %s rev %s (SCSI-%d)", vid, pid, revl,
		    inqbuf.version);
	}

	printf ("%s\n",
	    (sc->sc_flags & RZF_FAKEGEOM) ? "; using fake geometry": "");
	if (dp->blksize)
	    printf("rz%d: %ldMB, %d cyl, %d head, %d sec, %d bytes/sect x %ld sectors\n",
		sd->sd_unit,
		dp->disksize / (1048576 / dp->blksize), dp->cyls,
		dp->heads, dp->sectors, dp->blksize, dp->disksize);

	if (!inqbuf.rmb && sc->sc_blksize != DEV_BSIZE) {
		if (sc->sc_blksize < DEV_BSIZE) {
			printf("rz%d: need %d byte blocks - drive ignored\n",
				sd->sd_unit, DEV_BSIZE);
			goto bad;
		}
	}

	/* Attach the disk. */
	disk_attach(&sc->sc_dkdev);

	sc->sc_format_pid = 0;
	sc->sc_flags |= RZF_ALIVE;
	if (inqbuf.rmb)
		sc->sc_flags |= RZF_REMOVEABLE;
	sc->sc_buf.b_flags = 0;

	sd->sd_devp = &sc->sc_dev;				/* XXX */
	TAILQ_INSERT_TAIL(&alldevs, &sc->sc_dev, dv_list);	/* XXX */

#if NRND > 0
	/*
	 * attach the device into the random source list
	 */
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_DISK, 0);
#endif	/* NRND */

	return (1);

bad:
	/* doesn't exist or not a CCS device */
	sc->sc_format_pid = 0;
	sc->sc_buf.b_flags = 0;
	return (0);
}

/*
 * This routine is called for partial block transfers and non-aligned
 * transfers (the latter only being possible on devices with a block size
 * larger than DEV_BSIZE).  The operation is performed in three steps
 * using a locally allocated buffer:
 *	1. transfer any initial partial block
 *	2. transfer full blocks
 *	3. transfer any final partial block
 */
static void
rzlblkstrat(bp, bsize)
	struct buf *bp;
	int bsize;
{
	struct buf *cbp;
	caddr_t cbuf;
	int bn, resid;
	caddr_t addr;

	cbp = (struct buf *)malloc(sizeof(struct buf), M_DEVBUF, M_WAITOK);
	cbuf = (caddr_t)malloc(bsize, M_DEVBUF, M_WAITOK);
	bzero((caddr_t)cbp, sizeof(*cbp));
	cbp->b_proc = curproc;
	cbp->b_dev = bp->b_dev;
	bn = bp->b_blkno;
	resid = bp->b_bcount;
	addr = bp->b_data;
#ifdef DEBUG
	if (rzdebug & RZB_PARTIAL)
		printf("rzlblkstrat: bp %p flags %lx bn %x resid %x addr %p\n",
		       bp, bp->b_flags, bn, resid, addr);
#endif

	while (resid > 0) {
		int boff = dbtob(bn) & (bsize - 1);
		int count;

		if (boff || resid < bsize) {
			rz_softc[rzunit(bp->b_dev)].sc_stats.rzpartials++;
			count = min(resid, bsize - boff);
			cbp->b_flags = B_BUSY | B_PHYS | B_READ;
			cbp->b_blkno = bn - btodb(boff);
			cbp->b_data = cbuf;
			cbp->b_bcount = bsize;
#ifdef DEBUG
			if (rzdebug & RZB_PARTIAL)
				printf(" readahead: bn %x cnt %x off %x addr %p\n",
				       cbp->b_blkno, count, boff, addr);
#endif
			rzstrategy(cbp);
			biowait(cbp);
			if (cbp->b_flags & B_ERROR) {
				bp->b_flags |= B_ERROR;
				bp->b_error = cbp->b_error;
				break;
			}
			if (bp->b_flags & B_READ) {
				bcopy(&cbuf[boff], addr, count);
				goto done;
			}
			bcopy(addr, &cbuf[boff], count);
#ifdef DEBUG
			if (rzdebug & RZB_PARTIAL)
				printf(" writeback: bn %x cnt %x off %x addr %p\n",
				       cbp->b_blkno, count, boff, addr);
#endif
		} else {
			count = resid & ~(bsize - 1);
			cbp->b_blkno = bn;
			cbp->b_data = addr;
			cbp->b_bcount = count;
#ifdef DEBUG
			if (rzdebug & RZB_PARTIAL)
				printf(" fulltrans: bn %x cnt %x addr %p\n",
				       cbp->b_blkno, count, addr);
#endif
		}
		cbp->b_flags = B_BUSY | B_PHYS | (bp->b_flags & B_READ);
		rzstrategy(cbp);
		biowait(cbp);
		if (cbp->b_flags & B_ERROR) {
			bp->b_flags |= B_ERROR;
			bp->b_error = cbp->b_error;
			break;
		}
done:
		bn += btodb(count);
		resid -= count;
		addr += count;
#ifdef DEBUG
		if (rzdebug & RZB_PARTIAL)
			printf(" done: bn %x resid %x addr %p\n",
			       bn, resid, addr);
#endif
	}
	free(cbuf, M_DEVBUF);
	free(cbp, M_DEVBUF);
}

void
rzstrategy(bp)
	struct buf *bp;
{
	int unit = rzunit(bp->b_dev);
	int part = rzpart(bp->b_dev);
	struct rz_softc *sc = &rz_softc[unit];
	struct partition *pp = &sc->sc_label->d_partitions[part];
	daddr_t bn;
	long sz, s;

	if (sc->sc_format_pid) {
		if (sc->sc_format_pid != curproc->p_pid) {
			bp->b_error = EPERM;
			goto bad;
		}
		bp->b_rawblkno = 0;
	} else {
		bn = bp->b_blkno;
		sz = howmany(bp->b_bcount, DEV_BSIZE);
		if ((unsigned)bn + sz > pp->p_size) {
			sz = pp->p_size - bn;
			/* if exactly at end of disk, return an EOF */
			if (sz == 0) {
				bp->b_resid = bp->b_bcount;
				goto done;
			}
			/* if none of it fits, error */
			if (sz < 0) {
				bp->b_error = EINVAL;
				goto bad;
			}
			/* otherwise, truncate */
			bp->b_bcount = dbtob(sz);
		}
		/*
		 * Check for write to write protected label (except on the
		 * raw partition).
		 */
		if (part != RAW_PART &&
		    bn + pp->p_offset <= LABELSECTOR &&
#if LABELSECTOR != 0
		    bn + pp->p_offset + sz > LABELSECTOR &&
#endif
		    !(bp->b_flags & B_READ) && !(sc->sc_flags & RZF_WLABEL)) {
			bp->b_error = EROFS;
			goto bad;
		}
		/*
		 * Non-aligned or partial-block transfers handled specially.
		 */
		s = sc->sc_blksize - 1;
		if ((dbtob(bn) & s) || (bp->b_bcount & s)) {
			rzlblkstrat(bp, sc->sc_blksize);
			goto done;
		}
		bp->b_rawblkno = (bn + pp->p_offset) >> sc->sc_bshift;
	}
	/* don't let disksort() see sc_errbuf */
	while (sc->sc_flags & RZF_SENSEINPROGRESS)
		printf("SENSE\n"); /* XXX */
	s = splbio();
	disksort_blkno(&sc->sc_tab, bp);
	if (sc->sc_active == 0) {
		sc->sc_active = 1;
		rzstart(unit);
	}
	splx(s);
	return;
bad:
	bp->b_flags |= B_ERROR;
done:
	biodone(bp);
}

static void
rzstart(unit)
	int unit;
{
	struct rz_softc *sc = &rz_softc[unit];
	struct buf *bp = BUFQ_FIRST(&sc->sc_tab);
	int n;

	sc->sc_cmd.buf = bp->b_data;
	sc->sc_cmd.buflen = bp->b_bcount;

	if (sc->sc_format_pid ||
	    (sc->sc_flags & (RZF_SENSEINPROGRESS | RZF_ALTCMD))) {
		sc->sc_cmd.flags = !(bp->b_flags & B_READ) ?
			SCSICMD_DATA_TO_DEVICE : 0;
		sc->sc_cmd.cmd = sc->sc_cdb.cdb;
		sc->sc_cmd.cmdlen = sc->sc_cdb.len;
	} else {
		if (bp->b_flags & B_READ) {
			sc->sc_cmd.flags = 0;
			sc->sc_rwcmd.command = SCSI_READ_EXT;
		} else {
			sc->sc_cmd.flags = SCSICMD_DATA_TO_DEVICE;
			sc->sc_rwcmd.command = SCSI_WRITE_EXT;
		}
		sc->sc_cmd.cmd = (u_char *)&sc->sc_rwcmd;
		sc->sc_cmd.cmdlen = sizeof(sc->sc_rwcmd);
		n = bp->b_rawblkno;
		sc->sc_rwcmd.highAddr = n >> 24;
		sc->sc_rwcmd.midHighAddr = n >> 16;
		sc->sc_rwcmd.midLowAddr = n >> 8;
		sc->sc_rwcmd.lowAddr = n;
		n = howmany(bp->b_bcount, (sc->sc_blksize) ? sc->sc_blksize :
							     DEV_BSIZE);
		sc->sc_rwcmd.highBlockCount = n >> 8;
		sc->sc_rwcmd.lowBlockCount = n;

#ifdef DEBUG
		if ((bp->b_bcount & (sc->sc_blksize - 1)) != 0)
			printf("rz%d: partial block xfer -- %lx bytes\n",
				unit, bp->b_bcount);
#endif
		sc->sc_stats.rztransfers++;
	}


	/* Instrumentation. */
	disk_busy(&sc->sc_dkdev);
	sc->sc_dkdev.dk_seek++;		/* XXX */

	/* tell controller to start this command */
	(*sc->sc_sd->sd_cdriver->d_start)(&sc->sc_cmd);
}

/*
 * This is called by the controller driver when the command is done.
 */
static void
rzdone(unit, error, resid, status)
	int unit;
	int error;		/* error number from errno.h */
	int resid;		/* amount not transfered */
	int status;		/* SCSI status byte */
{
	struct rz_softc *sc = &rz_softc[unit];
	struct buf *bp = BUFQ_FIRST(&sc->sc_tab);
	struct pmax_scsi_device *sd = sc->sc_sd;

	if (bp == NULL) {
		printf("rz%d: bp == NULL\n", unit);
		return;
	}

	disk_unbusy(&sc->sc_dkdev, (bp->b_bcount - resid));

#if NRND > 0
	rnd_add_uint32(&sc->rnd_source, bp->b_blkno);
#endif	/* NRND */

	if (sc->sc_flags & RZF_SENSEINPROGRESS) {
		sc->sc_flags &= ~RZF_SENSEINPROGRESS;
		BUFQ_REMOVE(&sc->sc_tab, bp);	/* remove sc_errbuf */
		bp = BUFQ_FIRST(&sc->sc_tab);

		if (error || (status & SCSI_STATUS_CHECKCOND)) {
#ifdef DEBUG
			if (rzdebug & RZB_ERROR)
				printf("rz%d: error reading sense data: error %d scsi status 0x%x\n",
					unit, error, status);
#endif
			/*
			 * We got an error during the REQUEST_SENSE,
			 * fill in no sense for data.
			 */
			sc->sc_sense.sense[0] = 0x70;
			sc->sc_sense.sense[2] = SCSI_CLASS7_NO_SENSE;
		} else if (!(sc->sc_flags & RZF_NOERR)) {
			ScsiClass7Sense *sp;
			sp = (ScsiClass7Sense *)sc->sc_sense.sense;
			printf("rz%d: ", unit);
			scsiPrintSense(sp, sizeof(sc->sc_sense.sense) - resid);
			if (sp->error7 == 0x70 &&
			    sp->key == SCSI_CLASS7_RECOVERABLE) {
				/* Recoverable error - clear error status */
				bp->b_flags &= ~B_ERROR;
				bp->b_error = 0;
			}
		}
	} else if (error || (status & SCSI_STATUS_CHECKCOND)) {
#ifdef DEBUG
		if (!(sc->sc_flags & RZF_NOERR) && (rzdebug & RZB_ERROR))
			printf("rz%d: error %d scsi status 0x%x\n",
				unit, error, status);
#endif
		/* save error info */
		sc->sc_sense.status = status;
		bp->b_flags |= B_ERROR;
		bp->b_error = error;
		bp->b_resid = resid;

		if (status & SCSI_STATUS_CHECKCOND) {
			/*
			 * Start a REQUEST_SENSE command.
			 * Since we are called at interrupt time, we can't
			 * wait for the command to finish; that's why we use
			 * the sc_flags field.
			 */
			sc->sc_flags |= RZF_SENSEINPROGRESS;
			sc->sc_cdb.len = sizeof(ScsiGroup0Cmd);
			scsiGroup0Cmd(SCSI_REQUEST_SENSE, sd->sd_slave, 0,
				sizeof(sc->sc_sense.sense),
				(ScsiGroup0Cmd *)sc->sc_cdb.cdb);
			sc->sc_errbuf.b_flags = B_BUSY | B_PHYS | B_READ;
			sc->sc_errbuf.b_bcount = sizeof(sc->sc_sense.sense);
			sc->sc_errbuf.b_data = (caddr_t)sc->sc_sense.sense;
			BUFQ_INSERT_HEAD(&sc->sc_tab, &sc->sc_errbuf);
			rzstart(unit);
			return;
		}
	} else {
		sc->sc_sense.status = status;
		bp->b_resid = resid;
	}

	BUFQ_REMOVE(&sc->sc_tab, bp);
	biodone(bp);
	if (BUFQ_FIRST(&sc->sc_tab) != NULL)
		rzstart(unit);
	else {
		sc->sc_active = 0;
		/* finish close protocol */
		if (sc->sc_openpart == 0)
			wakeup((caddr_t)&sc->sc_tab);
	}
}


/*
 * Read or constuct a disklabel
 */
static void
rzgetinfo(dev)
	dev_t dev;
{
	int unit = rzunit(dev);
	struct rz_softc *sc = &rz_softc[unit];
	struct disklabel *lp = sc->sc_label;
	char *msg;
	int part;
	struct cpu_disklabel cd;

	part = rzpart(dev);
	sc->sc_flags |= RZF_HAVELABEL;

	if (sc->sc_type == SCSI_ROM_TYPE) {
		lp->d_type = DTYPE_SCSI;
		lp->d_secsize = sc->sc_blksize;
		lp->d_nsectors = 100;
		lp->d_ntracks = 1;
		lp->d_ncylinders = (sc->sc_blks / 100) + 1;
		lp->d_secpercyl	= 100;
		lp->d_secperunit = sc->sc_blks;
		lp->d_rpm = 300;
		lp->d_interleave = 1;
		lp->d_flags = D_REMOVABLE;
		/* 4.4bsd code set 'a'. Also set up 'c' for disklabel. */
		lp->d_npartitions = 3;
		lp->d_partitions[0].p_offset = 0;
		lp->d_partitions[0].p_size = sc->sc_blks;
		lp->d_partitions[0].p_fstype = FS_ISO9660;
		lp->d_partitions[1].p_offset = 0;
		lp->d_partitions[1].p_size = 0;
		lp->d_partitions[2].p_offset = 0;
		lp->d_partitions[2].p_size = sc->sc_blks;
		lp->d_partitions[2].p_fstype = FS_ISO9660;

		lp->d_magic = DISKMAGIC;
		lp->d_magic2 = DISKMAGIC;
		lp->d_checksum = dkcksum(lp);
		return;
	}

	lp->d_type = DTYPE_SCSI;
	lp->d_secsize = DEV_BSIZE;
	lp->d_secpercyl = 1 << sc->sc_bshift;
	lp->d_npartitions = MAXPARTITIONS;
	lp->d_partitions[part].p_offset = 0;
	lp->d_partitions[part].p_size = sc->sc_blks;
	rz_setlabelgeom(lp, &sc->params);

	/*
	 * Now try to read the disklabel
	 */
	msg = readdisklabel(dev, rzstrategy, lp, &cd);

	/*
	 * If this is an installation diskimage, the label geometry
	 * is from a vnd(4) diskimage, not the real SCSI disk, and so
	 * the RAW_PART info is wrong.  Fake up an entry for RAW_PART.
	 */
	if (msg == NULL &&
	    strncmp(lp->d_typename, "install diskimag", 16) == 0 &&
	    strlen(lp->d_packname) == 0 &&
	    lp->d_npartitions == RAW_PART+1 &&
	    lp->d_partitions[0].p_offset == 0 &&
	    (lp->d_partitions[0].p_size < 32768 ||
	     lp->d_partitions[0].p_size == 65536 ) &&
#if 0
	    lp->d_partitions[0].p_size == lp->d_partitions[RAW_PART].p_size &&
#endif
	    lp->d_partitions[RAW_PART].p_size != sc->sc_blks) {
		printf("rz%d: WARNING: %s\n", unit,
		       "install diskimage, recomputing label.");
		/* recompute label and rawpart size from softc params. */
		rz_setlabelgeom(lp, &sc->params);
		lp->d_partitions[RAW_PART].p_size = sc->sc_blks;
		lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
		lp->d_checksum = dkcksum(lp);
		return;
	}
	if (msg == NULL)
		return;
	printf("rz%d: WARNING: %s\n", unit, msg);

	/*
	 * No label found. Concoct one from compile-time default.
	 */
	rzgetdefaultlabel(sc, lp);
}

int
rzopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	int unit = rzunit(dev);
	struct rz_softc *sc = &rz_softc[unit];
	struct disklabel *lp;
	int i;
	int part;
	int mask;

	if (unit >= NRZ || !(sc->sc_flags & RZF_ALIVE))
		return (ENXIO);

	/* make sure disk is ready */
	if (sc->sc_flags & RZF_REMOVEABLE) {
		if (!rzready(sc))
			return (ENXIO);
	}

	/* try to read disk label and partition table information */
	part = rzpart(dev);
	if (!(sc->sc_flags & RZF_HAVELABEL))
		rzgetinfo(dev);

	lp = sc->sc_label;
	if (part >= lp->d_npartitions || lp->d_partitions[part].p_size == 0)
	{
		printf("rzopen: ENXIO on rz%d%c unit %d part %d\n",
			unit, "abcdefg"[part],  unit, part);
		printf("# partions %d, size of %d = %d\n",
		       lp->d_npartitions, part,
		       lp->d_partitions[part].p_size);
		return (ENXIO);
	}

	/*
	 * Warn if a partition is opened that overlaps another
	 * already open, unless either is the `raw' partition
	 * (whole disk).
	 */
	mask = 1 << part;
	if ((sc->sc_openpart & mask) == 0 && part != RAW_PART) {
		struct partition *pp;
		u_long start, end;

		pp = &lp->d_partitions[part];
		start = pp->p_offset;
		end = pp->p_offset + pp->p_size;
		for (pp = lp->d_partitions, i = 0;
		     i < lp->d_npartitions; pp++, i++) {
			if (pp->p_offset + pp->p_size <= start ||
			    pp->p_offset >= end || i == RAW_PART)
				continue;
			if (sc->sc_openpart & (1 << i))
				log(LOG_WARNING,
				    "rz%d%c: overlaps open partition (%c)\n",
				    unit, part + 'a', i + 'a');
		}
	}
	switch (mode) {
	case S_IFCHR:
		sc->sc_copenpart |= mask;
		break;
	case S_IFBLK:
		sc->sc_bopenpart |= mask;
		break;
	}
	sc->sc_openpart |= mask;

	return (0);
}

int
rzclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct rz_softc *sc = &rz_softc[rzunit(dev)];
	int mask = (1 << rzpart(dev));
	int s;

	switch (mode) {
	case S_IFCHR:
		sc->sc_copenpart &= ~mask;
		break;
	case S_IFBLK:
		sc->sc_bopenpart &= ~mask;
		break;
	}
	sc->sc_openpart = sc->sc_copenpart | sc->sc_bopenpart;

	/*
	 * Should wait for I/O to complete on this partition even if
	 * others are open, but wait for work on blkflush().
	 */
	if (sc->sc_openpart == 0) {
		s = splbio();
		while (BUFQ_FIRST(&sc->sc_tab) != NULL)
			(void) tsleep(&sc->sc_tab, PZERO - 1, "rzclose", 0);
		splx(s);
#if 0
		/* 4.4Lite semantics  breaks disklabel -[N|W] on close */
		sc->sc_flags &= ~RZF_WLABEL;
#endif
	}
	return (0);
}

int
rzread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	struct rz_softc *sc = &rz_softc[rzunit(dev)];

	if (sc->sc_format_pid && sc->sc_format_pid != curproc->p_pid)
		return (EPERM);

	return (physio(rzstrategy, (struct buf *)0, dev,
		B_READ, minphys, uio));
}

int
rzwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	struct rz_softc *sc = &rz_softc[rzunit(dev)];

	if (sc->sc_type == SCSI_ROM_TYPE)
		return (EROFS);

	if (sc->sc_format_pid && sc->sc_format_pid != curproc->p_pid)
		return (EPERM);

	return (physio(rzstrategy, (struct buf *)0, dev,
		B_WRITE, minphys, uio));
}

int
rzioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct rz_softc *sc = &rz_softc[rzunit(dev)];
	int error;
	int flags;
	struct cpu_disklabel cd;

	switch (cmd) {
	default:
		return (EINVAL);

	case SDIOCSFORMAT:
		/* take this device into or out of "format" mode */
		if (suser(p->p_ucred, &p->p_acflag))
			return (EPERM);

		if (*(int *)data) {
			if (sc->sc_format_pid)
				return (EPERM);
			sc->sc_format_pid = p->p_pid;
		} else
			sc->sc_format_pid = 0;
		return (0);

	case SDIOCGFORMAT:
		/* find out who has the device in format mode */
		*(int *)data = sc->sc_format_pid;
		return (0);

	case SDIOCSCSICOMMAND:
		/*
		 * Save what user gave us as SCSI cdb to use with next
		 * read or write to the char device.
		 */
		if (sc->sc_format_pid != p->p_pid)
			return (EPERM);
		if (legal_cmds[((struct scsi_fmt_cdb *)data)->cdb[0]] == 0)
			return (EINVAL);
		bcopy(data, (caddr_t)&sc->sc_cdb, sizeof(sc->sc_cdb));
		return (0);

	case SDIOCSENSE:
		/*
		 * return the SCSI sense data saved after the last
		 * operation that completed with "check condition" status.
		 */
		bcopy((caddr_t)&sc->sc_sense, data, sizeof(sc->sc_sense));
		return (0);

	case DIOCGDINFO:
		/* get the current disk label */
		*(struct disklabel *)data = *(sc->sc_label);
		return (0);

	case DIOCSDINFO:
		/* set the current disk label */
		if (!(flag & FWRITE))
			return (EBADF);
		error = setdisklabel(sc->sc_label,
				     (struct disklabel *)data,
				     (sc->sc_flags & RZF_WLABEL) ? 0 :
				     sc->sc_openpart, &cd);
		return (error);

	case DIOCGPART:
		/* return the disk partition data */
		((struct partinfo *)data)->disklab = sc->sc_label;
		((struct partinfo *)data)->part =
			&sc->sc_label->d_partitions[rzpart(dev)];
		return (0);

	case DIOCWLABEL:
		if (!(flag & FWRITE))
			return (EBADF);
		if (*(int *)data)
			sc->sc_flags |= RZF_WLABEL;
		else
			sc->sc_flags &= ~RZF_WLABEL;
		return (0);

	case DIOCWDINFO:
		/* write the disk label to disk */
		if (!(flag & FWRITE))
			return (EBADF);
		error = setdisklabel(sc->sc_label,
				     (struct disklabel *)data,
				     (sc->sc_flags & RZF_WLABEL) ? 0 :
				     sc->sc_openpart,
				     &cd);
		if (error)
			return (error);

		/* simulate opening partition 0 so write succeeds */
		flags = sc->sc_flags;
		sc->sc_flags = RZF_ALIVE | RZF_WLABEL;
		error = writedisklabel(dev, rzstrategy, sc->sc_label, &cd);
		sc->sc_flags = flags;
		return (error);

	case DIOCGDEFLABEL:
		rzgetdefaultlabel(sc, (struct disklabel *)data);
		return(0);
	}
	/*NOTREACHED*/
}


int
rzsize(dev)
	dev_t dev;
{
	int unit = rzunit(dev);
	int part = rzpart(dev);
	struct rz_softc *sc = &rz_softc[unit];
	int omask, size;

	if (unit >= NRZ || !(sc->sc_flags & RZF_ALIVE))
		return (-1);

	omask = sc->sc_openpart & (1 << part);

	if (omask == 0 && rzopen(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);

	if (part >= sc->sc_label->d_npartitions ||
	    sc->sc_label->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = sc->sc_label->d_partitions[part].p_size;

	if (omask == 0 && rzclose(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);

	return (size);
}

/*
 * Find out from a CD-rom device what it's capacity is
 */
static u_long
rz_cdsize(cd, flags)
	struct rz_softc *cd;
	int flags;
{
	struct scsipi_read_cd_cap_data rdcap;
	struct scsipi_read_cd_capacity scsipi_cmd;
	int blksize;
	u_long size;
	int error;

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
	error = rz_command(cd,
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),
	    (u_char *)&rdcap, sizeof(rdcap), CDRETRIES, 20000, NULL,
	    flags | SCSI_DATA_IN | SCSI_SILENT);
	if (error != sizeof(rdcap)) {
		/*
		 * the drive doesn't support the READ_CD_CAPACITY command
		 * use a fake size
		 */
#ifdef pmax
		cd->sc_flags |= RZF_FAKEGEOM;
		/*  Must jumper CDs to 512-byte blocks to boot on pmax. */
		cd->sc_blksize = 512;
		cd->sc_blks = 400000 * 4;
#else
		cd->sc_blksize = 2048;
		cd->sc_blks = 400000;
#endif
		return (cd->sc_blks);
	}

	blksize = _4btol(rdcap.length);
	if ((blksize < 512) || ((blksize & 511) != 0))
		blksize = 2048;	/* some drives lie ! */
	cd->sc_blksize = blksize;

	size = _4btol(rdcap.addr) + 1;
	if (size < 100)
		size = 400000;	/* ditto */
	cd->sc_blks = size;
	return (size);
}

/*
 * Send a SCSI command to a target drive, using the 4.4bsd/pmax driver
 * formatting support RZ_ALTCMD machinery.
 *
 * Returns byte count returned by cmd, computed as datalen - resid
 * or -1 on failure.
 */
static int
rz_command(sc, scsi_cmd, cmdlen, data_addr, datalen, nretries, timeout,
    bp, flags)
	struct rz_softc *sc;
	struct scsipi_generic *scsi_cmd;
	int cmdlen;
	u_char *data_addr;
	int datalen;
	int nretries;
	int timeout;
	struct buf *bp;
	u_int flags;
{
	int retried = 0;
	int recvlen, savedflags;

	/*
	 * Make sure the command will fit.
	 */
	if (cmdlen > sizeof(sc->sc_cdb)) {
		printf("%s: rz_commamd: command too large (%d > %d)\n",
		    sc->sc_dev.dv_xname, cmdlen, sizeof(sc->sc_cdb));
		return (-1);
	}

	/*
	 * Map NetBSD MI scsi command flags onto 4.4bsd SCSI (rz)
	 * flags.
	 */
	savedflags = sc->sc_flags;
	if (flags & SCSI_SILENT)
		sc->sc_flags |= RZF_NOERR;

	/*
	 * Zero out the data buffer.
	 */
	bzero(data_addr, datalen);

 again:
	/* copy request into cdb */
	bcopy(scsi_cmd, &sc->sc_cdb.cdb, cmdlen);
	sc->sc_cdb.len = cmdlen;

	/*
	 * Send the command to the drive and wait for it to
	 * complete.
	 */
	sc->sc_buf.b_flags = B_BUSY | B_PHYS |
	    (flags & SCSI_DATA_IN) ? B_READ : B_WRITE;
	sc->sc_buf.b_bcount = datalen;
	sc->sc_buf.b_data = (caddr_t)data_addr;
	BUFQ_INSERT_HEAD(&sc->sc_tab, &sc->sc_buf);
	sc->sc_flags |= RZF_ALTCMD;
	rzstart(sc->sc_cmd.unit);
	sc->sc_flags &= ~RZF_ALTCMD;
	DELAY(timeout);
	if (biowait(&sc->sc_buf)) {
		recvlen = -1;
		goto done;
	}

	recvlen = datalen - sc->sc_buf.b_resid;

	if (recvlen == 0) {
		/*
		 * Gack, command didn't work; try again.
		 */
		DELAY(timeout);
		if (retried++ < nretries)
			goto again;
	}

 done:
	sc->sc_flags = savedflags;	/* Restore flags. */
	return (recvlen);
}

/*
 * mode-sense code,  lifted from dev/scsipi/sd.c.
 * Returns 0 on no error.
 */
static int
rz_mode_sense(sd, scsipi_sense, page, pagelen, flags)
	struct rz_softc *sd;
	struct scsi_mode_sense_data *scsipi_sense;
	int page, pagelen, flags;
{
	struct scsi_mode_sense scsipi_cmd;
	int nbytes;

	/*
	 * Make sure the sense buffer is clean before we do
	 * the mode sense, so that checks for bogus values of
	 * 0 will work in case the mode sense fails.
	 */
	bzero(scsipi_sense, sizeof(*scsipi_sense));

	bzero(&scsipi_cmd, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = SCSI_MODE_SENSE;
	scsipi_cmd.page = page;
	scsipi_cmd.length = 0x20;	/* XXX verbatim from MI scsi sd.c */

	/*
	 * If the command worked, use the results to fill out
	 * the parameter structure
	 */
	nbytes = rz_command(sd,
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),
	    (u_char *)scsipi_sense,
	     sizeof(*scsipi_sense),	/* actual size of data buffer */
	    SDRETRIES, 6000, NULL, flags | SCSI_DATA_IN | SCSI_SILENT);

	/*
	 * the MI scsipi_cmd.len always sets cmd.length to 0x20 bytes.
	 * sizeof(*scsip_sense) is 44, causing a 12-byte residual error
	 *  with the pmax rz.c driver.
	 * So ask for sizeof(*scsipi_sense) but make sure we got at least
	 *  0x20 bytes back instead.
	 */

#if defined(RZ_DEBUG)
	printf("rz_mode_sense: page %d,rz_command, nbytes %d\n", page, nbytes);
#endif
	if (nbytes <= 0)
		return (-1);

	if (nbytes < pagelen)
		return (pagelen);

	/* We got at least as much as the caller wanted. Return 0 as OK. */
	return(0);
}


static void
rzgetgeom(sc,  flags)
	struct rz_softc *sc;
	int flags;
{

	struct disk_parms *dp = &sc->params;
	struct scsi_mode_sense_data scsipi_sense;
	u_long sectors;
	int page;
	int error;

	if ((error = rz_mode_sense(sc, &scsipi_sense, page = 4,
		sizeof(scsipi_sense.pages.rigid_geometry), flags)) == 0) {
		SC_DEBUG(periph, SDEV_DB3,
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

		/* Our caller just called rz_getsize(sc, flags). */
		sectors = sc->sc_blks;
		dp->disksize = sectors;
		sectors /= (dp->heads * dp->cyls);
		dp->sectors = sectors;	/* XXX dubious on SCSI */

		return ;
	}

	if ((error = rz_mode_sense(sc, &scsipi_sense, page = 5,
		sizeof(scsipi_sense.pages.flex_geometry), flags)) == 0) {
		dp->heads = scsipi_sense.pages.flex_geometry.nheads;
		dp->cyls = _2btol(scsipi_sense.pages.flex_geometry.ncyl);
		dp->blksize = _3btol(scsipi_sense.blk_desc.blklen);
		dp->sectors = scsipi_sense.pages.flex_geometry.ph_sec_tr;
		dp->disksize = dp->heads * dp->cyls * dp->sectors;

		if (dp->disksize == 0)
			goto fake_it;

		if (dp->blksize == 0)
			dp->blksize = 512;

		return;
	}

fake_it:
	sc->sc_flags |= RZF_FAKEGEOM;

	/*
	 * use adaptec standard fictitious geometry
	 * this depends on which controller (e.g. 1542C is
	 * different. but we have to put SOMETHING here..)
	 */
	sectors = /*scsipi_size(sd->sc_periph, flags);*/ sc->sc_blks;
	dp->heads = 64;
	dp->sectors = 32;
	dp->cyls = sectors / (64 * 32);
	dp->blksize = 512;
	dp->disksize = sectors;
	return;
}

/*
 * set fake or Ultrix label geometry info from softc params.
 */
static void
rz_setlabelgeom(lp, dp)
	struct disklabel *lp;
	struct disk_parms *dp;
{
	lp->d_secsize = dp->blksize;
	lp->d_nsectors =  dp->sectors;
	lp->d_ntracks = dp->heads;
	lp->d_ncylinders = dp->cyls;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;	/* XXX */
	lp->d_secperunit = dp->disksize;
}

static void
rzgetdefaultlabel(sc, lp)
	struct rz_softc *sc;
	struct disklabel *lp;
{
	int i;

	bzero(lp, sizeof(struct disklabel));

	strncpy(lp->d_packname, "fictitious", 16);
	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_type = DTYPE_SCSI;
	lp->d_subtype = 0;
	if (sc->sc_type == SCSI_ROM_TYPE)
		strncpy(lp->d_typename, "SCSI CD-ROM", 16);
	else
		strncpy(lp->d_typename, "SCSI disk", 16);

	rz_setlabelgeom(lp, &sc->params);
	lp->d_rpm = 3600;
	lp->d_interleave = 1;
	lp->d_flags = 0;
	lp->d_npartitions = MAXPARTITIONS;
	lp->d_bbsize = BBSIZE;
	lp->d_sbsize = SBSIZE;
	for (i = 0; i < MAXPARTITIONS; i++) {
		struct partition *pp = & lp->d_partitions[i];

		pp->p_size = rzdefaultpart[i].nblocks;
		pp->p_offset = rzdefaultpart[i].strtblk;

		/* Change RZ_END to be end-of-disk */
		if (pp->p_size == RZ_END) {
			pp->p_size = sc->sc_blks - pp->p_offset;
		}
		/*
		 * Clip end of partition against end of disk.
		 * If both start and end beyond end of disk, set to zero.
		 */
		if (pp->p_offset > sc->sc_blks) {
			pp->p_size = 0;
			pp->p_offset = 0;
		} else if ((pp->p_size + pp->p_offset) > sc->sc_blks)
			pp->p_size = sc->sc_blks - pp->p_offset;
	}

	/* set geometry info from softc info. */
	rz_setlabelgeom(lp, &sc->params);

	/*XXX redundancy */
	lp->d_partitions[RAW_PART].p_size = sc->sc_blks;
	lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;

	lp->d_checksum = dkcksum(lp);
}

/*
 * Non-interrupt driven, non-dma dump routine.
 */
int
rzdump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{
	static int rzdoingadump;/* mutex */
	static dev_t rzreadydev = NODEV; /* hint: device already rzready()ed */
	int sectorsize;		/* size of a disk sector */
	int nsects;		/* number of sectors in partition */
	int sectoff;		/* sector offset of partition */
	int nwrt, totwrt;	/* total number of sectors left to write */
	int unit, part;
	int error;
	struct rz_softc *sc;
	struct disklabel *lp;

	/* Check for recursive dump; if so, punt. */
	if (rzdoingadump)
		return (EFAULT);
	rzdoingadump = 1;

	/* Decompose unit and partition. */
	unit = rzunit(dev);
	part = rzpart(dev);

	if (unit >= NRZ)
		return (ENXIO);
	sc = &rz_softc[unit];
	if ((sc->sc_flags & RZF_ALIVE) == 0)
		return (ENXIO);

	/*
	 * XXX Prevent the tsleep() calls in biowait() in rzready()
	 * XXX or later here from performing a switch.
	 */
	cold = 1;

	/*
	 * Ready drive. rzready() does geometry-sense. Cache dev_t of
	 * last rzready()ed device to avoid seeks to modepage.
	 */
	if (rzreadydev != dev) {
		if (rzready(sc) == 0) {
			/* Drive didn't reset. */
			rzreadydev = NODEV;
			return (ENXIO);
		}
		rzreadydev = dev;
	}

	/*
	 * Convert to disk sectors.  Request must be a multiple of size.
	 */
	lp = sc->sc_label;
	sectorsize = lp->d_secsize;
	if ((size % sectorsize) != 0)
		return (EFAULT);
	totwrt = size / sectorsize;
	blkno = dbtob(blkno) / sectorsize;	/* blkno in DEV_BSIZE units */

	nsects = lp->d_partitions[part].p_size;
	sectoff = lp->d_partitions[part].p_offset;

	/* Check transfer bounds against partition size. */
	if ((blkno < 0) || (blkno + totwrt) > nsects)
		return (EINVAL);

	/* Offset block number to start of partition. */
	blkno += sectoff;

	while (totwrt > 0) {
		nwrt = totwrt;		/* XXX */
#ifndef RZ_DUMP_NOT_TRUSTED
		/*
		 * Create the SCSI command.
		 */
		sc->sc_rwcmd.command = SCSI_WRITE_EXT;
		sc->sc_rwcmd.highAddr = blkno >> 24;
		sc->sc_rwcmd.midHighAddr = blkno >> 16;
		sc->sc_rwcmd.midLowAddr = blkno >> 8;
		sc->sc_rwcmd.lowAddr = blkno;

		sc->sc_rwcmd.highBlockCount = nwrt >> 8;
		sc->sc_rwcmd.lowBlockCount = nwrt;

		/*
		 * ...and send it to the device.
		 */
		sc->sc_buf.b_flags = B_BUSY | B_PHYS | B_WRITE;
		sc->sc_buf.b_bcount = nwrt * sectorsize;
		sc->sc_buf.b_data = va;
		BUFQ_INSERT_HEAD(&sc->sc_tab, &sc->sc_buf);

		sc->sc_cmd.flags = SCSICMD_DATA_TO_DEVICE;
		sc->sc_cmd.cmd = (u_char *)&sc->sc_rwcmd;
		sc->sc_cmd.cmdlen = sizeof(sc->sc_rwcmd);
		sc->sc_cmd.buf = va;
		sc->sc_cmd.buflen = nwrt * sectorsize;

		disk_busy(&sc->sc_dkdev);	/* XXX */
		(*sc->sc_sd->sd_cdriver->d_start)(&sc->sc_cmd);
		if ((error = biowait(&sc->sc_buf)) != 0)
			return (error);
#else /* RZ_DUMP_NOT_TRUSTED */
		/* Let's just talk about it first. */
		printf("%s: dump addr %p, blk %d\n", sc->sc_dev.dv_xname,
		    va, blkno);
		delay(500 * 1000);	/* half a second */
#endif /* RZ_DUMP_NOT_TRUSTED */

		/* update block count */
		totwrt -= nwrt;
		blkno += nwrt;
		va += sectorsize * nwrt;
	}
	rzdoingadump = 0;
	return (0);
}
#endif /* NRZ */
