/*	$NetBSD: conf.c,v 1.5 1995/02/13 00:46:05 ragge Exp $	*/

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

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/ioctl.h"
#include "sys/tty.h"
#include "sys/conf.h"
#include "sys/vnode.h"

int nullop(), enxio(), enodev(), rawread(), rawwrite(), swstrategy();
int rawread(), rawwrite(), swstrategy();

#define	BLOCKDEV(open,close,strat,ioctl,dump,size,type)	\
	{(int(*)(dev_t, int, int, struct proc *))open,  \
	 (int(*)(dev_t, int, int, struct proc *))close, \
	 (void(*)(struct buf *))strat, \
	 (int(*)(dev_t, u_long, caddr_t, int, struct proc *))ioctl, \
	 (int(*)())dump, \
	 (int(*)(dev_t))size, type}

#define	CHARDEV(open,close,read,write,ioctl,stop,reset,ttys,select,mmap,strat)\
	{(int(*)(dev_t, int, int, struct proc *))open, \
	 (int(*)(dev_t, int, int, struct proc *))close, \
	 (int(*)(dev_t, struct uio *, int))read, \
	 (int(*)(dev_t, struct uio *, int))write, \
	 (int(*)(dev_t, u_long, caddr_t, int, struct proc *))ioctl, \
	 (int(*)(struct tty *, int))stop, \
	 (int(*)(int))reset, \
	 (struct  tty **)ttys, \
	 (int(*)(dev_t, int, struct proc *))select, \
	 (int(*)())mmap, \
	 (void(*)(struct buf *))strat }









#include "hp.h"
#if NHP > 0
int	hpopen(),hpclose();
int	hpioctl(),hpdump(),hpsize();
void	hpstrategy();
#else
#define	hpopen		enxio
#define	hpclose		enxio
#define	hpstrategy	enxio
#define	hpioctl		enxio
#define	hpdump		enxio
#define	hpsize		0
#endif
 
#include "tu.h"
#if NHT > 0
int	htopen(),htclose(),htstrategy(),htdump(),htioctl();
#else
#define	htopen		enxio
#define	htclose		enxio
#define	htstrategy	enxio
#define	htdump		enxio
#define	htioctl		enxio
#endif

#include "rk.h"
#if NHK > 0
int	rkopen(),rkstrategy(),rkintr(),rkdump(),rkreset(),rksize();
#else
#define	rkopen		enxio
#define	rkstrategy	enxio
#define	rkintr		enxio
#define	rkdump		enxio
#define	rkreset		enxio
#define	rksize		0
#endif

#include "te.h"
#if NTE > 0
int	tmopen(),tmclose(),tmstrategy(),tmioctl(),tmdump(),tmreset();
#else
#define	tmopen		enxio
#define	tmclose		enxio
#define	tmstrategy	enxio
#define	tmioctl		enxio
#define	tmdump		enxio
#define	tmreset		nullop
#endif

#include "tms.h"
#if NTMS > 0
int	tmscpopen(),tmscpclose(),tmscpstrategy();
int	tmscpioctl(),tmscpdump(),tmscpreset();
#else
#define	tmscpopen	enxio
#define	tmscpclose	enxio
#define	tmscpstrategy	enxio
#define	tmscpioctl	enxio
#define	tmscpdump	enxio
#define	tmscpreset	nullop
#endif

#include "ts.h"
#if NTS > 0
int	tsopen(),tsclose(),tsstrategy(),tsioctl(),tsdump(),tsreset();
#else
#define	tsopen		enxio
#define	tsclose		enxio
#define	tsstrategy	enxio
#define	tsioctl		enxio
#define	tsdump		enxio
#define	tsreset		nullop
#endif

#include "mu.h"
#if NMT > 0
int	mtopen(),mtclose(),mtstrategy(),mtioctl(),mtdump();
#else
#define	mtopen		enxio
#define	mtclose		enxio
#define	mtstrategy	enxio
#define	mtioctl		enxio
#define	mtdump		enxio
#endif

#include "uda.h"
#if NUDA > 0
extern int	udaopen(),udaclose();
extern int	udaioctl(),udareset(),udadump(),udasize();
extern void	udastrategy();
#else
#define	udaopen		enxio
#define	udaclose	enxio
#define	udastrategy	enxio
#define	udaioctl	enxio
#define	udareset	nullop
#define	udadump		enxio
#define	udasize		0
#endif

