/* $NetBSD: vidcvideo.c,v 1.9 2001/08/21 14:48:55 reinoud Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: vidcvideo.c,v 1.9 2001/08/21 14:48:55 reinoud Exp $");

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

#include <uvm/uvm_extern.h>
#include <arm/arm32/pmap.h>
#include <machine/irqhandler.h>

/* for vidc_mode ... needs to be MI indepenent one day */
#include <machine/vidc.h>
#include <arm32/vidc/vidc20config.h>
#include <machine/bootconfig.h>
extern videomemory_t videomemory;

#define machine_btop(x) arm_byte_to_page(x)
#define MACHINE_KSEG0_TO_PHYS(x) vtophys(x)

/* FOR DEBUG */
extern videomemory_t videomemory;

struct fb_devconfig {
	vaddr_t dc_vaddr;		/* memory space virtual base address */
	paddr_t dc_paddr;		/* memory space physical base address */
	vsize_t dc_size;		/* size of slot memory */
	int	dc_wid;			/* width of frame buffer */
	int	dc_ht;			/* height of frame buffer */
	int	dc_log2_depth;		/* log2 of bits per pixel */
	int	dc_depth;		/* depth, bits per pixel */
	int	dc_rowbytes;		/* bytes in a FB scan line */
	vaddr_t	dc_videobase;		/* base of flat frame buffer */
	int	dc_blanked;		/* currently has video disabled */
	void   *dc_hwscroll_cookie;	/* cookie for hardware scroll */

	struct vidc_mode   mode_info;
	struct rasops_info rinfo;
};


struct hwcmap256 {
#define	CMAP_SIZE	256	/* 256 R/G/B entries */
	u_int8_t r[CMAP_SIZE];
	u_int8_t g[CMAP_SIZE];
	u_int8_t b[CMAP_SIZE];
};


/* XXX for CURSOR_MAX_WIDTH = 32 */
struct hwcursor32 {
	struct wsdisplay_curpos cc_pos;
	struct wsdisplay_curpos cc_hot;
	struct wsdisplay_curpos cc_size;
	struct wsdisplay_curpos cc_magic;
	u_int8_t cc_color[6];		/* how many? */
	u_int32_t cc_image[(CURSOR_MAX_WIDTH/4) * CURSOR_MAX_HEIGHT];
	u_int32_t cc_mask[(CURSOR_MAX_WIDTH/4) * CURSOR_MAX_HEIGHT];
};


struct vidcvideo_softc {
	struct device sc_dev;
	struct fb_devconfig *sc_dc;	/* device configuration */
	struct hwcmap256 sc_cmap;	/* software copy of colormap */
	struct hwcursor32 sc_cursor;	/* software copy of cursor */
	int sc_curenb;			/* cursor sprite enabled */
	int sc_changed;			/* need update of hardware */
#define	WSDISPLAY_CMAP_DOLUT	0x20
#define WSDISPLAY_VIDEO_ONOFF	0x40
	int nscreens;
};


/* XXX has to go XXX */
#define	CX_MAGIC_X	220
#define	CX_MAGIC_Y 	35
#define	CX_FB_OFFSET	0x000000
#define	CX_FB_SIZE	0x100000
#define	CX_BT459_OFFSET	0x200000
#define	CX_OFFSET_IREQ	0x300000	/* Interrupt req. control */
/* XXX till here XXX */


/* Function prototypes for glue */
static int  vidcvideo_match __P((struct device *, struct cfdata *, void *));
static void vidcvideo_attach __P((struct device *, struct device *, void *));


/* config glue */
const struct cfattach vidcvideo_ca = {
	sizeof(struct vidcvideo_softc), vidcvideo_match, vidcvideo_attach,
};


static struct fb_devconfig vidcvideo_console_dc;
static int vidcvideo_is_console;


static struct wsscreen_descr vidcvideo_stdscreen = {
	"std", 0, 0,
	0, /* textops */
	0, 0,
	WSSCREEN_REVERSE
};

static const struct wsscreen_descr *_vidcvideo_scrlist[] = {
	&vidcvideo_stdscreen,
};

