/*	$NetBSD: hpcfb.c,v 1.1.4.2 2001/03/12 13:30:05 bouyer Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
 * Copyright (c) 2000,2001
 *         SATO Kazumi. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */

/*
 * jump scroll, scroll thread, multiscreen, virtual text vram 
 * and hpcfb_emulops functions
 * written by SATO Kazumi.
 */

#define FBDEBUG
static const char _copyright[] __attribute__ ((unused)) =
    "Copyright (c) 1999 Shin Takemura.  All rights reserved.";
static const char _rcsid[] __attribute__ ((unused)) =
    "$Id: hpcfb.c,v 1.1.4.2 2001/03/12 13:30:05 bouyer Exp $";

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/user.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/ioctl.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wscons_callbacks.h>

#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>

#include <dev/hpc/hpcfbvar.h>
#include <dev/hpc/hpcfbio.h>

#include "bivideo.h"
#if NBIVIDEO > 0
#include <dev/hpc/bivideovar.h>
#endif

#ifdef FBDEBUG
int	hpcfb_debug = 0;
#define	DPRINTF(arg) if (hpcfb_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

#ifndef HPCFB_MAX_COLUMN
#define HPCFB_MAX_COLUMN 130
#endif /* HPCFB_MAX_COLUMN */
#ifndef HPCFB_MAX_ROW
#define HPCFB_MAX_ROW 80
#endif /* HPCFB_MAX_ROW */

/*
 * currently experimental
#define HPCFB_JUMP
#define HPCFB_MULTI
*/

struct hpcfb_vchar {
	u_int c;
	long attr;
};

struct hpcfb_tvrow {
	int maxcol;
	int spacecol;
	struct hpcfb_vchar col[HPCFB_MAX_COLUMN];
};

struct hpcfb_devconfig {
	struct rasops_info	dc_rinfo;	/* rasops infomation */

	int		dc_blanked;	/* currently had video disabled */
	struct hpcfb_softc *dc_sc;
	int dc_rows;
	int dc_cols;
	struct hpcfb_tvrow *dc_tvram;
	int dc_curx;
	int dc_cury;
#ifdef HPCFB_JUMP
	int dc_min_row;
	int dc_max_row;
	int dc_scroll;
	struct callout dc_scroll_ch;
	int dc_scroll_src;
	int dc_scroll_dst;
	int dc_scroll_num;
#endif /* HPCFB_JUMP */
	volatile int dc_state;
#define HPCFB_DC_CURRENT		0x80000000
#define HPCFB_DC_DRAWING		0x01	/* drawing raster ops */
#define HPCFB_DC_TDRAWING		0x02	/* drawing tvram */
#define HPCFB_DC_SCROLLPENDING		0x04	/* scroll is pending */
#define HPCFB_DC_UPDATE			0x08	/* tvram update */
#define HPCFB_DC_SCRDELAY		0x10	/* scroll time but delay it */
#define HPCFB_DC_SCRTHREAD		0x20	/* in scroll thread or callout */
#define HPCFB_DC_UPDATEALL		0x40	/* need to redraw all */
#define HPCFB_DC_ABORT			0x80	/* abort redrawing */
#ifdef HPCFB_MULTI
	int dc_scrno;
	int	dc_memsize;
#endif /* HPCFB_MULTI */
	u_char *dc_fbaddr;
};

#define HPCFB_MAX_SCREEN 5
#define HPCFB_MAX_JUMP 5

struct hpcfb_softc {
	struct	device sc_dev;
	struct	hpcfb_devconfig *sc_dc;	/* device configuration */
#ifdef HPCFB_MULTI
	struct	hpcfb_devconfig *screens[HPCFB_MAX_SCREEN];
#endif /* HPCFB_MULTI */
	const struct hpcfb_accessops	*sc_accessops;
	void *sc_accessctx;
	int nscreens;
	void *sc_powerhook;	/* power management hook */
	struct device *sc_wsdisplay;
	int sc_screen_resumed;
	int sc_polling;
	int sc_mapping;
	struct proc *sc_thread;
	struct lock sc_lock;
	void *sc_wantedscreen;
	void (*sc_switchcb) __P((void *, int, int));
	void *sc_switchcbarg;
	struct callout sc_switch_callout;
};

/*
 *  function prototypes
 */
int	hpcfbmatch __P((struct device *, struct cfdata *, void *));
void	hpcfbattach __P((struct device *, struct device *, void *));
int	hpcfbprint __P((void *aux, const char *pnp));

int	hpcfb_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
paddr_t	hpcfb_mmap __P((void *, off_t, int));

void	hpcfb_refresh_screen __P((struct hpcfb_softc *sc));
void	hpcfb_doswitch __P((struct hpcfb_softc *sc));

#ifdef HPCFB_JUMP
static void	hpcfb_create_thread __P((void *));
static void	hpcfb_thread __P((void *));
#endif /* HPCFB_JUMP */

static int	hpcfb_init __P((struct hpcfb_fbconf *fbconf,
				struct hpcfb_devconfig *dc));
static int	hpcfb_alloc_screen __P((void *, const struct wsscreen_descr *,
				     void **, int *, int *, long *));
static void	hpcfb_free_screen __P((void *, void *));
static int	hpcfb_show_screen __P((void *, void *, int,
				    void (*) (void *, int, int), void *));
static void     hpcfb_pollc __P((void *, int));
static void	hpcfb_power __P((int, void *));
static void	hpcfb_cmap_reorder __P((struct hpcfb_fbconf *,
					struct hpcfb_devconfig *));

static int	pow __P((int, int));

void    hpcfb_cursor __P((void *c, int on, int row, int col));
int     hpcfb_mapchar __P((void *, int, unsigned int *));
void    hpcfb_putchar __P((void *c, int row, int col, u_int uc, long attr));
void    hpcfb_copycols __P((void *c, int row, int srccol, 
			int dstcol, int ncols));
void    hpcfb_erasecols __P((void *c, int row, int startcol, 
			int ncols, long attr));
void    hpcfb_redraw __P((void *c, int row, int nrows, int all));
void    hpcfb_copyrows __P((void *c, int srcrow, int dstrow, int nrows));
void    hpcfb_eraserows __P((void *c, int row, int nrows, long attr));
int     hpcfb_alloc_attr __P((void *c, int fg, int bg, int flags, long *attr));
void    hpcfb_cursor_raw __P((void *c, int on, int row, int col));

#ifdef HPCFB_JUMP
void	hpcfb_update __P((void *));
void	hpcfb_do_scroll __P((void *));
void	hpcfb_check_update __P((void *));
#endif /* HPCFB_JUMP */

