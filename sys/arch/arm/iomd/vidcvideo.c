/* $NetBSD: vidcvideo.c,v 1.35.14.1 2009/05/13 17:16:17 jym Exp $ */

/*
 * Copyright (c) 2001 Reinoud Zandijk
 * Copyright (c) 1998, 1999 Tohru Nishimura.  All rights reserved.
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
 *      This product includes software developed by Tohru Nishimura
 *	and Reinoud Zandijk for the NetBSD Project.
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
 *
 * Created vidcvideo.c from /dev/tc/cfb.c to fit the Acorn/ARM VIDC1 and VIDC20 chips
 *
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: vidcvideo.c,v 1.35.14.1 2009/05/13 17:16:17 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/ioctl.h>

#include <arm/mainbus/mainbus.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>

#include <dev/wscons/wsdisplay_vconsvar.h>

#include <uvm/uvm_extern.h>
#include <arm/arm32/pmap.h>
#include <arm/cpufunc.h>

/* for vidc_mode ... needs to be MI indepenent one day */
#include <arm/iomd/vidc.h>
#include <arm/iomd/vidc20config.h>
#include <arm/iomd/vidcvideo.h>
#include <machine/bootconfig.h>

/* FOR DEBUG */
extern videomemory_t videomemory;

struct hwcmap256 {
#define	CMAP_SIZE	256	/* 256 R/G/B entries */
	uint8_t r[CMAP_SIZE];
	uint8_t g[CMAP_SIZE];
	uint8_t b[CMAP_SIZE];
};


/* XXX for CURSOR_MAX_WIDTH = 32 */
struct hwcursor32 {
	struct wsdisplay_curpos cc_pos;
	struct wsdisplay_curpos cc_hot;
	struct wsdisplay_curpos cc_size;
	uint8_t cc_color[6];		/* how many? */
	uint32_t cc_image[(CURSOR_MAX_WIDTH/4) * CURSOR_MAX_HEIGHT];
	uint32_t cc_mask[(CURSOR_MAX_WIDTH/4) * CURSOR_MAX_HEIGHT];
};


struct fb_devconfig {
	vaddr_t dc_vaddr;		/* memory space virtual base address */
	paddr_t dc_paddr;		/* memory space physical base address */
	vsize_t dc_size;		/* size of slot memory */
	int	dc_width;		/* width of frame buffer */
	int	dc_height;		/* height of frame buffer */
	int	dc_log2_depth;		/* log2 of bits per pixel */
	int	dc_depth;		/* depth, bits per pixel */
	int	dc_rowbytes;		/* bytes in a FB scan line */
	vaddr_t	dc_videobase;		/* base of flat frame buffer */
	int	dc_blanked;		/* currently has video disabled */
	void   *dc_hwscroll_cookie;	/* cookie for hardware scroll */
	void   *dc_ih;			/* interrupt handler for dc */

	int	dc_curenb;		/* is cursor sprite enabled ?	*/
	int	_internal_dc_changed;	/* need update of hardware	*/
	int	dc_writeback_delay;	/* Screenarea write back vsync counter */
#define	WSDISPLAY_CMAP_DOLUT	0x20
#define WSDISPLAY_VIDEO_ONOFF	0x40
#define WSDISPLAY_WB_COUNTER	0x80

	struct hwcmap256	dc_cmap;   /* software copy of colormap	*/
	struct hwcursor32	dc_cursor; /* software copy of cursor	*/

	struct vidc_mode	mode_info;

	struct wsdisplay_emulops orig_ri_ops;	/* Rasops functions for deligation */

	/* virtual console support */
	struct vcons_data dc_vd;
	struct vcons_screen dc_console;
};


struct vidcvideo_softc {
	struct device sc_dev;
	struct fb_devconfig *sc_dc;	/* device configuration		*/
};


/* Function prototypes for glue */
static int  vidcvideo_match(struct device *, struct cfdata *, void *);
static void vidcvideo_attach(struct device *, struct device *, void *);


/* config glue */
CFATTACH_DECL(vidcvideo, sizeof(struct vidcvideo_softc),
    vidcvideo_match, vidcvideo_attach, NULL, NULL);

