/* $NetBSD: sfbplus.c,v 1.4 2000/06/26 04:56:28 simonb Exp $ */

/*
 * Copyright (c) 1999 Tohru Nishimura.  All rights reserved.
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
 *	for the NetBSD Project.
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

__KERNEL_RCSID(0, "$NetBSD: sfbplus.c,v 1.4 2000/06/26 04:56:28 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <vm/vm.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/rcons/raster.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_raster.h>
#include <dev/wscons/wsdisplayvar.h>

#include <dev/tc/tcvar.h>
#include <dev/ic/bt459reg.h>	
#include <dev/ic/bt463reg.h>
#include <dev/tc/sfbreg.h>
#include <dev/pci/tgareg.h>

#include <uvm/uvm_extern.h>

#if defined(pmax)
#define	machine_btop(x) mips_btop(x)
#define	MACHINE_KSEG0_TO_PHYS(x) MIPS_KSEG1_TO_PHYS(x)
#endif

#if defined(__alpha__) || defined(alpha)
#define machine_btop(x) alpha_btop(x)
#define MACHINE_KSEG0_TO_PHYS(x) ALPHA_K0SEG_TO_PHYS(x)
#endif

/* Bt459/Bt463 hardware registers */
#define bt_lo	0
#define bt_hi	1
#define bt_reg	2
#define bt_cmap 3

#define REG(base, index)	*((u_int32_t *)(base) + (index))
#define SELECT(vdac, regno) do {			\
	REG(vdac, bt_lo) = ((regno) & 0x00ff);		\
	REG(vdac, bt_hi) = ((regno) & 0x0f00) >> 8;	\
	tc_wmb();					\
   } while (0)

struct fb_devconfig {
	vaddr_t dc_vaddr;		/* memory space virtual base address */
	paddr_t dc_paddr;		/* memory space physical base address */
	vsize_t dc_size;		/* size of slot memory */
	int	dc_wid;			/* width of frame buffer */
	int	dc_ht;			/* height of frame buffer */
	int	dc_depth;		/* depth, bits per pixel */
	int	dc_rowbytes;		/* bytes in a FB scan line */
	vaddr_t	dc_videobase;		/* base of flat frame buffer */
	struct raster	dc_raster;	/* raster description */
	struct rcons	dc_rcons;	/* raster blitter control info */
	int	    dc_blanked;		/* currently has video disabled */
};

struct hwcmap256 {
#define	CMAP_SIZE	256	/* 256 R/G/B entries */
	u_int8_t r[CMAP_SIZE];
	u_int8_t g[CMAP_SIZE];
	u_int8_t b[CMAP_SIZE];
};

struct hwcursor64 {
	struct wsdisplay_curpos cc_pos;
	struct wsdisplay_curpos cc_hot;
	struct wsdisplay_curpos cc_size;
	struct wsdisplay_curpos cc_magic;
#define	CURSOR_MAX_SIZE	64
	u_int8_t cc_color[6];
	u_int64_t cc_image[64 + 64];
};

struct hwops {
	void (*setlut) __P((void *, struct hwcmap256 *));
	void (*getlut) __P((void *, struct hwcmap256 *));
	void (*visible) __P((void *, int));
	void (*locate) __P((void *, int, int));
	void (*shape) __P((void *, struct wsdisplay_curpos *, u_int64_t *));
	void (*color) __P((void *, u_int8_t *));
};

struct sfb_softc {
	struct device sc_dev;
	struct fb_devconfig *sc_dc;	/* device configuration */
	struct hwcmap256 sc_cmap;	/* software copy of colormap */
	struct hwcursor64 sc_cursor;	/* software copy of cursor */
	int sc_curenb;			/* cursor sprite enabled */
	int sc_changed;			/* need update of colormap */
#define	DATA_ENB_CHANGED	0x01	/* cursor enable changed */
#define	DATA_CURCMAP_CHANGED	0x02	/* cursor colormap changed */
#define	DATA_CURSHAPE_CHANGED	0x04	/* cursor size, image, mask changed */
#define	DATA_CMAP_CHANGED	0x08	/* colormap changed */
#define	DATA_ALL_CHANGED	0x0f
	int nscreens;
	struct hwops sc_hwops;
	void *sc_hw0, *sc_hw1;
};

#define	HX_MAGIC_X 368
#define	HX_MAGIC_Y 38

static int  sfbpmatch __P((struct device *, struct cfdata *, void *));
static void sfbpattach __P((struct device *, struct device *, void *));

struct cfattach sfbp_ca = {
	sizeof(struct sfb_softc), sfbpmatch, sfbpattach,
};