struct wsdisplay_emulops hpcfb_emulops = {
	hpcfb_cursor,
	hpcfb_mapchar,
	hpcfb_putchar,
	hpcfb_copycols,
	hpcfb_erasecols,
	hpcfb_copyrows,
	hpcfb_eraserows,
	hpcfb_alloc_attr
};

/*
 *  static variables
 */
struct cfattach hpcfb_ca = {
	sizeof(struct hpcfb_softc), hpcfbmatch, hpcfbattach,
};

struct wsscreen_descr hpcfb_stdscreen = {
	"std",
	0, 0,	/* will be filled in -- XXX shouldn't, it's global */
	&hpcfb_emulops,	/* XXX */
	0, 0,
	WSSCREEN_REVERSE
};

const struct wsscreen_descr *_hpcfb_scrlist[] = {
	&hpcfb_stdscreen,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list hpcfb_screenlist = {
	sizeof(_hpcfb_scrlist) / sizeof(struct wsscreen_descr *),
	_hpcfb_scrlist
};

struct wsdisplay_accessops hpcfb_accessops = {
	hpcfb_ioctl,
	hpcfb_mmap,
	hpcfb_alloc_screen,
	hpcfb_free_screen,
	hpcfb_show_screen,
	0 /* load_font */,
	hpcfb_pollc
};

void    hpcfb_tv_putchar __P((struct hpcfb_devconfig *, int, int, u_int, long));
void    hpcfb_tv_copycols __P((struct hpcfb_devconfig *, int, int, int, int));
void    hpcfb_tv_erasecols __P((struct hpcfb_devconfig *, int, int, int, long));
void    hpcfb_tv_copyrows __P((struct hpcfb_devconfig *, int, int, int));
void    hpcfb_tv_eraserows __P((struct hpcfb_devconfig *, int, int, long));

struct wsdisplay_emulops rasops_emul;

static int hpcfbconsole;
struct hpcfb_devconfig hpcfb_console_dc;
struct wsscreen_descr hpcfb_console_wsscreen;
struct hpcfb_tvrow hpcfb_console_tvram[HPCFB_MAX_ROW];

/*
 *  function bodies
 */
static int
pow(int x, int n)
{
	int res = 1;
	while (0 < n--) {
		res *= x;
	}
	return (res);
}

int
hpcfbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

void
hpcfbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct hpcfb_softc *sc = (struct hpcfb_softc *)self;
	struct hpcfb_attach_args *ha = aux;
	struct wsemuldisplaydev_attach_args wa;

	sc->sc_accessops = ha->ha_accessops;
	sc->sc_accessctx = ha->ha_accessctx;

	if (hpcfbconsole) {
#ifdef HPCFB_MULTI
		sc->screens[0] = 
#endif /* HPCFB_MULTI */
		sc->sc_dc = &hpcfb_console_dc;
		sc->nscreens = 1;
		hpcfb_console_dc.dc_sc = sc;
	} else {
#ifdef HPCFB_MULTI
		sc->screens[0] = 
#endif /* HPCFB_MULTI */
		sc->sc_dc = (struct hpcfb_devconfig *)
		    malloc(sizeof(struct hpcfb_devconfig), M_DEVBUF, M_WAITOK);
		sc->nscreens = 1;
		bzero(sc->sc_dc, sizeof(struct hpcfb_devconfig));
		if (hpcfb_init(&ha->ha_fbconflist[0], sc->sc_dc) != 0) {
			return;
		}
		sc->sc_dc->dc_tvram = hpcfb_console_tvram;
		bzero(hpcfb_console_tvram, sizeof(hpcfb_console_tvram));
		sc->sc_dc->dc_sc = sc;
	}
	sc->sc_polling = 0; /* XXX */
	sc->sc_mapping = 0; /* XXX */
	callout_init(&sc->sc_switch_callout);
	hpcfb_stdscreen.nrows = sc->sc_dc->dc_rows;
        hpcfb_stdscreen.ncols = sc->sc_dc->dc_cols;
	hpcfb_stdscreen.capabilities = sc->sc_dc->dc_rinfo.ri_caps;
	printf(": hpcrasops %dx%d pixels, %d colors, %dx%d chars: ",
	       sc->sc_dc->dc_rinfo.ri_width,
	       sc->sc_dc->dc_rinfo.ri_height,
	       pow(2, sc->sc_dc->dc_rinfo.ri_depth),
	       sc->sc_dc->dc_rinfo.ri_cols,
	       sc->sc_dc->dc_rinfo.ri_rows);
#ifdef HPCFB_MULTI
	printf(" multi");
#endif /* HPCFB_MULTI */
	printf("\n");

	/* Set video chip dependent CLUT if any. */
	if (hpcfbconsole && sc->sc_accessops->setclut) {
		sc->sc_accessops->setclut(sc->sc_accessctx, 
					  &hpcfb_console_dc.dc_rinfo);
	}

	/* set font for hardware accel */
	if (sc->sc_accessops->font) {
		sc->sc_accessops->font(sc->sc_accessctx, 
					sc->sc_dc->dc_rinfo.ri_font);
	}	

	/* Add a power hook to power management */
	sc->sc_powerhook = powerhook_establish(hpcfb_power, sc);
	if (sc->sc_powerhook == NULL)
		printf("%s: WARNING: unable to establish power hook\n",
			sc->sc_dev.dv_xname);

	wa.console = hpcfbconsole;
	wa.scrdata = &hpcfb_screenlist;
	wa.accessops = &hpcfb_accessops;
	wa.accesscookie = sc;

	sc->sc_wsdisplay = config_found(self, &wa, wsemuldisplaydevprint);

#ifdef HPCFB_JUMP
	/*
	 * Create a kernel thread to scroll,
	 */
	kthread_create(hpcfb_create_thread, sc);
#endif /* HPCFB_JUMP */
}

#ifdef HPCFB_JUMP
void
hpcfb_create_thread(arg)
	void *arg;
{
	struct hpcfb_softc *sc = arg;

	if (kthread_create1(hpcfb_thread, sc, &sc->sc_thread,
			    "%s", sc->sc_dev.dv_xname) == 0)
		return;

	/*
	 * We were unable to create the HPCFB thread; bail out.
	 */
	sc->sc_thread = 0;
	printf("%s: unable to create thread, kernel hpcfb scroll support disabled\n",
	       sc->sc_dev.dv_xname);
}

void
hpcfb_thread(arg)
	void *arg;
{
	struct hpcfb_softc *sc = arg;

	/*
	 * Loop forever, doing a periodic check for update events.
	 */
	for (;;) {
		/* HPCFB_LOCK(sc); */
		sc->sc_dc->dc_state |= HPCFB_DC_SCRTHREAD;	
		if (!sc->sc_mapping) /* draw only EMUL mode */
			hpcfb_update(sc->sc_dc);
		sc->sc_dc->dc_state &= ~HPCFB_DC_SCRTHREAD;	
		/* APM_UNLOCK(sc); */
		(void) tsleep(sc, PWAIT, "hpcfb",  (8 * hz) / 7 / 10);
	}
}
#endif /* HPCFB_JUMP */