static struct fb_devconfig vidcvideo_console_dc;
static bool vidcvideo_is_console = false; 


static struct wsscreen_descr vidcvideo_stdscreen = {
	"std", 0, 0,
	NULL, /* textops */
	8, 16,
	WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
	NULL
};

static const struct wsscreen_descr *_vidcvideo_scrlist[] = {
	&vidcvideo_stdscreen,
};

static const struct wsscreen_list vidcvideo_screenlist = {
	sizeof(_vidcvideo_scrlist) / sizeof(struct wsscreen_descr *),
	_vidcvideo_scrlist
};

static int	vidcvideoioctl(void *, void *, u_long, void *, int,
    struct lwp *);
static paddr_t	vidcvideommap(void *, void *, off_t, int);
static void	vidcvideoinit_screen(void *, struct vcons_screen *, int, long *);

static void	vidcvideo_queue_dc_change(struct fb_devconfig*, int);
static int	flush_dc_changes_to_screen(struct fb_devconfig*);

static struct wsdisplay_accessops vidcvideo_accessops = {
	vidcvideoioctl,
	vidcvideommap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL,	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};


/* Function prototypes */
int         vidcvideo_cnattach(vaddr_t);
static void vidcvideo_colourmap_and_cursor_init(struct fb_devconfig *);

static int  get_cmap(struct vidcvideo_softc *, struct wsdisplay_cmap *);
static int  set_cmap(struct vidcvideo_softc *, struct wsdisplay_cmap *);
static int  set_cursor(struct vidcvideo_softc *, struct wsdisplay_cursor *);
static int  get_cursor(struct vidcvideo_softc *, struct wsdisplay_cursor *);
static void set_curpos(struct vidcvideo_softc *, struct wsdisplay_curpos *);
static void vidcvideo_getdevconfig(vaddr_t, u_int, struct fb_devconfig *);

static int  vidcvideointr(void *);

/* Acceleration function prototypes */
static void vv_copyrows(void *, int, int, int);
static void vv_eraserows(void *, int, int, long);
static void vv_putchar(void *c, int row, int col, u_int uc, long attr);


static int
vidcvideo_match(struct device *parent, struct cfdata *match, void *aux)
{

	/* Can't probe AFAIK ; how ? */
	return 1;
}


static void
vidcvideo_getdevconfig(vaddr_t dense_addr, u_int mem_size,
		struct fb_devconfig *dc)
{

	dc->dc_vaddr = dense_addr;
	(void) pmap_extract(pmap_kernel(), dc->dc_vaddr, &(dc->dc_paddr));

	vidcvideo_getmode(&dc->mode_info);

	dc->dc_width = dc->mode_info.timings.hdisplay;
	dc->dc_height = dc->mode_info.timings.vdisplay;
	dc->dc_log2_depth = dc->mode_info.log2_bpp;
	dc->dc_depth = 1 << dc->dc_log2_depth;
	dc->dc_videobase = dc->dc_vaddr;
	dc->dc_blanked = 0;

	/* this should/could be done somewhat more elegant! */
	switch (dc->dc_depth) {
		case 1:
			dc->dc_rowbytes = dc->dc_width / 8;
			break;
		case 2:
			dc->dc_rowbytes = dc->dc_width / 4;
			break;
		case 4:
			dc->dc_rowbytes = dc->dc_width / 2;
			break;
		case 8:
			dc->dc_rowbytes = dc->dc_width;
			break;
		case 16:
			dc->dc_rowbytes = dc->dc_width * 2;
			break;
		case 32:
			dc->dc_rowbytes = dc->dc_width * 4;
			break;
		default:
			printf("Unknown colour depth %d ... what to do ?", dc->dc_depth);
			break;
	}

	/* setup the correct size */
	dc->dc_size = mem_size;

	/* initialize colormap and cursor resource */
	vidcvideo_colourmap_and_cursor_init(dc);

	/* blank the memory */
	memset((void*)dc->dc_vaddr, 0, dc->dc_size);

	/* intitialise miscelanious */
	dc->dc_writeback_delay = 0;
}