static const struct wsscreen_list vidcvideo_screenlist = {
	sizeof(_vidcvideo_scrlist) / sizeof(struct wsscreen_descr *), _vidcvideo_scrlist
};

static int	vidcvideoioctl __P((void *, u_long, caddr_t, int, struct proc *));
static paddr_t	vidcvideommap __P((void *, off_t, int));

static int	vidcvideo_alloc_screen __P((void *, const struct wsscreen_descr *,
				      void **, int *, int *, long *));
static void	vidcvideo_free_screen __P((void *, void *));
static int	vidcvideo_show_screen __P((void *, void *, int,
				     void (*) (void *, int, int), void *));

static const struct wsdisplay_accessops vidcvideo_accessops = {
	vidcvideoioctl,
	vidcvideommap,
	vidcvideo_alloc_screen,
	vidcvideo_free_screen,
	vidcvideo_show_screen,
	0 /* load_font */
};

/* Function prototypes */
int         vidcvideo_cnattach __P((vaddr_t));
static void vidcvideo_colourmap_and_cursor_init __P((struct fb_devconfig *));

static int  get_cmap __P((struct vidcvideo_softc *, struct wsdisplay_cmap *));
static int  set_cmap __P((struct vidcvideo_softc *, struct wsdisplay_cmap *));
static int  set_cursor __P((struct vidcvideo_softc *, struct wsdisplay_cursor *));
static int  get_cursor __P((struct vidcvideo_softc *, struct wsdisplay_cursor *));
static void set_curpos __P((struct vidcvideo_softc *, struct wsdisplay_curpos *));
static void vidcvideo_getdevconfig __P((vaddr_t, struct fb_devconfig *));

static int  vidcvideointr __P((void *));

/* Acceleration function prototypes */
static void vv_copyrows __P((void *, int, int, int));
static void vv_eraserows __P((void *, int, int, long));


static int
vidcvideo_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	/* Can't probe AFAIK ; how ? */
	return (1);
}


static void
vidcvideo_getdevconfig(dense_addr, dc)
	vaddr_t dense_addr;
	struct fb_devconfig *dc;
{
	dc->dc_vaddr = dense_addr;
	dc->dc_paddr = MACHINE_KSEG0_TO_PHYS(dc->dc_vaddr);

	vidcvideo_getmode(&dc->mode_info);

	dc->dc_wid = dc->mode_info.hder;
	dc->dc_ht = dc->mode_info.vder;
	dc->dc_log2_depth = dc->mode_info.log2_bpp;
	dc->dc_depth = 1 << dc->dc_log2_depth;
	dc->dc_videobase = dc->dc_vaddr;
	dc->dc_blanked = 0;

	/* this should/could be done somewhat more elegant! */
	switch (dc->dc_depth) {
		case 1:
			dc->dc_rowbytes = dc->dc_wid / 8;
			break;
		case 2:
			dc->dc_rowbytes = dc->dc_wid / 4;
			break;
		case 4:
			dc->dc_rowbytes = dc->dc_wid / 2;
			break;
		case 8:
			dc->dc_rowbytes = dc->dc_wid;
			break;
		case 16:
			dc->dc_rowbytes = dc->dc_wid * 2;
			break;
		case 32:
			dc->dc_rowbytes = dc->dc_wid * 4;
			break;
		default:
			printf("Unknown colour depth %d ... what to do ?", dc->dc_depth);
			break;
	};

	/* euhm... correct ? i.e. not complete VIDC memory */
	dc->dc_size = dc->mode_info.vder * dc->dc_rowbytes;

	/* initialize colormap and cursor resource */
	vidcvideo_colourmap_and_cursor_init(dc);

	dc->rinfo.ri_flg    = 0; /* RI_CENTER; */
	dc->rinfo.ri_depth  = dc->dc_depth;
	dc->rinfo.ri_bits   = (void *) dc->dc_videobase;
	dc->rinfo.ri_width  = dc->dc_wid;
	dc->rinfo.ri_height = dc->dc_ht;
	dc->rinfo.ri_stride = dc->dc_rowbytes;
	dc->rinfo.ri_hw	    = NULL;
}


