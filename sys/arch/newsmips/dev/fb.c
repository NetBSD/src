/*	$NetBSD: fb.c,v 1.3.18.1 2000/06/30 16:27:32 simonb Exp $	*/
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sony Corp. and Kazumasa Utashiro of Software Research Associates, Inc.
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
 * from: $Hdr: fb.c,v 4.300 91/06/27 20:43:06 root Rel41 $ SONY
 *
 *	@(#)fb.c	8.1 (Berkeley) 6/11/93
 */

#include "fb.h"
#if NFB > 0
/*
 * Frame buffer driver
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>
#include <machine/framebuf.h>

#include <newsmips/dev/fbreg.h>

#define FB_USED		1
#define VIDEO_USED	2

struct fb_softc {
	struct device sc_dev;
	int fbs_state;
	int fbs_device;
	lScrType fbs_type;
	int fbs_flag;
	struct fbreg fbreg;
};

static int	fbmatch __P((struct device *, struct cfdata *, void *));
static void	fbattach __P((struct device *, struct device *, void *));
static void	fblock __P((void));
static void	fbunlock __P((int));
static int	fbinit __P((struct fbreg *));
static void	fbreset __P((struct fbreg *));

struct cfattach fb_ca = {
	sizeof(struct fb_softc), fbmatch, fbattach
};

extern struct cfdriver fb_cd;

static char *devname[] = {
/* 0*/	"",
/* 1*/	"NWB-512",
/* 2*/	"NWB-225",
/* 3*/	"POP-MONO",
/* 4*/	"POP-COLOR",
/* 5*/	"NWB-514",
/* 6*/	"NWB-251",
/* 7*/	"LCD-MONO",
/* 8*/	"LDC-COLOR",
/* 9*/	"NWB-518",
/*10*/	"NWB-252",
/*11*/	"NWB-253",
/*12*/	"NWB-254",
/*13*/	"NWB-255",
/*14*/	"SLB-101",
/*15*/	"NWB-256",
/*16*/	"NWB-257",
};

static int fbstate = 0;

extern int fb253_probe();
extern void bmcnattach();
extern int fbstart __P((struct fbreg *, int));
extern int search_fbdev __P((int, int));
extern int fbgetscrtype();
extern int fbsetpalette();
extern int fbgetpalette();
extern int fbsetcursor();
extern int fbnsetcursor();
extern int fbsetxy();

static int
fbmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;
	struct fbreg fbreg;
	struct fbreg *fbp = &fbreg;
	int unit = cf->cf_unit;

	if (strcmp(ca->ca_name, "fb"))
		return 0;

	fbp->fb_command = FB_CPROBE;
	fbp->fb_device = 0;
	fbp->fb_unit = unit;
	fbp->fb_data = -1;
	fbstart(fbp, 1);

	if (fbp->fb_result != FB_ROK)
		return 0;

	return 1;
}

static void
fbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct fb_softc *fb = (struct fb_softc *)self;
	struct fbreg *fbp = &fb->fbreg;
	int unit = fb->sc_dev.dv_cfdata->cf_unit;
	extern int tty00_is_console;

	fb->fbs_device = unit;

	fbp->fb_command = FB_CATTACH;
	fbp->fb_device = fb->fbs_device;
	fbstart(fbp, 1);
	fb->fbs_type = fbp->fb_scrtype;

	if (fb->fbs_type.type) {
		fb->fbs_state = 0;
		fb->fbs_flag = 0;
		printf(": %s",
			(fb->fbs_type.type < sizeof(devname)/sizeof(*devname))
				? devname[(int)fb->fbs_type.type] : "UNKNOWN");
		printf(" (%d x %d %d plane)",
				fb->fbs_type.visiblerect.extent.x,
				fb->fbs_type.visiblerect.extent.y,
				fb->fbs_type.plane);
	}

	if (! tty00_is_console) {	/* XXX */
		printf(" (console)");
		bmcnattach();
	}
	printf("\n");
}