/* Print function (for parent devices). */
int
hpcfbprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	if (pnp)
		printf("hpcfb at %s", pnp);

	return (UNCONF);
}

int
hpcfb_cnattach(fbconf)
	struct hpcfb_fbconf *fbconf;
{
	struct hpcfb_fbconf __fbconf __attribute__((__unused__));
	long defattr;

#if NBIVIDEO > 0
	if (fbconf == 0) {
		memset(&__fbconf, 0, sizeof(struct hpcfb_fbconf));
		if (bivideo_getcnfb(&__fbconf) != 0)
			return (ENXIO);
		fbconf = &__fbconf;
	}
#endif /* NBIVIDEO > 0 */
	bzero(&hpcfb_console_dc, sizeof(struct hpcfb_devconfig));
	if (hpcfb_init(fbconf, &hpcfb_console_dc) != 0)
		return (ENXIO);

	hpcfb_console_dc.dc_tvram = hpcfb_console_tvram;
	bzero(hpcfb_console_tvram, sizeof(hpcfb_console_tvram));

	hpcfb_console_wsscreen = hpcfb_stdscreen;
	hpcfb_console_wsscreen.nrows = hpcfb_console_dc.dc_rows;
	hpcfb_console_wsscreen.ncols = hpcfb_console_dc.dc_cols;
	hpcfb_console_wsscreen.capabilities = hpcfb_console_dc.dc_rinfo.ri_caps;
	hpcfb_alloc_attr(&hpcfb_console_dc, 7, 0, 0, &defattr);
	wsdisplay_cnattach(&hpcfb_console_wsscreen, &hpcfb_console_dc,
			   0, 0, defattr);
	hpcfbconsole = 1;

	return (0);
}

int
hpcfb_init(fbconf, dc)
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_devconfig *dc;
{
	struct rasops_info *ri;
	vaddr_t fbaddr;

	fbaddr = (vaddr_t)fbconf->hf_baseaddr;
	dc->dc_fbaddr = (u_char *)fbaddr;

	/* init rasops */
	ri = &dc->dc_rinfo;
	bzero(ri, sizeof(struct rasops_info));
	ri->ri_depth = fbconf->hf_pixel_width;
	ri->ri_bits = (caddr_t)fbaddr;
	ri->ri_width = fbconf->hf_width;
	ri->ri_height = fbconf->hf_height;
	ri->ri_stride = fbconf->hf_bytes_per_line;
#if 0
	ri->ri_flg = RI_FORCEMONO | RI_CURSOR;
#else
	ri->ri_flg = RI_CURSOR;
#endif
	if (rasops_init(ri, HPCFB_MAX_ROW, HPCFB_MAX_COLUMN)) {
		panic("%s(%d): rasops_init() failed!", __FILE__, __LINE__);
	}

	/* over write color map of rasops */
	hpcfb_cmap_reorder (fbconf, dc);

	dc->dc_curx = -1;
	dc->dc_cury = -1;
	dc->dc_rows = dc->dc_rinfo.ri_rows;
	dc->dc_cols = dc->dc_rinfo.ri_cols;
	dc->dc_state |= HPCFB_DC_CURRENT;
#ifdef HPCFB_JUMP
	dc->dc_max_row = 0;
	dc->dc_min_row = dc->dc_rows;
	dc->dc_scroll = 0;
	callout_init(&dc->dc_scroll_ch);
#endif /* HPCFB_JUMP */
#if defined(HPCFB_MULTI)
	dc->dc_memsize = ri->ri_stride * ri->ri_height;
#endif /*  defined(HPCFB_MULTI) */
#ifdef HPCFB_MULTI
	dc->dc_scrno = 0;
#endif /* HPCFB_MULTI */
	/* hook rasops in hpcfb_ops */
	rasops_emul = ri->ri_ops; /* struct copy */
	ri->ri_ops = hpcfb_emulops; /* struct copy */

	return (0);
}

static void
hpcfb_cmap_reorder(fbconf, dc)
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_devconfig *dc;
{
	struct rasops_info *ri = &dc->dc_rinfo;
	int reverse = fbconf->hf_access_flags & HPCFB_ACCESS_REVERSE;
	int *cmap = ri->ri_devcmap;
	vaddr_t fbaddr = (vaddr_t)fbconf->hf_baseaddr;
	int i, j, bg, fg, tmp;

	/*
	 * Set forground and background so that the screen 
	 * looks black on white.
	 * Normally, black = 00 and white = ff.
	 * HPCFB_ACCESS_REVERSE means black = ff and white = 00.
	 */
	switch (fbconf->hf_pixel_width) {
	case 1:
		/* FALLTHROUGH */
	case 2:
		/* FALLTHROUGH */
	case 4:
		if (reverse) {
			bg = 0;
			fg = ~0;
		} else {
			bg = ~0;
			fg = 0;
		}
		/* for gray-scale LCD, hi-contrast color map */
		cmap[0] = bg;
		for (i = 1; i < 16; i++)
			cmap[i] = fg;
		break;
	case 8:
		/* FALLTHROUGH */
	case 16:
		if (reverse) {
			for (i = 0, j = 15; i < 8; i++, j--) {
				tmp = cmap[i];
				cmap[i] = cmap[j];
				cmap[j] = tmp;
			}
		}
		break;
	}

	/* clear the screen */
	bg = cmap[0];
	for (i = 0;
	     i < fbconf->hf_height * fbconf->hf_bytes_per_line;
	     i += sizeof(u_int32_t)) {
		*(u_int32_t *)(fbaddr + i) = bg;
	}
}

