/*
 * Copyright (c) 1993 Charles Hannum.
 * Copyright 1993 by Holger Veit (data part)
 * Copyright 1993 by Brian Moore (audio part)
 * Changes Copyright 1993 by Gary Clark II
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
 *
 *	$Id: mcd.c,v 1.1.2.7 1994/02/02 11:43:54 mycroft Exp $
 */

/*static char COPYRIGHT[] = "mcd-driver (C)1993 by H.Veit & B.Moore";*/

#include "mcd.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/cdio.h>
#include <sys/errno.h>
#include <sys/dkbad.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/disk.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <i386/isa/isavar.h>
#include <i386/isa/icu.h>
#include <i386/isa/mcdreg.h>

#ifndef MCDDEBUG
#define MCD_TRACE(fmt,a,b,c,d)
#else
#define MCD_TRACE(fmt,a,b,c,d)	{if (sc->debug) {printf("%s: st=%02x: ", sc->sc_dev.dv_xname, mcd->status); printf(fmt,a,b,c,d);}}
#endif

#define MCDPART(dev)	(((minor(dev)) & 0x07)     )
#define MCDUNIT(dev)	(((minor(dev)) & 0x78) >> 3)
#define MCDPHYS(dev)	(((minor(dev)) & 0x80) >> 7)
#define RAW_PART	3

/* flags */
#define MCDOPEN		0x0001	/* device opened */
#define MCDVALID	0x0002	/* parameters loaded */
#define MCDINIT		0x0004	/* device is init'd */
#define MCDWAIT		0x0008	/* waiting for something */
#define MCDLABEL	0x0010	/* label is read */
#define	MCDREADRAW	0x0020	/* read raw mode (2352 bytes) */
#define	MCDVOLINFO	0x0040	/* already read volinfo */
#define	MCDTOC		0x0080	/* already read toc */
#define	MCDMBXBSY	0x0100	/* local mbx is busy */

/* status */
#define	MCDAUDIOBSY	MCD_ST_AUDIOBSY		/* playing audio */
#define MCDDSKCHNG	MCD_ST_DSKCHNG		/* sensed change of disk */
#define MCDDSKIN	MCD_ST_DSKIN		/* sensed disk in drive */
#define MCDDOOROPEN	MCD_ST_DOOROPEN		/* sensed door open */

/* toc */
#define MCD_MAXTOCS	104	/* from the Linux driver */
#define MCD_LASTPLUS1	170	/* special toc entry */

struct mcd_mbx {
	short		unit;
	u_short		iobase;
	short		retry;
	short		nblk;
	int		sz;
	u_long		skip;
	struct buf	*bp;
	int		p_offset;
	short		count;
};

struct mcd_softc {
	struct	device sc_dev;
	struct	isadev sc_id;
	struct	intrhand sc_ih;
	struct	dkdevice sc_dk;

	short	config;
	short	flags;
	short	status;
	int	blksize;
	u_long	disksize;
	int	iobase;
	struct disklabel dlabel;
	int	partflags[MAXPARTITIONS];
	int	openflags;
	struct mcd_volinfo volinfo;
	struct mcd_qchninfo toc[MCD_MAXTOCS];
	short	audio_status;
	struct mcd_read2 lastpb;
	short	debug;
	struct buf head;		/* head of buf queue */
	struct mcd_mbx mbx;
};

int mcdprobe __P((struct device *, struct device *, void *));
void mcdattach __P((struct device *, struct device *, void *));

struct cfdriver mcdcd =
{ NULL, "mcd", mcdprobe, mcdattach, DV_DISK, sizeof(struct mcd_softc) };

void mcdstrategy __P((struct buf *));

struct dkdriver mcddkdriver = { mcdstrategy };

/* reader state machine */
#define MCD_S_BEGIN	0
#define MCD_S_BEGIN1	1
#define MCD_S_WAITSTAT	2
#define MCD_S_WAITMODE	3
#define MCD_S_WAITREAD	4

