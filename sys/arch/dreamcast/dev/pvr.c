/* $NetBSD: pvr.c,v 1.1 2001/01/16 00:33:20 marcus Exp $ */

/*
 * Copyright (c) 2001 Marcus Comstedt
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
 *      This product includes software developed by Matt DeBergalis
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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


#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/conf.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/cons.h>

#include <dreamcast/dev/pvrvar.h>
#include <dreamcast/dev/maple/mkbdvar.h>
#include <dev/wscons/wsconsio.h>

#include <dev/rcons/raster.h>
#include <dev/wscons/wscons_raster.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_callbacks.h>

#include <sh3/shbvar.h>

#include "wsdisplay.h"
#include "mkbd.h"

#if NWSDISPLAY > 0
cdev_decl(wsdisplay);
#endif

int pvr_match __P((struct device *, struct cfdata *, void *));
void pvr_attach __P((struct device *, struct device *, void *));

struct cfattach pvr_ca = {
	sizeof(struct pvr_softc), 
	pvr_match,
	pvr_attach,
};


const struct wsdisplay_emulops pvr_emulops = {
	rcons_cursor,
	rcons_mapchar,
	rcons_putchar,
	rcons_copycols,
	rcons_erasecols,
	rcons_copyrows,
	rcons_eraserows,
	rcons_alloc_attr
};

const struct wsscreen_descr pvr_stdscreen = {
	"52x20", 52, 20,
	&pvr_emulops,
	12, 24,
	WSSCREEN_WSCOLORS,
};

const struct wsscreen_descr *_pvr_scrlist[] = {
	&pvr_stdscreen,
};

const struct wsscreen_list pvr_screenlist = {
	sizeof(_pvr_scrlist) / sizeof(struct wsscreen_descr *),
	_pvr_scrlist
};

static int	pvr_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static paddr_t	pvr_mmap __P((void *, off_t, int));
static int	pvr_alloc_screen __P((void *, const struct wsscreen_descr *,
		    void **, int *, int *, long *));
static void	pvr_free_screen __P((void *, void *));
static int	pvr_show_screen __P((void *, void *, int,
		    void (*)(void *, int, int), void *));

static void	pvr_check_cable __P((int *, int *));
static void	pvr_init_dpy_hardware __P((void));


const struct wsdisplay_accessops pvr_accessops = {
	pvr_ioctl,
	pvr_mmap,
	pvr_alloc_screen,
	pvr_free_screen,
	pvr_show_screen,
	0 /* load_font */
};


static void
pvr_check_cable(vgamode_p, rgbmode_p)
	int *vgamode_p;
	int *rgbmode_p;
{
	volatile u_int32_t *porta = (volatile u_int32_t *)0xff80002c;
	u_int16_t v;

	/* PORT8 and PORT9 is input */
	*porta = (*porta & ~0xf0000) | 0xa0000;

	/* Read PORT8 and PORT9 */
	v = ((*(volatile u_int16_t *)(porta+1))>>8)&3;

	if (! (v&2) )
	  *vgamode_p = *rgbmode_p = 1;
	else {
	  *vgamode_p = 0;
	  *rgbmode_p = (v&1)? 0 : 1;
	}
}

