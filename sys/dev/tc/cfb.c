/* $NetBSD: cfb.c,v 1.6 1999/01/11 21:35:55 drochner Exp $ */

/*
 * Copyright (c) 1998 Tohru Nishimura.  All rights reserved.
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

__KERNEL_RCSID(0, "$NetBSD: cfb.c,v 1.6 1999/01/11 21:35:55 drochner Exp $");

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
#include <machine/autoconf.h>

#include <dev/tc/tcvar.h>
#include <dev/ic/bt459reg.h>	

#include "opt_uvm.h"
#if defined(UVM)
#include <uvm/uvm_extern.h>
#define useracc uvm_useracc
#endif

/* XXX BUS'IFYING XXX */
 
#if defined(__pmax__)
#define	machine_btop(x) mips_btop(x)
#define	MACHINE_KSEG0_TO_PHYS(x) MIPS_KSEG1_TO_PHYS(x)
#endif

#if defined(__alpha__) || defined(alpha)
/*
 * Digital UNIX never supports PMAG-BA
 */
#define machine_btop(x) alpha_btop(x)
#define MACHINE_KSEG0_TO_PHYS(x) ALPHA_K0SEG_TO_PHYS(x)
#endif

/*
 * N.B., Bt459 registers are 8bit width.  Some of TC framebuffers have
 * obscure register layout such as 2nd and 3rd Bt459 registers are
 * adjacent each other in a word, i.e.,
 *	struct bt459triplet {
 * 		struct {
 *			u_int8_t u0;
 *			u_int8_t u1;
 *			u_int8_t u2;
 *			unsigned :8; 
 *		} bt_lo;
 *		...
 * Although CX has single Bt459, 32bit R/W can be done w/o any trouble.
 */
struct bt459reg {
        u_int32_t       bt_lo;
        u_int32_t       bt_hi;
        u_int32_t       bt_reg;
        u_int32_t       bt_cmap;
};

/* XXX XXX XXX */

struct fb_devconfig {
	vaddr_t dc_vaddr;		/* memory space virtual base address */
	paddr_t dc_paddr;		/* memory space physical base address */
	vsize_t	dc_size;		/* size of slot memory */
	int	dc_wid;			/* width of frame buffer */
	int	dc_ht;			/* height of frame buffer */
	int	dc_depth;		/* depth, bits per pixel */
	int	dc_rowbytes;		/* bytes in a FB scan line */
	vaddr_t dc_videobase;		/* base of flat frame buffer */
	struct raster	dc_raster;	/* raster description */
	struct rcons	dc_rcons;	/* raster blitter control info */
	int	    dc_blanked;		/* currently has video disabled */
};

struct hwcmap {
#define	CMAP_SIZE	256	/* 256 R/G/B entries */
	u_int8_t r[CMAP_SIZE];
	u_int8_t g[CMAP_SIZE];
	u_int8_t b[CMAP_SIZE];
};

struct hwcursor {
	struct wsdisplay_curpos cc_pos;
	struct wsdisplay_curpos cc_hot;
	struct wsdisplay_curpos cc_size;
#define	CURSOR_MAX_SIZE	64
	u_int8_t cc_color[6];
	u_int64_t cc_image[64 + 64];
};

struct cfb_softc {
	struct device sc_dev;
	struct fb_devconfig *sc_dc;	/* device configuration */
	struct hwcmap sc_cmap;		/* software copy of colormap */
	struct hwcursor sc_cursor;	/* software copy of cursor */
	int sc_curenb;			/* cursor sprite enabled */
	int sc_changed;			/* need update of colormap */
#define	DATA_ENB_CHANGED	0x01	/* cursor enable changed */
#define	DATA_CURCMAP_CHANGED	0x02	/* cursor colormap changed */
#define	DATA_CURSHAPE_CHANGED	0x04	/* cursor size, image, mask changed */
#define	DATA_CMAP_CHANGED	0x08	/* colormap changed */
#define	DATA_ALL_CHANGED	0x0f
	int nscreens;
	short magic_x, magic_y;		/* CX cursor location offset */
#define	CX_MAGIC_X 220
#define	CX_MAGIC_Y 35
};

#define	CX_FB_OFFSET	0x000000
#define	CX_FB_SIZE	0x100000
#define	CX_BT459_OFFSET	0x200000
#define	CX_OFFSET_IREQ	0x300000	/* Interrupt req. control */

int  cfbmatch __P((struct device *, struct cfdata *, void *));
void cfbattach __P((struct device *, struct device *, void *));

struct cfattach cfb_ca = {
	sizeof(struct cfb_softc), cfbmatch, cfbattach,
};