#include "kra.h"
#if NKDB > 0
int	kdbopen(),kdbstrategy(),kdbdump(),kdbsize();
#else
#define	kdbopen		enxio
#define	kdbstrategy	enxio
#define	kdbdump		enxio
#define	kdbsize		0
#endif

#include "up.h"
#if NSC > 0
int	upopen(),upstrategy(),upreset(),updump(),upsize();
#else
#define	upopen		enxio
#define	upstrategy	enxio
#define	upreset		nullop
#define	updump		enxio
#define	upsize		0
#endif

#include "tj.h"
#if NUT > 0
int	utopen(),utclose(),utstrategy(),utioctl(),utreset(),utdump();
#else
#define	utopen		enxio
#define	utclose		enxio
#define	utstrategy	enxio
#define	utreset		nullop
#define	utioctl		enxio
#define	utdump		enxio
#endif

#include "rb.h"
#if NIDC > 0
int	idcopen(),idcstrategy(),idcreset(),idcdump(),idcsize();;
#else
#define	idcopen		enxio
#define	idcstrategy	enxio
#define	idcreset	nullop
#define	idcdump		enxio
#define	idcsize		0
#endif

#if 0 /* defined(VAX750) || defined(VAX730) */
int	tuopen(),tuclose(),tustrategy();
#else
#define	tuopen		enxio
#define	tuclose		enxio
#define	tustrategy	enxio
#endif

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
	BLOCKDEV(hpopen,hpclose,hpstrategy,hpioctl,hpdump,hpsize,0),	/* 0 */
	BLOCKDEV(htopen,htclose,htstrategy,htioctl,htdump,0,B_TAPE),	/* 1 */
	BLOCKDEV(upopen,nullop,upstrategy,enodev,updump,upsize,0),	/* 2 */
	BLOCKDEV(rkopen,nullop,rkstrategy,enodev,rkdump,rksize,0),	/* 3 */
	BLOCKDEV(enodev,enodev,swstrategy,enodev,enodev,0,0),		/* 4 */
	BLOCKDEV(tmopen,tmclose,tmstrategy,tmioctl,tmdump,0,B_TAPE),	/* 5 */
	BLOCKDEV(tsopen,tsclose,tsstrategy,tsioctl,tsdump,0,B_TAPE),	/* 6 */
	BLOCKDEV(mtopen,mtclose,mtstrategy,mtioctl,mtdump,0,B_TAPE),	/* 7 */
	BLOCKDEV(tuopen,tuclose,tustrategy,enodev,enodev,0,B_TAPE),	/* 8 */
	BLOCKDEV(udaopen,udaclose,udastrategy,udaioctl,udadump,
		udasize,0),						/* 9 */
	BLOCKDEV(utopen,utclose,utstrategy,utioctl,utdump,0,B_TAPE),	/* 10 */
	BLOCKDEV(idcopen,nullop,idcstrategy,enodev,idcdump,idcsize,0),	/* 11 */
	BLOCKDEV(rxopen,rxclose,rxstrategy,enodev,enodev,0,0),		/* 12 */
	BLOCKDEV(uuopen,uuclose,uustrategy,enodev,enodev,0,0),		/* 13 */
	BLOCKDEV(rlopen,nullop,rlstrategy,enodev,rldump,rlsize,0),	/* 14 */
	BLOCKDEV(tmscpopen,tmscpclose,tmscpstrategy,tmscpioctl,
		tmscpdump,0,B_TAPE),					/* 15 */
	BLOCKDEV(kdbopen,nullop,kdbstrategy,enodev,kdbdump,kdbsize,0),	/* 16 */
};
int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

/*
 * Console routines for VAX console. There are always an generic console,
 * but maybe we should care about RD etc?
 */
#include "dev/cons.h"

int	cnopen(),cnclose(),cnread(),cnwrite(),cnioctl(),cnselect();
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


/*
        gencnopen,      gencnclose,     gencnwrite,     gencnread,      25
        gencnioctl,     nullop,         nullop,         gencntty,
        ttselect,       enodev,         NULL,
*/



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
#if NDH == 0
#define	dhopen	enxio
#define	dhclose	enxio
#define	dhread	enxio
#define	dhwrite	enxio
#define	dhioctl	enxio
#define	dhstop	enxio
#define	dhreset	nullop
#define	dh11	0
#else
int	dhopen(),dhclose(),dhread(),dhwrite(),dhioctl(),dhstop(),dhreset();
struct	tty dh11[];
#endif