static void sfbp_getdevconfig __P((tc_addr_t, struct fb_devconfig *));
static struct fb_devconfig sfbp_console_dc;
static tc_addr_t sfbp_consaddr;

extern const struct wsdisplay_emulops sfbp_emulops, sfbp_emulops32;

static struct wsscreen_descr sfb_stdscreen = {
	"std", 0, 0,
	NULL, /* textops */
	0, 0,
	WSSCREEN_REVERSE
};

static const struct wsscreen_descr *_sfb_scrlist[] = {
	&sfb_stdscreen,
};

static const struct wsscreen_list sfb_screenlist = {
	sizeof(_sfb_scrlist) / sizeof(struct wsscreen_descr *), _sfb_scrlist
};

static int	sfbioctl __P((void *, u_long, caddr_t, int, struct proc *));
static paddr_t	sfbmmap __P((void *, off_t, int));

static int	sfb_alloc_screen __P((void *, const struct wsscreen_descr *,
				      void **, int *, int *, long *));
static void	sfb_free_screen __P((void *, void *));
static int	sfb_show_screen __P((void *, void *, int,
				     void (*) (void *, int, int), void *));
/* EXPORT */ int  sfb_alloc_attr __P((void *, int, int, int, long *));

static const struct wsdisplay_accessops sfb_accessops = {
	sfbioctl,
	sfbmmap,
	sfb_alloc_screen,
	sfb_free_screen,
	sfb_show_screen,
	0 /* load_font */
};

static void bt459visible __P((void *, int));
static void bt459locate __P((void *, int, int));
static void bt459shape __P((void *, struct wsdisplay_curpos *, u_int64_t *));
static void bt459color __P((void *, u_int8_t *));
static void bt459setlut __P((void *, struct hwcmap256 *));

static void sfbpvisible __P((void *, int));
static void sfbplocate __P((void *, int, int));
static void sfbpshape __P((void *, struct wsdisplay_curpos *, u_int64_t *));
static void bt463color __P((void *, u_int8_t *));
static void noplut __P((void *, struct hwcmap256 *));


/* EXPORT */ int sfbp_cnattach __P((tc_addr_t));
static int  sfbpintr __P((void *));
static void sfbpinit __P((struct fb_devconfig *));

static int  get_cmap __P((struct sfb_softc *, struct wsdisplay_cmap *));
static int  set_cmap __P((struct sfb_softc *, struct wsdisplay_cmap *));
static int  set_cursor __P((struct sfb_softc *, struct wsdisplay_cursor *));
static int  get_cursor __P((struct sfb_softc *, struct wsdisplay_cursor *));


/*
 * Compose 2 bit/pixel cursor image.  Bit order will be reversed.
 *   M M M M I I I I		M I M I M I M I
 *	[ before ]		   [ after ]
 *   3 2 1 0 3 2 1 0		0 0 1 1 2 2 3 3
 *   7 6 5 4 7 6 5 4		4 4 5 5 6 6 7 7
 */