static void
vidcvideoinit_screen(void *cookie, struct vcons_screen *scr,
		    int existing, long *defattr)
{
	struct rasops_info *ri = &scr->scr_ri;
	struct fb_devconfig *dc = cookie;

	if ((scr == &dc->dc_console) && (dc->dc_vd.active != NULL))
		return;

	ri->ri_flg    = 0; /* RI_CENTER | RI_FULLCLEAR; */
	ri->ri_depth  = dc->dc_depth;
	ri->ri_bits   = (void *) dc->dc_videobase;
	ri->ri_width  = dc->dc_width;
	ri->ri_height = dc->dc_height;
	ri->ri_stride = dc->dc_rowbytes;
	ri->ri_hw     = &dc->dc_console;	/* link back */

	rasops_init(ri,
	    ri->ri_height / 8,
	    ri->ri_width / 8);

	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri,
	    ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);

	/*
	 * Provide a hook for the acceleration functions and make a copy of the
	 * original rasops functions for passing on calls
	 */
	memcpy(&(dc->orig_ri_ops), &(ri->ri_ops),
	    sizeof(struct wsdisplay_emulops));

	/* add our accelerated functions */
	ri->ri_ops.eraserows = vv_eraserows;
	ri->ri_ops.copyrows  = vv_copyrows;

	/* add the extra activity measuring functions; they just delegate on */
	ri->ri_ops.putchar   = vv_putchar;

	vidcvideo_stdscreen.nrows = ri->ri_rows;
	vidcvideo_stdscreen.ncols = ri->ri_cols;
	vidcvideo_stdscreen.textops = &ri->ri_ops;
	vidcvideo_stdscreen.capabilities = ri->ri_caps;
}

static void
vidcvideo_attach(struct device *parent, struct device *self, void *aux)
{
	struct vidcvideo_softc *sc = (struct vidcvideo_softc *)self;
	struct fb_devconfig *dc;
	struct wsemuldisplaydev_attach_args waa;
	long defattr;

	dc = sc->sc_dc = &vidcvideo_console_dc;

	/*
	 * for reasons which are crazy we init vidcvideo twice,
	 * the second time sets up the cursor
	 */
	vidcvideo_init();
	if (!vidcvideo_is_console) {
		vidcvideo_getdevconfig(videomemory.vidm_vbase, 
				videomemory.vidm_size,
			       	sc->sc_dc);
	}

	vcons_init(&dc->dc_vd, dc, &vidcvideo_stdscreen, &vidcvideo_accessops);
	dc->dc_vd.init_screen = vidcvideoinit_screen;

	vcons_init_screen(&dc->dc_vd, &dc->dc_console, 1, &defattr);

	dc->dc_console.scr_flags |= VCONS_SCREEN_IS_STATIC;

	vidcvideo_printdetails();
	printf(": mode %s, %dbpp\n", dc->mode_info.timings.name,
	    dc->dc_depth);

	/* set up interrupt flags */
	vidcvideo_queue_dc_change(dc, WSDISPLAY_CMAP_DOLUT);

	/* Establish an interrupt handler, and clear any pending interrupts */
	dc->dc_ih = intr_claim(IRQ_FLYBACK, IPL_TTY, "vblank", vidcvideointr, dc);

	waa.console = (vidcvideo_is_console ? 1 : 0);
	waa.scrdata = &vidcvideo_screenlist;
	waa.accessops = &vidcvideo_accessops;
	waa.accesscookie = &dc->dc_vd;

	config_found(self, &waa, wsemuldisplaydevprint);
}


static int
vidcvideoioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct vcons_data *vd = v;
	struct vidcvideo_softc *sc = vd->cookie;
	struct fb_devconfig *dc = sc->sc_dc;
	struct vcons_screen *ms = vd->active;
	int state;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_VIDC;
		return 0;

	case WSDISPLAYIO_GINFO:
		if (ms == NULL)
			return ENODEV;
#define	wsd_fbip ((struct wsdisplay_fbinfo *)data)
		wsd_fbip->height = dc->dc_height;
		wsd_fbip->width  = dc->dc_width;
		wsd_fbip->depth  = dc->dc_depth;
		wsd_fbip->cmsize = CMAP_SIZE;