int mcdopen __P((dev_t));
int mcdclose __P((dev_t));
static void mcd_start __P((struct mcd_softc *));
int mcdioctl __P((dev_t, int, caddr_t, int));
static int mcd_getdisklabel __P((struct mcd_softc *));
int mcdsize __P((dev_t));
static void mcd_configure __P((struct mcd_softc *));
static int mcd_waitrdy __P((u_short, int));
static int mcd_getreply __P((struct mcd_softc *, int));
static int mcd_getstat __P((struct mcd_softc *, int));
static void mcd_setflags __P((struct mcd_softc *));
static int mcd_get __P((struct mcd_softc *, char *, int));
static int mcd_send __P((struct mcd_softc *, int, int));
static int bcd2bin __P((bcd_t));
static bcd_t bin2bcd __P((int));
static void hsg2msf __P((int, bcd_t *));
static int msf2hsg __P((bcd_t *));
static int mcd_volinfo __P((struct mcd_softc *));
int mcdintr __P((caddr_t));
static void mcd_doread __P((int, struct mcd_mbx *));
static int mcd_setmode __P((struct mcd_softc *, int));
static int mcd_toc_header __P((struct mcd_softc *, struct ioc_toc_header *));
static int mcd_read_toc __P((struct mcd_softc *));
static int mcd_toc_entry __P((struct mcd_softc *, struct ioc_read_toc_entry *));
static int mcd_stop __P((struct mcd_softc *));
static int mcd_getqchan __P((struct mcd_softc *, struct mcd_qchninfo *));
static int mcd_subchan __P((struct mcd_softc *, struct ioc_read_subchannel *));
static int mcd_playtracks __P((struct mcd_softc *, struct ioc_play_track *));
static int mcd_play __P((struct mcd_softc *, struct mcd_read2 *));
static int mcd_pause __P((struct mcd_softc *));
static int mcd_resume __P((struct mcd_softc *));

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
	struct isa_attach_args *ia = aux;
	struct mcd_softc *sc = (void *)self;
	
	sc->iobase = ia->ia_iobase;
	sc->flags = MCDINIT;

#ifdef notyet
	/* get irq/drq configuration word */
	sc->config = irqs[ia->ia_irq]; /* | drqs[ia->ia_drq];*/
	/* wire controller for interrupts and dma */
	mcd_configure(sc);
#endif

	printf(": Mitsumi CD-ROM\n");

	isa_establish(&sc->sc_id, &sc->sc_dev);
	sc->sc_ih.ih_fun = mcdintr;
	sc->sc_ih.ih_arg = sc;
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_DISK);
	sc->sc_dk.dk_driver = &mcddkdriver;
	dk_establish(&sc->sc_dk, &sc->sc_dev);
}

int
mcdopen(dev)
	dev_t dev;
{
	int unit, part, phys;
	struct mcd_softc *sc;
	
	unit = MCDUNIT(dev);
	part = MCDPART(dev);
	phys = MCDPHYS(dev);

	if (unit >= mcdcd.cd_ndevs)
		return ENXIO;
	sc = mcdcd.cd_devs[unit];
	if (!sc || !(sc->flags & MCDINIT))
		return ENXIO;
	
	/* invalidated in the meantime? mark all open part's invalid */
	if (!(sc->flags & MCDVALID) && sc->openflags)
		return EBUSY;

	if (mcd_getstat(sc, 1) < 0)
		return ENXIO;

	/* XXX get a default disklabel */
	mcd_getdisklabel(sc);

	if (mcdsize(dev) < 0) {
		printf("%s: failed to get disk size\n", sc->sc_dev.dv_xname);
		return ENXIO;
	} else
		sc->flags |= MCDVALID;

MCD_TRACE("open: partition=%d, disksize = %d, blksize=%d\n",
	part,sc->disksize,sc->blksize,0);

	if (part == RAW_PART ||
		(part < sc->dlabel.d_npartitions &&
		sc->dlabel.d_partitions[part].p_fstype != FS_UNUSED)) {
		sc->partflags[part] |= MCDOPEN;
		sc->openflags |= (1<<part);
		if (part == RAW_PART && phys != 0)
			sc->partflags[part] |= MCDREADRAW;
		return 0;
	}
	
	return ENXIO;
}

int
mcdclose(dev)
	dev_t dev;
{
	int unit, part;
	struct mcd_softc *sc;
	
	unit = MCDUNIT(dev);
	part = MCDPART(dev);
	sc = mcdcd.cd_devs[unit];
	
	mcd_getstat(sc, 1);	/* get status */

	/* close channel */
	sc->partflags[part] &= ~(MCDOPEN|MCDREADRAW);
	sc->openflags &= ~(1<<part);
	MCD_TRACE("close: partition=%d\n", part, 0, 0, 0);

	return 0;
}

void
mcdstrategy(bp)
	struct buf *bp;
{
	struct mcd_softc *sc;
	struct buf *qp;
	int s;
	
	sc = mcdcd.cd_devs[MCDUNIT(bp->b_dev)];

	/* test validity */
/*MCD_TRACE("strategy: buf=0x%lx, unit=%ld, block#=%ld bcount=%ld\n",
	bp, sc->sc_dev.dv_unit, bp->b_blkno, bp->b_bcount);*/
	if (bp->b_blkno < 0) {
		printf("mcdstrategy: unit = %d, blkno = %d, bcount = %d\n",
			sc->sc_dev.dv_unit, bp->b_blkno, bp->b_bcount);
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		goto bad;
	}