static const u_int8_t shuffle[256] = {
	0x00, 0x40, 0x10, 0x50, 0x04, 0x44, 0x14, 0x54,
	0x01, 0x41, 0x11, 0x51, 0x05, 0x45, 0x15, 0x55,
	0x80, 0xc0, 0x90, 0xd0, 0x84, 0xc4, 0x94, 0xd4,
	0x81, 0xc1, 0x91, 0xd1, 0x85, 0xc5, 0x95, 0xd5,
	0x20, 0x60, 0x30, 0x70, 0x24, 0x64, 0x34, 0x74,
	0x21, 0x61, 0x31, 0x71, 0x25, 0x65, 0x35, 0x75,
	0xa0, 0xe0, 0xb0, 0xf0, 0xa4, 0xe4, 0xb4, 0xf4,
	0xa1, 0xe1, 0xb1, 0xf1, 0xa5, 0xe5, 0xb5, 0xf5,
	0x08, 0x48, 0x18, 0x58, 0x0c, 0x4c, 0x1c, 0x5c,
	0x09, 0x49, 0x19, 0x59, 0x0d, 0x4d, 0x1d, 0x5d,
	0x88, 0xc8, 0x98, 0xd8, 0x8c, 0xcc, 0x9c, 0xdc,
	0x89, 0xc9, 0x99, 0xd9, 0x8d, 0xcd, 0x9d, 0xdd,
	0x28, 0x68, 0x38, 0x78, 0x2c, 0x6c, 0x3c, 0x7c,
	0x29, 0x69, 0x39, 0x79, 0x2d, 0x6d, 0x3d, 0x7d,
	0xa8, 0xe8, 0xb8, 0xf8, 0xac, 0xec, 0xbc, 0xfc,
	0xa9, 0xe9, 0xb9, 0xf9, 0xad, 0xed, 0xbd, 0xfd,
	0x02, 0x42, 0x12, 0x52, 0x06, 0x46, 0x16, 0x56,
	0x03, 0x43, 0x13, 0x53, 0x07, 0x47, 0x17, 0x57,
	0x82, 0xc2, 0x92, 0xd2, 0x86, 0xc6, 0x96, 0xd6,
	0x83, 0xc3, 0x93, 0xd3, 0x87, 0xc7, 0x97, 0xd7,
	0x22, 0x62, 0x32, 0x72, 0x26, 0x66, 0x36, 0x76,
	0x23, 0x63, 0x33, 0x73, 0x27, 0x67, 0x37, 0x77,
	0xa2, 0xe2, 0xb2, 0xf2, 0xa6, 0xe6, 0xb6, 0xf6,
	0xa3, 0xe3, 0xb3, 0xf3, 0xa7, 0xe7, 0xb7, 0xf7,
	0x0a, 0x4a, 0x1a, 0x5a, 0x0e, 0x4e, 0x1e, 0x5e,
	0x0b, 0x4b, 0x1b, 0x5b, 0x0f, 0x4f, 0x1f, 0x5f,
	0x8a, 0xca, 0x9a, 0xda, 0x8e, 0xce, 0x9e, 0xde,
	0x8b, 0xcb, 0x9b, 0xdb, 0x8f, 0xcf, 0x9f, 0xdf,
	0x2a, 0x6a, 0x3a, 0x7a, 0x2e, 0x6e, 0x3e, 0x7e,
	0x2b, 0x6b, 0x3b, 0x7b, 0x2f, 0x6f, 0x3f, 0x7f,
	0xaa, 0xea, 0xba, 0xfa, 0xae, 0xee, 0xbe, 0xfe,
	0xab, 0xeb, 0xbb, 0xfb, 0xaf, 0xef, 0xbf, 0xff,
};

static int
sfbpmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct tc_attach_args *ta = aux;

	if (strncmp("PMAGD", ta->ta_modname, 5) != 0)
		return (0);

	return (1);
}

static void
sfbp_getdevconfig(dense_addr, dc)
	tc_addr_t dense_addr;
	struct fb_devconfig *dc;
{
	struct raster *rap;
	struct rcons *rcp;
	caddr_t sfbasic;
	int i, hsetup, vsetup, vbase;

	dc->dc_vaddr = dense_addr;
	dc->dc_paddr = MACHINE_KSEG0_TO_PHYS(dc->dc_vaddr);

	sfbasic = (caddr_t)(dc->dc_vaddr + SFB_ASIC_OFFSET);
	hsetup = *(u_int32_t *)(sfbasic + SFB_ASIC_VIDEO_HSETUP);
	vsetup = *(u_int32_t *)(sfbasic + SFB_ASIC_VIDEO_VSETUP);
	i = *(u_int32_t *)(sfbasic + SFB_ASIC_DEEP);
	*(u_int32_t *)(sfbasic + SFB_ASIC_VIDEO_BASE) = vbase = 1;

	dc->dc_wid = (hsetup & 0x1ff) << 2;
	dc->dc_ht = (vsetup & 0x7ff);
	dc->dc_depth = (i & 1) ? 32 : 8;
	dc->dc_rowbytes = dc->dc_wid * (dc->dc_depth / 8);
	dc->dc_videobase = dc->dc_vaddr + 0x800000 + vbase * 4096; /* XXX */
	dc->dc_blanked = 0;

	/* initialize colormap and cursor resource */
	sfbpinit(dc);

	/* clear the screen */
	for (i = 0; i < dc->dc_ht * dc->dc_rowbytes; i += sizeof(u_int32_t))
		*(u_int32_t *)(dc->dc_videobase + i) = 0x0;

	/* initialize the raster */
	rap = &dc->dc_raster;
	rap->width = dc->dc_wid;
	rap->height = dc->dc_ht;
	rap->depth = dc->dc_depth;
	rap->linelongs = dc->dc_rowbytes / sizeof(u_int32_t);
	rap->pixels = (u_int32_t *)dc->dc_videobase;
	rap->data = sfbasic;

	/* initialize the raster console blitter */
	rcp = &dc->dc_rcons;
	rcp->rc_sp = rap;
	rcp->rc_crow = rcp->rc_ccol = -1;
	rcp->rc_crowp = &rcp->rc_crow;
	rcp->rc_ccolp = &rcp->rc_ccol;
	rcons_init(rcp, 34, 80);