#include "dmf.h"
#if NDMF == 0
#define	dmfopen		enxio
#define	dmfclose	enxio
#define	dmfread		enxio
#define	dmfwrite	enxio
#define	dmfioctl	enxio
#define	dmfstop		enxio
#define	dmfreset	nullop
#define	dmf_tty	0
#else
int	dmfopen(),dmfclose(),dmfread(),dmfwrite(),dmfioctl(),dmfstop(),dmfreset();
struct	tty dmf_tty[];
#endif

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
#if NDZ == 0
#define	dzopen	enxio
#define	dzclose	enxio
#define	dzread	enxio
#define	dzwrite	enxio
#define	dzioctl	enxio
#define	dzstop	enxio
#define	dzreset	nullop
#define	dz_tty	0
#else
int	dzopen(),dzclose(),dzread(),dzwrite(),dzioctl(),dzstop(),dzreset();
struct	tty dz_tty[];
#endif

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

int 	mmrw();
#define	mmselect	seltrue

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

#include "pty.h"
#if NPTY > 0
int	ptsopen(),ptsclose(),ptsread(),ptswrite(),ptsstop();
int	ptcopen(),ptcclose(),ptcread(),ptcwrite(),ptcselect();
int	ptyioctl();
struct	tty pt_tty[];
#else
#define ptsopen		enxio
#define ptsclose	enxio
#define ptsread		enxio
#define ptswrite	enxio
#define ptcopen		enxio
#define ptcclose	enxio
#define ptcread		enxio
#define ptcwrite	enxio
#define ptyioctl	enxio
#define	pt_tty		0
#define	ptcselect	enxio
#define	ptsstop		nullop
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
#if NDHU > 0
int dhuopen(),dhuclose(),dhuread(),dhuwrite(),dhuioctl(),dhustop(),dhureset();
struct tty dhu_tty[];
#else
#define dhuopen enxio
#define dhuclose enxio
#define dhuread enxio
#define dhuwrite enxio
#define dhuioctl enxio
#define dhustop enxio
#define dhureset nullop
#define dhu_tty 0
#endif

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
#if NDMZ > 0
int dmzopen(),dmzclose(),dmzread(),dmzwrite(),dmzioctl(),dmzstop(),dmzreset();
struct tty dmz_tty[];
#else
#define dmzopen enxio
#define dmzclose enxio
#define dmzread enxio
#define dmzwrite enxio
#define dmzioctl enxio
#define dmzstop enxio
#define dmzreset nullop
#define dmz_tty 0
#endif

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

int	logopen(), logclose(), logread(), logioctl(), logselect();

int	fdopen();

int	ttselect(), seltrue();