	/* if device invalidated (e.g. media change, door open), error */
	if (!(sc->flags & MCDVALID)) {
MCD_TRACE("strategy: drive not valid\n",0,0,0,0);
		bp->b_error = EIO;
		goto bad;
	}

	/* read only */
	if (!(bp->b_flags & B_READ)) {
		bp->b_error = EROFS;
		goto bad;
	}
	
	/* no data to read */
	if (bp->b_bcount == 0)
		goto done;
	
	/* for non raw access, check partition limits */
	if (MCDPART(bp->b_dev) != RAW_PART) {
		if (!(sc->flags & MCDLABEL)) {
			bp->b_error = EIO;
			goto bad;
		}
		/* adjust transfer if necessary */
		if (bounds_check_with_label(bp,&sc->dlabel,1) <= 0) {
			goto done;
		}
	}
	
	/* queue it */
	qp = &sc->head;
	s = splbio();
	disksort(qp,bp);
	splx(s);
	
	/* now check whether we can perform processing */
	mcd_start(sc);
	return;

bad:
	bp->b_flags |= B_ERROR;
done:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
	return;
}

static void
mcd_start(sc)
	struct mcd_softc *sc;
{
	struct buf *bp, *qp = &sc->head;
	struct partition *p;
	int s = splbio();
	
	if (sc->flags & MCDMBXBSY)
		return;

	if ((bp = qp->b_actf) != 0) {
		/* block found to process, dequeue */
		MCD_TRACE("mcd_start: found block bp=0x%x\n", bp, 0, 0, 0);
		qp->b_actf = bp->av_forw;
		splx(s);
	} else {
		/* nothing to do */
		splx(s);
		return;
	}

	/* changed media? */
	if (!(sc->flags	& MCDVALID)) {
		MCD_TRACE("mcd_start: drive not valid\n", 0, 0, 0, 0);
		return;
	}

	p = &sc->sc_dk.dk_label.d_partitions[MCDPART(bp->b_dev)];

	sc->flags |= MCDMBXBSY;
	sc->mbx.unit = sc->sc_dev.dv_unit;
	sc->mbx.iobase = sc->iobase;
	sc->mbx.retry = MCD_RETRIES;
	sc->mbx.bp = bp;
	sc->mbx.p_offset = p->p_offset;

	/* calling the read routine */
	mcd_doread(MCD_S_BEGIN, &sc->mbx);
}

int
mcdioctl(dev, cmd, addr, flags)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flags;
{
	struct mcd_softc *sc;
	int unit;
	
	unit = MCDUNIT(dev);
	sc = mcdcd.cd_devs[unit];

	if (!(sc->flags & MCDVALID))
		return EIO;
	MCD_TRACE("ioctl called 0x%x\n", cmd, 0, 0, 0);

	switch (cmd) {
	case DIOCSBAD:
		return EINVAL;
	case DIOCGDINFO:
	case DIOCGPART:
	case DIOCWDINFO:
	case DIOCSDINFO:
	case DIOCWLABEL:
		return ENOTTY;
	case CDIOCPLAYTRACKS:
		return mcd_playtracks(sc, (struct ioc_play_track *) addr);
	case CDIOCPLAYBLOCKS:
		return mcd_play(sc, (struct mcd_read2 *) addr);
	case CDIOCREADSUBCHANNEL:
		return mcd_subchan(sc, (struct ioc_read_subchannel *) addr);
	case CDIOREADTOCHEADER:
		return mcd_toc_header(sc, (struct ioc_toc_header *) addr);
	case CDIOREADTOCENTRYS:
		return mcd_toc_entry(sc, (struct ioc_read_toc_entry *) addr);
	case CDIOCSETPATCH:
	case CDIOCGETVOL:
	case CDIOCSETVOL:
	case CDIOCSETMONO:
	case CDIOCSETSTERIO:
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
		return ENOTTY;
	}
#ifdef DIAGNOSTIC
	panic("mcdioctl: impossible");
#endif
}

/* this could have been taken from scsi/cd.c, but it is not clear
 * whether the scsi cd driver is linked in
 */
static int
mcd_getdisklabel(sc)
	struct mcd_softc *sc;
{
	
	if (sc->flags & MCDLABEL)
		return -1;
	