static void
vidcvideo_config_wscons(dc)
	struct fb_devconfig *dc;
{
	int i, cookie, font_locked;

	/* clear the screen ; why not a memset ? - it was this way so keep it for now */
	for (i = 0; i < dc->dc_ht * dc->dc_rowbytes; i += sizeof(u_int32_t))
		*(u_int32_t *)(dc->dc_videobase + i) = 0x0;

	wsfont_init();

	/* prefer 8 pixel wide font */
	if ((cookie = wsfont_find(NULL, 8, 0, 0)) <= 0)
		cookie = wsfont_find(NULL, 0, 0, 0);

	if (cookie < 0) {
		/* Can I even print here ? */
		printf("Font table empty! exiting\n");
		return;
	};

	font_locked = wsfont_lock(cookie, &dc->rinfo.ri_font,
		WSDISPLAY_FONTORDER_L2R, WSDISPLAY_FONTORDER_L2R);

	dc->rinfo.ri_wsfcookie = cookie;

	rasops_init(&dc->rinfo,
		dc->rinfo.ri_height / dc->rinfo.ri_font->fontheight,
		dc->rinfo.ri_width / dc->rinfo.ri_font->fontwidth
	);

	/* XXX add our accelerated functions */
	dc->rinfo.ri_ops.eraserows = vv_eraserows;
	dc->rinfo.ri_ops.copyrows  = vv_copyrows;

	/* XXX shouldn't be global */
	vidcvideo_stdscreen.nrows = dc->rinfo.ri_rows;
	vidcvideo_stdscreen.ncols = dc->rinfo.ri_cols;
	vidcvideo_stdscreen.textops = &dc->rinfo.ri_ops;
	vidcvideo_stdscreen.capabilities = dc->rinfo.ri_caps;

	if (font_locked < 0) {
		printf(" warning ... couldn't lock font! ");
	};
}


static void
vidcvideo_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct vidcvideo_softc *sc = (struct vidcvideo_softc *)self;
	struct wsemuldisplaydev_attach_args waa;
	struct hwcmap256 *cm;
	const u_int8_t *p;
	int index;

	vidcvideo_init();
	if (sc->nscreens == 0) {
		if (vidcvideo_is_console) {
			sc->sc_dc = &vidcvideo_console_dc;
		} else {
			printf(" : non console vidcvideo fb ... can't cope with this\n");
			return;
			/*
			 * sc->sc_dc = (struct fb_devconfig *)
			 *	   malloc(sizeof(struct fb_devconfig), M_DEVBUF, M_WAITOK);
			 * vidcvideo_getdevconfig(videomemory.vidm_vbase, sc->sc_dc);
			 */
		};
		sc->nscreens = 1;
	} else {
			printf(": allready attached ... can't cope with this\n");
			return;
	};

	vidcvideo_printdetails();
	printf(": using %d x %d, %dbpp\n", sc->sc_dc->dc_wid, sc->sc_dc->dc_ht,
	    sc->sc_dc->dc_depth);

	/* initialise rasops */
	cm = &sc->sc_cmap;
	p = rasops_cmap;
	for (index = 0; index < CMAP_SIZE; index++, p += 3) {
		cm->r[index] = p[0];
		cm->g[index] = p[1];
		cm->b[index] = p[2];
	}

	/* what does these do ? */
	sc->sc_cursor.cc_magic.x = CX_MAGIC_X;
	sc->sc_cursor.cc_magic.y = CX_MAGIC_Y;

	/* set up interrupt flags */
	sc->sc_changed |= WSDISPLAY_CMAP_DOLUT;

	/* set up a link in the rasops structure to our softc for acceleration stuff */
	sc->sc_dc->rinfo.ri_hw = sc;

	/* Establish an interrupt handler, and clear any pending interrupts */
	intr_claim(IRQ_FLYBACK, IPL_TTY, "vblank", vidcvideointr, sc);

	waa.console = (vidcvideo_is_console ? 1 : 0);
	waa.scrdata = &vidcvideo_screenlist;
	waa.accessops = &vidcvideo_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}