	sfb_stdscreen.nrows = dc->dc_rcons.rc_maxrow;
	sfb_stdscreen.ncols = dc->dc_rcons.rc_maxcol;
	sfb_stdscreen.textops
	    = (dc->dc_depth == 8) ? &sfbp_emulops : &sfbp_emulops32;
}

static void
sfbpattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sfb_softc *sc = (struct sfb_softc *)self;
	struct tc_attach_args *ta = aux;
	struct wsemuldisplaydev_attach_args waa;
	struct hwcmap256 *cm;
	caddr_t sfbasic;
	int console;

	console = (ta->ta_addr == sfbp_consaddr);
	if (console) {
		sc->sc_dc = &sfbp_console_dc;
		sc->nscreens = 1;
	}
	else {
		sc->sc_dc = (struct fb_devconfig *)
		    malloc(sizeof(struct fb_devconfig), M_DEVBUF, M_WAITOK);
		sfbp_getdevconfig(ta->ta_addr, sc->sc_dc);
	}
	printf(": %d x %d, %dbpp\n", sc->sc_dc->dc_wid, sc->sc_dc->dc_ht,
	    sc->sc_dc->dc_depth);

	cm = &sc->sc_cmap;
	memset(cm, 255, sizeof(struct hwcmap256));	/* XXX */
	cm->r[0] = cm->g[0] = cm->b[0] = 0;		/* XXX */

	sc->sc_cursor.cc_magic.x = HX_MAGIC_X;
	sc->sc_cursor.cc_magic.y = HX_MAGIC_Y;

	if (sc->sc_dc->dc_depth == 8) {
		sc->sc_hw0 = (caddr_t)ta->ta_addr + SFB_RAMDAC_OFFSET;
		sc->sc_hw1 = sc->sc_hw0;
		sc->sc_hwops.visible = bt459visible;
		sc->sc_hwops.locate = bt459locate;
		sc->sc_hwops.shape = bt459shape;
		sc->sc_hwops.color = bt459color;
		sc->sc_hwops.setlut = bt459setlut;
		sc->sc_hwops.getlut = noplut;
	}
	else {
		sc->sc_hw0 = (caddr_t)ta->ta_addr + SFB_ASIC_OFFSET;
		sc->sc_hw1 = (caddr_t)ta->ta_addr + SFB_RAMDAC_OFFSET;
		sc->sc_hwops.visible = sfbpvisible;
		sc->sc_hwops.locate = sfbplocate;
		sc->sc_hwops.shape = sfbpshape;
		sc->sc_hwops.color = bt463color;
		sc->sc_hwops.setlut = noplut;
		sc->sc_hwops.getlut = noplut;
	}

        tc_intr_establish(parent, ta->ta_cookie, IPL_TTY, sfbpintr, sc);

	sfbasic = (caddr_t)(sc->sc_dc->dc_vaddr + SFB_ASIC_OFFSET);
	*(u_int32_t *)(sfbasic + SFB_ASIC_CLEAR_INTR) = 0;	tc_wmb();
	*(u_int32_t *)(sfbasic + SFB_ASIC_ENABLE_INTR) = 1;	tc_wmb();

	waa.console = console;
	waa.scrdata = &sfb_screenlist;
	waa.accessops = &sfb_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}

static int
sfbioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct sfb_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;
	int turnoff;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_SFB; /* XXX SFBP XXX */
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
		turnoff = *(int *)data == WSDISPLAYIO_VIDEO_OFF;
		if ((dc->dc_blanked == 0) ^ turnoff) {
			dc->dc_blanked = turnoff;
#if 0 /* XXX later XXX */
	Low order 3bit control visibilities of screen and builtin cursor.
#endif	/* XXX XXX XXX */
		}
		return (0);

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = dc->dc_blanked ?
		    WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON;
		return (0);

	case WSDISPLAYIO_GCURPOS:
		*(struct wsdisplay_curpos *)data = sc->sc_cursor.cc_pos;
		return (0);

	case WSDISPLAYIO_SCURPOS: {
		struct wsdisplay_curpos *pos = (void *)data;
		int x, y;

		x = pos->x;
		y = pos->y;
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
		x -= sc->sc_cursor.cc_hot.x;
		y -= sc->sc_cursor.cc_hot.y;
		x += sc->sc_cursor.cc_magic.x;
		y += sc->sc_cursor.cc_magic.y;
		(*sc->sc_hwops.locate)(sc->sc_hw0, x, y);
		return (0);
		}

	case WSDISPLAYIO_GCURMAX:
		((struct wsdisplay_curpos *)data)->x =
		((struct wsdisplay_curpos *)data)->y = CURSOR_MAX_SIZE;
		return (0);

	case WSDISPLAYIO_GCURSOR:
		return get_cursor(sc, (struct wsdisplay_cursor *)data);

	case WSDISPLAYIO_SCURSOR:
		return set_cursor(sc, (struct wsdisplay_cursor *)data);
	}
	return ENOTTY;
}