struct cdevsw	cdevsw[] =
{
	CHARDEV(cnopen,cnclose,cnread,cnwrite,cnioctl,nullop,nullop,NULL,
		cnselect,enodev,NULL),					/* 0 */
	CHARDEV(dzopen,dzclose,dzread,dzwrite,dzioctl,dzstop,dzreset,
		dz_tty,ttselect,enodev,NULL),				/* 1 */
	CHARDEV(cttyopen,nullop,cttyread,cttywrite,cttyioctl,nullop,
		nullop,NULL,cttyselect,enodev,NULL),			/* 2 */
	CHARDEV(nullop,nullop,mmrw,mmrw,enodev,nullop,nullop,NULL,
		mmselect,enodev,NULL),					/* 3 */
	CHARDEV(hpopen,hpclose,rawread,rawwrite,hpioctl,enodev,
		nullop,NULL,seltrue,enodev,htstrategy),			/* 4 */
	CHARDEV(htopen,htclose,rawread,rawwrite,htioctl,enodev,
		nullop,NULL,seltrue,enodev,htstrategy),			/* 5 */
	CHARDEV(vpopen,vpclose,enodev,vpwrite,vpioctl,nullop,
		vpreset,NULL,vpselect,enodev,NULL),			/* 6 */
	CHARDEV(nullop,nullop,rawread,rawwrite,enodev,enodev,
		nullop,NULL,enodev,enodev,swstrategy),			/* 7 */
	CHARDEV(flopen,flclose,flrw,flrw,enodev,enodev,nullop,
		NULL,seltrue,enodev,NULL),				/* 8 */
	CHARDEV(udaopen,udaclose,rawread,rawwrite,udaioctl,
		enodev,udareset,NULL,seltrue,enodev,udastrategy),	/* 9 */
	CHARDEV(vaopen,vaclose,enodev,vawrite,vaioctl,nullop,
		vareset,NULL,vaselect,enodev,NULL),			/* 10 */
	CHARDEV(rkopen,nullop,rawread,rawwrite,enodev,enodev,
		rkreset,NULL,seltrue,enodev,rkstrategy),		/* 11 */
	CHARDEV(dhopen,dhclose,dhread,dhwrite,dhioctl,dhstop,
		dhreset,dh11,ttselect,enodev,NULL),			/* 12 */
	CHARDEV(upopen,nullop,rawread,rawwrite,enodev,enodev,
		upreset,NULL,seltrue,enodev,upstrategy),		/* 13 */
	CHARDEV(tmopen,tmclose,rawread,rawwrite,tmioctl,enodev,tmreset,
		NULL,seltrue,enodev,tmstrategy),			/* 14 */
	CHARDEV(lpopen,lpclose,enodev,lpwrite,enodev,enodev,lpreset,
		NULL,seltrue,enodev,NULL),				/* 15 */
	CHARDEV(tsopen,tsclose,rawread,rawwrite,tsioctl,enodev,
		tsreset,NULL,seltrue,enodev,tsstrategy),		/* 16 */
	CHARDEV(utopen,utclose,rawread,rawwrite,utioctl,enodev,
		utreset,NULL,seltrue,enodev,utstrategy),		/* 17 */
	CHARDEV(ctopen,ctclose,enodev,ctwrite, enodev, enodev, nullop, NULL,
		seltrue,enodev, NULL),				/*18*/
	CHARDEV(mtopen,	 mtclose,rawread,rawwrite, /*19*/ mtioctl,enodev,
		 enodev, NULL, seltrue,	enodev,	 mtstrategy),
	CHARDEV(ptsopen,ptsclose, ptsread,ptswrite, /*20*/ ptyioctl,
	       ptsstop,	nullop, pt_tty, ttselect,enodev, NULL),
	CHARDEV(ptcopen,ptcclose,ptcread,ptcwrite,/*21*/ ptyioctl,
	       nullop, nullop, pt_tty, ptcselect,enodev,NULL),
	CHARDEV(dmfopen,dmfclose,dmfread,dmfwrite,/*22*/ dmfioctl,
		dmfstop,dmfreset, dmf_tty, ttselect,enodev,NULL),
	CHARDEV(idcopen,nullop, rawread,rawwrite, /*23*/ enodev, enodev,
		 idcreset,NULL,seltrue,	enodev,	 idcstrategy),
	CHARDEV(dnopen,	 dnclose,enodev, dnwrite,/*24*/ enodev,	 enodev,
		 nullop, NULL, seltrue,	enodev,	 NULL),
	CHARDEV(gencnopen,gencnclose,gencnread, gencnwrite,/*25*/ gencnioctl,
	     nullop,nullop, gencntty, ttselect, enodev,	 NULL),
	CHARDEV(lpaopen,lpaclose,lparead,lpawrite, /*26*/ lpaioctl,
	       enodev,	 nullop, NULL, seltrue,	enodev,	 NULL),
	CHARDEV(psopen,psclose,	psread,	pswrite,/*27*/ psioctl,	enodev,
		psreset,NULL, seltrue,	enodev,	NULL),
	CHARDEV(enodev,	enodev,	enodev,	enodev,	/*28*/ enodev,nullop,
		nullop,	NULL, enodev,enodev,NULL),
	CHARDEV(adopen,	adclose,enodev,	enodev,	/*29*/ adioctl,	enodev,
		adreset,NULL, seltrue,	enodev,	NULL),
	CHARDEV(rxopen,	rxclose,rxread,	rxwrite,/*30*/ rxioctl,	enodev,
		rxreset,NULL, seltrue,	enodev,	NULL),
	CHARDEV(ikopen,	ikclose,ikread,	ikwrite,/*31*/ ikioctl,	enodev,
		ikreset,NULL, seltrue,	enodev,	NULL),
	CHARDEV(rlopen,	enodev,	rawread,rawwrite,/*32*/ enodev,	enodev,
		rlreset,NULL, seltrue,	enodev,	rlstrategy),
	CHARDEV(logopen,logclose,logread,enodev,		/*33*/
	logioctl,	enodev,		nullop,	NULL,
	logselect,	enodev,		NULL),