static int
vidcvideoioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct vidcvideo_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;
	int state;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_VIDC;
		return (0);

	case WSDISPLAYIO_GINFO:
#define	wsd_fbip ((struct wsdisplay_fbinfo *)data)
		wsd_fbip->height = sc->sc_dc->dc_ht;
		wsd_fbip->width = sc->sc_dc->dc_wid;
		wsd_fbip->depth = sc->sc_dc->dc_depth;
		wsd_fbip->cmsize = CMAP_SIZE;
#undef fbt
		return (0);

	case WSDISPLAYIO_GETCMAP:
		return get_cmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return set_cmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_SVIDEO:
		state = *(int *)data;
		dc->dc_blanked = (state == WSDISPLAYIO_VIDEO_OFF);
		sc->sc_changed |= WSDISPLAY_VIDEO_ONOFF;
		/* done on video blank */
		return (0);

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = dc->dc_blanked ?
		    WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON;
		return (0);

	case WSDISPLAYIO_GCURPOS:
		*(struct wsdisplay_curpos *)data = sc->sc_cursor.cc_pos;
		return (0);

	case WSDISPLAYIO_SCURPOS:
		set_curpos(sc, (struct wsdisplay_curpos *)data);
		sc->sc_changed |= WSDISPLAY_CURSOR_DOPOS;
		return (0);

	case WSDISPLAYIO_GCURMAX:
		((struct wsdisplay_curpos *)data)->x = CURSOR_MAX_WIDTH;
		((struct wsdisplay_curpos *)data)->y = CURSOR_MAX_HEIGHT;
		return (0);

	case WSDISPLAYIO_GCURSOR:
		return get_cursor(sc, (struct wsdisplay_cursor *)data);

	case WSDISPLAYIO_SCURSOR:
		return set_cursor(sc, (struct wsdisplay_cursor *)data);

	case WSDISPLAYIO_SMODE:
		state = *(int *)data;
		if (state == WSDISPLAYIO_MODE_MAPPED) {
			dc->dc_hwscroll_cookie = vidcvideo_hwscroll_reset();
		};
		if (state == WSDISPLAYIO_MODE_EMUL) {
			vidcvideo_hwscroll_back(dc->dc_hwscroll_cookie);
		};
		vidcvideo_progr_scroll();
		return (0);
	}
	return ENOTTY;
}


paddr_t
vidcvideommap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct vidcvideo_softc *sc = v;

	if (offset >= sc->sc_dc->dc_size || offset < 0)
		return (-1);
	return machine_btop(sc->sc_dc->dc_paddr + offset);
}


static int
vidcvideo_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct vidcvideo_softc *sc = v;
	long defattr;

	if (sc->nscreens > 0)
		return (ENOMEM);

	*cookiep = &sc->sc_dc->rinfo; /* one and only for now */
	*curxp = 0;
	*curyp = 0;
	(*sc->sc_dc->rinfo.ri_ops.alloc_attr)(&sc->sc_dc->rinfo, 0, 0, 0, &defattr);
	*attrp = defattr;
	sc->nscreens++;
	return (0);
}


static void
vidcvideo_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct vidcvideo_softc *sc = v;

	if (sc->sc_dc == &vidcvideo_console_dc)
		panic("vidcvideo_free_screen: console");

	sc->nscreens--;
}


static int
vidcvideo_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb) __P((void *, int, int));
	void *cbarg;
{

	return (0);
}


/* EXPORT */ int
vidcvideo_cnattach(addr)
	vaddr_t addr;
{
	struct fb_devconfig *dcp = &vidcvideo_console_dc;
	long defattr;

	vidcvideo_init();
	vidcvideo_getdevconfig(addr, dcp);
	vidcvideo_config_wscons(dcp);
	(*dcp->rinfo.ri_ops.alloc_attr)(&dcp->rinfo, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&vidcvideo_stdscreen, &dcp->rinfo, 0, 0, defattr);

	vidcvideo_is_console = 1;
	return(0);
}


