/*	$NetBSD: mcd.c,v 1.27 1995/01/29 07:37:12 cgd Exp $	*/

/*
 * Copyright (c) 1993, 1994 Charles Hannum.
 * Copyright 1993 by Holger Veit (data part)
 * Copyright 1993 by Brian Moore (audio part)
 * All rights reserved.
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
 *	This software was developed by Holger Veit and Brian Moore
 *      for use with "386BSD" and similar operating systems.
 *    "Similar operating systems" includes mainly non-profit oriented
 *    systems for research and education, including but not restricted to
 *    "NetBSD", "FreeBSD", "Mach" (by CMU).
 * 4. Neither the name of the developer(s) nor the name "386BSD"
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE DEVELOPER(S) BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*static char COPYRIGHT[] = "mcd-driver (C)1993 by H.Veit & B.Moore";*/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/cdio.h>
#include <sys/errno.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/disk.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <i386/isa/isavar.h>
#include <dev/isa/mcdreg.h>

#ifndef MCDDEBUG
#define MCD_TRACE(fmt,a,b,c,d)
#else
#define MCD_TRACE(fmt,a,b,c,d)	{if (sc->debug) {printf("%s: st=%02x: ", sc->sc_dev.dv_xname, sc->status); printf(fmt,a,b,c,d);}}
#endif

#define	MCDPART(dev)	DISKPART(dev)
#define	MCDUNIT(dev)	DISKUNIT(dev)

/* status */
#define	MCDAUDIOBSY	MCD_ST_AUDIOBSY		/* playing audio */
#define MCDDSKCHNG	MCD_ST_DSKCHNG		/* sensed change of disk */
#define MCDDSKIN	MCD_ST_DSKIN		/* sensed disk in drive */
#define MCDDOOROPEN	MCD_ST_DOOROPEN		/* sensed door open */

/* toc */
#define MCD_MAXTOCS	104	/* from the Linux driver */
#define MCD_LASTPLUS1	170	/* special toc entry */

struct mcd_mbx {
	struct mcd_softc *softc;
	short		retry;
	short		nblk;
	int		sz;
	u_long		skip;
	struct buf	*bp;
	int		p_offset;
	short		count;
	short		state;
#define MCD_S_BEGIN	0
#define MCD_S_WAITSTAT	1
#define MCD_S_WAITMODE	2
#define MCD_S_WAITREAD	3
};

struct mcd_softc {
	struct	device sc_dev;
	struct	dkdevice sc_dk;
	struct	intrhand sc_ih;

	int	iobase;
	short	config;
	short	flags;
#define MCDOPEN		0x0001	/* device opened */
#define MCDVALID	0x0002	/* parameters loaded */
#define MCDLABEL	0x0004	/* label is read */
#define	MCDVOLINFO	0x0008	/* already read volinfo */
#define	MCDTOC		0x0010	/* already read toc */
#define	MCDMBXBSY	0x0020	/* local mbx is busy */
	short	status;
	int	blksize;
	u_long	disksize;
	struct	mcd_volinfo volinfo;
	struct	mcd_qchninfo toc[MCD_MAXTOCS];
	short	audio_status;
	struct	mcd_read2 lastpb;
	short	debug;
	struct	buf buf_queue;
	struct	mcd_mbx mbx;
};

/* prototypes */
int mcdopen __P((dev_t, int, int, struct proc *));
int mcdclose __P((dev_t, int, int));
int mcd_start __P((struct mcd_softc *));
int mcdioctl __P((dev_t, u_long, caddr_t, int, struct proc *));
int mcd_getdisklabel __P((struct mcd_softc *));
int mcdsize __P((dev_t));
void mcd_configure __P((struct mcd_softc *));
int mcd_waitrdy __P((int, int));
int mcd_getreply __P((struct mcd_softc *, int));
int mcd_getstat __P((struct mcd_softc *, int));
void mcd_setflags __P((struct mcd_softc *));
int mcd_get __P((struct mcd_softc *, char *, int));
int mcd_send __P((struct mcd_softc *, int, int));
int bcd2bin __P((bcd_t));
bcd_t bin2bcd __P((int));
void hsg2msf __P((int, bcd_t *));
int msf2hsg __P((bcd_t *));
int mcd_volinfo __P((struct mcd_softc *));
int mcdintr __P((struct mcd_softc *));
int mcd_setmode __P((struct mcd_softc *, int));
void mcd_doread __P((void *));
int mcd_toc_header __P((struct mcd_softc *, struct ioc_toc_header *));
int mcd_read_toc __P((struct mcd_softc *));
int mcd_toc_entry __P((struct mcd_softc *, struct ioc_read_toc_entry *));
int mcd_stop __P((struct mcd_softc *));
int mcd_getqchan __P((struct mcd_softc *, struct mcd_qchninfo *));
int mcd_subchan __P((struct mcd_softc *, struct ioc_read_subchannel *));
int mcd_playtracks __P((struct mcd_softc *, struct ioc_play_track *));
int mcd_play __P((struct mcd_softc *, struct mcd_read2 *));
int mcd_pause __P((struct mcd_softc *));
int mcd_resume __P((struct mcd_softc *));

int mcdprobe __P((struct device *, void *, void *));
void mcdattach __P((struct device *, struct device *, void *));
void mcdstrategy __P((struct buf *));