int
hpcfb_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct hpcfb_softc *sc = v;
	struct hpcfb_devconfig *dc = sc->sc_dc;
	struct wsdisplay_fbinfo *wdf;

	switch (cmd) {
	case WSKBDIO_BELL:
		return (0);
		break;

	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_HPCFB;
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = dc->dc_rinfo.ri_height;
		wdf->width = dc->dc_rinfo.ri_width;
		wdf->depth = dc->dc_rinfo.ri_depth;
		wdf->cmsize = 256;	/* XXXX */
		return 0;		
	
	case WSDISPLAYIO_SMODE:
		if (*(int *)data == WSDISPLAYIO_MODE_EMUL){ 
			if (sc->sc_mapping){
				sc->sc_mapping = 0;
				if (dc->dc_state&HPCFB_DC_DRAWING)
					dc->dc_state &= ~HPCFB_DC_ABORT;
#ifdef HPCFB_FORCE_REDRAW
				hpcfb_refresh_screen(sc);
#else
				dc->dc_state |= HPCFB_DC_UPDATEALL;
#endif
			}
		} else {
			if (!sc->sc_mapping) {
				sc->sc_mapping = 1;
				dc->dc_state |= HPCFB_DC_ABORT;
			}
			sc->sc_mapping = 1;
		}
		if (sc && sc->sc_accessops->iodone)
			(*sc->sc_accessops->iodone)(sc->sc_accessctx);
		return 0;	

	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
	case WSDISPLAYIO_GETPARAM:
	case WSDISPLAYIO_SETPARAM:
	case HPCFBIO_GCONF:
	case HPCFBIO_SCONF:
	case HPCFBIO_GDSPCONF:
	case HPCFBIO_SDSPCONF:
	case HPCFBIO_GOP:
	case HPCFBIO_SOP:
		return (*sc->sc_accessops->ioctl)(sc->sc_accessctx,
						  cmd, data, flag, p);

	default:
		if (IOCGROUP(cmd) != 't')
			DPRINTF(("%s(%d): hpcfb_ioctl(%lx, %lx) grp=%c num=%ld\n",
			 __FILE__, __LINE__,
			 cmd, (u_long)data, (char)IOCGROUP(cmd), cmd&0xff));
		break;
	}

	return (ENOTTY); /* Inappropriate ioctl for device */
}

paddr_t
hpcfb_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct hpcfb_softc *sc = v;

	return (*sc->sc_accessops->mmap)(sc->sc_accessctx, offset, prot);
}

static void 
hpcfb_power(why, arg)
	int why;
	void *arg;
{
	struct hpcfb_softc *sc = arg;

	switch (why) {
	case PWR_STANDBY:
		break;
	case PWR_SOFTSUSPEND:
		/* XXX, casting to 'struct wsdisplay_softc *' means
		   that you should not call the method here... */
		sc->sc_screen_resumed = wsdisplay_getactivescreen(
			(struct wsdisplay_softc *)sc->sc_wsdisplay);
		if (wsdisplay_switch(sc->sc_wsdisplay,
		    WSDISPLAY_NULLSCREEN,
		    1 /* waitok */) == 0) {
			wsscreen_switchwait(
				(struct wsdisplay_softc *)sc->sc_wsdisplay,
				WSDISPLAY_NULLSCREEN);
		} else {
			sc->sc_screen_resumed = WSDISPLAY_NULLSCREEN;
		}
		break;
	case PWR_SOFTRESUME:
		if (sc->sc_screen_resumed != WSDISPLAY_NULLSCREEN)
			wsdisplay_switch(sc->sc_wsdisplay,
			    sc->sc_screen_resumed,
			    1 /* waitok */);
		break;
	}
}

void
hpcfb_refresh_screen(sc)
	struct hpcfb_softc *sc;
{
	struct hpcfb_devconfig *dc = sc->sc_dc;
	int x, y;

	if (dc == NULL)
		return;

#ifdef HPCFB_JUMP
	if (dc->dc_state&HPCFB_DC_SCROLLPENDING) {
		dc->dc_state &= ~HPCFB_DC_SCROLLPENDING;
		dc->dc_state &= ~HPCFB_DC_UPDATE;
		callout_stop(&dc->dc_scroll_ch);
	}
#endif /* HPCFB_JUMP */
	/*
	 * refresh screen
	 */
	dc->dc_state &= ~HPCFB_DC_UPDATEALL;
	x = dc->dc_curx;
	y = dc->dc_cury;
	if (0 <= x && 0 <= y)
		hpcfb_cursor_raw(dc, 0,  y, x); /* disable cursor */
	/* redraw all text */
	hpcfb_redraw(dc, 0, dc->dc_rows, 1);
	if (0 <= x && 0 <= y)
		hpcfb_cursor_raw(dc, 1,  y, x); /* enable cursor */
}

static int
hpcfb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct hpcfb_softc *sc = v;
#ifdef HPCFB_MULTI
	struct hpcfb_devconfig *dc;
#endif /* HPCFB_MULTI */

	DPRINTF(("%s(%d): hpcfb_alloc_screen()\n", __FILE__, __LINE__));

#ifdef HPCFB_MULTI
	if (!hpcfbconsole && sc->nscreens > 0)	/* XXXXX */
		return ENOMEM;

	if (sc->nscreens > HPCFB_MAX_SCREEN)
		return (ENOMEM);

	if (sc->screens[sc->nscreens] == NULL){
		sc->screens[sc->nscreens] =
			malloc(sizeof(struct hpcfb_devconfig), M_DEVBUF, M_WAITOK);
		if (sc->screens[sc->nscreens] == NULL)
			return ENOMEM;
		bzero(sc->screens[sc->nscreens], sizeof(struct hpcfb_devconfig));
	}
	dc = sc->screens[sc->nscreens];
	dc->dc_sc = sc;

	/* copy master raster info */
	dc->dc_rinfo = sc->sc_dc->dc_rinfo;
	if (sc->sc_accessops->font) {
		sc->sc_accessops->font(sc->sc_accessctx, 
					sc->sc_dc->dc_rinfo.ri_font);
	}	

	dc->dc_fbaddr = dc->dc_rinfo.ri_bits;
	dc->dc_rows = dc->dc_rinfo.ri_rows;
	dc->dc_cols = dc->dc_rinfo.ri_cols;
	dc->dc_memsize = dc->dc_rinfo.ri_stride * dc->dc_rinfo.ri_height;

	dc->dc_scrno = sc->nscreens;
	dc->dc_curx = -1;
	dc->dc_cury = -1;
	if (dc->dc_tvram == NULL){
		dc->dc_tvram = 
			malloc(sizeof(struct hpcfb_tvrow)*dc->dc_rows,
				M_DEVBUF, M_WAITOK);
		if (dc->dc_tvram == NULL){
			free(sc->screens[sc->nscreens], M_DEVBUF);
			sc->screens[sc->nscreens] = NULL;
			return ENOMEM;
		}
		bzero(dc->dc_tvram, 
				sizeof(struct hpcfb_tvrow)*dc->dc_rows);
	}
				
	*curxp = 0;
	*curyp = 0;
	sc->nscreens++;
	*cookiep = dc; 
	hpcfb_alloc_attr(*cookiep, 7, 0, 0, attrp);
#else /* HPCFB_MULTI */
	if (sc->nscreens > 0)
		return (ENOMEM);
	*curxp = 0;
	*curyp = 0;
	sc->nscreens++;
	*cookiep = &sc->sc_dc->dc_rinfo;
	sc->sc_dc->dc_rinfo.ri_ops.alloc_attr(*cookiep, 
					      7, 0, 0, attrp);
#endif /* HPCFB_MULTI */
	return (0);
}