	bzero(&sc->sc_dk.dk_label, sizeof(struct disklabel));
	bzero(&sc->sc_dk.dk_cpulabel, sizeof(struct cpu_disklabel));
	strncpy(sc->sc_dk.dk_label.d_typename, "Mitsumi CD ROM ", 16);
	strncpy(sc->sc_dk.dk_label.d_packname, "unknown        ", 16);
	sc->sc_dk.dk_label.d_secsize 	= sc->blksize;
	sc->sc_dk.dk_label.d_nsectors	= 100;
	sc->sc_dk.dk_label.d_ntracks	= 1;
	sc->sc_dk.dk_label.d_ncylinders	= (sc->disksize/100)+1;
	sc->sc_dk.dk_label.d_secpercyl	= 100;
	sc->sc_dk.dk_label.d_secperunit	= sc->disksize;
	sc->sc_dk.dk_label.d_rpm	= 300;
	sc->sc_dk.dk_label.d_interleave	= 1;
	sc->sc_dk.dk_label.d_flags	= D_REMOVABLE;
	sc->sc_dk.dk_label.d_npartitions= 1;
	sc->sc_dk.dk_label.d_partitions[0].p_offset = 0;
	sc->sc_dk.dk_label.d_partitions[0].p_size = sc->disksize;
	sc->sc_dk.dk_label.d_partitions[0].p_fstype = 9;
	sc->sc_dk.dk_label.d_magic	= DISKMAGIC;
	sc->sc_dk.dk_label.d_magic2	= DISKMAGIC;
	sc->sc_dk.dk_label.d_checksum	= dkcksum(&sc->sc_dk.dk_label);
	
	sc->flags |= MCDLABEL;
	return 0;
}

int
mcdsize(dev)
	dev_t dev;
{
	int size;
	int unit = MCDUNIT(dev);
	struct mcd_softc *sc = mcdcd.cd_devs[unit];

	if (mcd_volinfo(sc) >= 0) {
		sc->blksize = MCDBLK;
		size = msf2hsg(sc->volinfo.vol_msf);
		sc->disksize = size * (MCDBLK/DEV_BSIZE);
		return 0;
	}
	return -1;
}

/***************************************************************
 * lower level of driver starts here
 **************************************************************/

#ifdef notyet
static char irqs[] = {
	0x00,0x00,0x10,0x20,0x00,0x30,0x00,0x00,
	0x00,0x10,0x40,0x50,0x00,0x00,0x00,0x00
};

static char drqs[] = {
	0x00,0x01,0x00,0x03,0x00,0x05,0x06,0x07,
};
#endif

static void
mcd_configure(sc)
	struct mcd_softc *sc;
{

	outb(sc->iobase + mcd_config, sc->config);
}

int
mcdprobe(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	struct mcd_softc *sc = (void *)self;
	u_short iobase = ia->ia_iobase;
	int i, j;
	u_char st, check, junk;

	if (iobase == IOBASEUNK)
		return 0;

	/* send a reset */
	outb(iobase + MCD_FLAGS, 0);
	delay(3000);

    for (j = 3; j; j--) {	/* XXX why the loop? */

	/* get any pending garbage (old data) and throw away...*/
	for (i = 10; i; i--)
		inb(iobase + MCD_DATA);

	delay(2000);
	outb(iobase + MCD_DATA, MCD_CMDCONTINFO);
	for (i = 30000; i; i--) {
		if ((inb(iobase + MCD_FLAGS) & M_STATUS_AVAIL) == M_STATUS_AVAIL) {	/* XXX looks bogus */
			delay(600);
			st = inb(iobase + MCD_DATA);
			delay(500);
			check = inb(iobase + MCD_DATA);
			delay(500);
			junk = inb(iobase + MCD_DATA);	/* XXX what is this? */

			if (check == 'M')
				goto found;
			else
				return 0;
			break;
		}
	}
    }
	return 0;

found:
	/* XXXX check irq and drq */
	printf("mcd%d: config register says %x\n", sc->sc_dev.dv_unit,
		inb(iobase + mcd_config));

	ia->ia_iosize = 4;
	ia->ia_msize = 0;
	return 1;
}

static int
mcd_waitrdy(iobase, dly)
	u_short iobase;
	int dly;
{
	int i;

	/* wait until xfer port senses data ready */
	for (i = dly; i; i--) {
		if ((inb(iobase + mcd_xfer) & MCD_ST_BUSY) == 0)
			return 0;
		delay(1);
	}
	return -1;
}

static int
mcd_getreply(sc, dly)
	struct mcd_softc *sc;
	int dly;
{
	u_short iobase = sc->iobase;

	/* wait data to become ready */
	if (mcd_waitrdy(iobase, dly)<0) {
		printf("%s: timeout getreply\n", sc->sc_dev.dv_xname);
		return -1;
	}

	/* get the data */
	return inb(iobase + mcd_status) & 0xff;
}