#undef fbt
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return get_cmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return set_cmap(sc, (struct wsdisplay_cmap *)data);
	
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = dc->dc_rowbytes;
		return 0;

	case WSDISPLAYIO_SVIDEO:
		state = *(int *)data;
		dc->dc_blanked = (state == WSDISPLAYIO_VIDEO_OFF);
		vidcvideo_queue_dc_change(dc, WSDISPLAY_VIDEO_ONOFF);
		/* done on video blank */
		return 0;

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = dc->dc_blanked ?
		    WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON;
		return 0;

	case WSDISPLAYIO_GCURPOS:
		*(struct wsdisplay_curpos *)data = dc->dc_cursor.cc_pos;
		return 0;

	case WSDISPLAYIO_SCURPOS:
		set_curpos(sc, (struct wsdisplay_curpos *)data);
		vidcvideo_queue_dc_change(dc, WSDISPLAY_CURSOR_DOPOS);
		return 0;

	case WSDISPLAYIO_GCURMAX:
		((struct wsdisplay_curpos *)data)->x = CURSOR_MAX_WIDTH;
		((struct wsdisplay_curpos *)data)->y = CURSOR_MAX_HEIGHT;
		return 0;

	case WSDISPLAYIO_GCURSOR:
		return get_cursor(sc, (struct wsdisplay_cursor *)data);

	case WSDISPLAYIO_SCURSOR:
		return set_cursor(sc, (struct wsdisplay_cursor *)data);

	case WSDISPLAYIO_SMODE:
		state = *(int *)data;
		if (state == WSDISPLAYIO_MODE_MAPPED)
			dc->dc_hwscroll_cookie = vidcvideo_hwscroll_reset();
		if (state == WSDISPLAYIO_MODE_EMUL)
			vidcvideo_hwscroll_back(dc->dc_hwscroll_cookie);
		vidcvideo_progr_scroll();

		return 0;
	}
	return EPASSTHROUGH;
}


paddr_t
vidcvideommap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct vidcvideo_softc *sc = vd->cookie;

	if (offset >= sc->sc_dc->dc_size || offset < 0)
		return -1;

	return arm_btop(sc->sc_dc->dc_paddr + offset);
}


/* EXPORT */ int
vidcvideo_cnattach(vaddr_t addr)
{
	struct fb_devconfig *dc = &vidcvideo_console_dc;
	struct rasops_info *ri;
	long defattr;

	vidcvideo_init();

	/* fetch current framebuffer config */
	vidcvideo_getdevconfig(videomemory.vidm_vbase,
			videomemory.vidm_size,
		       	dc);

	dc->dc_vd.active = NULL;
	vidcvideoinit_screen(dc, &dc->dc_console, 1, &defattr);

	ri = &(dc->dc_console.scr_ri);
	ri->ri_hw = &dc->dc_console;
	dc->dc_console.scr_cookie = dc;

	(*ri->ri_ops.allocattr)(ri,
				WS_DEFAULT_FG, /* fg */
				WS_DEFAULT_BG, /* bg */
				0,           /* wsattrs */
				&defattr);

	wsdisplay_cnattach(&vidcvideo_stdscreen,
			   ri,   /* emulcookie */
		       	   0, 0, /* cursor position */
			   defattr);
	
	vidcvideo_is_console = true;

	return 0;
}


static int
vidcvideointr(void *arg)
{
	struct fb_devconfig *dc = arg;

	return flush_dc_changes_to_screen(dc);
}