void cfb_getdevconfig __P((tc_addr_t, struct fb_devconfig *));
struct fb_devconfig cfb_console_dc;
tc_addr_t cfb_consaddr;

struct wsdisplay_emulops cfb_emulops = {
	rcons_cursor,			/* could use hardware cursor; punt */
	rcons_mapchar,
	rcons_putchar,
	rcons_copycols,
	rcons_erasecols,
	rcons_copyrows,
	rcons_eraserows,
	rcons_alloc_attr
};

struct wsscreen_descr cfb_stdscreen = {
	"std", 0, 0,
	&cfb_emulops,
	0, 0,
	0
};

const struct wsscreen_descr *_cfb_scrlist[] = {
	&cfb_stdscreen,
};

struct wsscreen_list cfb_screenlist = {
	sizeof(_cfb_scrlist) / sizeof(struct wsscreen_descr *), _cfb_scrlist
};

int	cfbioctl __P((void *, u_long, caddr_t, int, struct proc *));
int	cfbmmap __P((void *, off_t, int));

int	cfb_alloc_screen __P((void *, const struct wsscreen_descr *,
				      void **, int *, int *, long *));
void	cfb_free_screen __P((void *, void *));
void	cfb_show_screen __P((void *, void *));

struct wsdisplay_accessops cfb_accessops = {
	cfbioctl,
	cfbmmap,
	cfb_alloc_screen,
	cfb_free_screen,
	cfb_show_screen,
	0 /* load_font */
};

int  cfb_cnattach __P((tc_addr_t));
int  cfbintr __P((void *));
void cfbinit __P((struct fb_devconfig *));

static int  get_cmap __P((struct cfb_softc *, struct wsdisplay_cmap *));
static int  set_cmap __P((struct cfb_softc *, struct wsdisplay_cmap *));
static int  set_cursor __P((struct cfb_softc *, struct wsdisplay_cursor *));
static int  get_cursor __P((struct cfb_softc *, struct wsdisplay_cursor *));
static void set_curpos __P((struct cfb_softc *, struct wsdisplay_curpos *));
static void bt459_set_curpos __P((struct cfb_softc *));

/* XXX XXX XXX */
#define	BT459_SELECT(vdac, regno) do {		\
	vdac->bt_lo = (regno) & 0x00ff;		\
	vdac->bt_hi = ((regno)& 0x0f00) >> 8;	\
	tc_wmb();				\
   } while (0)
/* XXX XXX XXX */

/*
 * Compose 2 bit/pixel cursor image.  Bit order will be reversed.
 *   M M M M I I I I		M I M I M I M I
 *	[ before ]		   [ after ]
 *   3 2 1 0 3 2 1 0		0 0 1 1 2 2 3 3
 *   7 6 5 4 7 6 5 4		4 4 5 5 6 6 7 7
 */
const static u_int8_t shuffle[256] = {
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

int
cfbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct tc_attach_args *ta = aux;

	if (strncmp("PMAG-BA ", ta->ta_modname, TC_ROM_LLEN) != 0)
		return (0);

	return (1);
}

void
cfb_getdevconfig(dense_addr, dc)
	tc_addr_t dense_addr;
	struct fb_devconfig *dc;
{
	struct raster *rap;
	struct rcons *rcp;
	int i;

	dc->dc_vaddr = dense_addr;
	dc->dc_paddr = MACHINE_KSEG0_TO_PHYS(dc->dc_vaddr + CX_FB_OFFSET);

	dc->dc_wid = 1024;
	dc->dc_ht = 864;
	dc->dc_depth = 8;
	dc->dc_rowbytes = 1024;
	dc->dc_videobase = dc->dc_vaddr + CX_FB_OFFSET;
	dc->dc_blanked = 0;

	/* initialize colormap and cursor resource */
	cfbinit(dc);

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

	/* initialize the raster console blitter */
	rcp = &dc->dc_rcons;
	rcp->rc_sp = rap;
	rcp->rc_crow = rcp->rc_ccol = -1;
	rcp->rc_crowp = &rcp->rc_crow;
	rcp->rc_ccolp = &rcp->rc_ccol;
	rcons_init(rcp, 34, 80);

	cfb_stdscreen.nrows = dc->dc_rcons.rc_maxrow;
	cfb_stdscreen.ncols = dc->dc_rcons.rc_maxcol;
}

