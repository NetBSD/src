/*	$NetBSD: conf.c,v 1.8 1995/04/10 03:36:17 mycroft Exp $	*/

/*-
 * Copyright (c) 1982, 1986 The Regents of the University of California.
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
 *	@(#)conf.c	7.18 (Berkeley) 5/9/91
 */

/* XXXX: Make sure tmscpreset() and udareset() are called! */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vnode.h>

int	rawread		__P((dev_t, struct uio *, int));
int	rawwrite	__P((dev_t, struct uio *, int));
void	swstrategy	__P((struct buf *));
int	ttselect	__P((dev_t, int, struct proc *));

#ifndef LKM
#define lkmenodev       enodev
#else
int     lkmenodev();
#endif

#include "hp.h"
bdev_decl(hp);
#include "rk.h"
bdev_decl(rk);
#include "te.h"
bdev_decl(tm);
#include "tmscp.h"
bdev_decl(tmscp);
#include "ts.h"
bdev_decl(ts);
#include "mu.h"
bdev_decl(mt);
#include "uda.h"
bdev_decl(uda);

/*#include "kra.h" */
#if 0
int	kdbopen(),kdbstrategy(),kdbdump(),kdbsize();
#else
#define	kdbopen		enxio
#define	kdbstrategy	enxio
#define	kdbdump		enxio
#define	kdbsize		0
#endif

#include "up.h"
bdev_decl(up);
#include "tj.h"
bdev_decl(ut);
#include "rb.h"
bdev_decl(idc);

#include "rx.h"
#if NFX > 0
int	rxopen(),rxstrategy(),rxclose(),rxread(),rxwrite(),rxreset(),rxioctl();
#else
#define	rxopen		enxio
#define rxstrategy	enxio
#define	rxclose		enxio
#define	rxread		enxio
#define	rxwrite		enxio
#define	rxreset		nullop
#define	rxioctl		enxio
#endif

#include "uu.h"
#if NUU > 0
int	uuopen(),uustrategy(),uuclose(),uureset(),uuioctl();
#else
#define	uuopen		enxio
#define uustrategy	enxio
#define	uuclose		enxio
#define	uureset		nullop
#define	uuioctl		enxio
#endif

#include "rl.h"
#if NRL > 0
int	rlopen(),rlstrategy(),rlreset(),rldump(),rlsize();
#else
#define	rlopen		enxio
#define	rlstrategy	enxio
#define	rlreset		nullop
#define	rldump		enxio
#define	rlsize		0
#endif

#include "np.h"
#if NNP > 0
int	npopen(),npclose(),npread(),npwrite();
int	npreset(),npioctl();
#else
#define	npopen		enxio
#define	npclose		enxio
#define	npread		enxio
#define	npwrite		enxio
#define	npreset		nullop
#define	npioctl		enxio
#endif

struct bdevsw	bdevsw[] =
{
	bdev_disk_init(NHP,hp),		/* 0: ??? */
	bdev_notdef(),			/* 1 */
	bdev_disk_init(NUP,up),		/* 2: SC-21/SC-31 */
	bdev_disk_init(NRK,rk),		/* 3: RK06/07 */
	bdev_swap_init(),		/* 4: swap pseudo-device */
	bdev_tape_init(NTE,tm),		/* 5: TM11/TE10 */
	bdev_tape_init(NTS,ts),		/* 6: TS11 */
	bdev_tape_init(NMU,mt),		/* 7: TU78 */
	bdev_tape_init(NTU,tu),		/* 8: TU58 */
	bdev_disk_init(NUDA,uda),	/* 9: ??? */
	bdev_tape_init(NTJ,ut),		/* 10: TU45 */
	bdec_disk_init(NRB,idc),	/* 11: IDC (RB730) */
	BLOCKDEV(rxopen,rxclose,rxstrategy,enodev,enodev,0,0),		/* 12 */
	BLOCKDEV(uuopen,uuclose,uustrategy,enodev,enodev,0,0),		/* 13 */
	BLOCKDEV(rlopen,nullop,rlstrategy,enodev,rldump,rlsize,0),	/* 14 */
	bdev_tape_init(NTMSCP,tmscp),
	BLOCKDEV(kdbopen,nullop,kdbstrategy,enodev,kdbdump,kdbsize,0),	/* 16 */
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

/*
 * Console routines for VAX console. There are always an generic console,
 * but maybe we should care about RD etc?
 */
#include <dev/cons.h>

int	gencnprobe(), gencninit(), gencngetc(), gencnputc();
int	gencnopen(), gencnclose(), gencnwrite(), gencnread(), gencnioctl();

extern 	struct tty *gencntty[];

struct	consdev	constab[]={

/* Generic console, should always be present */
	{ gencnprobe, gencninit, gencngetc, gencnputc },

#ifdef notyet
/* We may not always use builtin console, sometimes RD */
	{ rdcnprobe, rdcninit, rdcngetc, rdcnputc },
#endif

