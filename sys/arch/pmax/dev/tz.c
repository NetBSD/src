/*	$NetBSD: tz.c,v 1.31 2000/06/02 20:15:41 mhitch Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)tz.c	8.5 (Berkeley) 6/2/95
 *
 * from: Header: /sprite/src/kernel/dev/RCS/devSCSITape.c,
 *	v 8.14 89/07/31 17:26:13 mendel Exp  SPRITE (Berkeley)
 */

/*
 * SCSI CCS (Command Command Set) tape driver.
 */
#include "tz.h"
#if NTZ > 0

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/mtio.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/tprintf.h>

#include <machine/conf.h>

#include <pmax/dev/device.h>
#include <pmax/dev/scsi.h>

static int	tzprobe __P((void *sd /*struct pmax_scsi_device *sd*/));
static int	tzcommand __P((dev_t dev, int command, int code,
		    int count, caddr_t data));
static void	tzstart __P((int unit));
static void	tzdone __P((int unit, int error, int resid, int status));
static int	tzmount __P((dev_t dev));

struct	pmax_driver tzdriver = {
	"tz", tzprobe,
	(void	(*) __P((struct ScsiCmd *cmd))) tzstart,
	tzdone,
};

struct tzmodes {
	u_int quirks;
	u_int8_t density;
};

struct tzquirk {
	char *product;
	u_int page_0_size;
	u_int quirks;
	int blklen;
	struct tzmodes modes[2];	/* low density, high density */
};

#define TZ_Q_FORCE_BLKSIZE	0x0001
#define TZ_Q_SENSE_HELP		0x0002	/* must do READ for good MODE SENSE */
#define TZ_Q_IGNORE_LOADS	0x0004
#define TZ_Q_BLKSIZE		0x0008	/* variable-block media_blksize > 0 */
#define TZ_Q_UNIMODAL		0x0010	/* unimode drive rejects mode select */
#define TZ_Q_DENSITY		0x0020	/* density codes valid */
#define MAX_PAGE_0_SIZE 64


static	struct tz_softc {
	struct	device sc_dev;		/* new config glue */
	struct	pmax_scsi_device *sc_sd;	/* physical unit info */
	int	sc_flags;		/* see below */
	int	sc_blklen;		/* 0 = variable len records */
	long	sc_numblks;		/* number of blocks on tape */
	tpr_t	sc_ctty;		/* terminal for error messages */
	daddr_t	sc_fileno;		/* file number of current position */
	daddr_t	sc_blkno;		/* block number of current position */
	struct	buf_queue sc_tab;	/* queue of pending operations */
	int	sc_active;		/* number of active operations */
	struct	buf sc_buf;		/* buf for doing I/O */
	struct	buf sc_errbuf;		/* buf for doing REQUEST_SENSE */
	struct	ScsiCmd sc_cmd;		/* command for controller */
	ScsiGroup0Cmd sc_rwcmd;		/* SCSI cmd for read/write */
	struct	scsi_fmt_cdb sc_cdb;	/* SCSI cmd if not read/write */
	struct	scsi_fmt_sense sc_sense;	/* sense data from last cmd */
	struct	ScsiTapeModeSelectHdr sc_mode;	/* SCSI_MODE_SENSE data */
	char	sc_modelen;		/* SCSI_MODE_SENSE data length */
	struct	tzquirk *sc_quirks;	/* quirks for this drive */

} tz_softc[NTZ];

/* sc_flags values */
#define TZF_ALIVE		0x001	/* drive found and ready */
#define TZF_MOUNTED		0x002	/* device is presently mounted */
#define TZF_OPEN		0x004	/* device is open */
#define TZF_SENSEINPROGRESS	0x008	/* REQUEST_SENSE command in progress */
#define TZF_ALTCMD		0x010	/* alternate command in progress */
#define TZF_WRITTEN		0x020	/* tape has been written to */
#define TZF_WAIT		0x040	/* waiting for sc_tab to drain */
#define TZF_SEENEOF		0x080	/* seen file mark on read */
#define TZF_RDONLY		0x100	/* mode sense says write protected */
#define TZF_DONTCOUNT		0x200	/* operation isn't to be counted for sc_blkno */