paddr_t
sfbmmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct sfb_softc *sc = v;

	if (offset >= 0x1000000 || offset < 0) /* XXX 16MB XXX */
		return (-1);
	return machine_btop(sc->sc_dc->dc_paddr + offset);
}

static int
sfb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct sfb_softc *sc = v;
	long defattr;

	if (sc->nscreens > 0)
		return (ENOMEM);

	*cookiep = &sc->sc_dc->dc_rcons; /* one and only for now */
	*curxp = 0;
	*curyp = 0;
	sfb_alloc_attr(&sc->sc_dc->dc_rcons, 0, 0, 0, &defattr);
	*attrp = defattr;
	sc->nscreens++;
	return (0);
}

void
sfb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct sfb_softc *sc = v;

	if (sc->sc_dc == &sfbp_console_dc)
		panic("sfb_free_screen: console");

	sc->nscreens--;
}

static int
sfb_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb) __P((void *, int, int));
	void *cbarg;
{

	return (0);
}

int
sfbp_cnattach(addr)
        tc_addr_t addr;
{
        struct fb_devconfig *dcp = &sfbp_console_dc;
        long defattr;

        sfbp_getdevconfig(addr, dcp);
 
        sfb_alloc_attr(&dcp->dc_rcons, 0, 0, 0, &defattr);

        wsdisplay_cnattach(&sfb_stdscreen, &dcp->dc_rcons,
                           0, 0, defattr);
        sfbp_consaddr = addr;
        return(0);
}

static int
sfbpintr(arg)
	void *arg;
{
	struct sfb_softc *sc = arg;
	caddr_t sfbasic = (caddr_t)sc->sc_dc->dc_vaddr + SFB_ASIC_OFFSET;
	int v;
	u_int32_t sisr;
	
#define	cc (&sc->sc_cursor)
	sisr = *((u_int32_t *)sfbasic + TGA_REG_SISR);

	*(u_int32_t *)(sfbasic + SFB_ASIC_CLEAR_INTR) = 0;

	if (sc->sc_changed == 0)
		goto finish;

	v = sc->sc_changed;
	sc->sc_changed = 0;

	if (v & DATA_ENB_CHANGED)
		(*sc->sc_hwops.visible)(sc->sc_hw0, sc->sc_curenb);
	if (v & DATA_CURCMAP_CHANGED)
		(*sc->sc_hwops.color)(sc->sc_hw1, cc->cc_color);
	if (v & DATA_CURSHAPE_CHANGED)
		(*sc->sc_hwops.shape)(sc->sc_hw0, &cc->cc_size, cc->cc_image);
	if (v & DATA_CMAP_CHANGED)
		(*sc->sc_hwops.setlut)(sc->sc_hw1, &sc->sc_cmap);

finish:
	*((u_int32_t *)sfbasic + TGA_REG_SISR) = sisr = 0x00000001; tc_wmb();
	return (1);
#undef cc
}

static void
sfbpinit(dc)
	struct fb_devconfig *dc;
{
	caddr_t sfbasic = (caddr_t)(dc->dc_vaddr + SFB_ASIC_OFFSET);
	caddr_t vdac = (caddr_t)(dc->dc_vaddr + SFB_RAMDAC_OFFSET);
	int i;

	*(u_int32_t *)(sfbasic + SFB_ASIC_PLANEMASK) = ~0;
	*(u_int32_t *)(sfbasic + SFB_ASIC_PIXELMASK) = ~0;
	*(u_int32_t *)(sfbasic + SFB_ASIC_MODE) = 0; /* MODE_SIMPLE */
	*(u_int32_t *)(sfbasic + SFB_ASIC_ROP) = 3;  /* ROP_COPY */
	