	CHARDEV(dhuopen,dhuclose,	dhuread,	dhuwrite,	/*34*/
	dhuioctl,	dhustop,	dhureset,	dhu_tty,
	ttselect,	enodev,		NULL),

 	CHARDEV(crlopen,crlclose,	crlrw,		crlrw,		/*35*/
 	enodev,		enodev,		nullop,	NULL,
 	seltrue,	enodev,		NULL),

	CHARDEV(vsopen,	vsclose,	enodev,		enodev,		/*36*/
	vsioctl,	enodev,		vsreset,	NULL,
	vsselect,	enodev,		NULL),

	CHARDEV(dmzopen,dmzclose,	dmzread,	dmzwrite,	/*37*/
	dmzioctl,	dmzstop,	dmzreset,	dmz_tty,
	ttselect,	enodev,		NULL),

	CHARDEV(tmscpopen,tmscpclose,	rawread,	rawwrite,	/*38*/
	tmscpioctl,	enodev,		tmscpreset,	NULL,
	seltrue,	enodev,		tmscpstrategy),

	CHARDEV(npopen,	npclose,	npread,		npwrite,	/*39*/
	npioctl,	enodev,		npreset,	NULL,
	seltrue,	enodev,		NULL),

	CHARDEV(qvopen,	qvclose,	qvread,		qvwrite,	/*40*/
	qvioctl,	qvstop,		qvreset,	NULL,
	qvselect,	enodev,		NULL),

	CHARDEV(qdopen,	qdclose,	qdread,		qdwrite,	/*41*/
	qdioctl,	qdstop,		qdreset,	NULL,
	qdselect,	enodev,		NULL),

	CHARDEV(enodev,	enodev,		enodev,		enodev,		/*42*/
	enodev,		nullop,	nullop,	NULL,
	enodev,		enodev,		NULL),

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

	CHARDEV(enodev,	enodev,		enodev,		enodev,		/*47*/
	enodev,		nullop,	nullop,	NULL,
	enodev,		enodev,		NULL),

	CHARDEV(enodev,	enodev,		enodev,		enodev,		/*48*/
	enodev,		nullop,	nullop,	NULL,
	enodev,		enodev,		NULL),

	CHARDEV(enodev,	enodev,		enodev,		enodev,		/*49*/
	enodev,		nullop,	nullop,	NULL,
	enodev,		enodev,		NULL),

	CHARDEV(enodev,	enodev,		enodev,		enodev,		/*50*/
	enodev,		nullop,	nullop,	NULL,
	enodev,		enodev,		NULL),

	CHARDEV(rx50open,rx50close,	rx50rw,		rx50rw,		/*51*/
	enodev,		enodev,		nullop,	0,
	seltrue,	enodev,		NULL),

/* kdb50 ra */
	CHARDEV(kdbopen,nullop/*XXX*/,	rawread,	rawwrite,	/*52*/
	enodev,		enodev,		nullop,	0,
	seltrue,	enodev,		kdbstrategy),

	CHARDEV(fdopen,	enodev,		enodev,		enodev,		/*53*/
	enodev,		enodev,		enodev,		NULL,
	enodev,		enodev,		NULL),
};
int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

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

int	chrtoblktbl[]={
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

isdisk(dev, type)
	dev_t dev;
	int type;
{
	switch(major(dev)){
	case 0:
	case 1:
	case 2:
	case 3:
	case 6:
	case 7:
	case 10:
	case 12:
	case 15:
		return type==VBLK;

	case 4:
	case 13:
	case 17:
	case 19:
	case 23:
	case 30:
	case 32:
	case 38:
	case 52:
		return type==VCHR;

	/* Both char and block */
	case 5:
	case 9:
	case 11:
	case 14:
	case 16:
		return 1;

	default:
		return 0;
	}
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