/* bits in minor device */
#define	tzunit(x)	(minor(x) >> 4)	/* tz%d unit number */
#define TZ_NOREWIND	0x01		/* don't rewind on close */
#define TZ_HIDENSITY	0x02
#define TZ_EXSFMK	0x04
#define TZ_FIXEDBLK	0x08

#ifdef DEBUG
int	tzdebug = 0;
#endif


struct tzquirk tz_quirk_table[] = {
	{ "EXB-8200", 17, 0, 0, {
		{ 0, 0,},
		{ 0, 0,}
	}},
	{ "TZK08", 17, 0, 0, {
		{ 0, 0,},
		{ 0, 0,}
	}},
	{ "EXB-8500", 17, 0, 0, {
		{ 0, 0,},
		{ 0, 0,}
	}},
	{ "TKZ09", 17, 0, 0, {
		{ 0, 0,},
		{ 0, 0,}
	}},

	{ "VIPER 150", 0, TZ_Q_FORCE_BLKSIZE, 512, {
		{ 0, 0, },
		{ 0, 0, }
	}},
	{ "5150ES SCSI", 0, TZ_Q_FORCE_BLKSIZE, 512, {
		{ 0, 0, },
		{ 0, 0, }
	}},

	/* The next two product ids are faked up in tzprobe() */
	{ "TK50 0x30", 14, 0, 0, {
		{ 0, 0,},
		{ 0, 0,}
	}},
	{ "MT02", 0, 0, 0, {
		{ TZ_Q_DENSITY, 0x4 },
		{ TZ_Q_DENSITY, 0 }
	}},
};

#define NQUIRKS	(sizeof(tz_quirk_table) / sizeof(tz_quirk_table[0]))

/*
 * Test to see if device is present.
 * Return true if found and initialized ok.
 */