static int
mcd_getstat(sc, sflg)
	struct mcd_softc *sc;
	int sflg;
{
	int i;
	u_short iobase = sc->iobase;

	/* get the status */
	if (sflg)
		outb(iobase + mcd_command, MCD_CMDGETSTAT);
	i = mcd_getreply(sc, DELAY_GETREPLY);
	if (i < 0)
		return -1;

	sc->status = i;

	mcd_setflags(sc);
	return sc->status;
}

static void
mcd_setflags(sc)
	struct mcd_softc *sc;
{

	/* check flags */
	if (sc->status & (MCDDSKCHNG|MCDDOOROPEN)) {
		MCD_TRACE("getstat: sensed DSKCHNG or DOOROPEN\n", 0, 0, 0, 0);
		sc->flags &= ~MCDVALID;
	}

	if (sc->status & MCDAUDIOBSY)
		sc->audio_status = CD_AS_PLAY_IN_PROGRESS;
	else if (sc->audio_status == CD_AS_PLAY_IN_PROGRESS)
		sc->audio_status = CD_AS_PLAY_COMPLETED;
}

static int
mcd_get(sc, buf, nmax)
	struct mcd_softc *sc;
	char *buf;
	int nmax;
{
	int i, k;
	
	for (i = 0; i < nmax; i++) {
		/* wait for data */
		if ((k = mcd_getreply(sc, DELAY_GETREPLY)) < 0) {
			printf("%s: timeout mcd_get\n", sc->sc_dev.dv_xname);
			return -1;
		}
		buf[i] = k;
	}
	return i;
}

static int
mcd_send(sc, cmd, nretries)
	struct mcd_softc *sc;
	int cmd, nretries;
{
	int i, k;
	u_short iobase = sc->iobase;
	
/*MCD_TRACE("mcd_send: command = 0x%x\n",cmd,0,0,0);*/
	for (i = nretries; i; i--) {
		outb(iobase + mcd_command, cmd);
		if ((k = mcd_getstat(sc, 0)) != -1)
			break;
	}
	if (!i) {
		printf("%s: mcd_send retry cnt exceeded\n",
			sc->sc_dev.dv_xname);
		return -1;
	}

/*MCD_TRACE("mcd_send: status = 0x%x\n",k,0,0,0);*/
	return 0;
}

static int
bcd2bin(b)
	bcd_t b;
{

	return (b >> 4) * 10 + (b & 15);
}

static bcd_t
bin2bcd(b)
	int b;
{

	return ((b / 10) << 4) | (b % 10);
}

static void
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

static int
msf2hsg(msf)
	bcd_t *msf;
{

	return (bcd2bin(M_msf(msf)) * 60 +
		bcd2bin(S_msf(msf))) * 75 +
		bcd2bin(F_msf(msf)) - 150;
}

static int
mcd_volinfo(sc)
	struct mcd_softc *sc;
{

/*MCD_TRACE("mcd_volinfo: enter\n",0,0,0,0);*/

	/* Get the status, in case the disc has been changed */
	if (mcd_getstat(sc, 1) < 0)
		return EIO;

	/* Just return if we already have it */
	if (sc->flags & MCDVOLINFO)
		return 0;

	/* send volume info command */
	if (mcd_send(sc, MCD_CMDGETVOLINFO, MCD_RETRIES) < 0)
		return -1;

	/* get data */
	if (mcd_get(sc, (char*) &sc->volinfo, sizeof(struct mcd_volinfo)) < 0) {
		printf("%s: mcd_volinfo: error read data\n",
			sc->sc_dev.dv_xname);
		return -1;
	}

	if (sc->volinfo.trk_low != 0 || sc->volinfo.trk_high != 0) {
		sc->flags |= MCDVOLINFO;	/* volinfo is OK */
		return 0;
	}

	return -1;
}

int
mcdintr(arg)
	caddr_t arg;
{
	struct mcd_softc *sc = (void *)arg;
	u_short iobase = sc->iobase;
	
	MCD_TRACE("stray interrupt xfer=0x%x\n", inb(iobase + mcd_xfer),
		0, 0, 0);

	/* just read out status and ignore the rest */
	if ((inb(iobase + mcd_xfer) & 0xff) != 0xff)
		(void) inb(iobase + mcd_status);
}

/* state machine to process read requests
 * initialize with MCD_S_BEGIN: calculate sizes, and read status
 * MCD_S_WAITSTAT: wait for status reply, set mode
 * MCD_S_WAITMODE: waits for status reply from set mode, set read command
 * MCD_S_WAITREAD: wait for read ready, read data
 */