int
fbopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	register int unit = FBUNIT(dev);
	register struct fb_softc *fb = fb_cd.cd_devs[unit];
	register struct fbreg *fbp = &fb->fbreg;

	if (unit >= NFB)
		return ENXIO;
	if (fb->fbs_flag && !FBVIDEO(dev))
		return EBUSY;

	if (fb->fbs_state == 0) {
		fbp->fb_device = fb->fbs_device;
		if (fbinit(fbp))
			return EBUSY;
	}

        fb->fbs_state |= FBVIDEO(dev) ? VIDEO_USED : FB_USED;

	return 0;
}

int
fbclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	register int unit = FBUNIT(dev);
	register struct fb_softc *fb = fb_cd.cd_devs[unit];
	register struct fbreg *fbp = &fb->fbreg;

	if (unit >= NFB)
		return ENXIO;

	if (fb->fbs_state == 0)
		return 0;

	if (!FBVIDEO(dev))
		fb->fbs_flag = 0;

	fb->fbs_state &= ~(FBVIDEO(dev) ? VIDEO_USED : FB_USED);

	if (fb->fbs_state == 0) {
		fbp->fb_device = fb->fbs_device;
		fbreset(fbp);
	}

	return 0;
}

int
fbioctl(dev, cmd, data, flags, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	register int unit = FBUNIT(dev);
	register struct fb_softc *fb = fb_cd.cd_devs[unit];
	register struct fbreg *fbp = &fb->fbreg;
	register int error = 0;

	if (unit >= NFB)
		return ENXIO;

	fblock();

	fbp->fb_device = fb->fbs_device;

	switch (cmd) {
	case FBIOCENABLE:
		fb->fbs_flag = 0;
		break;
	case FBIOCDISABLE:
		fb->fbs_flag = 1;
		break;
	case FBIOCAUTODIM:
		fbp->fb_command = FB_CAUTODIM;
		fbp->fb_data = *((int *)data);
		fbstart(fbp, 0);
		break;
	case FBIOCSETDIM:
		fbp->fb_command = FB_CSETDIM;
		fbp->fb_data = *((int*)data);
		fbstart(fbp, 0);
		break;
	case FBIOCGETDIM:
		fbp->fb_command = FB_CGETDIM;
		fbstart(fbp, 1);
		*((int*)data) = fbp->fb_data;
		break;
#ifdef notyet
	case FBIOCBITBLT:
		error = fbbitblt(fbp, (sBitblt *)data);
		break;
	case FBIOCNBITBLT:
		error = fbnbitblt(fbp, (lBitblt *)data, UIO_USERSPACE);
		break;
	case FBIOCBATCHBITBLT:
		error = fbbatchbitblt(fbp, (sBatchBitblt*)data, UIO_USERSPACE);
		break;
	case FBIOCNBATCHBITBLT:
		error = fbnbatchbitblt(fbp, (lBatchBitblt*)data, UIO_USERSPACE);
		break;
	case FBIOCTILEBITBLT:
		error = fbtilebitblt(fbp, (sTileBitblt *)data);
		break;
	case FBIOCNTILEBITBLT:
		error = fbntilebitblt(fbp, (lTileBitblt *)data);
		break;
	case FBIOCBITBLT3:
		error = fbbitblt3(fbp, (sBitblt3 *)data);
		break;
	case FBIOCNBITBLT3:
		error = fbnbitblt3(fbp, (lBitblt3 *)data);
		break;
	case FBIOCPOLYLINE:
		error = fbpolyline(fbp, (sPrimLine *)data, 0);
		break;
	case FBIOCNPOLYLINE:
		error = fbnpolyline(fbp, (lPrimLine *)data, 0, UIO_USERSPACE);
		break;
	case FBIOCDJPOLYLINE:
		error = fbpolyline(fbp, (sPrimLine *)data, 1);
		break;
	case FBIOCNDJPOLYLINE:
		error = fbnpolyline(fbp, (lPrimLine *)data, 1, UIO_USERSPACE);
		break;
	case FBIOCPOLYMARKER:
		error = fbpolymarker(fbp, (sPrimMarker *)data);
		break;
	case FBIOCNPOLYMARKER:
		error = fbnpolymarker(fbp, (lPrimMarker *)data, UIO_USERSPACE);
		break;
	case FBIOCRECTANGLE:
		error = fbrectangle(fbp, (sPrimRect *)data);
		break;
	case FBIOCNRECTANGLE:
		error = fbnrectangle(fbp, (lPrimRect *)data);
		break;
	case FBIOCFILLSCAN:
		error = fbfillscan(fbp, (sPrimFill *)data);
		break;
	case FBIOCNFILLSCAN:
		error = fbnfillscan(fbp, (lPrimFill *)data, UIO_USERSPACE);
		break;
	case FBIOCTEXT:
		error = fbtext(fbp, (sPrimText *)data);
		break;
	case FBIOCNTEXT:
		error = fbntext(fbp, (lPrimText *)data);
		break;
	case FBIOCPOLYDOT:
		error = fbpolydot(fbp, (sPrimDot *)data);
		break;
	case FBIOCNPOLYDOT:
		error = fbnpolydot(fbp, (lPrimDot *)data, UIO_USERSPACE);
		break;
#endif

	case FBIOCGETSCRTYPE:
		fbgetscrtype(fbp, (sScrType *)data);
		break;
	case FBIOCNGETSCRTYPE:
		fbp->fb_command = FB_CGETSCRTYPE;
		fbstart(fbp, 1);
		*((lScrType*)data) = fbp->fb_scrtype;
		break;
	case FBIOCSETPALETTE:
		fbsetpalette(fbp, (sPalette *)data);
		break;
#ifdef notyet
	case FBIOCNSETPALETTE:
		error = fbnsetpalette(fbp, (lPalette *)data);
		break;
#endif
	case FBIOCGETPALETTE:
		fbgetpalette(fbp, (sPalette *)data);
		break;
#ifdef notyet
	case FBIOCNGETPALETTE:
		error = fbngetpalette(fbp, (lPalette *)data);
		break;
#endif
	case FBIOCSETCURSOR:
		fbsetcursor(fbp, (sCursor *)data);
		break;
	case FBIOCNSETCURSOR:
		fbnsetcursor(fbp, (lCursor *)data);
		break;
	case FBIOCNSETCURSOR2:
		fbp->fb_command = FB_CSETCURSOR;
		fbp->fb_cursor = *((lCursor2 *)data);
		fbstart(fbp, 0);
		break;
	case FBIOCUNSETCURSOR:
		fbp->fb_command = FB_CUNSETCURSOR;
		fbstart(fbp, 0);
		break;
	case FBIOCNUNSETCURSOR:
		fbp->fb_command = FB_CUNSETCURSOR;
		fbstart(fbp, 0);
		break;
	case FBIOCSHOWCURSOR:
		fbp->fb_command = FB_CSHOWCURSOR;
		fbstart(fbp, 0);
		break;
	case FBIOCNSHOWCURSOR:
		fbp->fb_command = FB_CSHOWCURSOR;
		fbstart(fbp, 0);
		break;
	case FBIOCHIDECURSOR:
		fbp->fb_command = FB_CHIDECURSOR;
		fbstart(fbp, 0);
		break;
	case FBIOCNHIDECURSOR:
		fbp->fb_command = FB_CHIDECURSOR;
		fbstart(fbp, 0);
		break;
	case FBIOCSETXY:
		fbsetxy(fbp, (sPoint *)data);
		break;
	case FBIOCNSETXY:
		fbp->fb_command = FB_CSETXY;
		fbp->fb_point = *((lPoint *)data);
		fbstart(fbp, 0);
		break;
	case FBIOCNSETPALETTEMODE:
		fbp->fb_command = FB_CSETPMODE;
		fbp->fb_data = *((int*)data);
		fbstart(fbp, 0);
		break;
	case FBIOCNGETPALETTEMODE:
		fbp->fb_command = FB_CGETPMODE;
		fbstart(fbp, 1);
		*((int*)data) = fbp->fb_data;
		break;
	case FBIOCNSETVIDEO:
		fbp->fb_command = FB_CSETVIDEO;
		fbp->fb_videoctl = *((lVideoCtl*)data);
		fbstart(fbp, 0);
		break;
	case FBIOCNGETVIDEO:
		fbp->fb_command = FB_CGETVIDEO;
		fbp->fb_videostatus.request = VIDEO_STATUS;
		fbstart(fbp, 1);
		*((lVideoStatus*)data) = fbp->fb_videostatus;
		error = fbp->fb_result;
		break;
	case FBIOCNIOCTL:
		fbp->fb_command = FB_CIOCTL;
		fbp->fb_fbioctl = *((lFbIoctl*)data);
		fbstart(fbp, 1);
		*((lFbIoctl*)data) = fbp->fb_fbioctl;
		if (fbp->fb_result == FB_RERROR)
			error = EINVAL;
		break;

	default:
		error = ENXIO;
		break;
	}

	fbunlock(error);

	return error;
}