static int
vidcvideointr(arg)
	void *arg;
{
	struct vidcvideo_softc *sc = arg;
	int v;

	v = sc->sc_changed;
	if (v == 0)
		return (1);

	if (v & WSDISPLAY_CMAP_DOLUT) {
		struct hwcmap256 *cm = &sc->sc_cmap;
		int index;

		if (sc->sc_dc->dc_depth == 4) {
			/* palette for 4 bpp is different from 8bpp */
			vidcvideo_write(VIDC_PALREG, 0x00000000);
			for (index=0; index < 1<<sc->sc_dc->dc_depth; index++)
				vidcvideo_write(VIDC_PALETTE,
					VIDC_COL(cm->r[index],
						 cm->g[index],
						 cm->b[index]));
			;
		};

		if (sc->sc_dc->dc_depth == 8) {
			/* XXX dunno what to do in less than 8bpp */
			/* palettes only make sense in 8bpp and less modes on VIDC */
			vidcvideo_write(VIDC_PALREG, 0x00000000);
			for (index = 0; index < CMAP_SIZE; index++) {
				vidcvideo_write(VIDC_PALETTE,
					VIDC_COL(cm->r[index], cm->g[index], cm->b[index])
		 		);
			};
		};
	}

	if (v & WSDISPLAY_VIDEO_ONOFF) {
		vidcvideo_blank(sc->sc_dc->dc_blanked);
	};

	if (v & (WSDISPLAY_CURSOR_DOPOS | WSDISPLAY_CURSOR_DOHOT)) {
		int x, y;
		x = sc->sc_cursor.cc_pos.x - sc->sc_cursor.cc_hot.x;
		y = sc->sc_cursor.cc_pos.y - sc->sc_cursor.cc_hot.y;

		vidcvideo_updatecursor(x, y);
	};

	if (v & WSDISPLAY_CURSOR_DOCUR) {
		vidcvideo_enablecursor(sc->sc_curenb);
	};


#if 0	/* XXX snip XXX */
	/* XXX kept here as an archive for now XXX */

	vdac = vidcvideobase + CX_BT459_OFFSET;
	v = sc->sc_changed;
	if (v & WSDISPLAY_CURSOR_DOCUR) {
		SELECT(vdac, BT459_IREG_CCR);
		REG(vdac, bt_reg) = (sc->sc_curenb) ? 0xc0 : 0x00;
	}
	if (v & (WSDISPLAY_CURSOR_DOPOS | WSDISPLAY_CURSOR_DOHOT)) {
		int x, y;

		x = sc->sc_cursor.cc_pos.x - sc->sc_cursor.cc_hot.x;
		y = sc->sc_cursor.cc_pos.y - sc->sc_cursor.cc_hot.y;

		x += sc->sc_cursor.cc_magic.x;
		y += sc->sc_cursor.cc_magic.y;

		SELECT(vdac, BT459_IREG_CURSOR_X_LOW);
		REG(vdac, bt_reg) = x;		tc_wmb();
		REG(vdac, bt_reg) = x >> 8;	tc_wmb();
		REG(vdac, bt_reg) = y;		tc_wmb();
		REG(vdac, bt_reg) = y >> 8;	tc_wmb();
	}
	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		u_int8_t *cp = sc->sc_cursor.cc_color;

		SELECT(vdac, BT459_IREG_CCOLOR_2);
		REG(vdac, bt_reg) = cp[1];	tc_wmb();
		REG(vdac, bt_reg) = cp[3];	tc_wmb();
		REG(vdac, bt_reg) = cp[5];	tc_wmb();

		REG(vdac, bt_reg) = cp[0];	tc_wmb();
		REG(vdac, bt_reg) = cp[2];	tc_wmb();
		REG(vdac, bt_reg) = cp[4];	tc_wmb();
	}
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		u_int8_t *ip, *mp, img, msk;
		u_int8_t u;
		int bcnt;

		ip = (u_int8_t *)sc->sc_cursor.cc_image;
		mp = (u_int8_t *)(sc->sc_cursor.cc_image + CURSOR_MAX_HEIGHT);

		bcnt = 0;
		SELECT(vdac, BT459_IREG_CRAM_BASE+0);
		/* 64 pixel scan line is consisted with 16 byte cursor ram */
		while (bcnt < sc->sc_cursor.cc_size.y * 16) {
			/* pad right half 32 pixel when smaller than 33 */
			if ((bcnt & 0x8) && sc->sc_cursor.cc_size.x < 33) {
				REG(vdac, bt_reg) = 0; tc_wmb();
				REG(vdac, bt_reg) = 0; tc_wmb();
			}
			else {
				img = *ip++;
				msk = *mp++;
				img &= msk;	/* cookie off image */
				u = (msk & 0x0f) << 4 | (img & 0x0f);
				REG(vdac, bt_reg) = shuffle[u];	tc_wmb();
				u = (msk & 0xf0) | (img & 0xf0) >> 4;
				REG(vdac, bt_reg) = shuffle[u];	tc_wmb();
			}
			bcnt += 2;
		}
		/* pad unoccupied scan lines */
		while (bcnt < CURSOR_MAX_HEIGHT * 16) {
			REG(vdac, bt_reg) = 0; tc_wmb();
			REG(vdac, bt_reg) = 0; tc_wmb();
			bcnt += 2;
		}
	}