void
cfbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cfb_softc *sc = (struct cfb_softc *)self;
	struct tc_attach_args *ta = aux;
	struct wsemuldisplaydev_attach_args waa;
	struct hwcmap *cm;
	int console, i;

	console = (ta->ta_addr == cfb_consaddr);
	if (console) {
		sc->sc_dc = &cfb_console_dc;
		sc->nscreens = 1;
	}
	else {
		sc->sc_dc = (struct fb_devconfig *)
		    malloc(sizeof(struct fb_devconfig), M_DEVBUF, M_WAITOK);
		cfb_getdevconfig(ta->ta_addr, sc->sc_dc);
	}
	printf(": %d x %d, %dbpp\n", sc->sc_dc->dc_wid, sc->sc_dc->dc_ht,
	    sc->sc_dc->dc_depth);

	cm = &sc->sc_cmap;
	cm->r[0] = cm->g[0] = cm->b[0] = 0;
	for (i = 1; i < CMAP_SIZE; i++) {
		cm->r[i] = cm->g[i] = cm->b[i] = 0xff;
	}
	sc->magic_x = CX_MAGIC_X;
	sc->magic_y = CX_MAGIC_Y;

	/* Establish an interrupt handler, and clear any pending interrupts */
        tc_intr_establish(parent, ta->ta_cookie, TC_IPL_TTY, cfbintr, sc);
	*(u_int8_t *)(sc->sc_dc->dc_vaddr + CX_OFFSET_IREQ) = 0;

	waa.console = console;
	waa.scrdata = &cfb_screenlist;
	waa.accessops = &cfb_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}

int
cfbioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct cfb_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;
	int turnoff;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_CFB;
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
			/* XXX later XXX */
		}
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
		bt459_set_curpos(sc);
		return (0);

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

int
cfbmmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct cfb_softc *sc = v;

	if (offset >= CX_FB_SIZE || offset < 0)
		return (-1);
	return machine_btop(sc->sc_dc->dc_paddr + offset);
}

int
cfb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct cfb_softc *sc = v;
	long defattr;

	if (sc->nscreens > 0)
		return (ENOMEM);

	*cookiep = &sc->sc_dc->dc_rcons; /* one and only for now */
	*curxp = 0;
	*curyp = 0;
	rcons_alloc_attr(&sc->sc_dc->dc_rcons, 0, 0, 0, &defattr);
	*attrp = defattr;
	sc->nscreens++;
	return (0);
}

void
cfb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct cfb_softc *sc = v;

	if (sc->sc_dc == &cfb_console_dc)
		panic("cfb_free_screen: console");

	sc->nscreens--;
}

void
cfb_show_screen(v, cookie)
	void *v;
	void *cookie;
{
}

int
cfb_cnattach(addr)
        tc_addr_t addr;
{
        struct fb_devconfig *dcp = &cfb_console_dc;
        long defattr;

        cfb_getdevconfig(addr, dcp);
 
        rcons_alloc_attr(&dcp->dc_rcons, 0, 0, 0, &defattr);

        wsdisplay_cnattach(&cfb_stdscreen, &dcp->dc_rcons,
                           0, 0, defattr);
        cfb_consaddr = addr;
        return(0);
}


int
cfbintr(arg)
	void *arg;
{
	struct cfb_softc *sc = arg;
	caddr_t cfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	struct bt459reg *vdac;
	int v;
	
	*(u_int8_t *)(cfbbase + CX_OFFSET_IREQ) = 0;
	if (sc->sc_changed == 0)
		return (1);

	vdac = (void *)(cfbbase + CX_BT459_OFFSET);
	v = sc->sc_changed;
	sc->sc_changed = 0;
	if (v & DATA_ENB_CHANGED) {
		BT459_SELECT(vdac, BT459_REG_CCR);
		vdac->bt_reg = (sc->sc_curenb) ? 0xc0 : 0x00;
	}
	if (v & DATA_CURCMAP_CHANGED) {
		u_int8_t *cp = sc->sc_cursor.cc_color;

		BT459_SELECT(vdac, BT459_REG_CCOLOR_2);
		vdac->bt_reg = cp[1];	tc_wmb();
		vdac->bt_reg = cp[3];	tc_wmb();
		vdac->bt_reg = cp[5];	tc_wmb();

		vdac->bt_reg = cp[0];	tc_wmb();
		vdac->bt_reg = cp[2];	tc_wmb();
		vdac->bt_reg = cp[4];	tc_wmb();
	}
	if (v & DATA_CURSHAPE_CHANGED) {
		u_int8_t *ip, *mp, img, msk;
		u_int8_t u;
		int bcnt;

		ip = (u_int8_t *)sc->sc_cursor.cc_image;
		mp = (u_int8_t *)(sc->sc_cursor.cc_image + CURSOR_MAX_SIZE);

		bcnt = 0;
		BT459_SELECT(vdac, BT459_REG_CRAM_BASE+0);
		/* 64 pixel scan line is consisted with 16 byte cursor ram */
		while (bcnt < sc->sc_cursor.cc_size.y * 16) {
			/* pad right half 32 pixel when smaller than 33 */
			if ((bcnt & 0x8) && sc->sc_cursor.cc_size.x < 33) {
				vdac->bt_reg = 0; tc_wmb();
				vdac->bt_reg = 0; tc_wmb();
			}
			else {
				img = *ip++;
				msk = *mp++;
				img &= msk;	/* cookie off image */
				u = (msk & 0x0f) << 4 | (img & 0x0f);
				vdac->bt_reg = shuffle[u];	tc_wmb();
				u = (msk & 0xf0) | (img & 0xf0) >> 4;
				vdac->bt_reg = shuffle[u];	tc_wmb();
			}
			bcnt += 2;
		}
		/* pad unoccupied scan lines */
		while (bcnt < CURSOR_MAX_SIZE * 16) {
			vdac->bt_reg = 0; tc_wmb();
			vdac->bt_reg = 0; tc_wmb();
			bcnt += 2;
		}
	}
	if (v & DATA_CMAP_CHANGED) {
		struct hwcmap *cm = &sc->sc_cmap;
		int index;

		BT459_SELECT(vdac, 0);
		for (index = 0; index < CMAP_SIZE; index++) {
			vdac->bt_cmap = cm->r[index];	tc_wmb();
			vdac->bt_cmap = cm->g[index];	tc_wmb();
			vdac->bt_cmap = cm->b[index];	tc_wmb();
		}
	}
	return (1);
}