static int
flush_dc_changes_to_screen(struct fb_devconfig *dc)
{
	int v, cleared = 0;

	v = dc->_internal_dc_changed;

	if (v == 0) {
		disable_irq(IRQ_FLYBACK);
		return 1;
	}

	if (v & WSDISPLAY_WB_COUNTER) {
	    	dc->dc_writeback_delay--;
		if (dc->dc_writeback_delay == 0) {
		    	cpu_dcache_wb_range(dc->dc_vaddr, dc->dc_size);
			cleared |= WSDISPLAY_WB_COUNTER;
		}
	}

	if (v & WSDISPLAY_CMAP_DOLUT) {
		struct hwcmap256 *cm = &dc->dc_cmap;
		int index;

		if (dc->dc_depth == 4) {
			/* palette for 4 bpp is different from 8bpp */
			vidcvideo_write(VIDC_PALREG, 0x00000000);
			for (index=0; index < (1 << dc->dc_depth); index++)
				vidcvideo_write(VIDC_PALETTE,
					VIDC_COL(cm->r[index],
						 cm->g[index],
						 cm->b[index]));
		}

		if (dc->dc_depth == 8) {
			/*
			 * dunno what to do in more than 8bpp 
			 * palettes only make sense in 8bpp and less modes
			 * on VIDC
			 */
			vidcvideo_write(VIDC_PALREG, 0x00000000);
			for (index = 0; index < CMAP_SIZE; index++) {
				vidcvideo_write(VIDC_PALETTE,
					VIDC_COL(cm->r[index], cm->g[index],
					    cm->b[index]));
			}
		}
		cleared |= WSDISPLAY_CMAP_DOLUT;
	}

	if (v & WSDISPLAY_VIDEO_ONOFF) {
		vidcvideo_blank(dc->dc_blanked);
		cleared |= WSDISPLAY_VIDEO_ONOFF;
	}

	if (v & (WSDISPLAY_CURSOR_DOPOS | WSDISPLAY_CURSOR_DOHOT)) {
		int x, y;
		x = dc->dc_cursor.cc_pos.x - dc->dc_cursor.cc_hot.x;
		y = dc->dc_cursor.cc_pos.y - dc->dc_cursor.cc_hot.y;

		vidcvideo_updatecursor(x, y);
		cleared |= WSDISPLAY_CURSOR_DOPOS | WSDISPLAY_CURSOR_DOHOT;
	}

	if (v & WSDISPLAY_CURSOR_DOCUR) {
		vidcvideo_enablecursor(dc->dc_curenb);
		cleared |= WSDISPLAY_CURSOR_DOCUR;
	}

	dc->_internal_dc_changed ^= cleared;

	if (dc->_internal_dc_changed == 0) {
		disable_irq(IRQ_FLYBACK);
	}

	return 1;
}


static void vidcvideo_queue_dc_change(struct fb_devconfig *dc, int dc_change)
{
	dc->_internal_dc_changed |= dc_change;

	if (curcpl() == IPL_HIGH) {
		/* running in ddb or without interrupts */
	    	dc->dc_writeback_delay = 1;
		flush_dc_changes_to_screen(dc);
	} else {
		/*
		 * running with interrupts so handle this in the next
		 * vsync
		 */ 
		if (dc->dc_ih) {
			enable_irq(IRQ_FLYBACK);
		}
	}
}


static const u_char ri_col_data[6][6] = {
	{ 0,  0,  0,   0,  0,  0},	/*  1 bpp */
	{ 0,  0,  0,   0,  0,  0},	/*  2 bpp */
	{ 0,  0,  0,   0,  0,  0},	/*  4 bpp */
	{ 0,  0,  0,   0,  0,  0},	/*  8 bpp */
	{ 6,  5,  5,   0,  6, 11},	/* 16 bpp */
	{ 8,  8,  8,   0,  8, 16},	/* 32 bpp */
};

static void
vidcvideo_colourmap_and_cursor_init(struct fb_devconfig *dc)
{
	struct rasops_info *ri = &dc->dc_console.scr_ri;
	const u_char *rgbdat;
	struct hwcmap256 *cm;
	const u_int8_t *p;
	int index;

	/* Whatever we do later... just make sure we have a
	 * sane palette to start with
	 */
	vidcvideo_stdpalette();

	/* set up rgb bit pattern values for rasops_init */
	rgbdat = ri_col_data[dc->dc_log2_depth];
	ri->ri_rnum = rgbdat[0];
	ri->ri_gnum = rgbdat[1];
	ri->ri_bnum = rgbdat[2];
	ri->ri_rpos = rgbdat[3];
	ri->ri_gpos = rgbdat[4];
	ri->ri_bpos = rgbdat[5];

	/* initialise color map */
	cm = &dc->dc_cmap;
	p = rasops_cmap;
	for (index = 0; index < CMAP_SIZE; index++, p += 3) {
		cm->r[index] = p[0];
		cm->g[index] = p[1];
		cm->b[index] = p[2];
	}
	/* flush to hardware */
	vidcvideo_queue_dc_change(dc, WSDISPLAY_CMAP_DOLUT);
}