#endif /* XXX snip XXX */

	sc->sc_changed = 0;
	return (1);
}


static u_char ri_col_data[6][6] = {
	{ 0,  0,  0,   0,  0,  0},	/*  1 bpp */
	{ 0,  0,  0,   0,  0,  0},	/*  2 bpp */
	{ 0,  0,  0,   0,  0,  0},	/*  4 bpp */
	{ 0,  0,  0,   0,  0,  0},	/*  8 bpp */
	{ 5,  5,  5,   0,  5, 10},	/* 16 bpp */
	{ 8,  8,  8,   0,  8, 16},	/* 32 bpp */
};

static void
vidcvideo_colourmap_and_cursor_init(dc)
	struct fb_devconfig *dc;
{
	struct rasops_info *ri = &dc->rinfo;
	u_char *rgbdat;

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

}


static int
get_cmap(sc, p)
	struct vidcvideo_softc *sc;
	struct wsdisplay_cmap *p;
{
	u_int index = p->index, count = p->count;

	if (index >= CMAP_SIZE || (index + count) > CMAP_SIZE)
		return (EINVAL);

	if (!uvm_useracc(p->red, count, B_WRITE) ||
	    !uvm_useracc(p->green, count, B_WRITE) ||
	    !uvm_useracc(p->blue, count, B_WRITE))
		return (EFAULT);

	copyout(&sc->sc_cmap.r[index], p->red, count);
	copyout(&sc->sc_cmap.g[index], p->green, count);
	copyout(&sc->sc_cmap.b[index], p->blue, count);

	return (0);
}


static int
set_cmap(sc, p)
	struct vidcvideo_softc *sc;
	struct wsdisplay_cmap *p;
{
	u_int index = p->index, count = p->count;

	if (index >= CMAP_SIZE || (index + count) > CMAP_SIZE)
		return (EINVAL);

	if (!uvm_useracc(p->red, count, B_READ) ||
	    !uvm_useracc(p->green, count, B_READ) ||
	    !uvm_useracc(p->blue, count, B_READ))
		return (EFAULT);

	copyin(p->red, &sc->sc_cmap.r[index], count);
	copyin(p->green, &sc->sc_cmap.g[index], count);
	copyin(p->blue, &sc->sc_cmap.b[index], count);
	sc->sc_changed |= WSDISPLAY_CMAP_DOLUT;
	return (0);
}


static int
set_cursor(sc, p)
	struct vidcvideo_softc *sc;
	struct wsdisplay_cursor *p;
{
#define	cc (&sc->sc_cursor)
	u_int v, index, count, icount;