	{ 0 }
};

cdev_decl(cn);
cdev_decl(ctty);
#define	mmread	mmrw
#define	mmwrite	mmrw
cdev_decl(mm);
#include "pty.h"
#define	pts_tty		pt_tty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define	ptc_tty		pt_tty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);
cdev_decl(log);
#ifdef LKM
#define	NLKM	1
#else
#define	NLKM	0
#endif
cdev_decl(lkm);

cdev_decl(hp);
cdev_decl(rk);
cdev_decl(tm);
cdev_decl(tmscp);
cdev_decl(ts);
cdev_decl(mt);
cdev_decl(uda);
cdev_decl(up);
cdev_decl(ut);
cdev_decl(idc);

#include "acc.h"
#if NACC > 0
int     accreset();
#else
#define accreset nullop
#endif

#include "ct.h"
#if NCT > 0
int	ctopen(),ctclose(),ctwrite();
#else
#define	ctopen	nullop
#define	ctclose	nullop
#define	ctwrite	nullop
#endif

#include "dh.h"
cdev_decl(dh);
#include "dmf.h"
cdev_decl(dmf);

#if VAX8600
int	crlopen(),crlclose(),crlrw();
#else
#define	crlopen		enxio
#define	crlclose	enxio
#define	crlrw		enxio
#endif

#if VAX8200
int	rx50open(),rx50close(),rx50rw();
#else
#define	rx50open	enxio
#define	rx50close	enxio
#define	rx50rw		enxio
#endif

#if VAX780
int	flopen(),flclose(),flrw();
#else
#define	flopen	enxio
#define	flclose	enxio
#define	flrw	enxio
#endif

#include "dz.h"
cdev_decl(dz);

#include "lp.h"
#if NLP > 0
int	lpopen(),lpclose(),lpwrite(),lpreset();
#else
#define	lpopen		enxio
#define	lpclose		enxio
#define	lpwrite		enxio
#define	lpreset		nullop
#endif

int	cttyopen(),cttyread(),cttywrite(),cttyioctl(),cttyselect();

#include "va.h"
#if NVA > 0
int	vaopen(),vaclose(),vawrite(),vaioctl(),vareset(),vaselect();
#else
#define	vaopen		enxio
#define	vaclose		enxio
#define	vawrite		enxio
#define	vaopen		enxio
#define	vaioctl		enxio
#define	vareset		nullop
#define	vaselect	enxio
#endif

#include "vp.h"
#if NVP > 0
int	vpopen(),vpclose(),vpwrite(),vpioctl(),vpreset(),vpselect();
#else
#define	vpopen		enxio
#define	vpclose		enxio
#define	vpwrite		enxio
#define	vpioctl		enxio
#define	vpreset		nullop
#define	vpselect	enxio
#endif

#include "lpa.h"
#if NLPA > 0
int	lpaopen(),lpaclose(),lparead(),lpawrite(),lpaioctl();
#else
#define	lpaopen		enxio
#define	lpaclose	enxio
#define	lparead		enxio
#define	lpawrite	enxio
#define	lpaioctl	enxio
#endif

#include "dn.h"
#if NDN > 0
int	dnopen(),dnclose(),dnwrite();
#else
#define	dnopen		enxio
#define	dnclose		enxio
#define	dnwrite		enxio
#endif

#include "ik.h"
#if NIK > 0
int	ikopen(),ikclose(),ikread(),ikwrite(),ikioctl(),ikreset();
#else
#define ikopen enxio
#define ikclose enxio
#define ikread enxio
#define ikwrite enxio
#define ikioctl enxio
#define ikreset nullop
#endif

#include "ps.h"
#if NPS > 0
int	psopen(),psclose(),psread(),pswrite(),psioctl(),psreset();
#else
#define psopen enxio
#define psclose enxio
#define psread enxio
#define pswrite enxio
#define psopen enxio
#define psioctl enxio
#define psreset nullop
#endif

#include "ad.h"
#if NAD > 0
int	adopen(),adclose(),adioctl(),adreset();
#else
#define adopen enxio
#define adclose enxio
#define adioctl enxio
#define adreset nullop
#endif

#include "dhu.h"
cdev_decl(dhu);

#include "vs.h"
#if NVS > 0
int	vsopen(),vsclose(),vsioctl(),vsreset(),vsselect();
#else
#define vsopen enxio
#define vsclose enxio
#define vsioctl enxio
#define vsreset enxio
#define vsselect enxio
#endif

#include "dmz.h"
cdev_decl(dmz);

#include "qv.h"
#if NQV > 0
int	qvopen(), qvclose(), qvread(), qvwrite(), qvioctl(), qvstop(),
	qvreset(), qvselect(), qvcons_init();
#else
#define qvopen	enxio
#define qvclose	enxio
#define qvread	enxio
#define qvwrite	enxio
#define qvioctl	enxio
#define qvstop	enxio
#define qvreset	nullop
#define qvselect	enxio
#define qvcons_init	enxio
#endif

#include "qd.h"
#if NQD > 0
int	qdopen(), qdclose(), qdread(), qdwrite(), qdioctl(), qdstop(),
	qdreset(), qdselect(), qdcons_init();
#else
#define qdopen	enxio
#define qdclose	enxio
#define qdread	enxio
#define qdwrite	enxio
#define qdioctl	enxio
#define qdstop	enxio
#define qdreset	nullop
#define qdselect	enxio
#define qdcons_init	enxio
#endif

#if defined(INGRES)
int	iiioctl(), iiclose(), iiopen();
#else
#define iiopen enxio
#define iiclose enxio
#define iiioctl enxio
#endif

#ifdef	DATAKIT
#include "datakit.h"
#include "dktty.h"
#include "kmc.h"
#endif

#if !defined(NDATAKIT) || NDATAKIT == 0
#define	dkopen	enxio
#define	dkclose	enxio
#define	dkread	enxio
#define	dkwrite	enxio
#define	dkioctl	enxio
#else
int	dkopen(),dkclose(),dkread(),dkwrite(),dkioctl();
#endif

#if !defined(NDKTTY) || NDKTTY == 0
#define	dktopen		enxio
#define	dktclose	enxio
#define	dktread		enxio
#define	dktwrite	enxio
#define	dktioctl	enxio
#define	dktstop		nullop
#define	dkt		0
#else
int	dktopen(),dktclose(),dktread(),dktwrite(),dktioctl(), dktstop();
struct tty dkt[];
#endif

#if NKMC > 0
int kmcopen(), kmcclose(), kmcwrite(), kmcioctl(), kmcread();
int kmcrint(), kmcload(), kmcset(), kmcdclr();
#else
#define kmcopen enxio
#define kmcclose enxio
#define kmcwrite enxio
#define kmcioctl enxio
#define kmcread enxio
#define kmcdclr enxio
#endif

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_tty_init(NDZ,dz),		/* 1: DZ11 */
	cdev_ctty_init(1,ctty),		/* 2: controlling terminal */
	cdev_mm_init(1,mm),		/* 3: /dev/{null,mem,kmem,...} */
	cdev_disk_init(NHP,hp),		/* 4: ??? */
	cdev_notdef(),			/* 5 */
	CHARDEV(vpopen,vpclose,enodev,vpwrite,vpioctl,nullop,
		vpreset,NULL,vpselect,enodev,NULL),			/* 6 */
	cdev_swap_init(1,sw),		/* 7 */
	CHARDEV(flopen,flclose,flrw,flrw,enodev,enodev,nullop,
		NULL,seltrue,enodev,NULL),				/* 8 */
	cdev_disk_init(NUDA,uda),	/* 9: ??? */
	CHARDEV(vaopen,vaclose,enodev,vawrite,vaioctl,nullop,
		vareset,NULL,vaselect,enodev,NULL),			/* 10 */
	cdev_disk_init(NRK,rk),		/* 11: RK06/07 */
	cdev_tty_init(NDH,dh),		/* 12: DH-11/DM-11 */
	cdev_disk_init(NUP,up),		/* 13: SC-21/SC-31 */
	cdev_tape_init(NTE,tm),		/* 14: TM11/TE10 */
	CHARDEV(lpopen,lpclose,enodev,lpwrite,enodev,enodev,lpreset,
		NULL,seltrue,enodev,NULL),				/* 15 */
	cdev_tape_init(NTS,ts),		/* 16: TS11 */
	cdev_tape_init(NTJ,ut),		/* 17: TU45 */
 	CHARDEV(ctopen,ctclose,enodev,ctwrite, enodev, enodev, nullop, NULL,
		seltrue,enodev, NULL),				/* ct 	18 */
	cdev_tape_init(NMU,mt),		/* 19: TU78 */
	cdev_tty_init(NPTY,pts),	/* 20: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 21: pseudo-tty master */
	cdev_tty_init(NDMF,dmf),	/* 22: DMF32 */
	cdev_disk_init(NRB,idc),	/* 23: IDC (RB730) */
	CHARDEV(dnopen,dnclose,enodev,dnwrite,enodev,enodev,
		 nullop, NULL,seltrue,enodev,NULL),		/* 24 */
	CHARDEV(gencnopen,gencnclose,gencnread,gencnwrite,gencnioctl,
	     nullop,nullop,gencntty,ttselect,enodev,NULL),	/* cons 25 */
	CHARDEV(lpaopen,lpaclose,lparead,lpawrite,lpaioctl,
	       enodev,nullop,NULL,seltrue,enodev,NULL),		/* 26 */
	CHARDEV(psopen,psclose,	psread,	pswrite, psioctl,
		enodev,psreset,NULL,seltrue,enodev,NULL),	/* 27 */
	cdev_lkm_init(NLKM,lkm),	/* 28: loadable module driver */
	CHARDEV(adopen,	adclose,enodev,	enodev,	/*29*/ adioctl,	enodev,
		adreset,NULL, seltrue,	enodev,	NULL),
	CHARDEV(rxopen,	rxclose,rxread,	rxwrite,/*30*/ rxioctl,	enodev,
		rxreset,NULL, seltrue,	enodev,	NULL),
	CHARDEV(ikopen,	ikclose,ikread,	ikwrite,/*31*/ ikioctl,	enodev,
		ikreset,NULL, seltrue,	enodev,	NULL),
	CHARDEV(rlopen,	enodev,	rawread,rawwrite,/*32*/ enodev,	enodev,
		rlreset,NULL, seltrue,	enodev,	rlstrategy),
	cdev_log_init(1,log),		/* 33: /dev/klog */
	cdev_tty_init(NDHU,dhu),	/* 34: DHU-11 */
 	CHARDEV(crlopen,crlclose,	crlrw,		crlrw,		/*35*/
 	enodev,		enodev,		nullop,	NULL,
 	seltrue,	enodev,		NULL),
	CHARDEV(vsopen,	vsclose,	enodev,		enodev,		/*36*/
	vsioctl,	enodev,		vsreset,	NULL,
	vsselect,	enodev,		NULL),
	cdev_tty_init(NDMZ,dmz),	/* 37: DMZ32 */
	cdev_tape_init(NTMSCP,tmscp),
	CHARDEV(npopen,	npclose,	npread,		npwrite,	/*39*/
	npioctl,	enodev,		npreset,	NULL,
	seltrue,	enodev,		NULL),
	CHARDEV(qvopen,	qvclose,	qvread,		qvwrite,	/*40*/
	qvioctl,	qvstop,		qvreset,	NULL,
	qvselect,	enodev,		NULL),
	CHARDEV(qdopen,	qdclose,	qdread,		qdwrite,	/*41*/
	qdioctl,	qdstop,		qdreset,	NULL,
	qdselect,	enodev,		NULL),
	cdev_notdef(),
	CHARDEV(iiopen,	iiclose,	nullop,	nullop,	/*43*/
	iiioctl,	nullop,	nullop,	NULL,
	seltrue,	enodev,		NULL),
	CHARDEV(dkopen, dkclose,	dkread, 	dkwrite,	/*44*/
	dkioctl,	nullop,	nullop,	NULL,
	seltrue,	enodev,		NULL),
	CHARDEV(dktopen,dktclose,	dktread, 	dktwrite,	/*45*/
	dktioctl,	dktstop,	nullop,	dkt,
	ttselect,	enodev,		NULL),
	CHARDEV(kmcopen,kmcclose,	kmcread,	kmcwrite,	/*46*/
	kmcioctl,	nullop,	kmcdclr,	NULL,
	seltrue,	enodev,		NULL),
	cdev_notdef(),
	cdev_notdef(),
	cdev_notdef(),
	cdev_notdef(),
	CHARDEV(rx50open,rx50close,	rx50rw,		rx50rw,		/*51*/
	enodev,		enodev,		nullop,	0,
	seltrue,	enodev,		NULL),
/* kdb50 ra */
	CHARDEV(kdbopen,nullop/*XXX*/,	rawread,	rawwrite,	/*52*/
	enodev,		enodev,		nullop,	0,
	seltrue,	enodev,		kdbstrategy),
	cdev_fd_init(1,fd),		/* 53: file descriptor pseudo-device */
};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