static void
hpcfb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct hpcfb_softc *sc = v;

#ifdef HPCFB_MULTI
	if (sc->nscreens == 1 && sc->sc_dc == &hpcfb_console_dc)
		panic("hpcfb_free_screen: console");
	sc->nscreens--;
#else /* HPCFB_MULTI */
	if (sc->sc_dc == &hpcfb_console_dc)
		panic("hpcfb_free_screen: console");

	sc->nscreens--;
#endif /* HPCFB_MULTI */
}

static int
hpcfb_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb) __P((void *, int, int));
	void *cbarg;
{
	struct hpcfb_softc *sc = v;
#ifdef HPCFB_MULTI
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)cookie;
	struct hpcfb_devconfig *odc;
#endif /* HPCFB_MULTI */

	DPRINTF(("%s(%d): hpcfb_show_screen()\n", __FILE__, __LINE__));

#ifdef HPCFB_MULTI
	odc = sc->sc_dc;

	if (dc == NULL || odc == dc) {
		hpcfb_refresh_screen(sc);
		return 0;
	}

	sc->sc_wantedscreen = cookie;
	sc->sc_switchcb = cb;
	sc->sc_switchcbarg = cbarg;
	if (cb) {
		callout_reset(&sc->sc_switch_callout, 0,
			(void(*)(void *))hpcfb_doswitch, sc);
		return EAGAIN;
	}

	hpcfb_doswitch(sc);
	return 0;
#else /* HPCFB_MULTI */
	/* redraw screen image */
	hpcfb_refresh_screen(sc);

	return (0);
#endif /* HPCFB_MULTI */
}

void
hpcfb_doswitch(sc)
	struct hpcfb_softc *sc;
{
	struct hpcfb_devconfig *dc;
	struct hpcfb_devconfig *odc;

	odc = sc->sc_dc;
	dc = sc->sc_wantedscreen;

	if (!dc) {
		(*sc->sc_switchcb)(sc->sc_switchcbarg, EIO, 0);
		return;
	}

	if (odc == dc)
		return;

	if (odc) {
#ifdef HPCFB_JUMP
		odc->dc_state |= HPCFB_DC_ABORT;
#endif /* HPCFB_JUMP */

		if (odc->dc_curx >= 0 && odc->dc_cury >= 0)
			hpcfb_cursor_raw(odc, 0,  odc->dc_cury, odc->dc_curx);
			 /* disable cursor */
		/* disable old screen */
		odc->dc_state &= ~HPCFB_DC_CURRENT;
		odc->dc_rinfo.ri_bits = NULL;
	}
	/* switch screen to new one */
	dc->dc_state |= HPCFB_DC_CURRENT;
	dc->dc_rinfo.ri_bits = dc->dc_fbaddr;
	sc->sc_dc = dc;

	/* redraw screen image */
	hpcfb_refresh_screen(sc);

	sc->sc_wantedscreen = NULL;
	if (sc->sc_switchcb)
		(*sc->sc_switchcb)(sc->sc_switchcbarg, 0, 0);

	return;
}

static void
hpcfb_pollc(v, on)
	void *v;
	int on;
{
	struct hpcfb_softc *sc = v;

	if (sc == NULL)
		return;
	sc->sc_polling = on;
	if (sc->sc_accessops->iodone)
		(*sc->sc_accessops->iodone)(sc->sc_accessctx);
	if (on) {
		hpcfb_refresh_screen(sc);
		if (sc->sc_accessops->iodone)
			(*sc->sc_accessops->iodone)(sc->sc_accessctx);
	}

	return;
}

/*
 * cursor
 */
void
hpcfb_cursor(cookie, on, row, col)
	void *cookie;
	int on, row, col;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)cookie;

	if (on) {
		dc->dc_curx = col;
		dc->dc_cury = row;
	} else {
		dc->dc_curx = -1;
		dc->dc_cury = -1;
	}

	hpcfb_cursor_raw(cookie, on, row, col);
}

void
hpcfb_cursor_raw(cookie, on, row, col)
	void *cookie;
	int on, row, col;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)cookie;
	struct hpcfb_softc *sc = dc->dc_sc;
	struct rasops_info *ri = &dc->dc_rinfo;
	int curwidth, curheight;
	int xoff, yoff;

#ifdef HPCFB_JUMP
	if (dc->dc_state&HPCFB_DC_SCROLLPENDING) {
		dc->dc_state |= HPCFB_DC_UPDATE;
		return;
	}
#endif /* HPCFB_JUMP */
	if ((dc->dc_state&HPCFB_DC_CURRENT) == 0)
		return;

	if (ri->ri_bits == NULL)
		return;

	dc->dc_state |= HPCFB_DC_DRAWING;
	if (sc && sc->sc_accessops->cursor) {
		xoff = col * ri->ri_font->fontwidth;
		yoff = row * ri->ri_font->fontheight;
		curheight = ri->ri_font->fontheight;
		curwidth = ri->ri_font->fontwidth;
		(*sc->sc_accessops->cursor)(sc->sc_accessctx,
				on, xoff, yoff, curwidth, curheight);
	} else 
		rasops_emul.cursor(ri, on, row, col);
	dc->dc_state &= ~HPCFB_DC_DRAWING;
}

/*
 * mapchar
 */
int
hpcfb_mapchar(cookie, c, cp)
	void *cookie;
	int c;
	unsigned int *cp;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)cookie;
	struct rasops_info *ri = &dc->dc_rinfo;

	return rasops_emul.mapchar(ri, c, cp);
}

/*
 * putchar
 */
void
hpcfb_tv_putchar(dc, row, col, uc, attr)
	struct hpcfb_devconfig *dc;
	int row, col;
	u_int uc;
	long attr;
{
	struct hpcfb_tvrow *vscn = dc->dc_tvram;
	struct hpcfb_vchar *vc = &vscn[row].col[col];
	struct hpcfb_vchar *vcb;

	if (vscn == 0)
		return;

	dc->dc_state |= HPCFB_DC_TDRAWING;
#ifdef HPCFB_JUMP
	if (row < dc->dc_min_row)
		dc->dc_min_row = row;
	if (row > dc->dc_max_row)
		dc->dc_max_row = row;

#endif /* HPCFB_JUMP */
	if (vscn[row].maxcol +1 == col)
		vscn[row].maxcol = col;
	else if (vscn[row].maxcol < col) {
		vcb =  &vscn[row].col[vscn[row].maxcol+1];
		bzero(vcb, sizeof(struct hpcfb_vchar)*(col-vscn[row].maxcol-1));
		vscn[row].maxcol = col;
	}
	vc->c = uc;
	vc->attr = attr;
	dc->dc_state &= ~HPCFB_DC_TDRAWING;
#ifdef HPCFB_JUMP
	hpcfb_check_update(dc);
#endif /* HPCFB_JUMP */
}

