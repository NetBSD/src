/*
 * Copyright (c) 1990 University of Utah.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: tz.c 1.8 90/10/14$
 *
 *      @(#)tz.c	7.3 (Berkeley) 5/4/91
 */

/*
 * SCSI CCS (Command Command Set) tape driver.
 *
 * Specific to Exabyte:
 * mt status: residual="Mbytes to LEOM"
 * minor bit 4 [b1bbbb] (aka /dev/rst16) selects short filemarks
 * minor bit 5 [1bbbbb] (aka /dev/rst32) selects fix block mode, 1k blocks.
 *
 * Archive drive:
 * can read both QIC-24 and QIC-II. But only writes
 * QIC-24.
 * 
 * Supports Archive Viper QIC-150 tape drive, but scsi.c reports selection
 * errors.
 *
 * Supports Archive Python DAT drive, but will sometimes hang machine.
 *
 * Supports HP 35450A DAT drive, but will sometimes hang machine.
 * Partitioning of tape not supported.
 * Vendor unique support has not been added.
 *
 *
 * Supports Archive VIPER (QIC-150).
 * Mostly Supports Archive PYTHON (DAT). 
 *     Hangs if write after spin down. 
 *     Need scsi.c that does connect/disconnect.
 */

/*
 * support for the block device not implemented 
 */

#include "tz.h"
#if NTZ > 0

#include "param.h"
#include "systm.h"
#include "buf.h"
#include "device.h"
#include "siopreg.h"
#include "file.h"
#include "tty.h"
#include "proc.h"
#include "mtio.h"
#include "ioctl.h"
#include "kernel.h"
#include "tprintf.h"

#include "tzvar.h"

#define ADD_DELAY

struct tz_softc;
struct tz_xsense;

extern int tzinit (register struct amiga_device *ad);
extern int tzident (register struct tz_softc *sc, register struct amiga_device *ad);
extern int tzopen (dev_t dev, int flag, int type, struct proc *p);
extern int tzclose (dev_t dev, int flag);
extern int tzstrategy (register struct buf *bp);
extern int tzustart (int unit);
extern int tzstart (int unit);
extern int tzgo (int unit);
extern int tzfinish (int unit, struct tz_softc *sc, struct buf *bp);
extern int tzdump (dev_t dev);
extern int tzioctl (dev_t dev, int cmd, caddr_t data, int flag);
extern int tzintr (int unit, int stat);
extern int tzcommand (dev_t dev, u_int command, int cnt);
extern int tzerror (int unit, struct tz_softc *sc, int stat);
extern int tzxsense (int ctlr, int slave, int unit, struct tz_softc *sc);
extern int tzprtkey (int unit, struct tz_softc *sc);
extern int tzdumpxsense (struct tz_xsense *sensebuf, struct tz_softc *sc);
extern int tzprtmodsel (struct mode_select_data *msd, int modlen);
extern int tzprtmodstat (struct mode_sense *mode);

int	tzinit(), tzstart(), tzgo(), tzintr();
struct	driver tzdriver = {
	tzinit, "tz", tzstart, tzgo, tzintr, 0,
};

struct	tz_softc {
	struct	amiga_device *sc_ad;
	struct	devqueue sc_dq;
	long	sc_blkno;       /* (possible block device support?) */
	long	sc_resid;	/* (possible block device support?) */
	int	sc_flags;
	int	sc_blklen;	/* 0 = variable len records */
	int	sc_filepos;	/* file position on tape */
	long	sc_numblks;	/* number of blocks on tape */
	short	sc_type;	/* ansi scsi type */
	short	sc_punit;	/* physical unit (scsi lun) */
	short	sc_tapeid;	/* tape drive id */
	char	sc_datalen[32];	/* additional data length on some commands */
	short	sc_tticntdwn;	/* interrupts between TTi display updates */
	tpr_t	sc_ctty;
	struct	buf *sc_bp;
	u_char	sc_cmd;
} tz_softc[NTZ];

/* softc flags */
#define STF_ALIVE	0x0001
#define STF_OPEN	0x0002
#define STF_WMODE	0x0004
#define STF_WRTTN	0x0008
#define STF_CMD		0x0010
#define STF_LEOT	0x0020
#define STF_MOVED	0x0040

struct	tz_mode tz_mode[NTZ];

/*
 * Maybe this should not be global, but gives chance to get
 * tape remaining, Rewrites/ECC, etc outside the driver 
 */
static struct tz_xsense {
	struct	scsi_xsense sc_xsense;	/* data from sense */
	union {
	    struct exb_xsense uexb_xsense; /* additional info from exabyte */
	    struct cpr_xsense ucpr_xsense; /* additional info from caliper */
	} u;
} tz_xsense[NTZ];
#define exb_xsense u.uexb_xsense
#define cpr_xsense u.ucpr_xsense

static struct scsi_fmt_cdb tzcmd[NTZ];

static struct scsi_fmt_cdb tz_read_cmd = { 6, CMD_READ };
static struct scsi_fmt_cdb tz_write_cmd = { 6, CMD_WRITE };

struct buf tztab[NTZ];
struct buf tzbuf[NTZ];

#define UNIT(x)		(minor(x) & 3)
#define tzpunit(x)	((x) & 7)

#define STDEV_NOREWIND	0x04
#define STDEV_HIDENSITY	0x08
#define STDEV_EXSFMK	0x10
#define STDEV_FIXEDBLK	0x20

#ifdef DEBUG
#define ST_OPEN		0x0001
#define ST_GO		0x0002
#define ST_FMKS		0x0004
#define ST_OPENSTAT	0x0008
#define ST_BRESID	0x0010
#define ST_ODDIO	0x0020
int tz_debug = 0; /*ST_ODDIO|ST_BRESID;*/
#endif

/*
 * Patchable variable.  If an even length read is requested a dma transfer is
 * used.  Only after the read will we find out if the read had an odd number
 * of bytes.  The HP98658 hardware cannot do odd length transfers, the last
 * byte of the data will always be 0x00.  Normally, the driver will complain
 * about such transfers and return EIO.  However, if st_dmaoddretry is non-
 * zero, the driver will try and issue a BSR and then re-read the data using
 * 'programmed transfer mode'.  In most cases this works, however for unknown
 * reasons it will hang the machine in certain cases.
 *
 * Note:
 * Odd length read requests are always done using programmed transfer mode.
 */
int tz_dmaoddretry = 0;

/*
 * Exabyte only:
 * From adb can have access to fixed vs. variable length modes.
 * Use 0x400 for 1k (best capacity) fixed length records.
 * In st_open, if minor bit 4 set then 1k records are used.
 * If st_exblken is set to anything other then 0 we are in fixed length mode.
 * Minor bit 4 overrides any setting of st_exblklen.
 * 
 */
int tz_exblklen = 0;