static int
get_cmap(struct vidcvideo_softc *sc, struct wsdisplay_cmap *p)
{
	u_int index = p->index, count = p->count;
	int error;

	if (index >= CMAP_SIZE || count > CMAP_SIZE - index)
		return EINVAL;

	error = copyout(&sc->sc_dc->dc_cmap.r[index], p->red, count);
	if (error)
		return error;
	error = copyout(&sc->sc_dc->dc_cmap.g[index], p->green, count);
	if (error)
		return error;
	error = copyout(&sc->sc_dc->dc_cmap.b[index], p->blue, count);
	return error;
}


static int
set_cmap(struct vidcvideo_softc *sc, struct wsdisplay_cmap *p)
{
    	struct fb_devconfig *dc = sc->sc_dc;
	struct hwcmap256 cmap;
	u_int index = p->index, count = p->count;
	int error;

	if (index >= CMAP_SIZE || (index + count) > CMAP_SIZE)
		return EINVAL;

	error = copyin(p->red, &cmap.r[index], count);
	if (error)
		return error;
	error = copyin(p->green, &cmap.g[index], count);
	if (error)
		return error;
	error = copyin(p->blue, &cmap.b[index], count);
	if (error)
		return error;
	memcpy(&dc->dc_cmap.r[index], &cmap.r[index], count);
	memcpy(&dc->dc_cmap.g[index], &cmap.g[index], count);
	memcpy(&dc->dc_cmap.b[index], &cmap.b[index], count);
	vidcvideo_queue_dc_change(dc, WSDISPLAY_CMAP_DOLUT);
	return 0;
}


static int
set_cursor(struct vidcvideo_softc *sc, struct wsdisplay_cursor *p)
{
#define	cc (&dc->dc_cursor)
    	struct fb_devconfig *dc = sc->sc_dc;
	u_int v, index = 0, count = 0, icount = 0;
	uint8_t r[2], g[2], b[2], image[512], mask[512];
	int error;

	/* XXX gcc does not detect identical conditions */
	index = count = icount = 0;

	v = p->which;
	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		index = p->cmap.index;
		count = p->cmap.count;
		if (index >= CURSOR_MAX_COLOURS ||
		    (index + count) > CURSOR_MAX_COLOURS)
			return EINVAL;
		error = copyin(p->cmap.red, &r[index], count);
		if (error)
			return error;
		error = copyin(p->cmap.green, &g[index], count);
		if (error)
			return error;
		error = copyin(p->cmap.blue, &b[index], count);
		if (error)
			return error;
	}
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		if (p->size.x > CURSOR_MAX_WIDTH ||
		    p->size.y > CURSOR_MAX_HEIGHT)
			return EINVAL;
		icount = sizeof(u_int32_t) * p->size.y;
		error = copyin(p->image, &image, icount);
		if (error)
			return error;
		error = copyin(p->mask, &mask, icount);
		if (error)
			return error;
	}

	if (v & WSDISPLAY_CURSOR_DOCUR) 
		dc->dc_curenb = p->enable;
	if (v & WSDISPLAY_CURSOR_DOPOS)
		set_curpos(sc, &p->pos);
	if (v & WSDISPLAY_CURSOR_DOHOT)
		cc->cc_hot = p->hot;
	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		memcpy(&cc->cc_color[index], &r[index], count);
		memcpy(&cc->cc_color[index + 2], &g[index], count);
		memcpy(&cc->cc_color[index + 4], &b[index], count);
	}
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		cc->cc_size = p->size;
		memset(cc->cc_image, 0, sizeof cc->cc_image);
		memcpy(cc->cc_image, image, icount);
		memset(cc->cc_mask, 0, sizeof cc->cc_mask);
		memcpy(cc->cc_mask, mask, icount);
	}
	vidcvideo_queue_dc_change(dc, v);

	return 0;