void
hpcfb_putchar(cookie, row, col, uc, attr)
	void *cookie;
	int row, col;
	u_int uc;
	long attr;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)cookie;
	struct hpcfb_softc *sc = dc->dc_sc;
	struct rasops_info *ri = &dc->dc_rinfo;
	int xoff;
	int yoff;
	int fclr, uclr;
	struct wsdisplay_font *font;

	hpcfb_tv_putchar(dc, row, col, uc, attr);
#ifdef HPCFB_JUMP
	if (dc->dc_state&HPCFB_DC_SCROLLPENDING) {
		dc->dc_state |= HPCFB_DC_UPDATE;
		return;
	}
#endif /* HPCFB_JUMP */

	if ((dc->dc_state&HPCFB_DC_CURRENT) == 0)
		return;
	if (ri->ri_bits == NULL)
		return;

	dc->dc_state |= HPCFB_DC_DRAWING;
	if (sc && sc->sc_accessops->putchar 
	       && (dc->dc_state&HPCFB_DC_CURRENT)) {
		font = ri->ri_font;
		yoff = row * ri->ri_font->fontheight;
		xoff =  col * ri->ri_font->fontwidth;
		fclr = ri->ri_devcmap[((u_int)attr >> 24) & 15];
		uclr = ri->ri_devcmap[((u_int)attr >> 16) & 15];

		(*sc->sc_accessops->putchar)(sc->sc_accessctx,
				xoff, yoff, font, fclr, uclr, uc, attr);
	} else
		rasops_emul.putchar(ri, row, col, uc, attr);
	dc->dc_state &= ~HPCFB_DC_DRAWING;
#ifdef HPCFB_JUMP
	hpcfb_check_update(dc);
#endif /* HPCFB_JUMP */
}

/*
 * copycols
 */
void
hpcfb_tv_copycols(dc, row, srccol, dstcol, ncols)
	struct hpcfb_devconfig *dc;
	int row, srccol, dstcol, ncols;
{
	struct hpcfb_tvrow *vscn = dc->dc_tvram;
	struct hpcfb_vchar *svc = &vscn[row].col[srccol];
	struct hpcfb_vchar *dvc = &vscn[row].col[dstcol];

	if (vscn == 0)
		return;

	dc->dc_state |= HPCFB_DC_TDRAWING;
#ifdef HPCFB_JUMP
	if (row < dc->dc_min_row)
		dc->dc_min_row = row;
	if (row > dc->dc_max_row)
		dc->dc_max_row = row;
#endif /* HPCFB_JUMP */

	bcopy(svc, dvc, ncols*sizeof(struct hpcfb_vchar));
	if (vscn[row].maxcol < srccol+ncols-1)
		vscn[row].maxcol = srccol+ncols-1;
	if (vscn[row].maxcol < dstcol+ncols-1)
		vscn[row].maxcol = dstcol+ncols-1;
	dc->dc_state &= ~HPCFB_DC_TDRAWING;
#ifdef HPCFB_JUMP
	hpcfb_check_update(dc);
#endif /* HPCFB_JUMP */
}

void
hpcfb_copycols(cookie, row, srccol, dstcol, ncols)
	void *cookie;
	int row, srccol, dstcol, ncols;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)cookie;
	struct hpcfb_softc *sc = dc->dc_sc;
	struct rasops_info *ri = &dc->dc_rinfo;
	int srcxoff,dstxoff;
	int srcyoff,dstyoff;
	int height, width;

	hpcfb_tv_copycols(dc, row, srccol, dstcol, ncols);
#ifdef HPCFB_JUMP
	if (dc->dc_state&HPCFB_DC_SCROLLPENDING) {
		dc->dc_state |= HPCFB_DC_UPDATE;
		return;
	}
#endif /* HPCFB_JUMP */
	if ((dc->dc_state&HPCFB_DC_CURRENT) == 0)
		return;
	if (ri->ri_bits == NULL)
		return;

	dc->dc_state |= HPCFB_DC_DRAWING;
	if (sc && sc->sc_accessops->bitblit
	       && (dc->dc_state&HPCFB_DC_CURRENT)) {
		srcxoff = srccol * ri->ri_font->fontwidth;
		srcyoff = row * ri->ri_font->fontheight;
		dstxoff = dstcol * ri->ri_font->fontwidth;
		dstyoff = row * ri->ri_font->fontheight;
		width = ncols * ri->ri_font->fontwidth;
		height = ri->ri_font->fontheight;
		(*sc->sc_accessops->bitblit)(sc->sc_accessctx,
			srcxoff, srcyoff, dstxoff, dstyoff, height, width);
	} else
		rasops_emul.copycols(ri, row, srccol, dstcol, ncols);
	dc->dc_state &= ~HPCFB_DC_DRAWING;
#ifdef HPCFB_JUMP
	hpcfb_check_update(dc);
#endif /* HPCFB_JUMP */
}


/*
 * erasecols
 */
void
hpcfb_tv_erasecols(dc, row, startcol, ncols, attr)
	struct hpcfb_devconfig *dc;
	int row, startcol, ncols;
	long attr;
{
	struct hpcfb_tvrow *vscn = dc->dc_tvram;

	if (vscn == 0)
		return;

	dc->dc_state |= HPCFB_DC_TDRAWING;
#ifdef HPCFB_JUMP
	if (row < dc->dc_min_row)
		dc->dc_min_row = row;
	if (row > dc->dc_max_row)
		dc->dc_max_row = row;
#endif /* HPCFB_JUMP */

	vscn[row].maxcol = startcol-1;
	if (vscn[row].spacecol < startcol+ncols-1)
		vscn[row].spacecol = startcol+ncols-1;
	dc->dc_state &= ~HPCFB_DC_TDRAWING;
#ifdef HPCFB_JUMP
	hpcfb_check_update(dc);
#endif /* HPCFB_JUMP */
}

void
hpcfb_erasecols(cookie, row, startcol, ncols, attr)
	void *cookie;
	int row, startcol, ncols;
	long attr;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)cookie;
	struct hpcfb_softc *sc = dc->dc_sc;
	struct rasops_info *ri = &dc->dc_rinfo;
	int xoff, yoff;
	int width, height;

	hpcfb_tv_erasecols(dc, row, startcol, ncols, attr);
#ifdef HPCFB_JUMP
	if (dc->dc_state&HPCFB_DC_SCROLLPENDING) {
		dc->dc_state |= HPCFB_DC_UPDATE;
		return;
	}