	v = p->which;
	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		index = p->cmap.index;
		count = p->cmap.count;
		if (index >= CURSOR_MAX_COLOURS || (index + count) > CURSOR_MAX_COLOURS)
			return (EINVAL);
		if (!uvm_useracc(p->cmap.red, count, B_READ) ||
		    !uvm_useracc(p->cmap.green, count, B_READ) ||
		    !uvm_useracc(p->cmap.blue, count, B_READ))
			return (EFAULT);
	}
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		if (p->size.x > CURSOR_MAX_WIDTH || p->size.y > CURSOR_MAX_HEIGHT)
			return (EINVAL);
		icount = sizeof(u_int32_t) * p->size.y;
		if (!uvm_useracc(p->image, icount, B_READ) ||
		    !uvm_useracc(p->mask, icount, B_READ))
			return (EFAULT);
	}

	if (v & WSDISPLAY_CURSOR_DOCUR) 
		sc->sc_curenb = p->enable;
	if (v & WSDISPLAY_CURSOR_DOPOS)
		set_curpos(sc, &p->pos);
	if (v & WSDISPLAY_CURSOR_DOHOT)
		cc->cc_hot = p->hot;
	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		copyin(p->cmap.red, &cc->cc_color[index], count);
		copyin(p->cmap.green, &cc->cc_color[index + 2], count);
		copyin(p->cmap.blue, &cc->cc_color[index + 4], count);
	}
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		cc->cc_size = p->size;
		memset(cc->cc_image, 0, sizeof cc->cc_image);
		memset(cc->cc_mask, 0, sizeof cc->cc_mask);
		copyin(p->image, cc->cc_image, icount);
		copyin(p->mask, cc->cc_mask, icount);
	}
	sc->sc_changed |= v;

	return (0);
#undef cc
}


static int
get_cursor(sc, p)
	struct vidcvideo_softc *sc;
	struct wsdisplay_cursor *p;
{
	return (ENOTTY); /* XXX */
}


static void
set_curpos(sc, curpos)
	struct vidcvideo_softc *sc;
	struct wsdisplay_curpos *curpos;
{
	struct fb_devconfig *dc = sc->sc_dc;
	int x = curpos->x, y = curpos->y;

	if (y < 0)
		y = 0;
	else if (y > dc->dc_ht)
		y = dc->dc_ht;
	if (x < 0)
		x = 0;
	else if (x > dc->dc_wid)
		x = dc->dc_wid;
	sc->sc_cursor.cc_pos.x = x;
	sc->sc_cursor.cc_pos.y = y;
}


static void vv_copyrows(id, srcrow, dstrow, nrows)
	void *id;
	int srcrow, dstrow, nrows;
{
	struct rasops_info *ri = id;
	int height, offset, size;
	int scrollup, scrolldown;
	unsigned char *src, *dst;

	/* All movements are done in multiples of character heigths */
	height = ri->ri_font->fontheight * nrows;
	offset = (srcrow - dstrow) * ri->ri_yscale;
	size   = height * ri->ri_stride;

	/* check if we are full screen scrolling */
	scrollup   = (srcrow + nrows >= ri->ri_rows);
	scrolldown = (dstrow + nrows >= ri->ri_rows);

	if ((scrollup || scrolldown) && (videomemory.vidm_type == VIDEOMEM_TYPE_VRAM)) {
		ri->ri_bits = vidcvideo_hwscroll(offset);
		vidcvideo_progr_scroll();	/* sadistic ; shouldnt this be on vsync? */

		/* wipe out remains of the screen if nessisary */
		if (ri->ri_emuheight != ri->ri_height) vv_eraserows(id, ri->ri_rows, 1, NULL);
		return;
	};

	/* Else we just copy the area : we're braindead for now 
	 * Note: we can't use hardware scrolling when the softc isnt known yet...
	 * if its not known we dont have interrupts and we can't change the display
	 * address reliable other than in a Vsync
	 */

	src = ri->ri_bits + srcrow * ri->ri_font->fontheight * ri->ri_stride;
	dst = ri->ri_bits + dstrow * ri->ri_font->fontheight * ri->ri_stride;

	bcopy(src, dst, size);
}


static void vv_eraserows(id, startrow, nrows, attr)
	void *id;
	int startrow, nrows;
	long attr;
{
	struct rasops_info *ri = id;
	int height;
	unsigned char *src;

	/* we're braindead for now */
	height = ri->ri_font->fontheight * nrows * ri->ri_stride;

	src = ri->ri_bits + startrow * ri->ri_font->fontheight * ri->ri_stride;

	bzero(src, height);
}