void
cfbinit(dc)
	struct fb_devconfig *dc;
{
	caddr_t cfbbase = (caddr_t)dc->dc_vaddr;
	struct bt459reg *vdac = (void *)(cfbbase + CX_BT459_OFFSET);
	int i;

	BT459_SELECT(vdac, BT459_REG_COMMAND_0);
	vdac->bt_reg = 0x40; /* CMD0 */	tc_wmb();
	vdac->bt_reg = 0x0;  /* CMD1 */	tc_wmb();
	vdac->bt_reg = 0xc0; /* CMD2 */	tc_wmb();
	vdac->bt_reg = 0xff; /* PRM */	tc_wmb();
	vdac->bt_reg = 0;    /* 205 */	tc_wmb();
	vdac->bt_reg = 0x0;  /* PBM */	tc_wmb();
	vdac->bt_reg = 0;    /* 207 */	tc_wmb();
	vdac->bt_reg = 0x0;  /* ORM */	tc_wmb();
	vdac->bt_reg = 0x0;  /* OBM */	tc_wmb();
	vdac->bt_reg = 0x0;  /* ILV */	tc_wmb();
	vdac->bt_reg = 0x0;  /* TEST */	tc_wmb();

	BT459_SELECT(vdac, BT459_REG_CCR);
	vdac->bt_reg = 0x0;	tc_wmb();
	vdac->bt_reg = 0x0;	tc_wmb();
	vdac->bt_reg = 0x0;	tc_wmb();
	vdac->bt_reg = 0x0;	tc_wmb();
	vdac->bt_reg = 0x0;	tc_wmb();
	vdac->bt_reg = 0x0;	tc_wmb();
	vdac->bt_reg = 0x0;	tc_wmb();
	vdac->bt_reg = 0x0;	tc_wmb();
	vdac->bt_reg = 0x0;	tc_wmb();
	vdac->bt_reg = 0x0;	tc_wmb();
	vdac->bt_reg = 0x0;	tc_wmb();
	vdac->bt_reg = 0x0;	tc_wmb();
	vdac->bt_reg = 0x0;	tc_wmb();

	/* build sane colormap */
	BT459_SELECT(vdac, 0);
	vdac->bt_cmap = 0;	tc_wmb();
	vdac->bt_cmap = 0;	tc_wmb();
	vdac->bt_cmap = 0;	tc_wmb();
	for (i = 1; i < CMAP_SIZE; i++) {
		vdac->bt_cmap = 0xff;	tc_wmb();
		vdac->bt_cmap = 0xff;	tc_wmb();
		vdac->bt_cmap = 0xff;	tc_wmb();
	}

	/* clear out cursor image */
	BT459_SELECT(vdac, BT459_REG_CRAM_BASE);
	for (i = 0; i < 1024; i++)
		vdac->bt_reg = 0xff;	tc_wmb();