#endif /* HPCFB_JUMP */
	if ((dc->dc_state&HPCFB_DC_CURRENT) == 0)
		return;
	if (ri->ri_bits == NULL)
		return;

	dc->dc_state |= HPCFB_DC_DRAWING;
	if (sc && sc->sc_accessops->erase
	       && (dc->dc_state&HPCFB_DC_CURRENT)) {
		xoff = startcol * ri->ri_font->fontwidth;
		yoff = row * ri->ri_font->fontheight;
		width = ncols * ri->ri_font->fontwidth;
		height = ri->ri_font->fontheight;
		(*sc->sc_accessops->erase)(sc->sc_accessctx,
			xoff, yoff, height, width, attr);
	} else 
		rasops_emul.erasecols(ri, row, startcol, ncols, attr);
	dc->dc_state &= ~HPCFB_DC_DRAWING;
#ifdef HPCFB_JUMP
	hpcfb_check_update(dc);
#endif /* HPCFB_JUMP */
}

/*
 * Copy rows.
 */
void
hpcfb_tv_copyrows(dc, src, dst, num)
	struct hpcfb_devconfig *dc;
	int src, dst, num;
{
	struct hpcfb_tvrow *vscn = dc->dc_tvram;
	struct hpcfb_tvrow *svc = &vscn[src];
	struct hpcfb_tvrow *dvc = &vscn[dst];
	int i;
	int d;

	if (vscn == 0)
		return;

	dc->dc_state |= HPCFB_DC_TDRAWING;
#ifdef HPCFB_JUMP
	if (dst < dc->dc_min_row)
		dc->dc_min_row = dst;
	if (dst + num > dc->dc_max_row)
		dc->dc_max_row = dst + num -1;
#endif /* HPCFB_JUMP */

	if (svc > dvc)
		d = 1;
	else if (svc < dvc) {
		svc += num-1;
		dvc += num-1;
		d = -1;
	} else  {
		dc->dc_state &= ~HPCFB_DC_TDRAWING;
#ifdef HPCFB_JUMP
		hpcfb_check_update(dc);
#endif /* HPCFB_JUMP */
		return;	
	}

	for (i = 0; i < num; i++) {
		bcopy(&svc->col[0], &dvc->col[0], sizeof(struct hpcfb_vchar)*(svc->maxcol+1));
		if (svc->maxcol < dvc->maxcol && dvc->spacecol < dvc->maxcol)
			dvc->spacecol = dvc->maxcol;
		dvc->maxcol = svc->maxcol;
		svc+=d;
		dvc+=d;
	}
	dc->dc_state &= ~HPCFB_DC_TDRAWING;
#ifdef HPCFB_JUMP
	hpcfb_check_update(dc);
#endif /* HPCFB_JUMP */
}

void
hpcfb_redraw(cookie, row, num, all)
	void *cookie;
	int row, num;
	int all;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)cookie;
	struct rasops_info *ri = &dc->dc_rinfo;
	int cols;
	struct hpcfb_tvrow *vscn = dc->dc_tvram;
	struct hpcfb_vchar *svc;
	int i, j;

#ifdef HPCFB_JUMP
	if (dc->dc_state&HPCFB_DC_SCROLLPENDING) {
		dc->dc_state |= HPCFB_DC_UPDATE;
		return;
	}
#endif /* HPCFB_JUMP */
	if (dc->dc_sc != NULL
	    && !dc->dc_sc->sc_polling
	    && dc->dc_sc->sc_mapping)
		return;

	dc->dc_state &= ~HPCFB_DC_ABORT;

	if ((dc->dc_state&HPCFB_DC_CURRENT) == 0)
		return;
	if (vscn == 0)
		return;

	if (ri->ri_bits == NULL)
		return;

	dc->dc_state |= HPCFB_DC_DRAWING;
	dc->dc_state |= HPCFB_DC_TDRAWING;
	for (i = 0; i < num; i++) {
		if (dc->dc_state&HPCFB_DC_ABORT)
			break;
		cols = vscn[row+i].maxcol;
		for (j = 0; j <= cols; j++) {
			if (dc->dc_state&HPCFB_DC_ABORT)
				continue;
			svc = &vscn[row+i].col[j];
			rasops_emul.putchar(ri, row + i, j, svc->c, svc->attr);
		}
		if (all)
			cols = dc->dc_cols-1;
		else
			cols = vscn[row+i].spacecol;
		for (; j <= cols; j++) {
			if (dc->dc_state&HPCFB_DC_ABORT)
				continue;
			rasops_emul.putchar(ri, row + i, j, ' ', 0);
		}
		vscn[row+i].spacecol = 0;
	}
	if (dc->dc_state&HPCFB_DC_ABORT)
		dc->dc_state &= ~HPCFB_DC_ABORT;
	dc->dc_state &= ~HPCFB_DC_DRAWING;
	dc->dc_state &= ~HPCFB_DC_TDRAWING;
#ifdef HPCFB_JUMP
	hpcfb_check_update(dc);
#endif /* HPCFB_JUMP */
}

#ifdef HPCFB_JUMP
void
hpcfb_update(v)
	void *v;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)v;

	/* callout_stop(&dc->dc_scroll_ch); */
	dc->dc_state &= ~HPCFB_DC_SCROLLPENDING;
	if (dc->dc_curx > 0 && dc->dc_cury > 0)
		hpcfb_cursor_raw(dc, 0,  dc->dc_cury, dc->dc_curx); 
	if ((dc->dc_state&HPCFB_DC_UPDATEALL)) {
		hpcfb_redraw(dc, 0, dc->dc_rows, 1);
		dc->dc_state &= ~(HPCFB_DC_UPDATE|HPCFB_DC_UPDATEALL);
	} else if ((dc->dc_state&HPCFB_DC_UPDATE)) {
		hpcfb_redraw(dc, dc->dc_min_row, 
				dc->dc_max_row - dc->dc_min_row, 0);
		dc->dc_state &= ~HPCFB_DC_UPDATE;
	} else {
		hpcfb_redraw(dc, dc->dc_scroll_dst, dc->dc_scroll_num, 0);
	}
	if (dc->dc_curx > 0 && dc->dc_cury > 0)
		hpcfb_cursor_raw(dc, 1,  dc->dc_cury, dc->dc_curx); 
}

void
hpcfb_do_scroll(v)
	void *v;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)v;

	dc->dc_state |= HPCFB_DC_SCRTHREAD;	
	if (dc->dc_state&(HPCFB_DC_DRAWING|HPCFB_DC_TDRAWING))
		dc->dc_state |= HPCFB_DC_SCRDELAY;
	else if (dc->dc_sc != NULL && dc->dc_sc->sc_thread)
		wakeup(dc->dc_sc);
	else if (dc->dc_sc != NULL && !dc->dc_sc->sc_mapping) {
		/* draw only EMUL mode */
		hpcfb_update(v);
	}
	dc->dc_state &= ~HPCFB_DC_SCRTHREAD;	
}