static struct mcd_mbx *mbxsave;

static void
mcd_doread(state, mbxin)
	int state;
	struct mcd_mbx *mbxin;
{
	struct mcd_mbx *mbx = (state != MCD_S_BEGIN) ? mbxsave : mbxin;
	struct mcd_softc *sc = mcdcd.cd_devs[mbx->unit];
	u_short iobase = mbx->iobase;
	struct buf *bp = mbx->bp;

	int rm, i, k;
	struct mcd_read2 rbuf;
	int blkno;
	caddr_t	addr;

loop:
	switch (state) {
	case MCD_S_BEGIN:
		mbx = mbxsave = mbxin;

	case MCD_S_BEGIN1:
		/* get status */
		outb(iobase + mcd_command, MCD_CMDGETSTAT);
		mbx->count = RDELAY_WAITSTAT;
		timeout((timeout_t) mcd_doread, (caddr_t) MCD_S_WAITSTAT,
			hz/100);
		return;

	case MCD_S_WAITSTAT:
		untimeout((timeout_t) mcd_doread, (caddr_t) MCD_S_WAITSTAT);
		if (mbx->count-- >= 0) {
			if (inb(iobase + mcd_xfer) & MCD_ST_BUSY) {
				timeout((timeout_t) mcd_doread,
					(caddr_t) MCD_S_WAITSTAT, hz/100);
				return;
			}
			mcd_setflags(sc);
			MCD_TRACE("got WAITSTAT delay=%d\n",
				RDELAY_WAITSTAT-mbx->count, 0, 0, 0);
			/* reject, if audio active */
			if (sc->status & MCDAUDIOBSY) {
				printf("%s: audio is active\n",
					sc->sc_dev.dv_xname);
				goto readerr;
			}

			/* to check for raw/cooked mode */
			if (sc->flags & MCDREADRAW) {
				rm = MCD_MD_RAW;
				mbx->sz = MCDRBLK;
			} else {
				rm = MCD_MD_COOKED;
				mbx->sz = sc->blksize;
			}

			mbx->count = RDELAY_WAITMODE;
		
			mcd_put(iobase + mcd_command, MCD_CMDSETMODE);
			mcd_put(iobase + mcd_command, rm);
			timeout((timeout_t) mcd_doread,
				(caddr_t) MCD_S_WAITMODE, hz/100);
			return;
		} else {
			printf("%s: timeout getstatus\n", sc->sc_dev.dv_xname);
			goto readerr;
		}

	case MCD_S_WAITMODE:
		untimeout((timeout_t) mcd_doread, (caddr_t) MCD_S_WAITMODE);
		if (mbx->count-- < 0) {
			printf("%s: timeout set mode\n", sc->sc_dev.dv_xname);
			goto readerr;
		}
		if (inb(iobase + mcd_xfer) & MCD_ST_BUSY) {
			timeout((timeout_t) mcd_doread,
				(caddr_t) MCD_S_WAITMODE, hz/100);
			return;
		}
		mcd_setflags(sc);
		MCD_TRACE("got WAITMODE delay=%d\n",
			RDELAY_WAITMODE-mbx->count, 0, 0, 0);
		/* for first block */
		mbx->nblk = (bp->b_bcount + (mbx->sz-1)) / mbx->sz;
		mbx->skip = 0;

nextblock:
		blkno = (bp->b_blkno / (mbx->sz/DEV_BSIZE))
			+ mbx->p_offset + mbx->skip/mbx->sz;

		MCD_TRACE("mcd_doread: read blkno=%d for bp=0x%x\n",
			blkno, bp, 0, 0);

		/* build parameter block */
		hsg2msf(blkno, rbuf.start_msf);

		/* send the read command */
		mcd_put(iobase + mcd_command,MCD_CMDREAD2);
		mcd_put(iobase + mcd_command,rbuf.start_msf[0]);
		mcd_put(iobase + mcd_command,rbuf.start_msf[1]);
		mcd_put(iobase + mcd_command,rbuf.start_msf[2]);
		mcd_put(iobase + mcd_command,0);
		mcd_put(iobase + mcd_command,0);
		mcd_put(iobase + mcd_command,1);
		mbx->count = RDELAY_WAITREAD;
		timeout((timeout_t) mcd_doread, (caddr_t) MCD_S_WAITREAD,
			hz/100);
		return;

	case MCD_S_WAITREAD:
		untimeout((timeout_t) mcd_doread, (caddr_t) MCD_S_WAITREAD);
		if (mbx->count-- > 0) {
			k = inb(iobase + mcd_xfer);
			if ((k & 2)==0) {
				MCD_TRACE("got data delay=%d\n",
					RDELAY_WAITREAD-mbx->count, 0, 0, 0);
				/* data is ready */
				addr = bp->b_un.b_addr + mbx->skip;
				outb(iobase + mcd_ctl2,0x04);	/* XXX */
				for (i=0; i<mbx->sz; i++)
					*addr++	= inb(iobase + mcd_rdata);
				outb(iobase + mcd_ctl2,0x0c);	/* XXX */

				if (--mbx->nblk > 0) {
					mbx->skip += mbx->sz;
					goto nextblock;
				}

				/* return buffer */
				bp->b_resid = 0;
				biodone(bp);

				sc->flags &= ~MCDMBXBSY;
				mcd_start(sc);
				return;
			}
			if ((k & 4) == 0)
				mcd_getstat(sc, 0);
			timeout((timeout_t) mcd_doread,
				(caddr_t) MCD_S_WAITREAD, hz/100);
			return;
		} else {
			printf("%s: timeout read data\n", sc->sc_dev.dv_xname);
			goto readerr;
		}
	}

readerr:
	if (mbx->retry-- > 0) {
		printf("%s: retrying\n", sc->sc_dev.dv_xname);
		state = MCD_S_BEGIN1;
		goto loop;
	}