/* exabyte here for adb access, set at open time */
#define EX_CT 	0x80		/* international cart - more space W/P6  */
#define EX_ND	0x20		/* no disconnect  */
#define EX_NBE	0x08		/* no busy enable  */
#define EX_EBD	0x04		/* even byte disconnect  */
#define EX_PE	0x02		/* parity enable  */
#define EX_NAL	0x01		/* no auto load  */
int tz_exvup = (EX_CT|EX_ND|EX_NBE); /* vendor unique parameters */

/*
 * motion and reconnect thresholds guidelines:
 * write operation; lower motion threshold for faster transfer
 *                  raise reconnect threshold for slower transfer
 * read operation; lower motion threshold for slower transfer
 *                 raise reconnect threshold for faster transfer
 */
int tz_exmotthr = 0x80;		/* motion threshold, 0x80 default */
int tz_exreconthr = 0xa0;	/* reconnect threshold, 0xa0 default */
int tz_exgapthr = 7;		/* gap threshold, 7 default */
#ifdef TTI
int tz_extti = 0x01;		/* bitmask of unit numbers, do extra */
				/* sensing so TTi display gets updated */
#endif

tzinit(ad)
	register struct amiga_device *ad;
{
	register struct tz_softc *sc = &tz_softc[ad->amiga_unit];

	sc->sc_ad = ad;
	sc->sc_punit = tzpunit(ad->amiga_flags);
	sc->sc_type = tzident(sc, ad);
	if (sc->sc_type < 0)
		return(0);
	sc->sc_dq.dq_ctlr = ad->amiga_ctlr;
	sc->sc_dq.dq_unit = ad->amiga_unit;
	sc->sc_dq.dq_slave = ad->amiga_slave;
	sc->sc_dq.dq_driver = &tzdriver;
	sc->sc_blkno = 0;
	sc->sc_flags = STF_ALIVE;
	return(1);
}

tzident(sc, ad)
	register struct tz_softc *sc;
	register struct amiga_device *ad;
{
	int unit;
	int ctlr, slave;
	int i, stat, inqlen;
	char idstr[32];
#ifdef ADD_DELAY
	static int havetz = 0;
#endif
	struct tz_inquiry {
		struct	scsi_inquiry inqbuf;
		struct  exb_inquiry exb_inquiry;
	} tz_inqbuf;
	static struct scsi_fmt_cdb tz_inq = {
		6,
		CMD_INQUIRY, 0, 0, 0, sizeof(tz_inqbuf), 0
	};

	ctlr = ad->amiga_ctlr;
	slave = ad->amiga_slave;
	unit = sc->sc_punit;
	siop_delay(-1);

	inqlen = 0x05; /* min */
	tz_inq.cdb[4] = 0x05;
	stat = siop_immed_command(ctlr, slave, unit, &tz_inq, 
				  (u_char *)&tz_inqbuf, inqlen, B_READ);
	/* do twice as first command on some scsi tapes always fails */
	stat = siop_immed_command(ctlr, slave, unit, &tz_inq, 
				  (u_char *)&tz_inqbuf, inqlen, B_READ);
	if (stat == -1)
		goto failed;

	if (tz_inqbuf.inqbuf.type != 0x01 ||  /* sequential access device */
	    tz_inqbuf.inqbuf.qual != 0x80 ||  /* removable media */
	    (tz_inqbuf.inqbuf.version != 0x01 && /* current ANSI SCSI spec */
	     tz_inqbuf.inqbuf.version != 0x02))  /* 0x02 is for HP DAT */
		goto failed;

	/* now get additonal info */
	inqlen = 0x05 + tz_inqbuf.inqbuf.len;
	tz_inq.cdb[4] = inqlen;
	bzero(&tz_inqbuf, sizeof(tz_inqbuf));
	stat = siop_immed_command(ctlr, slave, unit, &tz_inq, 
				  (u_char *)&tz_inqbuf, inqlen, B_READ);

	if (tz_inqbuf.inqbuf.len >= 28) {
		bcopy((caddr_t)&tz_inqbuf.inqbuf.vendor_id, (caddr_t)idstr, 28);
		for (i = 27; i > 23; --i)
			if (idstr[i] != ' ')
				break;
		idstr[i+1] = 0;
		for (i = 23; i > 7; --i)
			if (idstr[i] != ' ')
				break;
		idstr[i+1] = 0;
		for (i = 7; i >= 0; --i)
			if (idstr[i] != ' ')
				break;
		idstr[i+1] = 0;
		printf("tz%d: %s %s rev %s\n", ad->amiga_unit, idstr, &idstr[8],
		       &idstr[24]);
	} else if (inqlen == 5)
		/* great it's a stupid device, doesn't know it's own name */
		idstr[0] = idstr[8] = '\0';
	else
		idstr[8] = '\0';

	if (stat == 0xff) { 
		printf("tz%d: Cant handle this tape drive\n", ad->amiga_unit);
		goto failed;
	}

#define IS_ATAPE(x) (bcmp(x,&idstr[8],sizeof(x) -1) == 0)

	if (bcmp("EXB-8200", &idstr[8], 8) == 0) {
		sc->sc_tapeid = MT_ISEXABYTE;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 26;
		sc->sc_datalen[CMD_INQUIRY] = 52;
		sc->sc_datalen[CMD_MODE_SELECT] = 17;
		sc->sc_datalen[CMD_MODE_SENSE] = 17;
      } else if (IS_ATAPE("VIPER 2525") ||
                  (bcmp("VIPER 150", &idstr[8], 9) == 0)) {
		sc->sc_tapeid = MT_ISVIPER1;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 14;
		sc->sc_datalen[CMD_INQUIRY] = 36;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
	} else if (bcmp("Python 27216", &idstr[8], 12) == 0
		   || bcmp("Python 25501", &idstr[8], 12) == 0) {
		sc->sc_tapeid = MT_ISPYTHON;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 14;
		sc->sc_datalen[CMD_INQUIRY] = 36;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
	} else if (bcmp("HP35450A", &idstr[8], 8) == 0) {
		/* XXX "extra" stat makes the HP drive happy at boot time */
		stat = siop_test_unit_rdy(ctlr, slave, unit);
		sc->sc_tapeid = MT_ISHPDAT;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 14;
		sc->sc_datalen[CMD_INQUIRY] = 36;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
	} else if (bcmp("5150ES", &idstr[8], 6) == 0) {
		sc->sc_tapeid = MT_ISWANGTEK;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 14;
		sc->sc_datalen[CMD_INQUIRY] = 36;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
	} else if (bcmp("CP150", &idstr[8], 5) == 0) {
		sc->sc_tapeid = MT_ISCALIPER;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 14;
		sc->sc_datalen[CMD_INQUIRY] = 36;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
	} else if (bcmp("5099ES", &idstr[8], 6) == 0) {
	        sc->sc_tapeid = MT_ISWTEK5099;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 14;
		sc->sc_datalen[CMD_INQUIRY] = 36;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
	} else {
		if (idstr[8] == '\0')
			printf("tz%d: No ID, assuming Archive\n", ad->amiga_unit);
		else
			printf("tz%d: Unsupported tape device\n", ad->amiga_unit);
#if 0
		sc->sc_tapeid = MT_ISAR;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 8;
		sc->sc_datalen[CMD_INQUIRY] = 5;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
#else
		sc->sc_tapeid = MT_ISWANGTEK;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 14;
		sc->sc_datalen[CMD_INQUIRY] = 36;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
#endif
	}

	sc->sc_filepos = 0;
	
	/* load xsense */
	tzxsense(ctlr, slave, unit, sc);

	siop_delay(0);
#ifdef ADD_DELAY
	/* XXX if we have a tape, we must up the delays in the HA driver */
	if (!havetz) {
		havetz = 1;
		siop_delay(20000);
	}
#endif
	return(tz_inqbuf.inqbuf.type);
failed:
	siop_delay(0);
	return(-1);
}