paddr_t
fbmmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	int unit = FBUNIT(dev);
	struct fb_softc *fb = fb_cd.cd_devs[unit];
	struct fbreg *fbp = &fb->fbreg;
	paddr_t page;

	if (unit >= NFB)
		return -1;

        if (off & PGOFSET)
                return -1;

	if (off < 0)
		return -1;

	fblock();
	fbp->fb_device = fb->fbs_device;
	fbp->fb_command = FB_CGETPAGE;
	fbp->fb_data = off;
	fbstart(fbp, 1);
	page = fbp->fb_data;
	if (fbp->fb_result == FB_RERROR)
		page = -1;
	else
		fb->fbs_flag = 1;
	fbunlock(fbp->fb_result);

	return page;
}

#if 0	/* not needed? */
int
fbpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	return -1;
}
#endif

static void
fblock()
{
	int s;

#ifdef USE_RAW_INTR
	fbwait();
#endif
	s = splfb();
	while (fbstate & FB_BUSY) {
		fbstate |= FB_WANTED;
		tsleep(&fbstate, FBPRI | PCATCH, "fblock", 0);
	}
	fbstate |= FB_BUSY;
	splx(s);
}

static void
fbunlock(error)
	int error;
{
	int s = splfb();

#ifdef CPU_SINGLE
	fbstate &= ~FB_BUSY;
	if (fbstate & FB_WANTED) {
		fbstate &= ~FB_WANTED;
		wakeup(&fbstate);
	}
#else
#ifdef USE_RAW_INTR
	fbstate &= ~FB_BUSY;
	if (fbstate & FB_WANTED) {
		fbstate &= ~FB_WANTED;
		wakeup(&fbstate);
	}
#else
	if (error || (fbstate & FB_DELAY) == 0) {
		fbstate &= ~(FB_BUSY | FB_WAIT | FB_DELAY);
		if (fbstate & FB_WANTED) {
			fbstate &= ~FB_WANTED;
			wakeup(&fbstate);
		}
	}
	if (fbstate & FB_DELAY) {
		fbstate &= ~FB_DELAY;
		fbstate |= FB_DELAY2;
	}
#endif
#endif /* CPU_SINGLE */
	splx(s);
}

static int
fbinit(fbp)
	struct fbreg *fbp;
{
	fblock();

	fbp->fb_command = FB_COPEN;
	fbstart(fbp, 1);
	if (fbp->fb_result != FB_ROK) {
		fbunlock(0);
		return FB_RERROR;
	}

	fbp->fb_command = FB_CUNSETCURSOR;
	fbstart(fbp, 0);

	fbunlock(0);

	return FB_ROK;
}

static void
fbreset(fbp)
	struct fbreg *fbp;
{
	fblock();

	fbp->fb_command = FB_CUNSETCURSOR;
	fbstart(fbp, 1);

	fbp->fb_command = FB_CCLOSE;
	fbstart(fbp, 0);

	fbunlock(0);
}
#endif /* NFB > 0 */