static void
pvr_init_dpy_hardware()
{
	volatile u_int32_t *pvr = (volatile u_int32_t *)0xa05f8000;
	int display_lines_per_field = 240;
	int modulo = 1, voffset;
	int vgamode, rgbmode;

	pvr_check_cable(&vgamode, &rgbmode);

	pvr[8/4] = 0; /* reset */
	pvr[0x40/4] = 0; /* black border */

	if(vgamode) {
	  pvr[0x44/4] = 0x800004; /* 31kHz, RGB565 */
	  pvr[0xd0/4] = 0x100;	  /* video output */
	  display_lines_per_field = 480;
	  voffset = 36;
	} else {
	  pvr[0x44/4] = 0x000004; /* 15kHz, RGB565 */
	  pvr[0xd0/4] = 0x110;	  /* video output, NTSC, interlace */
	  modulo += 640*2/4;	  /* interlace -> skip every other line */
	  voffset = 18;
	}

	pvr[0x50/4]=0;          /* video base address, long field */
	pvr[0x54/4]=640*2;	/* video base address, short field */

	pvr[0x5c/4]=(modulo<<20)|((display_lines_per_field-1)<<10)|(640*2/4-1);

	voffset = (voffset<<16) | voffset;

	pvr[0xf0/4]=voffset;		/* V start		*/
	pvr[0xdc/4]=voffset+display_lines_per_field;	/* V border	*/
	pvr[0xec/4]=164;		/* H start		*/
	pvr[0xd8/4]=(524<<16)|857;	/* HV counter		*/
	pvr[0xd4/4]=(126<<16)|837;	/* H border		*/
	pvr[0xe8/4]=22<<16;

	/* RGB / composite */
	*(volatile u_int32_t *)0xa0702c00 = (rgbmode? 0 : 3) << 8;

	pvr[0x44/4] |= 1;  /* display on */
}




int
pvr_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
	case WSDISPLAYIO_GINFO:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_PUTCMAP:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_SCURSOR:
	case WSDISPLAYIO_SVIDEO:
		/* NONE of these operations are supported. */
		return ENOTTY;
	}

	return -1;
}

static paddr_t
pvr_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	return (-1);
}

int
pvr_alloc_screen(v, type, cookiep, curxp, curyp, defattrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *defattrp;
{
	return (ENOMEM);
}

void
pvr_free_screen(v, cookie)
	void *v;
	void *cookie;
{
}

int
pvr_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb) __P((void *, int, int));
	void *cbarg;
{
	return 0;
}


int
pvr_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
  	struct shb_attach_args *sa = aux;

	if(strcmp("pvr", match->cf_driver->cd_name))
	  return 0;

	sa->ia_iosize = 0 /* 0x1400 */;
	return (1);
}

void
pvr_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct pvr_softc *sc;
	struct wsemuldisplaydev_attach_args waa;
	static int pvr_console_initted = 0;

	sc = (struct pvr_softc *)self;

	printf("\n");

	/* initialize the raster */
	waa.console = (++pvr_console_initted == 1);
	waa.scrdata = &pvr_screenlist;
	waa.accessops = &pvr_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}


/* Console stuff */

void pvrcnprobe __P((struct consdev *));
void pvrcninit __P((struct consdev *));

void
pvrcninit(cndev)
	struct	consdev *cndev;
{
	static struct rcons rcons;
	static struct raster raster;
	struct rcons *rcp;
	struct raster *rap;

	/* initialize the raster console blitter */
	rap = &raster;
	rap->width = 640;
	rap->height = 480;
	rap->depth = 16;
	rap->linelongs = 640*2 / sizeof(u_int32_t);
	rap->pixels = (void *)0xa5000000;

	bzero(rap->pixels, 640*480*2);

	pvr_init_dpy_hardware();

	rcp = &rcons;
	rcp->rc_sp = rap;
	rcp->rc_crow = rcp->rc_ccol = -1;
	rcp->rc_crowp = &rcp->rc_crow;
	rcp->rc_ccolp = &rcp->rc_ccol;
	rcons_init(rcp, 20, 52);

	wsdisplay_cnattach(&pvr_stdscreen, rcp, 0, 0, 0);
	cn_tab->cn_pri = CN_INTERNAL;

#if NMKBD > 0
	mkbd_cnattach(); /* Connect keyboard and screen together */
#endif
}


void
pvrcnprobe(cndev)
	struct  consdev *cndev;
{
#if NWSDISPLAY > 0
	int     maj, unit;
#endif
	cndev->cn_dev = NODEV;
	cndev->cn_pri = CN_NORMAL;

#if NWSDISPLAY > 0
	unit = 0;
	for (maj = 0; maj < nchrdev; maj++) {
		if (cdevsw[maj].d_open == wsdisplayopen) {
			break;
		}
	}
	if (maj != nchrdev) {
		cndev->cn_pri = CN_INTERNAL;
		cndev->cn_dev = makedev(maj, unit);
	}
#endif
}