	/* invalidate the buffer */
	bp->b_flags |= B_ERROR;
	bp->b_resid = bp->b_bcount;
	biodone(bp);
	mcd_start(sc);

#ifdef notdef
	printf("%s: unit timeout, resetting\n", sc->sc_dev.dv_xname);
	outb(mbx->iobase + mcd_reset,MCD_CMDRESET);
	delay(300000);
	(void)mcd_getstat(sc, 1);
	(void)mcd_getstat(sc, 1);
	/*sc->status &= ~MCDDSKCHNG; */
	sc->debug = 1; /* preventive set debug mode */
#endif
}

static int
mcd_setmode(sc, mode)
	struct mcd_softc *sc;
	int mode;
{
	u_short iobase = sc->iobase;
	int retry;

	printf("%s: setting mode to %d\n", sc->sc_dev.dv_xname, mode);
	for(retry = MCD_RETRIES; retry; retry--) {
		outb(iobase + mcd_command, MCD_CMDSETMODE);
		outb(iobase + mcd_command, mode);
		if (mcd_getstat(sc, 0) != -1)
			return 0;
	}

	return -1;
}

static int
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

static int
mcd_read_toc(sc)
	struct mcd_softc *sc;
{
	struct ioc_toc_header th;
	struct mcd_qchninfo q;
	int rc, trk, idx, retry;

	/* Only read TOC if needed */
	if (sc->flags & MCDTOC)
		return 0;

	printf("%s: reading toc header\n", sc->sc_dev.dv_xname);
	if (mcd_toc_header(sc, &th) != 0)
		return ENXIO;

	printf("%s: stopping play\n", sc->sc_dev.dv_xname);
	if ((rc = mcd_stop(sc)) != 0)
		return rc;

	/* try setting the mode twice */
	if (mcd_setmode(sc, MCD_MD_TOC) != 0)
		return EIO;
	if (mcd_setmode(sc, MCD_MD_TOC) != 0)
		return EIO;

	printf("%s: get_toc reading qchannel info\n", sc->sc_dev.dv_xname);	
	for(trk = th.starting_track; trk <= th.ending_track; trk++)
		sc->toc[trk].idx_no = 0;
	trk = th.ending_track - th.starting_track + 1;
	for(retry = 300; retry && trk > 0; retry--) {
		if (mcd_getqchan(sc, &q) < 0)
			break;
		idx = bcd2bin(q.idx_no);
		if (idx > 0 && idx < MCD_MAXTOCS && q.trk_no==0)
			if (sc->toc[idx].idx_no == 0) {
				sc->toc[idx] = q;
				trk--;
			}
	}

	if (mcd_setmode(sc, MCD_MD_COOKED) != 0)
		return EIO;

	if (trk != 0)
		return ENXIO;

	/* add a fake last+1 */
	idx = th.ending_track + 1;
	sc->toc[idx].ctrl_adr = sc->toc[idx-1].ctrl_adr;
	sc->toc[idx].trk_no = 0;
	sc->toc[idx].idx_no = 0xAA;
	sc->toc[idx].hd_pos_msf[0] = sc->volinfo.vol_msf[0];
	sc->toc[idx].hd_pos_msf[1] = sc->volinfo.vol_msf[1];
	sc->toc[idx].hd_pos_msf[2] = sc->volinfo.vol_msf[2];