void
hpcfb_check_update(v)
	void *v;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)v;

	if (dc->dc_sc != NULL 
		&& dc->dc_sc->sc_polling 
		&& (dc->dc_state&HPCFB_DC_SCROLLPENDING)){
		callout_stop(&dc->dc_scroll_ch);
		dc->dc_state &= ~HPCFB_DC_SCRDELAY;
		hpcfb_update(v);
	}
	else if (dc->dc_state&HPCFB_DC_SCRDELAY){
		dc->dc_state &= ~HPCFB_DC_SCRDELAY;
		hpcfb_update(v);
	} else if (dc->dc_state&HPCFB_DC_UPDATEALL){
		dc->dc_state &= ~HPCFB_DC_UPDATEALL;
		hpcfb_update(v);
	}
}
#endif /* HPCFB_JUMP */

void
hpcfb_copyrows(cookie, src, dst, num)
	void *cookie;
	int src, dst, num;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)cookie;
	struct rasops_info *ri = &dc->dc_rinfo;
	struct hpcfb_softc *sc = dc->dc_sc;
	int srcyoff, dstyoff;
	int width, height;

	hpcfb_tv_copyrows(cookie, src, dst, num);

	if ((dc->dc_state&HPCFB_DC_CURRENT) == 0)
		return;
	if (ri->ri_bits == NULL)
		return;

	if (sc && sc->sc_accessops->bitblit
	       && (dc->dc_state&HPCFB_DC_CURRENT)) {
		dc->dc_state |= HPCFB_DC_DRAWING;
		srcyoff = src * ri->ri_font->fontheight;
		dstyoff = dst * ri->ri_font->fontheight;
		width = dc->dc_cols * ri->ri_font->fontwidth;
		height = num * ri->ri_font->fontheight;
		(*sc->sc_accessops->bitblit)(sc->sc_accessctx,
			0, srcyoff, 0, dstyoff, height, width);
		dc->dc_state &= ~HPCFB_DC_DRAWING;
	}
	else {
#ifdef HPCFB_JUMP
		if (sc && sc->sc_polling) {
			hpcfb_check_update(dc);
		} else if ((dc->dc_state&HPCFB_DC_SCROLLPENDING) == 0) {
			dc->dc_state |= HPCFB_DC_SCROLLPENDING;
			dc->dc_scroll = 1;
			dc->dc_scroll_src = src;
			dc->dc_scroll_dst = dst;
			dc->dc_scroll_num = num;
			callout_reset(&dc->dc_scroll_ch, hz/100, &hpcfb_do_scroll, dc);
			return;
		} else if (dc->dc_scroll++ < dc->dc_rows/HPCFB_MAX_JUMP) {
			dc->dc_state |= HPCFB_DC_UPDATE;
			return;
		} else {
			dc->dc_state &= ~HPCFB_DC_SCROLLPENDING;
			callout_stop(&dc->dc_scroll_ch);
		}
		if (dc->dc_state&HPCFB_DC_UPDATE) {
			dc->dc_state &= ~HPCFB_DC_UPDATE;
			hpcfb_redraw(cookie, dc->dc_min_row, 
					dc->dc_max_row - dc->dc_min_row, 0);
			dc->dc_max_row = 0;
			dc->dc_min_row = dc->dc_rows;
			if (dc->dc_curx > 0 && dc->dc_cury > 0)
				hpcfb_cursor(dc, 1,  dc->dc_cury, dc->dc_curx); 
			return;
		}
#endif /* HPCFB_JUMP */
		hpcfb_redraw(cookie, dst, num, 0);
	}
#ifdef HPCFB_JUMP
	hpcfb_check_update(dc);
#endif /* HPCFB_JUMP */
}

/*
 * eraserows
 */
void
hpcfb_tv_eraserows(dc, row, nrow, attr)
	struct hpcfb_devconfig *dc;
	int row, nrow;
	long attr;
{
	struct hpcfb_tvrow *vscn = dc->dc_tvram;
	int cols;
	int i;

	if (vscn == 0)
		return;

	dc->dc_state |= HPCFB_DC_TDRAWING;
	dc->dc_state &= ~HPCFB_DC_TDRAWING;
#ifdef HPCFB_JUMP
	if (row < dc->dc_min_row)
		dc->dc_min_row = row;
	if (row + nrow > dc->dc_max_row)
		dc->dc_max_row = row + nrow;
#endif /* HPCFB_JUMP */

	for (i = 0; i < nrow; i++) {
		cols = vscn[row+i].maxcol;
		if (vscn[row+i].spacecol < cols)
			vscn[row+i].spacecol = cols;
		vscn[row+i].maxcol = -1;
	}
#ifdef HPCFB_JUMP
	hpcfb_check_update(dc);
#endif /* HPCFB_JUMP */
}

void
hpcfb_eraserows(cookie, row, nrow, attr)
	void *cookie;
	int row, nrow;
	long attr;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)cookie;
	struct hpcfb_softc *sc = dc->dc_sc;
	struct rasops_info *ri = &dc->dc_rinfo;
	int yoff;
	int width;
	int height;

	hpcfb_tv_eraserows(dc, row, nrow, attr);
#ifdef HPCFB_JUMP
	if (dc->dc_state&HPCFB_DC_SCROLLPENDING) {
		dc->dc_state |= HPCFB_DC_UPDATE;
		return;
	}
#endif /* HPCFB_JUMP */
	if ((dc->dc_state&HPCFB_DC_CURRENT) == 0)
		return;
	if (ri->ri_bits == NULL)
		return;

	dc->dc_state |= HPCFB_DC_DRAWING;
	if (sc && sc->sc_accessops->erase
	       && (dc->dc_state&HPCFB_DC_CURRENT)) {
		yoff = row * ri->ri_font->fontheight;
		width = dc->dc_cols * ri->ri_font->fontwidth;
		height = nrow * ri->ri_font->fontheight;
		(*sc->sc_accessops->erase)(sc->sc_accessctx,
			0, yoff, height, width, attr);
	} else 
		rasops_emul.eraserows(ri, row, nrow, attr);
	dc->dc_state &= ~HPCFB_DC_DRAWING;
#ifdef HPCFB_JUMP
	hpcfb_check_update(dc);
#endif /* HPCFB_JUMP */
}

/*
 * alloc_attr
 */
int
hpcfb_alloc_attr(cookie, fg, bg, flags, attrp)
	void *cookie;
	int fg, bg, flags;
	long *attrp;
{
	struct hpcfb_devconfig *dc = (struct hpcfb_devconfig *)cookie;
	struct rasops_info *ri = &dc->dc_rinfo;

	return rasops_emul.alloc_attr(ri, fg, bg, flags, attrp);
}