tzopen(dev, flag, type, p)
	dev_t dev;
	int flag, type;
	struct proc *p;
{
	register struct tz_softc *sc = &tz_softc[UNIT(dev)];
	register struct tz_xsense *xsense;
	register int count;
	register int stat;
	int ctlr, slave, unit;
	struct mode_select_data msd;
	struct mode_sense mode;
	int modlen;
	int skip_modsel = 0;
	static struct scsi_fmt_cdb modsel = {
		6,
		CMD_MODE_SELECT, 0, 0, 0, sizeof(msd), 0
	};
	static struct scsi_fmt_cdb unitready = {
		6,
		CMD_TEST_UNIT_READY, 0, 0, 0, 0, 0
	};
	static struct scsi_fmt_cdb modsense = {
		6,
		CMD_MODE_SENSE, 0, 0, 0, sizeof(mode), 0
	};

	ctlr = sc->sc_dq.dq_ctlr;
	slave = sc->sc_dq.dq_slave;
	unit = sc->sc_punit;
	xsense = &tz_xsense[UNIT(dev)];

	if (UNIT(dev) > NTZ || (sc->sc_flags & STF_ALIVE) == 0)
		return(ENXIO);
	if (sc->sc_flags & STF_OPEN)
		return(EBUSY);

	/* do a mode sense to get current */
	modlen = sc->sc_datalen[CMD_MODE_SENSE];
	modsense.cdb[4] = modlen;
	stat = siop_immed_command(ctlr, slave, unit, &modsense,
				  (u_char *)&mode, modlen, B_READ);

	/* do a mode sense to get current */
	modlen = sc->sc_datalen[CMD_MODE_SENSE];
	modsense.cdb[4] = modlen;
	stat = siop_immed_command(ctlr, slave, unit, &modsense,
				  (u_char *)&mode, modlen, B_READ);

printf("tz: stat = %d, blklen = %d\n", stat, 
       (mode.md.blklen2 << 16 | mode.md.blklen1 << 8 | mode.md.blklen0));

	/* set record length */
	switch (sc->sc_tapeid) {
	case MT_ISAR:
		sc->sc_blklen = 512;
		break;
	case MT_ISEXABYTE:
		if (minor(dev) & STDEV_FIXEDBLK)
			sc->sc_blklen = 0x400;
		else
			sc->sc_blklen = tz_exblklen;
		break;
	case MT_ISHPDAT:
		sc->sc_blklen = 512;
		break;
	case MT_ISVIPER1:
		sc->sc_blklen = 512;
		break;
	case MT_ISPYTHON:
		if (minor(dev) & STDEV_FIXEDBLK)
		  sc->sc_blklen = 20*512;
		else
		  sc->sc_blklen = 512;
		break;
	case MT_ISWANGTEK:
	case MT_ISCALIPER:
 		tzxsense(ctlr, slave, unit, sc);
 		/* Calipers can't handle a mode-select if the tape
 		 * isn't rewound.
 		 */
 		if (xsense->cpr_xsense.b11)
 			skip_modsel = 1;
		sc->sc_blklen = 512;
		break;
	case MT_ISWTEK5099:
		sc->sc_blklen = 512;
		break;
	default:
		if ((mode.md.blklen2 << 16 |
		     mode.md.blklen1 << 8 |
		     mode.md.blklen0) != 0)
			sc->sc_blklen = mode.md.blklen2 << 16 |
					mode.md.blklen1 << 8 |
					mode.md.blklen0;
		else
			sc->sc_blklen = 512;
	}

	if (skip_modsel)
		goto mode_selected;

	/* setup for mode select */
	msd.rsvd1 = 0;
	msd.rsvd2 = 0;
	msd.rsvd3 = 0;
	msd.buff = 1;
	msd.speed = 0;
	msd.blkdeslen = 0x08;
	msd.density = 0;
	msd.blks2 = 0;
	msd.blks1 = 0;
	msd.blks0 = 0;
	msd.rsvd4 = 0;
	msd.blklen2 = (sc->sc_blklen >> 16) & 0xff;
	msd.blklen1 = (sc->sc_blklen >> 8) & 0xff;
	msd.blklen0 = sc->sc_blklen & 0xff;

	switch (sc->sc_tapeid) {
	case MT_ISAR:
		if (minor(dev) & STDEV_HIDENSITY)
			msd.density = 0x5;
		else {
			if (flag & FWRITE) {
				uprintf("Can only write QIC-24\n");
				return(EIO);
			}
			msd.density = 0x4;
		}
		break;
	case MT_ISCALIPER:
	case MT_ISWANGTEK:
		if (minor (dev) & STDEV_HIDENSITY)
			msd.density = 0x10;
		else
			msd.density = 0x0F;
		break;
	case MT_ISEXABYTE:
		if (minor(dev) & STDEV_HIDENSITY)
			uprintf("EXB-8200 density support only\n");
		msd.vupb = (u_char)tz_exvup;
		msd.rsvd5 = 0;
		msd.p5 = 0;
		msd.motionthres = (u_char)tz_exmotthr;
		msd.reconthres = (u_char)tz_exreconthr;
		msd.gapthres = (u_char)tz_exgapthr;
		break;
	case MT_ISHPDAT:
	case MT_ISVIPER1:
	case MT_ISPYTHON:
	case MT_ISWTEK5099:
		if (minor(dev) & STDEV_HIDENSITY)
			uprintf("Only one density supported\n");
		break;
	default:
		uprintf("Unsupported drive\n");
		return(EIO);
	}

	modlen = sc->sc_datalen[CMD_MODE_SELECT];
	modsel.cdb[4] = modlen;