static int
tzprobe(xxxsd)
	void *xxxsd;
{
	int i;
	char vid[9], pid[17], revl[5];
	struct pmax_scsi_device *sd = xxxsd;
	struct tz_softc *sc = &tz_softc[sd->sd_unit];
	ScsiInquiryData inqbuf;

	if (sd->sd_unit >= NTZ)
		return (0);

	BUFQ_INIT(&sc->sc_tab);
	callout_init(&sc->sc_cmd.timo_ch);

	/* init some parameters that don't change */
	sc->sc_sd = sd;
	sc->sc_cmd.sd = sd;
	sc->sc_cmd.unit = sd->sd_unit;
	sc->sc_cmd.flags = 0;
	sc->sc_cmd.lun = 0;
	sc->sc_rwcmd.unitNumber = sd->sd_slave;

	/* XXX set up device info */				/* XXX */
	bzero(&sc->sc_dev, sizeof(sc->sc_dev));			/* XXX */
	sprintf(sc->sc_dev.dv_xname, "tz%d", sd->sd_unit);	/* XXX */
	sc->sc_dev.dv_unit = sd->sd_unit;			/* XXX */
	sc->sc_dev.dv_class = DV_TAPE;				/* XXX */

	sc->sc_fileno = -1;
	sc->sc_blkno = -1;

	/* try to find out what type of device this is */
	sc->sc_flags = TZF_ALTCMD;	/* force use of sc_cdb */
	sc->sc_cdb.len = sizeof(ScsiGroup0Cmd);
	scsiGroup0Cmd(SCSI_INQUIRY, sd->sd_slave, 0, sizeof(inqbuf),
		(ScsiGroup0Cmd *)sc->sc_cdb.cdb);
	sc->sc_buf.b_flags = B_BUSY | B_READ;
	sc->sc_buf.b_bcount = sizeof(inqbuf);
	sc->sc_buf.b_data = (caddr_t)&inqbuf;
	BUFQ_INSERT_HEAD(&sc->sc_tab, &sc->sc_buf);
	tzstart(sd->sd_unit);
	if (biowait(&sc->sc_buf) ||
	    (i = sizeof(inqbuf) - sc->sc_buf.b_resid) < 5)
		goto bad;
	if (inqbuf.type != SCSI_TAPE_TYPE || !inqbuf.rmb)
		goto bad;

	/* check for device ready to clear UNIT_ATTN */
	sc->sc_cdb.len = sizeof(ScsiGroup0Cmd);
	scsiGroup0Cmd(SCSI_TEST_UNIT_READY, sd->sd_slave, 0, 0,
		(ScsiGroup0Cmd *)sc->sc_cdb.cdb);
	sc->sc_buf.b_flags = B_BUSY | B_READ;
	sc->sc_buf.b_bcount = 0;
	sc->sc_buf.b_data = (caddr_t)0;
	BUFQ_INSERT_HEAD(&sc->sc_tab, &sc->sc_buf);
	tzstart(sd->sd_unit);
	(void) biowait(&sc->sc_buf);

	sc->sc_flags = TZF_ALIVE;
	sc->sc_modelen = 12;
	sc->sc_buf.b_flags = 0;
	printf("tz%d at %s%d drive %d slave %d", sd->sd_unit,
		sd->sd_cdriver->d_name, sd->sd_ctlr, sd->sd_drive,
		sd->sd_slave);
	if (i == 5 && inqbuf.version == 1 && (inqbuf.qualifier == 0x50 ||
	    inqbuf.qualifier == 0x30)) {
		strcpy(pid, "TK50");
		printf(" %s", pid);
		if (inqbuf.qualifier == 0x30)
			strcpy(pid, "TK50 0x30");
	} else if (i >= 5 && inqbuf.version == 1 && inqbuf.qualifier == 0 &&
	    inqbuf.length == 0) {
		/* assume Emultex MT02 controller */
		strcpy(pid, "MT02");
		printf(" %s", pid);
	} else if (inqbuf.version > 2 || i < 36) {
		printf(" GENERIC SCSI tape device: qual 0x%x, ver %d",
			inqbuf.qualifier, inqbuf.version);
	} else {

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
	printf("\n");

	sc->sc_quirks = NULL;
	for (i = 0; i < NQUIRKS; i++) {
		if (bcmp(pid, tz_quirk_table[i].product,
		    strlen(tz_quirk_table[i].product)) == 0) {
			sc->sc_quirks = &tz_quirk_table[i];
			if (tz_quirk_table[i].quirks & TZ_Q_FORCE_BLKSIZE)
				sc->sc_blklen = tz_quirk_table[i].blklen;
			if (tz_quirk_table[i].page_0_size > 0) {
				sc->sc_modelen = tz_quirk_table[i].page_0_size;
			}
			break;
		}
	}

	sd->sd_devp = &sc->sc_dev;				/* XXX */
	TAILQ_INSERT_TAIL(&alldevs, &sc->sc_dev, dv_list);	/* XXX */

	return (1);

bad:
	/* doesn't exist or not a CCS device */
	sc->sc_flags = 0;
	sc->sc_buf.b_flags = 0;
	return (0);
}

/*
 * Perform a special tape command on a SCSI Tape drive.
 */
static int
tzcommand(dev, command, code, count, data)
	dev_t dev;
	int command;
	int code;
	int count;
	caddr_t data;
{
	struct tz_softc *sc = &tz_softc[tzunit(dev)];
	ScsiGroup0Cmd *c;
	int s, error;

	s = splbio();
	/* wait for pending operations to finish */
	while (BUFQ_FIRST(&sc->sc_tab) != NULL) {
		sc->sc_flags |= TZF_WAIT;
		(void) tsleep(&sc->sc_flags, PZERO, "tzcmd", 0);
	}
	sc->sc_flags |= TZF_DONTCOUNT;	/* don't count any operations in blkno */
	sc->sc_flags |= TZF_ALTCMD;	/* force use of sc_cdb */
	sc->sc_cdb.len = sizeof(ScsiGroup0Cmd);
	c = (ScsiGroup0Cmd *)sc->sc_cdb.cdb;
	c->command = command;
	c->unitNumber = sc->sc_sd->sd_slave;
	c->highAddr = code;
	c->midAddr = count >> 16;
	c->lowAddr = count >> 8;
	c->blockCount = count;
	c->control = 0;
	if (command == SCSI_MODE_SELECT)
		sc->sc_buf.b_flags = B_BUSY;
	else {
		sc->sc_buf.b_flags = B_BUSY | B_READ;
	}
	sc->sc_buf.b_bcount = data ? count : 0;
	sc->sc_buf.b_data = data;
	BUFQ_INSERT_HEAD(&sc->sc_tab, &sc->sc_buf);
	tzstart(sc->sc_sd->sd_unit);
	error = biowait(&sc->sc_buf);
	sc->sc_flags &= ~TZF_DONTCOUNT;	/* clear don't count flag */
	sc->sc_flags &= ~TZF_ALTCMD;	/* force use of sc_cdb */
	sc->sc_buf.b_flags = 0;
	sc->sc_cmd.flags = 0;
	if (sc->sc_buf.b_resid)
		printf("tzcommand: resid %ld\n", sc->sc_buf.b_resid); /* XXX */
	if (error == 0) {
		switch (command) {
		case SCSI_SPACE:
			if (sc->sc_blkno != -1) {
				if (code == 0) {
					sc->sc_blkno += count;
				} else {
					sc->sc_fileno += count;
					sc->sc_blkno = 0;
				}
			}
			sc->sc_flags &= ~TZF_SEENEOF;
			break;
			
		case SCSI_WRITE_EOF:
			if (sc->sc_blkno != -1) {
				sc->sc_fileno += count;
				sc->sc_blkno = 0;
			}
			sc->sc_flags &= ~TZF_SEENEOF;
			break;
		case SCSI_LOAD_UNLOAD:
		case SCSI_REWIND:
			sc->sc_fileno = 0;
			sc->sc_blkno = 0;
			sc->sc_flags &= ~TZF_SEENEOF;
			break;
		}
	} else {
		switch (command) {
		case SCSI_SPACE:
		case SCSI_WRITE_EOF:
		case SCSI_LOAD_UNLOAD:
		case SCSI_REWIND:
			sc->sc_fileno = -1;
			sc->sc_blkno = -1;
		}
	}
	splx(s);
	return (error);
}

static void
tzstart(unit)
	int unit;
{
	struct tz_softc *sc = &tz_softc[unit];
	struct buf *bp = BUFQ_FIRST(&sc->sc_tab);
	int n;

	sc->sc_cmd.buf = bp->b_data;
	sc->sc_cmd.buflen = bp->b_bcount;

	if (sc->sc_flags & (TZF_SENSEINPROGRESS | TZF_ALTCMD)) {
		if (bp->b_flags & B_READ)
			sc->sc_cmd.flags &= ~SCSICMD_DATA_TO_DEVICE;
		else
			sc->sc_cmd.flags |= SCSICMD_DATA_TO_DEVICE;
		sc->sc_cmd.cmd = sc->sc_cdb.cdb;
		sc->sc_cmd.cmdlen = sc->sc_cdb.len;
	} else {
		if (bp->b_flags & B_READ) {
			sc->sc_cmd.flags = 0;
			sc->sc_rwcmd.command = SCSI_READ;
			sc->sc_flags &= ~TZF_WRITTEN;
		} else {
			sc->sc_cmd.flags = SCSICMD_DATA_TO_DEVICE;
			sc->sc_rwcmd.command = SCSI_WRITE;
			sc->sc_flags |= TZF_WRITTEN;
		}
		sc->sc_cmd.cmd = (u_char *)&sc->sc_rwcmd;
		sc->sc_cmd.cmdlen = sizeof(sc->sc_rwcmd);
		if (sc->sc_blklen) {
			/* fixed sized records */
			n = bp->b_bcount / sc->sc_blklen;
			if (bp->b_bcount % sc->sc_blklen) {
				tprintf(sc->sc_ctty,
					"tz%d: I/O not block aligned %d/%ld\n",
					unit, sc->sc_blklen, bp->b_bcount);
				tzdone(unit, EIO, bp->b_bcount, 0);
			}
			sc->sc_rwcmd.highAddr = 1;
		} else {
			/* variable sized records */
			n = bp->b_bcount;
			sc->sc_rwcmd.highAddr = 0;
		}
		sc->sc_rwcmd.midAddr = n >> 16;
		sc->sc_rwcmd.lowAddr = n >> 8;
		sc->sc_rwcmd.blockCount = n;
	}

	/* tell controller to start this command */
	(*sc->sc_sd->sd_cdriver->d_start)(&sc->sc_cmd);
}

/*
 * This is called by the controller driver when the command is done.
 */
static void
tzdone(unit, error, resid, status)
	int unit;
	int error;		/* error number from errno.h */
	int resid;		/* amount not transfered */
	int status;		/* SCSI status byte */
{
	struct tz_softc *sc = &tz_softc[unit];
	struct buf *bp = BUFQ_FIRST(&sc->sc_tab);

	if (bp == NULL) {
		printf("tz%d: bp == NULL\n", unit);
		return;
	}
	if (sc->sc_flags & TZF_SENSEINPROGRESS) {
		sc->sc_flags &= ~TZF_SENSEINPROGRESS;
		BUFQ_REMOVE(&sc->sc_tab, bp);	/* remove sc_errbuf */
		bp = BUFQ_FIRST(&sc->sc_tab);
#ifdef DIAGNOSTIC
		if (bp == NULL)
			panic("tzdone");
#endif

		if (error || (status & SCSI_STATUS_CHECKCOND)) {
			printf("tz%d: error reading sense data: error %d scsi status 0x%x\n",
				unit, error, status);
			/*
			 * We got an error during the REQUEST_SENSE,
			 * fill in no sense for data.
			 */
			sc->sc_sense.sense[0] = 0x70;
			sc->sc_sense.sense[2] = SCSI_CLASS7_NO_SENSE;
		} else if (!cold) {
			ScsiClass7Sense *sp;
			long resid = 0;

			sp = (ScsiClass7Sense *)sc->sc_sense.sense;
			if (sp->error7 != 0x70)
				goto prerr;
			if (sp->valid) {
				resid = (sp->info1 << 24) | (sp->info2 << 16) |
					(sp->info3 << 8) | sp->info4;
				if (sc->sc_blklen)
					resid *= sc->sc_blklen;
			} else
				resid = 0;
			switch (sp->key) {
			case SCSI_CLASS7_NO_SENSE:
				/*
				 * Hit a filemark, end of media, or
				 * end of record.
				 * Fixed length blocks, an error.
				 */
				if (sp->endOfMedia) {
					bp->b_error = ENOSPC;
					bp->b_resid = resid;
					break;
				}
				if (sp->fileMark) {
					if (sc->sc_fileno != -1) {
						sc->sc_fileno++;
						sc->sc_blkno = 0;
					}
				}
				if (sc->sc_blklen && sp->badBlockLen) {
					tprintf(sc->sc_ctty,
						"tz%d: Incorrect Block Length, expected %d got %ld\n",
						unit, sc->sc_blklen, resid);
					break;
				}
				if (resid < 0) {
					/*
					 * Variable length records but
					 * attempted to read less than a
					 * full record.
					 */
					tprintf(sc->sc_ctty,
						"tz%d: Partial Read of Variable Length Tape Block, expected %ld read %ld\n",
						unit, bp->b_bcount - resid,
						bp->b_bcount);
					bp->b_resid = 0;
					break;
				}
				if (sp->fileMark)
					sc->sc_flags |= TZF_SEENEOF;
				/*
				 * Attempting to read more than a record is
				 * OK. Just record how much was actually read.
				 */
				bp->b_flags &= ~B_ERROR;
				bp->b_error = 0;
				bp->b_resid = resid;
				break;

			case SCSI_CLASS7_UNIT_ATTN:
				if (!(sc->sc_flags & TZF_OPEN))
					break;

			default:
			prerr:
				printf("tz%d: ", unit);
				scsiPrintSense((ScsiClass7Sense *)
					sc->sc_sense.sense,
					sizeof(sc->sc_sense.sense) - resid);
			}
		}
	} else if (error || (status & SCSI_STATUS_CHECKCOND)) {
#ifdef DEBUG
		if (!cold && tzdebug)
			printf("tz%d: error %d scsi status 0x%x\n",
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
			sc->sc_flags |= TZF_SENSEINPROGRESS;
			sc->sc_cdb.len = sizeof(ScsiGroup0Cmd);
			scsiGroup0Cmd(SCSI_REQUEST_SENSE, sc->sc_sd->sd_slave,
				0, sizeof(sc->sc_sense.sense),
				(ScsiGroup0Cmd *)sc->sc_cdb.cdb);
			sc->sc_errbuf.b_flags = B_BUSY | B_PHYS | B_READ;
			sc->sc_errbuf.b_bcount = sizeof(sc->sc_sense.sense);
			sc->sc_errbuf.b_data = (caddr_t)sc->sc_sense.sense;
			BUFQ_INSERT_HEAD(&sc->sc_tab, &sc->sc_errbuf);
			tzstart(unit);
			return;
		}
	} else {
		sc->sc_sense.status = status;
		bp->b_resid = resid;
		if (!(sc->sc_flags & TZF_DONTCOUNT) && sc->sc_blkno != -1) {
			if (sc->sc_blklen != 0)
				sc->sc_blkno += bp->b_bcount / sc->sc_blklen;
			else
				sc->sc_blkno++;
		}
	}

	BUFQ_REMOVE(&sc->sc_tab, bp);
	biodone(bp);
	if (BUFQ_FIRST(&sc->sc_tab) != NULL)
		tzstart(unit);
	else {
		sc->sc_active = 0;
		if (sc->sc_flags & TZF_WAIT) {
			sc->sc_flags &= ~TZF_WAIT;
			wakeup(&sc->sc_flags);
		}
	}
}

static int
tzmount(dev)
	dev_t dev;
{
	int unit = tzunit(dev);
	struct tz_softc *sc = &tz_softc[unit];
	int densmode, error;

	/* clear UNIT_ATTENTION */
	error = tzcommand(dev, SCSI_TEST_UNIT_READY, 0, 0, 0);
	while (error) {
		ScsiClass7Sense *sp = (ScsiClass7Sense *)sc->sc_sense.sense;

		/* return error if last error was not UNIT_ATTENTION */
		if (!(sc->sc_sense.status & SCSI_STATUS_CHECKCOND) ||
		    sp->error7 != 0x70 || sp->key != SCSI_CLASS7_UNIT_ATTN)
			return (error);

		/*
		 * Try it again just to be sure and
		 * try to negotiate synchonous transfers.
		 */
		error = tzcommand(dev, SCSI_TEST_UNIT_READY, 0, 0, 0);
	}

	if ((sc->sc_flags & TZF_MOUNTED) == 0) {
		/* get the current mode settings */
		error = tzcommand(dev, SCSI_MODE_SENSE, 0,
			sc->sc_modelen, (caddr_t)&sc->sc_mode);
		if (error)
			return (error);

		sc->sc_flags = TZF_ALIVE;

		/* *Very* first off, make sure we're loaded to BOT. */
		error = tzcommand(dev, SCSI_LOAD_UNLOAD, 0, SCSI_LD_LOAD, 0);
		/* In case this doesn't work, do a REWIND instead */
		if (error)
			error = tzcommand(dev, SCSI_REWIND, 0, 0, 0);
		if (error)
			return (error);

		/* check for write protected tape */
		if (sc->sc_mode.writeprot)
			sc->sc_flags |= TZF_RDONLY;
		else
			sc->sc_flags &= ~TZF_RDONLY;
#if 0	/* XXX not the right place!  Do we need to check at all? */
		if ((flags & FWRITE) && sc->sc_mode.writeprot) {
			uprintf("tz%d: write protected\n", unit);
			return (EACCES);
		}
#endif

		/* save total number of blocks on tape */
		sc->sc_numblks = (sc->sc_mode.blocks_2 << 16) |
			(sc->sc_mode.blocks_1 << 8) | sc->sc_mode.blocks_0;

		/* setup for mode select */
		sc->sc_mode.len = 0;
		sc->sc_mode.media = 0;
		sc->sc_mode.bufferedMode = 1;
		sc->sc_mode.blocks_0 = 0;
		sc->sc_mode.blocks_1 = 0;
		sc->sc_mode.blocks_2 = 0;
		sc->sc_mode.block_size0 = sc->sc_blklen >> 16;
		sc->sc_mode.block_size1 = sc->sc_blklen >> 8;
		sc->sc_mode.block_size2 = sc->sc_blklen;

		/* check for tape density changes */
		densmode = (minor(dev) & TZ_HIDENSITY) ? 1 : 0;
		if ((sc->sc_quirks != NULL) && 
		    (sc->sc_quirks->modes[densmode].quirks & TZ_Q_DENSITY))
			sc->sc_mode.density = sc->sc_quirks->modes[densmode].density;
		else {
			if (densmode != 0)
				uprintf("tz%d: Only one density supported\n", unit);
		}

		/* set the current mode settings */
		error = tzcommand(dev, SCSI_MODE_SELECT, 0,
			sc->sc_modelen, (caddr_t)&sc->sc_mode);
		
		if (error == 0) {
			sc->sc_flags |= TZF_MOUNTED;
			sc->sc_fileno = 0;
			sc->sc_blkno = 0;
		}
	}
	return(error);
}

/* ARGSUSED */
int
tzopen(dev, flags, type, p)
	dev_t dev;
	int flags, type;
	struct proc *p;
{
	int unit = tzunit(dev);
	struct tz_softc *sc = &tz_softc[unit];
	int error;

	if (unit >= NTZ || sc->sc_sd == NULL)
		return (ENXIO);
	if (!(sc->sc_flags & TZF_ALIVE)) {
		/* check again, tape may have been turned off at boot time */
		if (!tzprobe(sc->sc_sd))
			return (ENXIO);
	}
	if (sc->sc_flags & TZF_OPEN)
		return (EBUSY);

	error = tzmount(dev);
	if (error)
		return (error);

	sc->sc_ctty = tprintf_open(p);
	sc->sc_flags |= TZF_OPEN;
	return (0);
}

int
tzclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;

{
	struct tz_softc *sc = &tz_softc[tzunit(dev)];
	int error = 0;

	if (!(sc->sc_flags & TZF_OPEN))
		return (0);
	if (flag == FWRITE ||
	    ((flag & FWRITE) && (sc->sc_flags & TZF_WRITTEN)))
		error = tzcommand(dev, SCSI_WRITE_EOF, 0, 1, 0);
	if ((minor(dev) & TZ_NOREWIND) == 0)
		(void) tzcommand(dev, SCSI_REWIND, 0, 0, 0);
	sc->sc_flags &= ~(TZF_OPEN | TZF_WRITTEN);
	tprintf_close(sc->sc_ctty);
	return (error);
}

int
tzread(dev, uio, iomode)
	dev_t dev;
	struct uio *uio;
	int iomode;
{

	return (physio(tzstrategy, NULL, dev, B_READ, minphys, uio));
}

int
tzwrite(dev, uio, iomode)
	dev_t dev;
	struct uio *uio;
	int iomode;
{

	/* XXX: check for hardware write-protect? */
	return (physio(tzstrategy, NULL, dev, B_WRITE, minphys, uio));
}

int
tzioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct tz_softc *sc = &tz_softc[tzunit(dev)];
	struct mtop *mtop;
	struct mtget *mtget;
	int code, count;
	static int tzops[] = {
		SCSI_WRITE_EOF,		/* MTWEOF */
		SCSI_SPACE,		/* MTFSF */
		SCSI_SPACE,		/* MTBSF */
		SCSI_SPACE,		/* MTFSR */
		SCSI_SPACE,		/* MTBSR */
		SCSI_REWIND,		/* MTREW */
		SCSI_LOAD_UNLOAD,	/* MTOFFL */
		SCSI_TEST_UNIT_READY,	/* MTNOP */
		SCSI_LOAD_UNLOAD,	/* MTRETEN */
		SCSI_ERASE_TAPE,	/* MTERASE */
		MTEOM,			/* MTEOM */
		MTNBSF,			/* MTNBSF */
		SCSI_TEST_UNIT_READY,	/* MTCACHE */
		SCSI_TEST_UNIT_READY,	/* MTNOCACHE */
		MTSETBSIZ,		/* MTSETBSIZ */
		MTSETDNSTY,		/* MTSETDNSTY */
		MTCMPRESS,		/* MTCMPRESS */
		MTEWARN			/* MTEWARN */
	};

	switch (cmd) {

	case MTIOCTOP:	/* tape operation */
		mtop = (struct mtop *)data;
		count = mtop->mt_count;
		code = 0;

		if ((unsigned)mtop->mt_op < MTREW && mtop->mt_count <= 0)
			return (EINVAL);
		switch (mtop->mt_op) {

		case MTWEOF:
			if (sc->sc_blkno != -1) {
				sc->sc_fileno += count;
				sc->sc_blkno = 0;
			}
			break;

		case MTBSF:
			count = -count;
			/* FALLTHROUGH */
		case MTFSF:
			code = 1;
			if (sc->sc_blkno != -1) {
				sc->sc_fileno += count;
				sc->sc_blkno = 0;
			}
			break;

		case MTBSR:
			count = -count;
			/* FALLTHROUGH */
		case MTFSR:
			if (sc->sc_blkno != -1)
				sc->sc_blkno += count;
			break;

		case MTREW:
		case MTNOP:
		case MTCACHE:
		case MTNOCACHE:
			count = 0;
			break;

		case MTOFFL:
			count = SCSI_LD_UNLOAD;
			break;

		case MTRETEN:
			count = SCSI_LD_RETENSION;
			break;

		default:
			return (EINVAL);
		}
		return (tzcommand(dev, tzops[mtop->mt_op], code, count, 0));

	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_type = MT_ISAR;	/* "GENERIC" SCSI */
		mtget->mt_dsreg = 0;
		if (sc->sc_flags & TZF_RDONLY)
			mtget->mt_dsreg |= MT_DS_RDONLY;
		if (sc->sc_flags & TZF_MOUNTED)
			mtget->mt_dsreg |= MT_DS_MOUNTED;
		mtget->mt_erreg = sc->sc_sense.status;
		mtget->mt_resid = sc->sc_buf.b_resid;
		mtget->mt_fileno = sc->sc_fileno;
		mtget->mt_blkno = sc->sc_blkno;

		/* clear latched errors */
		sc->sc_sense.status = 0;
		sc->sc_buf.b_resid = 0;

		break;

	case MTIOCIEOT:
	case MTIOCEEOT:
		break;

	default:
		return (EINVAL);
	}
	return (0);
}

void
tzstrategy(bp)
	struct buf *bp;
{
	int unit = tzunit(bp->b_dev);
	struct tz_softc *sc = &tz_softc[unit];
	int s;

	if (sc->sc_flags & TZF_SEENEOF) {
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}
	s = splbio();
	BUFQ_INSERT_TAIL(&sc->sc_tab, bp);
	if (sc->sc_active == 0) {
		sc->sc_active = 1;
		tzstart(unit);
	}
	splx(s);
}

/*
 * Non-interrupt driven, non-dma dump routine.
 */
int
tzdump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{

	/* Not implemented. */
	return (ENXIO);
}
#endif