    if (dc->dc_depth == 8) {
	*(u_int32_t *)(sfbasic + 0x180000) = 0; /* Bt459 reset */

	SELECT(vdac, BT459_IREG_COMMAND_0);
	REG(vdac, bt_reg) = 0x40; /* CMD0 */	tc_wmb();
	REG(vdac, bt_reg) = 0x0;  /* CMD1 */	tc_wmb();
	REG(vdac, bt_reg) = 0xc0; /* CMD2 */	tc_wmb();
	REG(vdac, bt_reg) = 0xff; /* PRM */	tc_wmb();
	REG(vdac, bt_reg) = 0;    /* 205 */	tc_wmb();
	REG(vdac, bt_reg) = 0x0;  /* PBM */	tc_wmb();
	REG(vdac, bt_reg) = 0;    /* 207 */	tc_wmb();
	REG(vdac, bt_reg) = 0x0;  /* ORM */	tc_wmb();
	REG(vdac, bt_reg) = 0x0;  /* OBM */	tc_wmb();
	REG(vdac, bt_reg) = 0x0;  /* ILV */	tc_wmb();
	REG(vdac, bt_reg) = 0x0;  /* TEST */	tc_wmb();

	SELECT(vdac, BT459_IREG_CCR);
	REG(vdac, bt_reg) = 0x0;	tc_wmb();
	REG(vdac, bt_reg) = 0x0;	tc_wmb();
	REG(vdac, bt_reg) = 0x0;	tc_wmb();
	REG(vdac, bt_reg) = 0x0;	tc_wmb();
	REG(vdac, bt_reg) = 0x0;	tc_wmb();
	REG(vdac, bt_reg) = 0x0;	tc_wmb();
	REG(vdac, bt_reg) = 0x0;	tc_wmb();
	REG(vdac, bt_reg) = 0x0;	tc_wmb();
	REG(vdac, bt_reg) = 0x0;	tc_wmb();
	REG(vdac, bt_reg) = 0x0;	tc_wmb();
	REG(vdac, bt_reg) = 0x0;	tc_wmb();
	REG(vdac, bt_reg) = 0x0;	tc_wmb();
	REG(vdac, bt_reg) = 0x0;	tc_wmb();

	/* build sane colormap */
	SELECT(vdac, 0);
	REG(vdac, bt_cmap) = 0;	tc_wmb();
	REG(vdac, bt_cmap) = 0;	tc_wmb();
	REG(vdac, bt_cmap) = 0;	tc_wmb();
	for (i = 1; i < CMAP_SIZE; i++) {
		REG(vdac, bt_cmap) = 0xff;	tc_wmb();
		REG(vdac, bt_cmap) = 0xff;	tc_wmb();
		REG(vdac, bt_cmap) = 0xff;	tc_wmb();
	}

	/* clear out cursor image */
	SELECT(vdac, BT459_IREG_CRAM_BASE);
	for (i = 0; i < 1024; i++)
		REG(vdac, bt_reg) = 0xff;	tc_wmb();

	/*
	 * 2 bit/pixel cursor.  Assign MSB for cursor mask and LSB for
	 * cursor image.  CCOLOR_2 for mask color, while CCOLOR_3 for
	 * image color.  CCOLOR_1 will be never used.
	 */
	SELECT(vdac, BT459_IREG_CCOLOR_1);
	REG(vdac, bt_reg) = 0xff;	tc_wmb();
	REG(vdac, bt_reg) = 0xff;	tc_wmb();
	REG(vdac, bt_reg) = 0xff;	tc_wmb();

	REG(vdac, bt_reg) = 0;		tc_wmb();
	REG(vdac, bt_reg) = 0;		tc_wmb();
	REG(vdac, bt_reg) = 0;		tc_wmb();

	REG(vdac, bt_reg) = 0xff;	tc_wmb();
	REG(vdac, bt_reg) = 0xff;	tc_wmb();
	REG(vdac, bt_reg) = 0xff;	tc_wmb();
    } else {
	SELECT(vdac, BT463_IREG_COMMAND_0);
	REG(vdac, bt_reg) = 0x40;	tc_wmb();	/* CMD 0 */
	REG(vdac, bt_reg) = 0x46;	tc_wmb();	/* CMD 1 */
	REG(vdac, bt_reg) = 0xc0;	tc_wmb();	/* CMD 2 */
	REG(vdac, bt_reg) = 0;		tc_wmb();	/* !? 204 !? */
	REG(vdac, bt_reg) = 0xff;	tc_wmb();	/* plane  0:7  */
	REG(vdac, bt_reg) = 0xff;	tc_wmb();	/* plane  8:15 */
	REG(vdac, bt_reg) = 0xff;	tc_wmb();	/* plane 16:23 */
	REG(vdac, bt_reg) = 0xff;	tc_wmb();	/* plane 24:27 */
	REG(vdac, bt_reg) = 0x00;	tc_wmb();	/* blink  0:7  */
	REG(vdac, bt_reg) = 0x00;	tc_wmb();	/* blink  8:15 */
	REG(vdac, bt_reg) = 0x00;	tc_wmb();	/* blink 16:23 */
	REG(vdac, bt_reg) = 0x00;	tc_wmb();	/* blink 24:27 */
	REG(vdac, bt_reg) = 0x00;	tc_wmb();

#if 0 /* XXX ULTRIX does initialize 16 entry window type here XXX */
  {
	static u_int32_t windowtype[BT463_IREG_WINDOW_TYPE_TABLE] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	SELECT(vdac, BT463_IREG_WINDOW_TYPE_TABLE);
	for (i = 0; i < BT463_NWTYPE_ENTRIES; i++) {
		REG(vdac, bt_reg) = windowtype[i];	  /*   0:7  */
		REG(vdac, bt_reg) = windowtype[i] >> 8;  /*   8:15 */
		REG(vdac, bt_reg) = windowtype[i] >> 16; /*  16:23 */
	}
  }
#endif

	SELECT(vdac, BT463_IREG_CPALETTE_RAM);
	REG(vdac, bt_cmap) = 0;		tc_wmb();
	REG(vdac, bt_cmap) = 0;		tc_wmb();
	REG(vdac, bt_cmap) = 0;		tc_wmb();
	for (i = 1; i < 256; i++) {
		REG(vdac, bt_cmap) = 0xff;	tc_wmb();
		REG(vdac, bt_cmap) = 0xff;	tc_wmb();
		REG(vdac, bt_cmap) = 0xff;	tc_wmb();
	}

	/* !? Eeeh !? */
	SELECT(vdac, 0x0100 /* BT463_IREG_CURSOR_COLOR_0 */);
	for (i = 0; i < 256; i++) {
		REG(vdac, bt_cmap) = i;	tc_wmb();
		REG(vdac, bt_cmap) = i;	tc_wmb();
		REG(vdac, bt_cmap) = i;	tc_wmb();
	}
    } 
}