	/* mode select */
	count = 0;
retryselect:
	stat = siop_immed_command(ctlr, slave, unit, &modsel,
				  (u_char *)&msd, modlen, B_WRITE);
	/*
	 * First command after power cycle, bus reset or tape change 
	 * will error. Try command again
	 */
	if (stat == STS_CHECKCOND) {
		sc->sc_filepos = 0;
		tzxsense(ctlr, slave, unit, sc);
		stat = siop_immed_command(ctlr, slave, unit, &modsel,
					  (u_char *)&msd, modlen, B_WRITE);
#ifdef DEBUG
		if (stat && (tz_debug & ST_OPEN))
			printf("tzopen: stat on mode select 0x%x second try\n", stat);
#endif
		if (stat == STS_CHECKCOND) {
			tzxsense(ctlr, slave, unit, sc);
			tzprtkey(UNIT(dev), sc);
		}
		if (stat)
			return(EIO);
	}
	if (stat == -1 || stat == STS_BUSY) {
		/*
		 * XXX it might just be that the bus is busy because
		 * another tape is doing a command. This would change
		 * with connect/disconnect, ie. the other drive would
		 * not hold onto the bus.
		 *
		 * Sleep on lbolt for up to 20 minutes (max time to FSF
		 * an exabyte to EOT: 16:xx minutes)
		 */
		if (++count > 60*20) {
			uprintf("SCSI bus timeout\n");
			return(EBUSY);
		}
		sleep((caddr_t)&lbolt, PZERO-1);
		goto retryselect;
	}

mode_selected:
	/* drive ready ? */
	stat = siop_test_unit_rdy(ctlr, slave, unit);

	if (stat == STS_CHECKCOND) {
		tzxsense(ctlr, slave, unit, sc);
		switch (sc->sc_tapeid) {
		case MT_ISEXABYTE:
			if ((xsense->sc_xsense.key == XSK_NOTRDY) &&
			    xsense->exb_xsense.tnp)
				uprintf("no cartridge\n");
			else if (xsense->sc_xsense.key == XSK_NOTRDY)
				uprintf("cartridge not loaded\n");
			else if (xsense->sc_xsense.key == XSK_UNTATTEN) {
				uprintf("new cart/power interrupt\n");
				stat = 0;
			} else if ((xsense->sc_xsense.key == XSK_UNTATTEN) &&
				   xsense->exb_xsense.tnp)
				uprintf("cartridge unloading\n");
			else 
				tzprtkey(UNIT(dev), sc);
			break;
		case MT_ISAR:
			if (xsense->sc_xsense.key == XSK_UNTATTEN)
				stat = siop_test_unit_rdy(ctlr, slave, unit);
			if (stat == STS_CHECKCOND) {
				tzxsense(ctlr, slave, unit, sc);
				if (xsense->sc_xsense.key)
					tzprtkey(UNIT(dev), sc);
			} else { 
				sc->sc_filepos = 0; /* new tape */
				stat = 0;
			}
			break;
		case MT_ISHPDAT:
		case MT_ISVIPER1:
		case MT_ISPYTHON:
		case MT_ISCALIPER:
		case MT_ISWANGTEK:
		case MT_ISWTEK5099:
			if (xsense->sc_xsense.key == XSK_UNTATTEN)
				stat = siop_test_unit_rdy(ctlr, slave, unit);
			if (stat == STS_CHECKCOND) {
				tzxsense(ctlr, slave, unit, sc);
				if (xsense->sc_xsense.key)
					tzprtkey(UNIT(dev), sc);
			}
			break;
		default:
			uprintf("tz%d: not ready\n", UNIT(dev));
			tzprtkey(UNIT(dev), sc);
			break;
		}
	}
	if (stat)
		return(EIO);

	/* mode sense */
	modlen = sc->sc_datalen[CMD_MODE_SENSE];
	modsense.cdb[4] = modlen;
	stat = siop_immed_command(ctlr, slave, unit, &modsense,
				  (u_char *)&mode, modlen, B_READ);
#ifdef DEBUG
	if (tz_debug & ST_OPENSTAT)
		tzprtmodstat(&mode);
#endif

	if (stat == STS_CHECKCOND) {
		tzxsense(ctlr, slave, unit, sc);
#ifdef DEBUG
		if (tz_debug & ST_OPEN)
			tzdumpxsense(xsense, sc);
#endif
	}
	if (stat)
		return(EIO);

	if ((flag & FWRITE) && mode.md.wp) {
		uprintf("tz:%d write protected\n", UNIT(dev));
		return(EACCES);
	}

	sc->sc_ctty = tprintf_open(p);
	/* save total number of blocks on tape */
	sc->sc_numblks = mode.md.numblk2 << 16 |
			 mode.md.numblk1 << 8 |
			 mode.md.numblk0;

	if (xsense->sc_xsense.eom && !(sc->sc_flags & STF_LEOT))
		sc->sc_filepos = 0;
#ifdef DEBUG
	if (tz_debug & ST_FMKS)
		printf("tz%d: open filepos = %d\n", UNIT(dev), sc->sc_filepos);
#endif

	sc->sc_flags |= (STF_OPEN);
	if (flag & FWRITE)
		sc->sc_flags |= STF_WMODE;
	sc->sc_flags &= ~STF_MOVED;

#ifdef TTI
	/* make stats available, also lit up TTi display */
	sc->sc_tticntdwn = 100;
#endif
	tzxsense(ctlr, slave, unit, sc);

	return(0);
}

/*ARGSUSED*/
tzclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tz_softc *sc = &tz_softc[UNIT(dev)];
	register int hit = 0;

	if ((sc->sc_flags & (STF_WMODE|STF_WRTTN)) == (STF_WMODE|STF_WRTTN)) {
		/*
		 * XXX driver only supports cartridge tapes.
		 * Cartridge tapes don't do double EOFs on EOT.
		 */
		tzcommand(dev, MTWEOF, 1); 
		hit++;
	}
	if ((minor(dev) & STDEV_NOREWIND) == 0) {
		tzcommand(dev, MTREW, 1);
		hit++;
	}
#ifdef NOTDEF
	/* wait until more stable before trying [XXX Needed ?] */
	if (!hit && (sc->sc_flags & SFT_WMODE))
		/* force out any any bufferd write data */
		tzcommand(dev, MTFSR, 0); 
#endif
	/* make stats available */
	tzxsense(sc->sc_dq.dq_ctlr, sc->sc_dq.dq_slave, sc->sc_punit, sc);

	sc->sc_flags &= ~(STF_OPEN|STF_WMODE|STF_WRTTN);
	tprintf_close(sc->sc_ctty);
	return(0);	/* XXX */
}

tzstrategy(bp)
	register struct buf *bp;
{
	struct buf *dp;
	int unit, s;

	unit = UNIT(bp->b_dev);
	dp = &tztab[unit];
	bp->av_forw = NULL;
	s = splbio();
	if (dp->b_actf == NULL)
		dp->b_actf = bp;
	else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	if (dp->b_active == 0) {
		dp->b_active = 1;
		tzustart(unit);
	}
	splx(s);
}

tzustart(unit)
	int unit;
{
	if (siopreq(&tz_softc[unit].sc_dq))
		tzstart(unit);
}

tzstart(unit)
	int unit;
{
	struct amiga_device *am = tz_softc[unit].sc_ad;

	if (siopustart(am->amiga_ctlr))
		tzgo(unit);
}