	sc->flags |= MCDTOC;

	return 0;
}

static int
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

	/* Make sure we have a valid toc */
	if ((rc = mcd_read_toc(sc)) != 0)
		return rc;

	/* find the toc to copy*/
	i = te->starting_track;
	if (i == MCD_LASTPLUS1)
		i = bcd2bin(sc->volinfo.trk_high) + 1;
	
	/* verify starting track */
	if (i < bcd2bin(sc->volinfo.trk_low) ||
	    i > bcd2bin(sc->volinfo.trk_high) + 1)
		return EINVAL;

	/* do we have room */
	if (te->data_len < sizeof(struct ioc_toc_header) +
	    sizeof(struct cd_toc_entry))
		return EINVAL;

	/* Copy the toc header */
	if (mcd_toc_header(sc, &th) < 0)
		return EIO;
	ret_toc.th = th;

	/* copy the toc data */
	ret_toc.rt.control = sc->toc[i].ctrl_adr;
	ret_toc.rt.addr_type = te->address_format;
	ret_toc.rt.track = i;
	if (te->address_format == CD_MSF_FORMAT) {
		ret_toc.rt.addr[1] = sc->toc[i].hd_pos_msf[0];
		ret_toc.rt.addr[2] = sc->toc[i].hd_pos_msf[1];
		ret_toc.rt.addr[3] = sc->toc[i].hd_pos_msf[2];
	}

	/* copy the data back */
	copyout(&ret_toc, te->data, sizeof(struct cd_toc_entry)
		+ sizeof(struct ioc_toc_header));

	return 0;
}

static int
mcd_stop(sc)
	struct mcd_softc *sc;
{

	if (mcd_send(sc, MCD_CMDSTOPAUDIO, MCD_RETRIES) < 0)
		return ENXIO;
	sc->audio_status = CD_AS_PLAY_COMPLETED;
	return 0;
}

static int
mcd_getqchan(sc, q)
	struct mcd_softc *sc;
	struct mcd_qchninfo *q;
{

	if (mcd_send(sc, MCD_CMDGETQCHN, MCD_RETRIES) < 0)
		return -1;
	if (mcd_get(sc, (char *) q, sizeof(struct mcd_qchninfo)) < 0)
		return -1;
	if (sc->debug)
	printf("%s: qchannel ctl=%d, t=%d, i=%d, ttm=%d:%d.%d dtm=%d:%d.%d\n",
		sc->sc_dev.dv_xname, q->ctrl_adr, q->trk_no, q->idx_no,
		q->trk_size_msf[0], q->trk_size_msf[1], q->trk_size_msf[2],
		q->trk_size_msf[0], q->trk_size_msf[1], q->trk_size_msf[2]);
	return 0;
}

static int
mcd_subchan(sc, ch)
	struct mcd_softc *sc;
	struct ioc_read_subchannel *ch;
{
	struct mcd_qchninfo q;
	struct cd_sub_channel_info data;

	printf("%s: subchan af=%d, df=%d\n", sc->sc_dev.dv_xname,
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

static int
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

	printf("%s: playtracks from %d:%d to %d:%d\n", sc->sc_dev.dv_xname,
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

static int
mcd_play(sc, pb)
	struct mcd_softc *sc;
	struct mcd_read2 *pb;
{
	u_short iobase = sc->iobase;
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
	if (!retry)
		return ENXIO;

	sc->audio_status = CD_AS_PLAY_IN_PROGRESS;
	return 0;
}

static int
mcd_pause(sc)
	struct mcd_softc *sc;
{
	struct mcd_qchninfo q;
	int rc;

	/* Verify current status */
	if (sc->audio_status != CD_AS_PLAY_IN_PROGRESS) {
		printf("%s: pause attempted when not playing\n",
			sc->sc_dev.dv_xname);
		return EINVAL;
	}

	/* Get the current position */
	if (mcd_getqchan(sc, &q) < 0)
		return EIO;

	/* Copy it into lastpb */
	sc->lastpb.start_msf[0] = q.hd_pos_msf[0];
	sc->lastpb.start_msf[1] = q.hd_pos_msf[1];
	sc->lastpb.start_msf[2] = q.hd_pos_msf[2];

	/* Stop playing */
	if ((rc = mcd_stop(sc)) != 0)
		return rc;

	/* Set the proper status and exit */
	sc->audio_status = CD_AS_PLAY_PAUSED;
	return 0;
}

static int
mcd_resume(sc)
	struct mcd_softc *sc;
{

	if (sc->audio_status != CD_AS_PLAY_PAUSED)
		return EINVAL;
	return mcd_play(sc, &sc->lastpb);
}
