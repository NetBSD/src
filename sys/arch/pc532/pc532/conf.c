/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)conf.c	5.8 (Berkeley) 5/12/91
 *
 *	$Id: conf.c,v 1.8 1994/05/25 00:03:06 phil Exp $
 */

#include "param.h"
#include "systm.h"
#include "buf.h"
#include "ioctl.h"
#include "tty.h"
#include "conf.h"
#include "scn.h"

int	rawread(), rawwrite();


/* Block devices  */

#include "sd.h"
#if NSD > 0
int	sdopen(), sdclose(), sdioctl();
int	/*sddump(),*/ sdsize();
int	sdstrategy();
#define	sddump		enxio
#else
#define	sdopen		enxio
#define	sdclose		enxio
#define	sdstrategy	voidop
#define	sdioctl		enxio
#define	sddump		enxio
#define	sdsize		NULL
#endif

#include "st.h"
#if NST > 0
int	stopen(), stclose(), stioctl();
/*int	stdump(), stsize();*/
int	ststrategy();
#define	stdump		enxio
#define	stsize		NULL
#else
#define	stopen		enxio
#define	stclose		enxio
#define	ststrategy	voidop
#define	stioctl		enxio
#define	stdump		enxio
#define	stsize		NULL
#endif

#include "cd.h"
#if NCD > 0
int	cdopen(), cdclose(), cdioctl();
int	/*cddump(), */cdsize();
int	cdstrategy();
#define	cddump		enxio
#else
#define	cdopen		enxio
#define	cdclose		enxio
#define	cdstrategy	voidop
#define	cdioctl		enxio
#define	cddump		enxio
#define	cdsize		NULL
#endif

#if 0
#include "ch.h"
#if NCH > 0
int	chopen(), chclose(), chioctl();
#else
#define	chopen		enxio
#define	chclose		enxio
#define	chioctl		enxio
#endif
#endif

#ifdef RAMD_SIZE
int rdopen(), rdclose(), rdsize();
int  rdstrategy();
#endif

int	swread(), swwrite();
int	swstrategy();

struct bdevsw	bdevsw[] =
{
	{ sdopen,	sdclose,	sdstrategy,	sdioctl,	/*0*/
	  sddump,	sdsize,		NULL },			/* sdX */
	{ enodev,	enodev,		swstrategy,	enodev,		/*1*/
	  enodev,	enodev,		NULL },			/* swap */
	{ stopen,	stclose,	ststrategy,	stioctl,	/*2*/
	  stdump,	stsize,		NULL },			/* stX */
#ifdef RAMD_SIZE
	{ rdopen,	rdclose,	rdstrategy,	enodev,		/*3*/
	  enodev,	rdsize,		NULL },			/* rd */
#endif
};
int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);


/* Character devices */

int	cnopen(), cnclose(), cnread(), cnwrite(), cnioctl(), cnselect();

int	cttyopen(), cttyread(), cttywrite(), cttyioctl(), cttyselect();

int 	mmrw(), mmmmap();
#define	mmselect	seltrue

#include "pty.h"
#if NPTY > 0
int	ptsopen(),ptsclose(),ptsread(),ptswrite(),ptsstop();
int	ptcopen(),ptcclose(),ptcread(),ptcwrite(),ptcselect();
int	ptyioctl();
struct	tty *pt_tty[];
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
#define	pt_tty		NULL
#define	ptcselect	enxio
#define	ptsstop		nullop
#endif

#include "scn.h"
#if NSCN > 0
int	scnopen(), scnclose(), scnread(), scnwrite(), scnioctl();
#define scnreset	enxio
extern	struct tty *scn_tty[];
#else
#define scnopen		enxio
#define scnclose	enxio
#define scnread		enxio
#define scnwrite	enxio
#define scnioctl	enxio
#define scnreset	enxio
#define	scn_tty		NULL
#endif

int	logopen(),logclose(),logread(),logioctl(),logselect();

int	ttselect(), seltrue();

int	fdopen();

struct cdevsw	cdevsw[] =
{
	{ cnopen,	cnclose,	cnread,		cnwrite,	/*0*/
	  cnioctl,	nullop,		nullop,		NULL,	/* console */
	  cnselect,	enodev,		NULL },
	{ cttyopen,	nullop,		cttyread,	cttywrite,	/*1*/
	  cttyioctl,	nullop,		nullop,		NULL,	/* tty */
	  cttyselect,	enodev,		NULL },
        { nullop,       nullop,         mmrw,           mmrw,           /*2*/
          enodev,       nullop,         nullop,         NULL,	/* memory */
          mmselect,     enodev,         NULL },
	{ sdopen,	sdclose,	rawread,	rawwrite,	/*3*/
	  sdioctl,	enodev,		nullop,		NULL,	/* rsdX */
	  seltrue,	enodev,		sdstrategy },
	{ nullop,	nullop,		rawread,	rawwrite,	/*4*/
	  enodev,	enodev,		nullop,		NULL,	/* swap */
	  enodev,	enodev,		swstrategy },
	{ ptsopen,	ptsclose,	ptsread,	ptswrite,	/*5*/
	  ptyioctl,	ptsstop,	nullop,		pt_tty, /* ttyp */
	  ttselect,	enodev,		NULL },
	{ ptcopen,	ptcclose,	ptcread,	ptcwrite,	/*6*/
	  ptyioctl,	nullop,		nullop,		pt_tty,	/* ptyp */
	  ptcselect,	enodev,		NULL },
	{ logopen,	logclose,	logread,	enodev,		/*7*/
	  logioctl,	enodev,		nullop,		NULL,	/* klog */
	  logselect,	enodev,		NULL },
	{ scnopen,	scnclose,	scnread,	scnwrite,	/*8*/
	  scnioctl,	enodev,		scnreset,	scn_tty, /* ttyXX */
	  ttselect,	enodev,		NULL },
	{ enxio,	enxio,		enxio,		enxio, /*9*/
	  enxio,	enxio,		enxio,		NULL,	/* nothing */
	  enxio,	enxio,		enxio },
	{ stopen,	stclose,	rawread,	rawwrite,	/*10*/
	  stioctl,	enodev,		nullop,		NULL,	/* rstXX */
	  seltrue,	enodev,		ststrategy },
	{ fdopen,	enxio,		enxio,		enxio,		/*11*/
	  enxio,	enxio,		enxio,		NULL,	/* fd */
	  enxio,	enxio,		enxio },
};
int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

int	mem_no = 2; 	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(1, 0);


/*
 * constab is the console configuration for this type of machine.
 * XXX - it should probably all be configured automatically.
 */
#include <dev/cons.h>

/* XXX - all this could be autoconfig()ed */
#if NSCN > 0
int scncnprobe(), scncninit(), scncngetc(), scncnputc();
#endif

struct	consdev constab[] = {
#if NSCN > 0
	{ scncnprobe,	scncninit,	scncngetc,	scncnputc },
#endif
	{ 0 },
};
/* end XXX */


/*
 * Returns true if dev is /dev/zero.
 */
iszerodev(dev)
	dev_t dev;
{

	return (major(dev) == mem_no && minor(dev) == 12);
}