tzgo(unit)
	int unit;
{
	register struct tz_softc *sc = &tz_softc[unit];
	register struct scsi_fmt_cdb *cmd;
	register struct buf *bp = tztab[unit].b_actf;
	struct amiga_device *am = sc->sc_ad;
	int pad, stat;
	long nblks;

	if (! bp)
	  {
	    printf ("tzgo (%d) with empty buffer!!\n", unit);
	    return;
	  }

	if (sc->sc_flags & STF_CMD) {
		cmd = &tzcmd[unit];
		pad = 0;
	} else {
		cmd = bp->b_flags & B_READ ? &tz_read_cmd : &tz_write_cmd;
		if (sc->sc_blklen)
			cmd->cdb[1] |= 0x01; /* fixed mode */
		else
			cmd->cdb[1] &= 0xfe;
		if (bp->b_flags & B_READ)
			sc->sc_flags &= ~STF_WRTTN;
		else
			sc->sc_flags |= STF_WRTTN;

		if (sc->sc_blklen) { /* fixed mode */
			nblks = bp->b_bcount / sc->sc_blklen;
			if (bp->b_bcount % sc->sc_blklen) {
				tprintf(sc->sc_ctty,
					"tz%d: I/O not block aligned %d/%ld\n",
					unit, sc->sc_blklen, bp->b_bcount);
				cmd->cdb[1] &= 0xfe; /* force error */
			}
		} else	/* variable len */
			nblks = bp->b_bcount;

		*(u_char *)(&cmd->cdb[2]) = (u_char) (nblks >> 16);
		*(u_char *)(&cmd->cdb[3]) = (u_char) (nblks >> 8);
		*(u_char *)(&cmd->cdb[4]) = (u_char) nblks;
		/*
		 * Always Zero. We are either writing in variable
		 * length mode we are writing in fixed block mode,
		 * or we are going to do odd length IO and are not
		 * going to use DMA.
		 */
		pad = 0; 
	}

#ifdef DEBUG
	if (tz_debug & ST_GO)
		printf("tzgo: cmd len %d [0]0x%x [1]0x%x [2]0x%x [3]0x%x [4]0x%x [5]0x%x\n",
		       cmd->len, cmd->cdb[0], cmd->cdb[1], cmd->cdb[2],
		       cmd->cdb[3], cmd->cdb[4], cmd->cdb[5]);
#endif

	sc->sc_flags |= STF_MOVED;
	if (bp->b_bcount & 1) {
#ifdef DEBUG
		if (tz_debug & ST_ODDIO)
			printf("tzgo%d: odd count %d using manual transfer\n",
			       unit, bp->b_bcount);
#endif
		stat = siop_tt_oddio(am->amiga_ctlr, am->amiga_slave, sc->sc_punit,
				     bp->b_un.b_addr, bp->b_bcount,
				     bp->b_flags, 1);
		if (stat == 0) {
			bp->b_resid = 0;
			tzfinish(unit, sc, bp);
		}
	} else
		stat = siopgo(am->amiga_ctlr, am->amiga_slave, sc->sc_punit,
			      bp, cmd, pad);
	if (stat) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		tzxsense(sc->sc_dq.dq_ctlr, sc->sc_dq.dq_slave, 
			 sc->sc_punit, sc);
		tzerror(unit, sc, stat);
		tzfinish(unit, sc, bp);
	}
}

tzfinish(unit, sc, bp)
	int unit;
	struct tz_softc *sc;
	struct buf *bp;
{
	tztab[unit].b_errcnt = 0;
	tztab[unit].b_actf = bp->b_actf;
	iodone(bp);
	siopfree(&sc->sc_dq);
	if (tztab[unit].b_actf)
		tzustart(unit);
	else
		tztab[unit].b_active = 0;
}

#if 1
tzread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int unit = UNIT(dev);

	/*return(physio(tzstrategy, &tzbuf[unit], dev, B_READ, minphys, uio));*/
	return(physio(tzstrategy, 0, dev, B_READ, minphys, uio));
}

tzwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int unit = UNIT(dev);

	/*return(physio(tzstrategy, &tzbuf[unit], dev, B_WRITE, minphys, uio));*/
	return(physio(tzstrategy, 0, dev, B_WRITE, minphys, uio));
}
#endif

/*ARGSUSED*/
tzdump(dev)
	dev_t dev;
{
	return(ENXIO);
}

/*ARGSUSED*/
tzioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data; 
	int flag;
{
	register struct tz_softc *sc = &tz_softc[UNIT(dev)];
	register int cnt;
	register struct mtget *mtget;
	register struct tz_xsense *xp = &tz_xsense[UNIT(dev)];
	register struct mtop *op;
	long resid;

	switch (cmd) {

	/* tape operation */
	case MTIOCTOP:
		op = (struct mtop *)data;
		switch (op->mt_op) {

		case MTBSR:
		case MTFSR:
			if (sc->sc_tapeid == MT_ISAR)
				return(ENXIO);
		case MTWEOF:
		case MTFSF:
		case MTBSF:
			cnt = (int)op->mt_count;
			break;

		case MTREW:
		case MTOFFL:
			cnt = 1;
			break;

		case MTNOP:
			return(0);
		default:
			return(EINVAL);
		}
		if (cnt <= 0)
			return(EINVAL);
		tzcommand(dev, (u_int)op->mt_op, cnt);
		break;

	/* drive status */
	case MTIOCGET:
		mtget = (struct mtget *)data;
		tzxsense(sc->sc_dq.dq_ctlr, sc->sc_dq.dq_slave, 
			 sc->sc_punit, sc);
		mtget->mt_type = sc->sc_tapeid;
		mtget->mt_dsreg = 0;
		mtget->mt_erreg = ((xp->sc_xsense.valid << 15) |
				   (xp->sc_xsense.filemark << 14) |
				   (xp->sc_xsense.eom << 13) |
				   (xp->sc_xsense.ili << 12) |
				   (xp->sc_xsense.key << 8));
		
		if (sc->sc_tapeid == MT_ISEXABYTE) {
			mtget->mt_dsreg = sc->sc_flags;
			resid = (xp->exb_xsense.tplft2 << 16 |
				 xp->exb_xsense.tplft1 << 8 |
				 xp->exb_xsense.tplft0);
			mtget->mt_resid = resid / 1000;
			mtget->mt_erreg |= (((xp->exb_xsense.rwerrcnt2 << 16 |
					      xp->exb_xsense.rwerrcnt1 << 8 |
					      xp->exb_xsense.rwerrcnt0) * 100) / 
					    (sc->sc_numblks - resid)) & 0xff;
		} else if (xp->sc_xsense.valid) {
			resid = ((xp->sc_xsense.info1 << 24) |
				 (xp->sc_xsense.info2 << 16) |
				 (xp->sc_xsense.info3 << 8) |
				 (xp->sc_xsense.info4));
			if (sc->sc_blklen) /* if fixed mode */
				resid *= sc->sc_blklen;
			mtget->mt_resid = resid;
		} else
			mtget->mt_resid = 0;
		break;

	default:
		return(ENXIO);
	}
	return(0);
}