static int
get_cmap(sc, p)
	struct sfb_softc *sc;
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
	struct sfb_softc *sc;
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

	sc->sc_changed |= DATA_CMAP_CHANGED;

	return (0);
}


static int
set_cursor(sc, p)
	struct sfb_softc *sc;
	struct wsdisplay_cursor *p;
{
#define	cc (&sc->sc_cursor)
	int v, index, count, icount, x, y;

	v = p->which;
	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		index = p->cmap.index;
		count = p->cmap.count;
		if (index >= 2 || (index + count) > 2)
			return (EINVAL);
		if (!uvm_useracc(p->cmap.red, count, B_READ) ||
		    !uvm_useracc(p->cmap.green, count, B_READ) ||
		    !uvm_useracc(p->cmap.blue, count, B_READ))
			return (EFAULT);
	}
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		if (p->size.x > CURSOR_MAX_SIZE || p->size.y > CURSOR_MAX_SIZE)
			return (EINVAL);
		icount = ((p->size.x < 33) ? 4 : 8) * p->size.y;
		if (!uvm_useracc(p->image, icount, B_READ) ||
		    !uvm_useracc(p->mask, icount, B_READ))
			return (EFAULT);
	}
	if (v & (WSDISPLAY_CURSOR_DOPOS | WSDISPLAY_CURSOR_DOCUR)) {
		if (v & WSDISPLAY_CURSOR_DOCUR)
			cc->cc_hot = p->hot;
		if (v & WSDISPLAY_CURSOR_DOPOS) {
			struct fb_devconfig *dc = sc->sc_dc;

			x = p->pos.x;
			y = p->pos.y;
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
		x = sc->sc_cursor.cc_pos.x - sc->sc_cursor.cc_hot.x;
		y = sc->sc_cursor.cc_pos.y - sc->sc_cursor.cc_hot.y;
		x += sc->sc_cursor.cc_magic.x;
		y += sc->sc_cursor.cc_magic.y;
		(*sc->sc_hwops.locate)(sc->sc_hw0, x, y);
	}

	sc->sc_changed = 0;
	if (v & WSDISPLAY_CURSOR_DOCUR) {
		sc->sc_curenb = p->enable;
		sc->sc_changed |= DATA_ENB_CHANGED;
	}
	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		copyin(p->cmap.red, &cc->cc_color[index], count);
		copyin(p->cmap.green, &cc->cc_color[index + 2], count);
		copyin(p->cmap.blue, &cc->cc_color[index + 4], count);
		sc->sc_changed |= DATA_CURCMAP_CHANGED;
	}
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		cc->cc_size = p->size;
		memset(cc->cc_image, 0, sizeof cc->cc_image);
		copyin(p->image, cc->cc_image, icount);
		copyin(p->mask, cc->cc_image+CURSOR_MAX_SIZE, icount);
		sc->sc_changed |= DATA_CURSHAPE_CHANGED;
	}

	return (0);