#undef cc
}


static int
get_cursor(struct vidcvideo_softc *sc, struct wsdisplay_cursor *p)
{

	return EPASSTHROUGH; /* XXX */
}


static void
set_curpos(struct vidcvideo_softc *sc, struct wsdisplay_curpos *curpos)
{
	struct fb_devconfig *dc = sc->sc_dc;
	int x = curpos->x, y = curpos->y;

	if (y < 0)
		y = 0;
	else if (y > dc->dc_height)
		y = dc->dc_height;
	if (x < 0)
		x = 0;
	else if (x > dc->dc_width)
		x = dc->dc_width;
	dc->dc_cursor.cc_pos.x = x;
	dc->dc_cursor.cc_pos.y = y;
}


static void vv_copyrows(void *id, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = id;
	int height, offset, size;
	int scrollup, scrolldown;
	unsigned char *src, *dst;
	struct vcons_screen *scr = ri->ri_hw;
	struct fb_devconfig *dc = (struct fb_devconfig *) (scr->scr_cookie);

	/* All movements are done in multiples of character heigths */
	height = ri->ri_font->fontheight * nrows;
	offset = (srcrow - dstrow) * ri->ri_yscale;
	size   = height * ri->ri_stride;

	/* check if we are full screen scrolling */
	scrollup   = (srcrow + nrows >= ri->ri_rows);
	scrolldown = (dstrow + nrows >= ri->ri_rows);

#if 0
	if ((scrollup || scrolldown) &&
	    (videomemory.vidm_type == VIDEOMEM_TYPE_VRAM)) {
		ri->ri_bits = vidcvideo_hwscroll(offset);
		vidcvideo_progr_scroll();	/* sadistic ; shouldnt this be on vsync? */

		/* wipe out remains of the screen if nessisary */
		if (ri->ri_emuheight != ri->ri_height)
			vv_eraserows(id, ri->ri_rows, 1, 0);
		return;
	}
#endif

	/*
	 * Else we just copy the area : we're braindead for now 
	 * Note: we can't use hardware scrolling when the softc isnt
	 * known yet...  if its not known we dont have interrupts and
	 * we can't change the display address reliable other than in
	 * a Vsync
	 */

	src = ri->ri_bits + srcrow * ri->ri_font->fontheight * ri->ri_stride;
	dst = ri->ri_bits + dstrow * ri->ri_font->fontheight * ri->ri_stride;

	memmove(dst, src, size);

	/* delay the write back operation of the screen area */
	dc->dc_writeback_delay = SCREEN_WRITE_BACK_DELAY;
	vidcvideo_queue_dc_change(dc, WSDISPLAY_WB_COUNTER);
}


static void vv_eraserows(void *id, int startrow, int nrows, long attr)
{
	struct rasops_info *ri = id;
	int height;
	unsigned char *src;
	struct vcons_screen *scr = ri->ri_hw;
	struct fb_devconfig *dc = (struct fb_devconfig *) (scr->scr_cookie);

	/* we're braindead for now */
	height = ri->ri_font->fontheight * nrows * ri->ri_stride;

	src = ri->ri_bits + startrow * ri->ri_font->fontheight * ri->ri_stride;

	memset(src, 0, height);

	/* delay the write back operation of the screen area */
	dc->dc_writeback_delay = SCREEN_WRITE_BACK_DELAY;
	vidcvideo_queue_dc_change(dc, WSDISPLAY_WB_COUNTER);
}


static void vv_putchar(void *id, int row, int col, u_int uc, long attr)
{
    	struct rasops_info *ri = id;
	struct vcons_screen *scr = ri->ri_hw;
	struct fb_devconfig *dc = (struct fb_devconfig *) (scr->scr_cookie);

	/* just delegate */
	dc->orig_ri_ops.putchar(id, row, col, uc, attr);

	/* delay the write back operation of the screen area */
	dc->dc_writeback_delay = SCREEN_WRITE_BACK_DELAY;
	vidcvideo_queue_dc_change(dc, WSDISPLAY_WB_COUNTER);
}