tzintr(unit, stat)
	int unit, stat;
{
	register struct tz_softc *sc = &tz_softc[unit];
	register struct tz_xsense *xp = &tz_xsense[unit];
	register struct buf *bp = tztab[unit].b_actf;
	struct amiga_device *am = sc->sc_ad;

#ifdef DEBUG
	if (bp == NULL) {
		printf("tz%d: bp == NULL\n", unit);
		return;
	}
#endif
	switch (stat) {
	/* scsi command completed ok */
	case 0:
		bp->b_resid = 0;
		break;

	/* more status */
	case STS_CHECKCOND:
		tzxsense(sc->sc_dq.dq_ctlr, sc->sc_dq.dq_slave, 
			 sc->sc_punit, sc);
		if (xp->sc_xsense.valid) {
			bp->b_resid = (u_long)((xp->sc_xsense.info1 << 24) |
					      (xp->sc_xsense.info2 << 16) |
					      (xp->sc_xsense.info3 << 8) |
					      (xp->sc_xsense.info4));
			if (sc->sc_blklen) /* fixed mode */
				bp->b_resid *= sc->sc_blklen;
		}
		if (xp->sc_xsense.filemark) {
			sc->sc_filepos++;
			break;
		}
		if (xp->sc_xsense.key) {
			tzerror(unit, sc, stat);
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			break;
		}
		if (xp->sc_xsense.ili) {
			/*
			 * Fixed length blocks, an error.
			 */
			if (sc->sc_blklen) {
				tprintf(sc->sc_ctty,
					"tz%d: Incorrect Length Indicator, blkcnt diff %d\n",
					unit, sc->sc_blklen - bp->b_resid);
				bp->b_flags |= B_ERROR;
				bp->b_error = EIO;
				break;
			}
			/*
			 * Variable length but read more than requested,
			 * an error.
			 */
			if (bp->b_resid < 0) {
				bp->b_resid = 0;
				bp->b_flags |= B_ERROR;
				bp->b_error = ENOMEM;
				break;
			}
			/*
			 * Variable length and odd, may require special
			 * handling.
			 */
			if (bp->b_resid & 1 && sc->sc_tapeid != MT_ISAR) {
				/*
				 * Normal behavior, treat as an error.
				 */
				if (!tz_dmaoddretry) {
					tprintf(sc->sc_ctty,
						"tz%d: Odd length read %d\n", 
						UNIT(bp->b_dev),
						bp->b_bcount - bp->b_resid);
					bp->b_error = EIO;
					bp->b_flags |= B_ERROR;
					break;
				}
				/*
				 * Attempt to back up and re-read using oddio.
				 */
#ifdef DEBUG
				if (tz_debug & ST_ODDIO)
					printf("tz%d: tzintr odd count %d, do BSR then oddio\n",
					       UNIT(bp->b_dev),
					       bp->b_bcount - bp->b_resid);
#endif
				stat = siop_tt_oddio(am->amiga_ctlr, am->amiga_slave,
						     sc->sc_punit, 0, -1, 0, 0);
				if (stat == 0)
					stat = siop_tt_oddio(am->amiga_ctlr,
							     am->amiga_slave,
							     sc->sc_punit,
							     bp->b_un.b_addr,
							     bp->b_bcount - bp->b_resid,
							     bp->b_flags, 0);
				if (stat) {
					bp->b_error = EIO;
					bp->b_flags |= B_ERROR;
					tzxsense(sc->sc_dq.dq_ctlr, sc->sc_dq.dq_slave,
						 sc->sc_punit, sc);
					tzerror(unit, sc, stat);
				}
			}
			break;
		}
		if (xp->sc_xsense.eom) {
			bp->b_flags |= B_ERROR;
			bp->b_error = ENOSPC;
			break;
		}
		tprintf(sc->sc_ctty, "tz%d: unknown scsi error\n", unit);
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		break;

	default:
		printf("tz%d: tzintr unknown stat 0x%x\n", unit, stat);
		break;
	}
#ifdef DEBUG
	if ((tz_debug & ST_BRESID) && bp->b_resid != 0)
		printf("b_resid %d b_flags 0x%x b_error 0x%x\n", 
		       bp->b_resid, bp->b_flags, bp->b_error);
#endif
	/* asked for more filemarks then on tape */
	if (bp->b_resid != 0 &&
	    (sc->sc_flags & STF_CMD) && sc->sc_cmd == CMD_SPACE) {
		sc->sc_filepos -= bp->b_resid;
		if (sc->sc_filepos < 0)
			sc->sc_filepos = 0;
	}

#ifdef TTI
	if (tz_extti & (1<<unit) &&
	    sc->sc_type == MT_ISEXABYTE) /* to make display lit up */
		/*
		 * XXX severe performance penality for this.
		 * Try and throttle by not calling stxsense on every intr.
		 * Mostly for TTi we, get a stxsense call in open and close.
		 */
		if (sc->sc_tticntdwn-- == 0) {
			tzxsense(sc->sc_dq.dq_ctlr, sc->sc_dq.dq_slave,
				 sc->sc_punit, sc);
			sc->sc_tticntdwn = 100;
		}
#endif

	tzfinish(unit, sc, bp);
}