struct cfdriver mcdcd = {
	NULL, "mcd", mcdprobe, mcdattach, DV_DISK, sizeof(struct mcd_softc)
};
struct dkdriver mcddkdriver = { mcdstrategy };

#define mcd_put(port,byte)	outb(port,byte)

#define MCD_RETRIES	5
#define MCD_RDRETRIES	8

#define MCDBLK	2048	/* for cooked mode */
#define MCDRBLK	2352	/* for raw mode */

/* several delays */
#define RDELAY_WAITSTAT	300
#define RDELAY_WAITMODE	300
#define RDELAY_WAITREAD	800

#define DELAY_STATUS	10000l		/* 10000 * 1us */
#define DELAY_GETREPLY	200000l		/* 200000 * 2us */
#define DELAY_SEEKREAD	20000l		/* 20000 * 1us */

void
mcdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mcd_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;

#ifdef notyet
	/* Wire controller for interrupts and DMA. */
	mcd_configure(sc);
#endif
	
	printf("\n");

	sc->flags = 0;
	sc->sc_dk.dk_driver = &mcddkdriver;

	sc->sc_ih.ih_fun = mcdintr;
	sc->sc_ih.ih_arg = sc;
	sc->sc_ih.ih_level = IPL_BIO;
	intr_establish(ia->ia_irq, IST_EDGE, &sc->sc_ih);
}

int
mcdopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	int unit, part;
	struct mcd_softc *sc;
	
	unit = MCDUNIT(dev);
	if (unit >= mcdcd.cd_ndevs)
		return ENXIO;
	sc = mcdcd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	part = MCDPART(dev);
	
	/* If it's been invalidated forget the label. */
	if ((sc->flags & MCDVALID) == 0) {
		sc->flags &= ~(MCDLABEL | MCDVOLINFO | MCDTOC);

		/* If any partition still open, then don't allow fresh open. */
		if (sc->sc_dk.dk_openmask != 0)
			return ENXIO;
	}

	if (mcd_getstat(sc, 1) < 0)
		return ENXIO;

	if (mcdsize(dev) < 0) {
		printf("%s: failed to get disk size\n", sc->sc_dev.dv_xname);
		return ENXIO;
	}

	sc->flags |= MCDVALID;

	/* XXX Get a default disklabel. */
	mcd_getdisklabel(sc);

	MCD_TRACE("open: partition=%d disksize=%d blksize=%d\n", part,
	    sc->disksize, sc->blksize, 0);

	if (part != RAW_PART &&
	    (part >= sc->sc_dk.dk_label.d_npartitions ||
	     sc->sc_dk.dk_label.d_partitions[part].p_fstype == FS_UNUSED))
		return ENXIO;

	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		sc->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		sc->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	sc->sc_dk.dk_openmask = sc->sc_dk.dk_copenmask | sc->sc_dk.dk_bopenmask;

	return 0;
}