	/*
	 * 2 bit/pixel cursor.  Assign MSB for cursor mask and LSB for
	 * cursor image.  CCOLOR_2 for mask color, while CCOLOR_3 for
	 * image color.  CCOLOR_1 will be never used.
	 */
	BT459_SELECT(vdac, BT459_REG_CCOLOR_1);
	vdac->bt_reg = 0xff;	tc_wmb();
	vdac->bt_reg = 0xff;	tc_wmb();
	vdac->bt_reg = 0xff;	tc_wmb();

	vdac->bt_reg = 0;	tc_wmb();
	vdac->bt_reg = 0;	tc_wmb();
	vdac->bt_reg = 0;	tc_wmb();

	vdac->bt_reg = 0xff;	tc_wmb();
	vdac->bt_reg = 0xff;	tc_wmb();
	vdac->bt_reg = 0xff;	tc_wmb();
}

static int
get_cmap(sc, p)
	struct cfb_softc *sc;
	struct wsdisplay_cmap *p;
{
	u_int index = p->index, count = p->count;

	if (index >= CMAP_SIZE || (index + count) > CMAP_SIZE)
		return (EINVAL);

	if (!useracc(p->red, count, B_WRITE) ||
	    !useracc(p->green, count, B_WRITE) ||
	    !useracc(p->blue, count, B_WRITE))
		return (EFAULT);

	copyout(&sc->sc_cmap.r[index], p->red, count);
	copyout(&sc->sc_cmap.g[index], p->green, count);
	copyout(&sc->sc_cmap.b[index], p->blue, count);

	return (0);
}

static int
set_cmap(sc, p)
	struct cfb_softc *sc;
	struct wsdisplay_cmap *p;
{
	u_int index = p->index, count = p->count;

	if (index >= CMAP_SIZE || (index + count) > CMAP_SIZE)
		return (EINVAL);

	if (!useracc(p->red, count, B_READ) ||
	    !useracc(p->green, count, B_READ) ||
	    !useracc(p->blue, count, B_READ))
		return (EFAULT);

	copyin(p->red, &sc->sc_cmap.r[index], count);
	copyin(p->green, &sc->sc_cmap.g[index], count);
	copyin(p->blue, &sc->sc_cmap.b[index], count);

	sc->sc_changed |= DATA_CMAP_CHANGED;

	return (0);
}

static int
set_cursor(sc, p)
	struct cfb_softc *sc;
	struct wsdisplay_cursor *p;
{
#define	cc (&sc->sc_cursor)
	int v, index, count, icount;

	v = p->which;
	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		index = p->cmap.index;
		count = p->cmap.count;
		if (index >= 2 || (index + count) > 2)
			return (EINVAL);
		if (!useracc(p->cmap.red, count, B_READ) ||
		    !useracc(p->cmap.green, count, B_READ) ||
		    !useracc(p->cmap.blue, count, B_READ))
			return (EFAULT);
	}
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		if (p->size.x > CURSOR_MAX_SIZE || p->size.y > CURSOR_MAX_SIZE)
			return (EINVAL);
		icount = ((p->size.x < 33) ? 4 : 8) * p->size.y;
		if (!useracc(p->image, count, B_READ) ||
		    !useracc(p->mask, count, B_READ))
			return (EFAULT);
	}
	if (v & (WSDISPLAY_CURSOR_DOPOS | WSDISPLAY_CURSOR_DOCUR)) {
		if (v & WSDISPLAY_CURSOR_DOCUR)
			cc->cc_hot = p->hot;
		if (v & WSDISPLAY_CURSOR_DOPOS)
			set_curpos(sc, &p->pos);
		bt459_set_curpos(sc);
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
	struct cfb_softc *sc;
	struct wsdisplay_cursor *p;
{
	return (ENOTTY); /* XXX */
}

static void
set_curpos(sc, curpos)
	struct cfb_softc *sc;
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

void
bt459_set_curpos(sc)
	struct cfb_softc *sc;
{
	caddr_t cfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	struct bt459reg *vdac = (void *)(cfbbase + CX_BT459_OFFSET);
	int x, y, s;

	x = sc->sc_cursor.cc_pos.x - sc->sc_cursor.cc_hot.x;
	y = sc->sc_cursor.cc_pos.y - sc->sc_cursor.cc_hot.y;
	x += sc->magic_x; y += sc->magic_y; /* magic offset of CX coordinate */

	s = spltty();

	BT459_SELECT(vdac, BT459_REG_CURSOR_X_LOW);
	vdac->bt_reg = x;	tc_wmb();
	vdac->bt_reg = x >> 8;	tc_wmb();
	vdac->bt_reg = y;	tc_wmb();
	vdac->bt_reg = y >> 8;	tc_wmb();

	splx(s);
}