tzcommand(dev, command, cnt)
	dev_t dev;
	u_int command;
	int cnt;
{
	register struct tz_softc *sc = &tz_softc[UNIT(dev)];
	register struct buf *bp = &tzbuf[UNIT(dev)];
	register struct scsi_fmt_cdb *cmd = &tzcmd[UNIT(dev)];
	register cmdcnt;
	int s;

	cmd->len = 6; /* all tape commands are cdb6 */
	cmd->cdb[1] = sc->sc_punit;
	cmd->cdb[2] = cmd->cdb[3] = cmd->cdb[4] = cmd->cdb[5] = 0;
	cmdcnt = 0;

	/*
	 * XXX Assumption is that everything except Archive can take
	 * repeat count in cdb block.
	 */
	switch (command) {
	case MTWEOF:
		cmd->cdb[0] = CMD_WRITE_FILEMARK;
		if (sc->sc_tapeid != MT_ISAR) {
			cmdcnt = cnt;
			cnt = 1;
		} else
			cmdcnt = 1;
		*(u_char *)(&cmd->cdb[2]) = (u_char) (cmdcnt >> 16);
		*(u_char *)(&cmd->cdb[3]) = (u_char) (cmdcnt >> 8);
		*(u_char *)(&cmd->cdb[4]) = (u_char) cmdcnt;

		if (sc->sc_tapeid == MT_ISEXABYTE &&
		    (minor(dev) & STDEV_EXSFMK)) /* short filemarks */
			cmd->cdb[5] |= 0x80;
		else
			cmd->cdb[5] &= 0x7f;
		break;
	case MTBSF:
		/* Archive can't back up, will not get to BSR case */
		if (sc->sc_tapeid == MT_ISAR) {
			if ((sc->sc_filepos - cnt) < 0) {
				tzcommand(dev, MTREW, 1);
				return;
			}
			cmdcnt = sc->sc_filepos - cnt + 1;
			tzcommand(dev, MTREW, 1);
			tzcommand(dev, MTFSF, cmdcnt);
			return;
		}
	case MTBSR:
	case MTFSR:
	case MTFSF:
		if (command == MTBSF || command == MTBSR)
			cnt = cnt * (-1); /* backward move */
		if (command == MTFSF || command == MTBSF)
			cmd->cdb[1] |= 0x01; /* filemarks */
		else
			cmd->cdb[1] |= 0x00; /* logical blocks */
		if (sc->sc_tapeid != MT_ISAR) {
			cmdcnt = cnt;
			cnt = 1;
		} else
			cmdcnt = 1;
		*(u_char *)(&cmd->cdb[2]) = (u_char) (cmdcnt >> 16);
		*(u_char *)(&cmd->cdb[3]) = (u_char) (cmdcnt >> 8);
		*(u_char *)(&cmd->cdb[4]) = (u_char) cmdcnt;
		cmd->cdb[0] = CMD_SPACE;
		break;
	case MTREW:
		cmd->cdb[0] = CMD_REWIND;
		sc->sc_filepos = 0;
		break;
	case MTOFFL:
		cmd->cdb[0] = CMD_LOADUNLOAD;
		sc->sc_filepos = 0;
		break;
	default:
		printf("tz%d: tzcommand bad command 0x%x\n", 
		       UNIT(dev), command);
	}

	sc->sc_flags |= STF_CMD;
	sc->sc_cmd = cmd->cdb[0];

	sc->sc_bp = bp;
again:
#ifdef DEBUG
	if (tz_debug & ST_FMKS)
		printf("tz%d: tzcommand filepos %d cmdcnt %d cnt %d\n", 
		       UNIT(dev), sc->sc_filepos, cmdcnt, cnt);
#endif
	s = splbio();
	while (bp->b_flags & B_BUSY) {
		if (bp->b_flags & B_DONE)
			break;
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	bp->b_flags = B_BUSY|B_READ;
	splx(s);
	bp->b_dev = dev;
	bp->b_bcount = 0;
	bp->b_resid = 0;
	bp->b_blkno = 0;
	bp->b_error = 0;
	tzstrategy(bp);
	iowait(bp);
	if (bp->b_flags & B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags &= B_ERROR;

	if (command == MTWEOF || command == MTFSF || command == MTBSF)
		sc->sc_filepos += cmdcnt;

	if (--cnt > 0)
		goto again;

	sc->sc_flags |= STF_MOVED;
	sc->sc_flags &= ~(STF_CMD|STF_WRTTN);
}

tzerror(unit, sc, stat)
	int unit, stat;
	struct tz_softc *sc;
{
	/* stxsense must have been called before sterror() */
	if (stat & STS_CHECKCOND)
		tzprtkey(unit, sc);
	else if (stat)
		tprintf(sc->sc_ctty,
			"tz%d: bad scsi status 0x%x\n", unit, stat);

	if ((sc->sc_flags & STF_CMD) && sc->sc_cmd == CMD_SPACE) /* fsf */
		sc->sc_filepos--;
}

tzxsense(ctlr, slave, unit, sc)
	int ctlr, slave, unit;
	struct tz_softc *sc;
{
	u_char *sensebuf;
	unsigned len;

	sensebuf = (u_char *)&tz_xsense[sc->sc_dq.dq_unit];
	len = sc->sc_datalen[CMD_REQUEST_SENSE];
	siop_request_sense(ctlr, slave, unit, sensebuf, len);
}

tzprtkey(unit, sc)
	int unit;
	struct tz_softc *sc;
{
	register struct tz_xsense *xp = &tz_xsense[unit];

	switch (xp->sc_xsense.key) {
	case XSK_NOSENCE:
		break;
	case XSK_NOTUSED1:
	case XSK_NOTUSEDC:
	case XSK_NOTUSEDE:
		break;
	case XSK_REVERVED:
		tprintf(sc->sc_ctty, "tz%d: Reserved sense key 0x%x\n",
			unit, xp->sc_xsense.key);
		break;
	case XSK_NOTRDY:
		tprintf(sc->sc_ctty, "tz%d: NOT READY\n", unit);
		break;
	case XSK_MEDERR:
		tprintf(sc->sc_ctty, "tz%d: MEDIUM ERROR\n", unit);
		break;
	case XSK_HRDWERR:
		tprintf(sc->sc_ctty, "tz%d: HARDWARE ERROR\n", unit);
		break;
	case XSK_ILLREQ:
		tprintf(sc->sc_ctty, "tz%d: ILLEGAL REQUEST\n", unit);
		break;
	case XSK_UNTATTEN:
		tprintf(sc->sc_ctty, "tz%d: UNIT ATTENTION\n", unit);
		break;
	case XSK_DATAPROT:
		tprintf(sc->sc_ctty, "tz%d: DATA PROTECT\n", unit);
		break;
	case XSK_BLNKCHK:
		tprintf(sc->sc_ctty, "tz%d: BLANK CHECK\n", unit);
		break;
	case XSK_VENDOR:
		tprintf(sc->sc_ctty, "tz%d: VENDER UNIQUE SENSE KEY ", unit);
		switch (sc->sc_tapeid) {
		case MT_ISEXABYTE:
			tprintf(sc->sc_ctty, "Exabyte: ");
			if (xp->exb_xsense.xfr)
				tprintf(sc->sc_ctty,
					"Transfer Abort Error\n");
			if (xp->exb_xsense.tmd)
				tprintf(sc->sc_ctty,
					"Tape Mark Detect Error\n");
			break;
		default:
			tprintf(sc->sc_ctty, "\n");
		}
		break;
	case XSK_CPYABORT:
		tprintf(sc->sc_ctty, "tz%d: COPY ABORTED\n", unit);
		break;
	case XSK_ABORTCMD:
		tprintf(sc->sc_ctty, "tz%d: ABORTED COMMAND\n", unit);
		break;
	case XSK_VOLOVER:
		tprintf(sc->sc_ctty, "tz%d: VOLUME OVERFLOW\n", unit);
		break;
	default:
		tprintf(sc->sc_ctty, "tz%d: unknown sense key 0x%x\n",
			unit, xp->sc_xsense.key);
	}
	if (sc->sc_tapeid == MT_ISEXABYTE) {
		if (xp->exb_xsense.bpe)
			tprintf(sc->sc_ctty, "tz%d: Bus Parity Errorn", unit);
		if (xp->exb_xsense.fpe)
			tprintf(sc->sc_ctty,
				"tz%d: Formatted Buffer Parity Errorn", unit);
		if (xp->exb_xsense.eco)
			tprintf(sc->sc_ctty, "tz%d: Error Counter Overflown",
				unit);
		if (xp->exb_xsense.tme)
			tprintf(sc->sc_ctty, "tz%d: Tape Motion Errorn", unit);
		if (xp->exb_xsense.xfr)
			tprintf(sc->sc_ctty, "tz%d: Transfer About Errorn",
				unit);
		if (xp->exb_xsense.tmd)
			tprintf(sc->sc_ctty, "tz%d: Tape Mark Detect Errorn",
				unit);
		if (xp->exb_xsense.fmke)
			tprintf(sc->sc_ctty, "tz%d: Filemark Errorn", unit);
		if (xp->exb_xsense.ure)
			tprintf(sc->sc_ctty, "tz%d: Under Run Errorn", unit);
		if (xp->exb_xsense.sse)
			tprintf(sc->sc_ctty, "tz%d: Servo System Errorn",
				unit);
		if (xp->exb_xsense.fe)
			tprintf(sc->sc_ctty, "tz%d: Formatter Errorn", unit);
		if (xp->exb_xsense.wseb)
			tprintf(sc->sc_ctty, "tz%d: WSEB Errorn", unit);
		if (xp->exb_xsense.wseo)
			tprintf(sc->sc_ctty, "tz%d: WSEO Errorn", unit);
	}
}

#ifdef DEBUG

tzdumpxsense(sensebuf, sc)
	struct tz_xsense *sensebuf;
	struct tz_softc *sc;
{
        struct tz_xsense *xp = sensebuf;

	printf("valid 0x%x errorclass 0x%x errorcode 0x%x\n", 
	       xp->sc_xsense.valid, 
	       xp->sc_xsense.class, xp->sc_xsense.code);
	printf("seg number 0x%x\n", xp->sc_xsense.segment);
	printf("FMK 0x%x EOM 0x%x ILI 0x%x RSVD 0x%x sensekey 0x%x\n", 
	       xp->sc_xsense.filemark, xp->sc_xsense.eom, xp->sc_xsense.ili, 
	       xp->sc_xsense.rsvd, xp->sc_xsense.key);
	printf("info 0x%lx\n", 
	       (u_long)((xp->sc_xsense.info1<<24)|(xp->sc_xsense.info2<<16)|
			(xp->sc_xsense.info3<<8)|(xp->sc_xsense.info4)) );
	printf("ASenseL 0x%x\n", xp->sc_xsense.len);

	switch (sc->sc_tapeid) {
	case MT_ISEXABYTE:
		printf("ASenseC 0x%x\n", xp->exb_xsense.addsens);
		printf("AsenseQ 0x%x\n", xp->exb_xsense.addsensq);
		printf("R/W Errors 0x%lx\n", 
		       (u_long)((xp->exb_xsense.rwerrcnt2<<16)|
				(xp->exb_xsense.rwerrcnt1<<8)|
				(xp->exb_xsense.rwerrcnt1)) );
		printf("PF   0x%x BPE  0x%x FPE 0x%x ME   0x%x ECO 0x%x TME 0x%x TNP 0x%x BOT 0x%x\n",
		       xp->exb_xsense.pf, xp->exb_xsense.bpe,
		       xp->exb_xsense.fpe, xp->exb_xsense.me,
		       xp->exb_xsense.eco, xp->exb_xsense.tme,
		       xp->exb_xsense.tnp, xp->exb_xsense.bot);
		printf("XFR  0x%x TMD  0x%x WP  0x%x FMKE 0x%x URE 0x%x WE1 0x%x SSE 0x%x FE  0x%x\n",
		       xp->exb_xsense.xfr, xp->exb_xsense.tmd,
		       xp->exb_xsense.wp, xp->exb_xsense.fmke,
		       xp->exb_xsense.ure, xp->exb_xsense.we1,
		       xp->exb_xsense.sse, xp->exb_xsense.fe);
		printf("WSEB 0x%x WSEO 0x%x\n",
		       xp->exb_xsense.wseb, xp->exb_xsense.wseo);
		printf("Remaining Tape 0x%lx\n", 
		       (u_long)((xp->exb_xsense.tplft2<<16)|
				(xp->exb_xsense.tplft1<<8)|
				(xp->exb_xsense.tplft0)) );
		break;

	case MT_ISWANGTEK:
	case MT_ISCALIPER:
		printf("b8 0x%x (wp 0x%x) b9 0x%x b10 0x%x b11 0x%x b12 0x%x b13 0x%x\n",
		       xp->cpr_xsense.b8, xp->cpr_xsense.b8 & CPR_WP,
		       xp->cpr_xsense.b9, xp->cpr_xsense.b10,
		       xp->cpr_xsense.b11, xp->cpr_xsense.b12,
		       xp->cpr_xsense.b13);
		break;

	default:
		break;
	}
}

tzprtmodsel(msd, modlen)
	struct mode_select_data *msd;
	int modlen;
{
	printf("Modsel command. len is 0x%x.\n", modlen);
	printf("rsvd1 0x%x rsvd2 0x%x rsvd3 0x%x bufferd 0x%x speed 0x%x bckdeslen 0x%x\n",
	       msd->rsvd1,msd->rsvd2,msd->rsvd3,msd->buff,msd->speed,msd->blkdeslen);
	printf("density 0x%x blks2 0x%x blks1 0x%x blks0 0x%x rsvd 0x%x blklen2 0x%x blklen1 0x%x blklen0 0x%x\n",
	       msd->density,msd->blks2,msd->blks1,msd->blks0,msd->rsvd4,msd->blklen2,msd->blklen1,msd->blklen0);
	printf("vupb 0x%x rsvd 0x%x p5 0x%x motionthres 0x%x reconthres 0x%x gapthres 0x%x \n",
	       msd->vupb,msd->rsvd5,msd->p5,msd->motionthres,msd->reconthres,msd->gapthres);
}

tzprtmodstat(mode)
	struct mode_sense *mode;
{
	printf("Mode Status\n");
	printf("sdl 0x%x medtype 0x%x wp 0x%x bfmd 0x%x speed 0x%x bdl 0x%x\n",
	       mode->md.sdl, mode->md.medtype, mode->md.wp, mode->md.bfmd, 
	       mode->md.speed, mode->md.bdl);
	printf("dencod 0x%x numblk 0x%x blklen 0x%x\n",
	       mode->md.dencod, 
	       (mode->md.numblk2<<16)|(mode->md.numblk1<<8)|(mode->md.numblk0),
	       (mode->md.blklen2<<16)|(mode->md.blklen1<<8)|(mode->md.blklen0) );
	printf("ct 0x%x nd 0x%x nbe 0x%x edb 0x%x pe 0x%x nal 0x%x p5 0x%x\n",
	       mode->ex.ct, mode->ex.nd, mode->ex.nbe, 
	       mode->ex.ebd, mode->ex.pe, mode->ex.nal, mode->ex.p5);
	printf("motionthres 0x%x reconthres 0x%x gapthres 0x%x\n",
	       mode->ex.motionthres, mode->ex.reconthres,  mode->ex.gapthres);
}
#endif /* DEBUG */

#endif