#undef cc
}

static int
get_cursor(sc, p)
	struct sfb_softc *sc;
	struct wsdisplay_cursor *p;
{
	return (ENOTTY); /* XXX */
}

int
sfb_alloc_attr(id, fg, bg, flags, attrp)
	void *id;
	int fg, bg, flags;
	long *attrp;
{
	if (flags & (WSATTR_HILIT | WSATTR_BLINK |
		     WSATTR_UNDERLINE | WSATTR_WSCOLORS))
		return (EINVAL);
	if (flags & WSATTR_REVERSE)
		*attrp = 1;
	else
		*attrp = 0;
	return (0);
}

static void
bt459visible(hw, on)
	void *hw;
	int on;
{
	SELECT(hw, BT459_IREG_CCR);
	REG(hw, bt_reg) = (on) ? 0xc0 : 0x00;
	tc_wmb();
}

static void
sfbpvisible(hw, on)
	void *hw;
	int on;
{
}

static void
bt459locate(hw, x, y)
	void *hw;
	int x, y;
{
	int s;

	s = spltty();
	SELECT(hw, BT459_IREG_CURSOR_X_LOW);
	REG(hw, bt_reg) = x;		tc_wmb();
	REG(hw, bt_reg) = x >> 8;	tc_wmb();
	REG(hw, bt_reg) = y;		tc_wmb();
	REG(hw, bt_reg) = y >> 8;	tc_wmb();
	splx(s);
}

static void
sfbplocate(hw, x, y)
	void *hw;
	int x, y;
{
	*((u_int32_t *)hw + TGA_REG_CXYR) = ((y & 0xfff) << 12) | (x & 0xfff);
	tc_wmb();
}

static void 
bt459color(hw, cp)
	void *hw;
	u_int8_t *cp;
{
	SELECT(hw, BT459_IREG_CCOLOR_2);
	REG(hw, bt_reg) = cp[1]; tc_wmb();
	REG(hw, bt_reg) = cp[3]; tc_wmb();
	REG(hw, bt_reg) = cp[5]; tc_wmb();

	REG(hw, bt_reg) = cp[0]; tc_wmb();
	REG(hw, bt_reg) = cp[2]; tc_wmb();
	REG(hw, bt_reg) = cp[4]; tc_wmb();
}

static void
bt463color(hw, cp)
	void *hw;
	u_int8_t *cp;
{
}

static void
bt459shape(hw, size, image)
	void *hw;
	struct wsdisplay_curpos *size;
	u_int64_t *image;
{
	u_int8_t *ip, *mp, img, msk;
	u_int8_t u;
	int bcnt;

	ip = (u_int8_t *)image;
	mp = (u_int8_t *)(image + CURSOR_MAX_SIZE);

	bcnt = 0;
	SELECT(hw, BT459_IREG_CRAM_BASE+0);
	/* 64 pixel scan line is consisted with 16 byte cursor ram */
	while (bcnt < size->y * 16) {
		/* pad right half 32 pixel when smaller than 33 */
		if ((bcnt & 0x8) && size->x < 33) {
			REG(hw, bt_reg) = 0; tc_wmb();
			REG(hw, bt_reg) = 0; tc_wmb();
		}
		else {
			img = *ip++;
			msk = *mp++;
			img &= msk;	/* cookie off image */
			u = (msk & 0x0f) << 4 | (img & 0x0f);
			REG(hw, bt_reg) = shuffle[u];	tc_wmb();
			u = (msk & 0xf0) | (img & 0xf0) >> 4;
			REG(hw, bt_reg) = shuffle[u];	tc_wmb();
		}
		bcnt += 2;
	}
	/* pad unoccupied scan lines */
	while (bcnt < CURSOR_MAX_SIZE * 16) {
		REG(hw, bt_reg) = 0; tc_wmb();
		REG(hw, bt_reg) = 0; tc_wmb();
		bcnt += 2;
	}
}

static void
sfbpshape(hw, size, image)
	void *hw;
	struct wsdisplay_curpos *size;
	u_int64_t *image;
{
}

static void
bt459setlut(hw, cm)
	void *hw;
	struct hwcmap256 *cm;
{
	int index;

	SELECT(hw, 0);
	for (index = 0; index < CMAP_SIZE; index++) {
		REG(hw, bt_cmap) = cm->r[index];	tc_wmb();
		REG(hw, bt_cmap) = cm->g[index];	tc_wmb();
		REG(hw, bt_cmap) = cm->b[index];	tc_wmb();
	}
}

static void
noplut(hw, cm)
	void *hw;
	struct hwcmap256 *cm;
{
}