int	mem_no = 3; 	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(4, 0);

int	chrtoblktbl[] = {
	NODEV,	/* 0 */
	NODEV,	/* 1 */
	NODEV,	/* 2 */
	NODEV,	/* 3 */
	0,    	/* 4 */
	1,    	/* 5 */
	NODEV,	/* 6 */
	NODEV,	/* 7 */
	NODEV,	/* 8 */
	9,   	/* 9 */
	NODEV,	/* 10 */
	3,    	/* 11 */
	NODEV,	/* 12 */
	2,    	/* 13 */
	5,	/* 14 */
	NODEV,	/* 15 */
	6,	/* 16 */
	10,	/* 17 */
	NODEV,	/* 18 */
	7,	/* 19 */
	NODEV,	/* 20 */
	NODEV,	/* 21 */
	NODEV,	/* 22 */
	11,	/* 23 */
	NODEV,	/* 24 */
	NODEV,	/* 25 */
	NODEV,	/* 26 */
	NODEV,	/* 27 */
	NODEV,	/* 28 */
	NODEV,	/* 29 */
	12,	/* 30 */
	NODEV,	/* 31 */
	14,	/* 32 */
	NODEV,	/* 33 */
	NODEV,	/* 34 */
	NODEV,	/* 35 */
	NODEV,	/* 36 */
	NODEV,	/* 37 */
	15,	/* 38 */
	NODEV,	/* 39 */
	NODEV,	/* 40 */
	NODEV,	/* 41 */
	NODEV,	/* 42 */
	NODEV,	/* 43 */
	NODEV,	/* 44 */
	NODEV,	/* 45 */
	NODEV,	/* 46 */
	NODEV,	/* 47 */
	NODEV,	/* 48 */
	NODEV,	/* 49 */
	NODEV,	/* 50 */
	NODEV, 	/* 51 */
	16,	/* 52 */
	NODEV,	/* 53 */
};

chrtoblk(dev)
	dev_t dev;
{
	if(major(dev)>=nchrdev) return(NODEV);
	return chrtoblktbl[major(dev)]==NODEV?NODEV:
		makedev(chrtoblktbl[major(dev)],minor(dev));
}

/*
 * Returns true if dev is /dev/mem or /dev/kmem.
 */
iskmemdev(dev)
	dev_t dev;
{

	return (major(dev) == 2 && minor(dev) < 2);
}

/*
 * Returns true if dev is /dev/zero.
 * ?? Shall I use 12 as /dev/zero?
 */
iszerodev(dev)
	dev_t dev;
{

	return (major(dev) == 2 && minor(dev) == 12);
}