int
mcdclose(dev, flag, fmt)
	dev_t dev;
	int flag, fmt;
{
	struct mcd_softc *sc = mcdcd.cd_devs[MCDUNIT(dev)];
	int part = MCDPART(dev);
	
	MCD_TRACE("close: partition=%d\n", part, 0, 0, 0);

	/* Get status. */
	mcd_getstat(sc, 1);

	switch (fmt) {
	case S_IFCHR:
		sc->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		sc->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	sc->sc_dk.dk_openmask = sc->sc_dk.dk_copenmask | sc->sc_dk.dk_bopenmask;

	return 0;
}

void
mcdstrategy(bp)
	struct buf *bp;
{
	struct mcd_softc *sc = mcdcd.cd_devs[MCDUNIT(bp->b_dev)];
	struct buf *qp;
	int s;
	
	/* Test validity. */
	MCD_TRACE("strategy: buf=0x%lx blkno=%ld bcount=%ld\n", bp,
	    bp->b_blkno, bp->b_bcount, 0);
	if (bp->b_blkno < 0) {
		printf("%s: strategy: blkno=%d bcount=%d\n",
		    sc->sc_dev.dv_xname, bp->b_blkno, bp->b_bcount);
		bp->b_error = EINVAL;
		goto bad;
	}

	/* If device invalidated (e.g. media change, door open), error. */
	if (!(sc->flags & MCDVALID)) {
		MCD_TRACE("strategy: drive not valid\n", 0, 0, 0, 0);
		bp->b_error = EIO;
		goto bad;
	}

	/* Check for read only. */
	if (!(bp->b_flags & B_READ)) {
		bp->b_error = EROFS;
		goto bad;
	}
	
	/* No data to read. */
	if (bp->b_bcount == 0)
		goto done;
	
	/* For non raw access, check partition limits. */
	if (MCDPART(bp->b_dev) != RAW_PART) {
		if (!(sc->flags & MCDLABEL)) {
			bp->b_error = EIO;
			goto bad;
		}
		/* Adjust transfer if necessary. */
		if (bounds_check_with_label(bp, &sc->sc_dk.dk_label, 0) <= 0)
			goto done;
	}
	
	/* Queue it. */
	qp = &sc->buf_queue;
	s = splbio();
	disksort(qp, bp);
	splx(s);
	
	/* Now check whether we can perform processing. */
	mcd_start(sc);
	return;

bad:
	bp->b_flags |= B_ERROR;
done:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

int
mcd_start(sc)
	struct mcd_softc *sc;
{
	struct buf *bp, *qp = &sc->buf_queue;
	int part;
	int s;
	
loop:
	s = splbio();

	if (sc->flags & MCDMBXBSY) {
		splx(s);
		return;
	}

	if ((bp = qp->b_actf) == 0) {
		/* Nothing to do; */
		splx(s);
		return;
	}

	/* Block found to process; dequeue. */
	MCD_TRACE("start: found block bp=0x%x\n", bp, 0, 0, 0);
	qp->b_actf = bp->b_actf;
	splx(s);

	/* Changed media? */
	if (!(sc->flags	& MCDVALID)) {
		MCD_TRACE("start: drive not valid\n", 0, 0, 0, 0);
		sc->flags &= ~(MCDLABEL | MCDVOLINFO | MCDTOC);
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		goto loop;
	}

	sc->flags |= MCDMBXBSY;
	sc->mbx.softc = sc;
	sc->mbx.retry = MCD_RETRIES;
	sc->mbx.bp = bp;
	part = MCDPART(bp->b_dev);
	if (part == RAW_PART)
		sc->mbx.p_offset = 0;
	else
		sc->mbx.p_offset =
		    sc->sc_dk.dk_label.d_partitions[part].p_offset;

	/* Calling the read routine. */
	sc->mbx.state = MCD_S_BEGIN;
	mcd_doread(&sc->mbx);
	/* triggers mcd_start, when successful finished. */
}

int
mcdioctl(dev, cmd, addr, flags, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flags;
	struct proc *p;
{
	struct mcd_softc *sc = mcdcd.cd_devs[MCDUNIT(dev)];
	
	if (!(sc->flags & MCDVALID))
		return EIO;

	MCD_TRACE("ioctl: cmd=0x%x\n", cmd, 0, 0, 0);

	switch (cmd) {
	case DIOCSBAD:
		return EINVAL;
	case CDIOCPLAYTRACKS:
		return mcd_playtracks(sc, (struct ioc_play_track *)addr);
	case CDIOCPLAYBLOCKS:
		return mcd_play(sc, (struct mcd_read2 *)addr);
	case CDIOCREADSUBCHANNEL:
		return mcd_subchan(sc, (struct ioc_read_subchannel *)addr);
	case CDIOREADTOCHEADER:
		return mcd_toc_header(sc, (struct ioc_toc_header *)addr);
	case CDIOREADTOCENTRYS:
		return mcd_toc_entry(sc, (struct ioc_read_toc_entry *)addr);
	case CDIOCSETPATCH:
	case CDIOCGETVOL:
	case CDIOCSETVOL:
	case CDIOCSETMONO:
	case CDIOCSETSTEREO:
	case CDIOCSETMUTE:
	case CDIOCSETLEFT:
	case CDIOCSETRIGHT:
		return EINVAL;
	case CDIOCRESUME:
		return mcd_resume(sc);
	case CDIOCPAUSE:
		return mcd_pause(sc);
	case CDIOCSTART:
		return EINVAL;
	case CDIOCSTOP:
		return mcd_stop(sc);
	case CDIOCEJECT:
		return EINVAL;
	case CDIOCSETDEBUG:
		sc->debug = 1;
		return 0;
	case CDIOCCLRDEBUG:
		sc->debug = 0;
		return 0;
	case CDIOCRESET:
		return EINVAL;
	default:
#if 0
	case DIOCGDINFO:
	case DIOCGPART:
	case DIOCWDINFO:
	case DIOCSDINFO:
	case DIOCWLABEL:
#endif
		return ENOTTY;
	}
#ifdef DIAGNOSTIC
	panic("mcdioctl: impossible");
#endif
}

/*
 * This could have been taken from scsi/cd.c, but it is not clear
 * whether the scsi cd driver is linked in.
 */
int
mcd_getdisklabel(sc)
	struct mcd_softc *sc;
{
	
	if (sc->flags & MCDLABEL)
		return 0;
	
	bzero(&sc->sc_dk.dk_label, sizeof(struct disklabel));
	bzero(&sc->sc_dk.dk_cpulabel, sizeof(struct cpu_disklabel));
	strncpy(sc->sc_dk.dk_label.d_typename, "Mitsumi CD ROM", 16);
	strncpy(sc->sc_dk.dk_label.d_packname, "unknown", 16);
	sc->sc_dk.dk_label.d_secsize 	= sc->blksize;
	sc->sc_dk.dk_label.d_nsectors	= 100;
	sc->sc_dk.dk_label.d_ntracks	= 1;
	sc->sc_dk.dk_label.d_ncylinders	= (sc->disksize /100) + 1;
	sc->sc_dk.dk_label.d_secpercyl	= 100;
	sc->sc_dk.dk_label.d_secperunit	= sc->disksize;
	sc->sc_dk.dk_label.d_rpm	= 300;
	sc->sc_dk.dk_label.d_interleave	= 1;
	sc->sc_dk.dk_label.d_flags	= D_REMOVABLE;
	sc->sc_dk.dk_label.d_npartitions= RAW_PART + 1;
	sc->sc_dk.dk_label.d_partitions[0].p_offset = 0;
	sc->sc_dk.dk_label.d_partitions[0].p_size = sc->disksize;
	sc->sc_dk.dk_label.d_partitions[0].p_fstype = 9;
	sc->sc_dk.dk_label.d_partitions[RAW_PART].p_offset = 0;
	sc->sc_dk.dk_label.d_partitions[RAW_PART].p_size = sc->disksize;
	sc->sc_dk.dk_label.d_partitions[RAW_PART].p_fstype = 9;
	
	sc->flags |= MCDLABEL;
	return 0;
}

int
mcdsize(dev)
	dev_t dev;
{
	int size;
	struct mcd_softc *sc = mcdcd.cd_devs[MCDUNIT(dev)];

	if (mcd_volinfo(sc) >= 0) {
		sc->blksize = MCDBLK;
		size = msf2hsg(sc->volinfo.vol_msf);
		sc->disksize = size * (MCDBLK / DEV_BSIZE);
		return 0;
	}
	return -1;
}

int
mcddump()
{

	/* Not implemented. */
	return EINVAL;
}

/***************************************************************
 * lower level of driver starts here
 **************************************************************/

#ifdef notyet
static char irqs[] = {
	0x00, 0x00, 0x10, 0x20, 0x00, 0x30, 0x00, 0x00,
	0x00, 0x10, 0x40, 0x50, 0x00, 0x00, 0x00, 0x00
};

static char drqs[] = {
	0x00, 0x01, 0x00, 0x03, 0x00, 0x05, 0x06, 0x07
};
#endif

void
mcd_configure(sc)
	struct mcd_softc *sc;
{

	outb(sc->iobase + mcd_config, sc->config);
}

int
mcdprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct mcd_softc *sc = match;
	struct isa_attach_args *ia = aux;
	int iobase = ia->ia_iobase;
	int i;
	int st, check, version;

#ifdef notyet
	/* Get irq/drq configuration word. */
	sc->config = irqs[ia->ia_irq];
#endif
	sc->iobase = iobase;

	/* Send a reset. */
	outb(iobase + mcd_reset, 0);
	delay(1000000);
	/* Get any pending status and throw away. */
	for (i = 10; i; i--)
		inb(iobase + mcd_status);
	delay(1000);

	/* Send get status command. */
	outb(iobase + mcd_command, MCD_CMDGETSTAT);
	st = mcd_getreply(sc, DELAY_GETREPLY);

	if (st < 0) {
#ifdef DEBUG
		printf("Mitsumi drive NOT detected\n");
#endif
		return 0;
	}

/*
 * The following code uses the 0xDC command, it returns a M from the
 * second byte and a number in the third.
 * (I hope you have the right drive for that, most drives don't do!)
 * Whole code entirely rewriten by veit@gmd.de, the changes accessed
 * the drive in an illegal way. Proper way is to use the timeout
 * driven routines mcd_getreply etc. rather than arbitrary delays.
 */

	delay(2000);
	outb(iobase + mcd_command, MCD_CMDCONTINFO);
	st = mcd_getreply(sc, DELAY_GETREPLY);

	if (st < 0) {
#ifdef DEBUG
		printf("Mitsumi drive error\n");
#endif
		return 0;
	}
	check = mcd_getreply(sc, DELAY_GETREPLY);
	if (check < 0)
		return 0;
	version = mcd_getreply(sc, DELAY_GETREPLY);
	if (version < 0)
		return 0;
	/* Flush junk. */
	(void) mcd_getreply(sc, DELAY_GETREPLY);

	/*
	 * The following is code which is not guaranteed to work for all
	 * drives, because the meaning of the expected 'M' is not clear
	 * (M_itsumi is an obvious assumption, but I don't trust that).
	 * Also, the original hack had a bogus condition that always
	 * returned true.
	 */
	if (check != 'D' && check != 'M') {
		printf("%s: unrecognized drive version %c%02x; will try to use it anyway\n",
		    sc->sc_dev.dv_xname, check, version);
	}

#ifdef DEBUG
	printf("Mitsumi drive detected\n");
#endif
	ia->ia_iosize = 4;
	ia->ia_msize = 0;
	return 1;
}

int
mcd_waitrdy(iobase, dly)
	int iobase;
	int dly;
{
	int i;

	/* Wait until xfer port senses data ready. */
	for (i = dly; i; i--) {
		if ((inb(iobase + mcd_xfer) & MCD_ST_BUSY) == 0)
			return 0;
		delay(1);
	}
	return -1;
}

int
mcd_getreply(sc, dly)
	struct mcd_softc *sc;
	int dly;
{
	int iobase = sc->iobase;

	/* Wait data to become ready. */
	if (mcd_waitrdy(iobase, dly) < 0) {
		printf("%s: timeout in getreply\n", sc->sc_dev.dv_xname);
		return -1;
	}

	/* Get the data. */
	return inb(iobase + mcd_status);
}

int
mcd_getstat(sc, sflg)
	struct mcd_softc *sc;
	int sflg;
{
	int i;
	int iobase = sc->iobase;

	/* Get the status. */
	if (sflg)
		outb(iobase + mcd_command, MCD_CMDGETSTAT);
	i = mcd_getreply(sc, DELAY_GETREPLY);
	if (i < 0) {
		printf("%s: timeout in getstat\n", sc->sc_dev.dv_xname);
		return -1;
	}

	sc->status = i;

	mcd_setflags(sc);
	return sc->status;
}

void
mcd_setflags(sc)
	struct mcd_softc *sc;
{

	/* Check flags. */
	if (sc->status & (MCDDSKCHNG | MCDDOOROPEN)) {
		MCD_TRACE("getstat: sensed DSKCHNG or DOOROPEN\n", 0, 0, 0, 0);
		sc->flags &= ~MCDVALID;
	}

	if (sc->status & MCDAUDIOBSY)
		sc->audio_status = CD_AS_PLAY_IN_PROGRESS;
	else if (sc->audio_status == CD_AS_PLAY_IN_PROGRESS)
		sc->audio_status = CD_AS_PLAY_COMPLETED;
}

int
mcd_get(sc, buf, nmax)
	struct mcd_softc *sc;
	char *buf;
	int nmax;
{
	int i, k;
	
	for (i = 0; i < nmax; i++) {
		/* Wait for data. */
		if ((k = mcd_getreply(sc, DELAY_GETREPLY)) < 0) {
			printf("%s: timeout in get\n", sc->sc_dev.dv_xname);
			return -1;
		}
		buf[i] = k;
	}
	return i;
}

int
mcd_send(sc, cmd, nretries)
	struct mcd_softc *sc;
	int cmd, nretries;
{
	int i, k;
	int iobase = sc->iobase;
	
	MCD_TRACE("send: cmd=0x%x\n", cmd, 0, 0, 0);

	for (i = nretries; i; i--) {
		outb(iobase + mcd_command, cmd);
		if ((k = mcd_getstat(sc, 0)) != -1)
			break;
	}
	if (!i) {
		printf("%s: send: retry count exceeded\n", sc->sc_dev.dv_xname);
		return -1;
	}

	MCD_TRACE("send: status=0x%x\n", k, 0, 0, 0);

	return 0;
}

int
bcd2bin(b)
	bcd_t b;
{

	return (b >> 4) * 10 + (b & 15);
}

bcd_t
bin2bcd(b)
	int b;
{

	return ((b / 10) << 4) | (b % 10);
}

void
hsg2msf(hsg, msf)
	int hsg;
	bcd_t *msf;
{

	hsg += 150;
	M_msf(msf) = bin2bcd(hsg / 4500);
	hsg %= 4500;
	S_msf(msf) = bin2bcd(hsg / 75);
	F_msf(msf) = bin2bcd(hsg % 75);
}

int
msf2hsg(msf)
	bcd_t *msf;
{

	return (bcd2bin(M_msf(msf)) * 60 +
		bcd2bin(S_msf(msf))) * 75 +
		bcd2bin(F_msf(msf)) - 150;
}

int
mcd_volinfo(sc)
	struct mcd_softc *sc;
{

	MCD_TRACE("volinfo: enter\n", 0, 0, 0, 0);

	/* Get the status, in case the disc has been changed. */
	if (mcd_getstat(sc, 1) < 0)
		return EIO;

	/* Just return if we already have it. */
	if (sc->flags & MCDVOLINFO)
		return 0;

	/* Send volume info command. */
	if (mcd_send(sc, MCD_CMDGETVOLINFO, MCD_RETRIES) < 0)
		return -1;

	/* Get the data. */
	if (mcd_get(sc, (char*) &sc->volinfo, sizeof(struct mcd_volinfo)) < 0) {
		printf("%s: volinfo: error reading data\n",
		    sc->sc_dev.dv_xname);
		return -1;
	}

	if (sc->volinfo.trk_low != 0 || sc->volinfo.trk_high != 0) {
		/* Volinfo is OK. */
		sc->flags |= MCDVOLINFO;
		return 0;
	}

	return -1;
}

int
mcdintr(sc)
	struct mcd_softc *sc;
{
	int iobase = sc->iobase;
	
	MCD_TRACE("stray interrupt xfer=0x%x\n", inb(iobase + mcd_xfer),
	    0, 0, 0);

	/* Just read out status and ignore the rest. */
	if (inb(iobase + mcd_xfer) != 0xff)
		(void) inb(iobase + mcd_status);

	return -1;
}

/*
 * State machine to process read requests.
 * Initialize with MCD_S_BEGIN: calculate sizes, and read status
 * MCD_S_WAITSTAT: wait for status reply, set mode
 * MCD_S_WAITMODE: waits for status reply from set mode, set read command
 * MCD_S_WAITREAD: wait for read ready, read data.
 */
void
mcd_doread(arg)
	void *arg;
{
	struct mcd_mbx *mbx = arg;
	struct mcd_softc *sc = mbx->softc;
	int iobase = sc->iobase;
	struct buf *bp = mbx->bp;

	int i, k;
	struct mcd_read2 rbuf;
	int blkno;
	caddr_t	addr;

loop:
	switch (mbx->state) {
	case MCD_S_BEGIN:
		/* Get status. */
		outb(iobase + mcd_command, MCD_CMDGETSTAT);

		mbx->count = RDELAY_WAITSTAT;
		mbx->state = MCD_S_WAITSTAT;
		timeout(mcd_doread, mbx, hz / 100);
		return;

	case MCD_S_WAITSTAT:
		untimeout(mcd_doread, mbx);
		if (mbx->count-- < 0) {
			printf("%s: timeout getting status\n",
			    sc->sc_dev.dv_xname);
			goto readerr;
		}
		if (inb(iobase + mcd_xfer) & MCD_ST_BUSY) {
			timeout(mcd_doread, mbx, hz / 100);
			return;
		}
		mcd_setflags(sc);
		MCD_TRACE("doread: got WAITSTAT delay=%d\n",
		    RDELAY_WAITSTAT - mbx->count, 0, 0, 0);

		/* Reject, if audio active. */
		if (sc->status & MCDAUDIOBSY) {
			printf("%s: audio is active\n",
			    sc->sc_dev.dv_xname);
			goto readerr;
		}

		mcd_put(iobase + mcd_command, MCD_CMDSETMODE);
		mcd_put(iobase + mcd_command, MCD_MD_COOKED);

		mbx->sz = sc->blksize;
		mbx->count = RDELAY_WAITMODE;
		mbx->state = MCD_S_WAITMODE;
		timeout(mcd_doread, mbx, hz / 100);
		return;

	case MCD_S_WAITMODE:
		untimeout(mcd_doread, mbx);
		if (mbx->count-- < 0) {
			printf("%s: timeout setting mode\n",
			    sc->sc_dev.dv_xname);
			goto readerr;
		}
		if (inb(iobase + mcd_xfer) & MCD_ST_BUSY) {
			timeout(mcd_doread, mbx, hz / 100);
			return;
		}
		mcd_setflags(sc);
		MCD_TRACE("doread: got WAITMODE delay=%d\n",
		    RDELAY_WAITMODE - mbx->count, 0, 0, 0);

		/* For first block. */
		mbx->nblk = (bp->b_bcount + (mbx->sz - 1)) / mbx->sz;
		mbx->skip = 0;

	nextblock:
		blkno = (bp->b_blkno / (mbx->sz / DEV_BSIZE)) + mbx->p_offset +
		    (mbx->skip / mbx->sz);

		MCD_TRACE("doread: read blkno=%d for bp=0x%x\n", blkno, bp, 0,
		    0);

		/* Build parameter block. */
		hsg2msf(blkno, rbuf.start_msf);

		/* Send the read command. */
		mcd_put(iobase + mcd_command, MCD_CMDREAD2);
		mcd_put(iobase + mcd_command, rbuf.start_msf[0]);
		mcd_put(iobase + mcd_command, rbuf.start_msf[1]);
		mcd_put(iobase + mcd_command, rbuf.start_msf[2]);
		mcd_put(iobase + mcd_command, 0);
		mcd_put(iobase + mcd_command, 0);
		mcd_put(iobase + mcd_command, 1);

		mbx->count = RDELAY_WAITREAD;
		mbx->state = MCD_S_WAITREAD;
		timeout(mcd_doread, mbx, hz / 100);
		return;

	case MCD_S_WAITREAD:
		untimeout(mcd_doread, mbx);
		if (mbx->count-- < 0) {
			printf("%s: timeout reading data\n",
			    sc->sc_dev.dv_xname);
			goto readerr;
		}

		k = inb(iobase + mcd_xfer);
		if ((k & 2) == 0) {	/* XXX MCD_ST_AUDIOBSY? */
			MCD_TRACE("doread: got data delay=%d\n",
			    RDELAY_WAITREAD - mbx->count, 0, 0, 0);
			/* Data is ready. */
			addr = bp->b_data + mbx->skip;
			outb(iobase + mcd_ctl2, 0x04);	/* XXX */
			for (i = 0; i < mbx->sz; i++)
				*addr++	= inb(iobase + mcd_rdata);
			outb(iobase + mcd_ctl2, 0x0c);	/* XXX */

			if (--mbx->nblk > 0) {
				mbx->skip += mbx->sz;
				goto nextblock;
			}

			/* Return buffer. */
			bp->b_resid = 0;
			biodone(bp);

			sc->flags &= ~MCDMBXBSY;
			mcd_start(sc);
			return;
		}
		if ((k & MCD_ST_BUSY) == 0)
			mcd_getstat(sc, 0);
		timeout(mcd_doread, mbx, hz / 100);
		return;
	}

readerr:
	if (mbx->retry-- > 0) {
		printf("%s: retrying\n", sc->sc_dev.dv_xname);
		mbx->state = MCD_S_BEGIN;
		goto loop;
	}

	/* Invalidate the buffer. */
	bp->b_flags |= B_ERROR;
	bp->b_resid = bp->b_bcount;
	biodone(bp);
	mcd_start(sc);

#ifdef notyet
	printf("%s: unit timeout; resetting\n", sc->sc_dev.dv_xname);
	outb(mbx->iobase + mcd_reset, MCD_CMDRESET);
	delay(300000);
	(void)mcd_getstat(sc, 1);
	(void)mcd_getstat(sc, 1);
	/*sc->status &= ~MCDDSKCHNG; */
	sc->debug = 1; /* preventive set debug mode */
#endif
}

int
mcd_setmode(sc, mode)
	struct mcd_softc *sc;
	int mode;
{
	int iobase = sc->iobase;
	int retry;

	printf("%s: setting mode to %d\n", sc->sc_dev.dv_xname, mode);
	for (retry = MCD_RETRIES; retry; retry--) {
		outb(iobase + mcd_command, MCD_CMDSETMODE);
		outb(iobase + mcd_command, mode);
		if (mcd_getstat(sc, 0) != -1)
			return 0;
	}

	return -1;
}

int
mcd_toc_header(sc, th)
	struct mcd_softc *sc;
	struct ioc_toc_header *th;
{

	if (mcd_volinfo(sc) < 0)
		return ENXIO;

	th->len = msf2hsg(sc->volinfo.vol_msf);
	th->starting_track = bcd2bin(sc->volinfo.trk_low);
	th->ending_track = bcd2bin(sc->volinfo.trk_high);

	return 0;
}

int
mcd_read_toc(sc)
	struct mcd_softc *sc;
{
	struct ioc_toc_header th;
	struct mcd_qchninfo q;
	int rc, trk, idx, retry;

	/* Only read TOC if needed. */
	if (sc->flags & MCDTOC)
		return 0;

	if (sc->debug)
		printf("%s: read_toc: reading toc header\n",
		    sc->sc_dev.dv_xname);
	if (mcd_toc_header(sc, &th) != 0)
		return ENXIO;

	if (sc->debug)
		printf("%s: read_toc: stopping play\n", sc->sc_dev.dv_xname);
	if ((rc = mcd_stop(sc)) != 0)
		return rc;

	/* Try setting the mode twice. */
	if (mcd_setmode(sc, MCD_MD_TOC) != 0)
		return EIO;
	if (mcd_setmode(sc, MCD_MD_TOC) != 0)
		return EIO;

	if (sc->debug)
		printf("%s: read_toc: reading qchannel info\n",
		    sc->sc_dev.dv_xname);
	for (trk = th.starting_track; trk <= th.ending_track; trk++)
		sc->toc[trk].idx_no = 0;
	trk = th.ending_track - th.starting_track + 1;
	for (retry = 300; retry && trk > 0; retry--) {
		if (mcd_getqchan(sc, &q) < 0)
			break;
		idx = bcd2bin(q.idx_no);
		if (idx > 0 && idx < MCD_MAXTOCS && q.trk_no == 0 &&
		    sc->toc[idx].idx_no == 0) {
			sc->toc[idx] = q;
			trk--;
		}
	}

	if (mcd_setmode(sc, MCD_MD_COOKED) != 0)
		return EIO;

	if (trk != 0)
		return ENXIO;

	/* Add a fake last+1. */
	idx = th.ending_track + 1;
	sc->toc[idx].ctrl_adr = sc->toc[idx-1].ctrl_adr;
	sc->toc[idx].trk_no = 0;
	sc->toc[idx].idx_no = 0xaa;
	sc->toc[idx].hd_pos_msf[0] = sc->volinfo.vol_msf[0];
	sc->toc[idx].hd_pos_msf[1] = sc->volinfo.vol_msf[1];
	sc->toc[idx].hd_pos_msf[2] = sc->volinfo.vol_msf[2];

	sc->flags |= MCDTOC;
	return 0;
}

int
mcd_toc_entry(sc, te)
	struct mcd_softc *sc;
	struct ioc_read_toc_entry *te;
{
	struct ret_toc {
		struct ioc_toc_header th;
		struct cd_toc_entry rt;
	} ret_toc;
	struct ioc_toc_header th;
	int rc, i;

	/* Make sure we have a valid TOC. */
	if ((rc = mcd_read_toc(sc)) != 0)
		return rc;

	/* Find the TOC to copy. */
	i = te->starting_track;
	if (i == MCD_LASTPLUS1)
		i = bcd2bin(sc->volinfo.trk_high) + 1;
	
	/* Verify starting track. */
	if (i < bcd2bin(sc->volinfo.trk_low) ||
	    i > bcd2bin(sc->volinfo.trk_high) + 1)
		return EINVAL;

	/* Do we have room? */
	if (te->data_len < sizeof(struct ioc_toc_header) +
	    sizeof(struct cd_toc_entry))
		return EINVAL;

	/* Copy the TOC header. */
	if (mcd_toc_header(sc, &th) < 0)
		return EIO;
	ret_toc.th = th;

	/* Copy the TOC data. */
	ret_toc.rt.control = sc->toc[i].ctrl_adr;
	ret_toc.rt.addr_type = te->address_format;
	ret_toc.rt.track = i;
	if (te->address_format == CD_MSF_FORMAT) {
		ret_toc.rt.addr[1] = sc->toc[i].hd_pos_msf[0];
		ret_toc.rt.addr[2] = sc->toc[i].hd_pos_msf[1];
		ret_toc.rt.addr[3] = sc->toc[i].hd_pos_msf[2];
	}

	/* Copy the data back. */
	copyout(&ret_toc, te->data,
	    sizeof(struct cd_toc_entry) + sizeof(struct ioc_toc_header));

	return 0;
}

int
mcd_stop(sc)
	struct mcd_softc *sc;
{

	if (mcd_send(sc, MCD_CMDSTOPAUDIO, MCD_RETRIES) < 0)
		return ENXIO;
	sc->audio_status = CD_AS_PLAY_COMPLETED;
	return 0;
}

int
mcd_getqchan(sc, q)
	struct mcd_softc *sc;
	struct mcd_qchninfo *q;
{

	if (mcd_send(sc, MCD_CMDGETQCHN, MCD_RETRIES) < 0)
		return -1;
	if (mcd_get(sc, (char *) q, sizeof(struct mcd_qchninfo)) < 0)
		return -1;
	if (sc->debug)
		printf("%s: getqchan: ctl=%d t=%d i=%d ttm=%d:%d.%d dtm=%d:%d.%d\n",
		    sc->sc_dev.dv_xname, q->ctrl_adr, q->trk_no, q->idx_no,
		    q->trk_size_msf[0], q->trk_size_msf[1], q->trk_size_msf[2],
		    q->trk_size_msf[0], q->trk_size_msf[1], q->trk_size_msf[2]);
	return 0;
}

int
mcd_subchan(sc, ch)
	struct mcd_softc *sc;
	struct ioc_read_subchannel *ch;
{
	struct mcd_qchninfo q;
	struct cd_sub_channel_info data;

	if (sc->debug)
		printf("%s: subchan: af=%d df=%d\n", sc->sc_dev.dv_xname,
		    ch->address_format, ch->data_format);

	if (ch->address_format != CD_MSF_FORMAT)
		return EIO;
	if (ch->data_format != CD_CURRENT_POSITION)
		return EIO;
	if (mcd_getqchan(sc, &q) < 0)
		return EIO;

	data.header.audio_status = sc->audio_status;
	data.what.position.data_format = CD_MSF_FORMAT;
	data.what.position.track_number = bcd2bin(q.trk_no);

	if (copyout(&data, ch->data, sizeof(struct cd_sub_channel_info)) != 0)
		return EFAULT;
	return 0;
}

int
mcd_playtracks(sc, pt)
	struct mcd_softc *sc;
	struct ioc_play_track *pt;
{
	struct mcd_read2 pb;
	int a = pt->start_track;
	int z = pt->end_track;
	int rc;

	if ((rc = mcd_read_toc(sc)) != 0)
		return rc;

	printf("%s: playtracks: from %d:%d to %d:%d\n", sc->sc_dev.dv_xname,
	    a, pt->start_index, z, pt->end_index);

	if (a < sc->volinfo.trk_low || a > sc->volinfo.trk_high || a > z ||
	    z < sc->volinfo.trk_low || z > sc->volinfo.trk_high)
		return EINVAL;

	pb.start_msf[0] = sc->toc[a].hd_pos_msf[0];
	pb.start_msf[1] = sc->toc[a].hd_pos_msf[1];
	pb.start_msf[2] = sc->toc[a].hd_pos_msf[2];
	pb.end_msf[0] = sc->toc[z+1].hd_pos_msf[0];
	pb.end_msf[1] = sc->toc[z+1].hd_pos_msf[1];
	pb.end_msf[2] = sc->toc[z+1].hd_pos_msf[2];

	return mcd_play(sc, &pb);
}

int
mcd_play(sc, pb)
	struct mcd_softc *sc;
	struct mcd_read2 *pb;
{
	int iobase = sc->iobase;
	int retry, st;

	sc->lastpb = *pb;
	for (retry = MCD_RETRIES; retry; retry--) {
		outb(iobase + mcd_command, MCD_CMDREAD2);
		outb(iobase + mcd_command, pb->start_msf[0]);
		outb(iobase + mcd_command, pb->start_msf[1]);
		outb(iobase + mcd_command, pb->start_msf[2]);
		outb(iobase + mcd_command, pb->end_msf[0]);
		outb(iobase + mcd_command, pb->end_msf[1]);
		outb(iobase + mcd_command, pb->end_msf[2]);
		if ((st = mcd_getstat(sc, 0)) != -1)
			break;
	}
	if (sc->debug)
		printf("%s: play: retry=%d status=%d\n", sc->sc_dev.dv_xname,
		    retry, st);
	if (!retry)
		return ENXIO;

	sc->audio_status = CD_AS_PLAY_IN_PROGRESS;
	return 0;
}

int
mcd_pause(sc)
	struct mcd_softc *sc;
{
	struct mcd_qchninfo q;
	int rc;

	/* Verify current status. */
	if (sc->audio_status != CD_AS_PLAY_IN_PROGRESS)	{
		printf("%s: pause: attempted when not playing\n",
		    sc->sc_dev.dv_xname);
		return EINVAL;
	}

	/* Get the current position. */
	if (mcd_getqchan(sc, &q) < 0)
		return EIO;

	/* Copy it into lastpb. */
	sc->lastpb.start_msf[0] = q.hd_pos_msf[0];
	sc->lastpb.start_msf[1] = q.hd_pos_msf[1];
	sc->lastpb.start_msf[2] = q.hd_pos_msf[2];

	/* Stop playing. */
	if ((rc = mcd_stop(sc)) != 0)
		return rc;

	/* Set the proper status and exit. */
	sc->audio_status = CD_AS_PLAY_PAUSED;
	return 0;
}

int
mcd_resume(sc)
	struct mcd_softc *sc;
{

	if (sc->audio_status != CD_AS_PLAY_PAUSED)
		return EINVAL;
	return mcd_play(sc, &sc->lastpb);
}
