/* $NetBSD: mfb.c,v 1.14 1999/06/24 03:51:14 nisimura Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: mfb.c,v 1.14 1999/06/24 03:51:14 nisimura Exp $");

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
#include <dev/ic/bt431reg.h>	

#include <uvm/uvm_extern.h>

#define	machine_btop(x) mips_btop(x)
#define	MACHINE_KSEG0_TO_PHYS(x) MIPS_KSEG0_TO_PHYS(x)

#define	BYTE(base, index)	*((u_int8_t *)(base) + ((index)<<2))
#define	HALF(base, index)	*((u_int16_t *)(base) + ((index)<<1))

/* Bt431 hardware registers */
#define	bt_reg	0
#define	bt_cmap	1
#define	bt_clr	2
#define	bt_ovly	3

/* Bt455 hardware registers */
#define	bt_lo	0
#define	bt_hi	1
#define	bt_ram	2
#define	bt_ctl	3

#define BT455_SELECT(vdac, regno) do {	\
	BYTE(vdac, bt_reg) = (regno);	\
	BYTE(vdac, bt_clr) = 0;		\
	tc_wmb();			\
   } while (0)

#define	TWIN(x)    ((x)|((x) << 8))
#define	TWIN_LO(x) (twin = (x) & 0x00ff, twin << 8 | twin)
#define	TWIN_HI(x) (twin = (x) & 0xff00, twin | twin >> 8)

#define	BT431_SELECT(curs, regno) do {	\
	HALF(curs, bt_lo) = TWIN(regno);\
	HALF(curs, bt_hi) = 0;		\
	tc_wmb();			\
   } while (0);


struct fb_devconfig {
	vaddr_t dc_vaddr;		/* memory space virtual base address */
	paddr_t dc_paddr;		/* memory space physical base address */
	vsize_t dc_size;		/* size of slot memory */
	int	dc_wid;			/* width of frame buffer */
	int	dc_ht;			/* height of frame buffer */
	int	dc_depth;		/* depth, bits per pixel */
	int	dc_rowbytes;		/* bytes in a FB scan line */
	vaddr_t dc_videobase;		/* base of flat frame buffer */
	struct raster	dc_raster;	/* raster description */
	struct rcons	dc_rcons;	/* raster blitter control info */
	int	    dc_blanked;		/* currently has video disabled */
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

struct mfb_softc {
	struct device sc_dev;
	struct fb_devconfig *sc_dc;	/* device configuration */
	struct hwcursor64 sc_cursor;	/* software copy of cursor */
	int sc_curenb;			/* cursor sprite enabled */
	int sc_changed;			/* need update of colormap */
#define	DATA_ENB_CHANGED	0x01	/* cursor enable changed */
#define	DATA_CURCMAP_CHANGED	0x02	/* cursor colormap changed */
#define	DATA_CURSHAPE_CHANGED	0x04	/* cursor size, image, mask changed */
#define	DATA_CMAP_CHANGED	0x08	/* colormap changed */
#define	DATA_ALL_CHANGED	0x0f
	int nscreens;
};

#define	MX_MAGIC_X 360
#define	MX_MAGIC_Y 36

#define	MX_FB_OFFSET	0x200000
#define	MX_FB_SIZE	 0x40000
#define	MX_BT455_OFFSET	0x100000
#define	MX_BT431_OFFSET	0x180000
#define	MX_IREQ_OFFSET	0x080000	/* Interrupt req. control */

int  mfbmatch __P((struct device *, struct cfdata *, void *));
void mfbattach __P((struct device *, struct device *, void *));

struct cfattach mfb_ca = {
	sizeof(struct mfb_softc), mfbmatch, mfbattach,
};

void mfb_getdevconfig __P((tc_addr_t, struct fb_devconfig *));
struct fb_devconfig mfb_console_dc;
tc_addr_t mfb_consaddr;

struct wsdisplay_emulops mfb_emulops = {
	rcons_cursor,			/* could use hardware cursor; punt */
	rcons_mapchar,
	rcons_putchar,
	rcons_copycols,
	rcons_erasecols,
	rcons_copyrows,
	rcons_eraserows,
	rcons_alloc_attr
};

struct wsscreen_descr mfb_stdscreen = {
	"std", 0, 0,
	&mfb_emulops,
	0, 0,
	0
};

const struct wsscreen_descr *_mfb_scrlist[] = {
	&mfb_stdscreen,
};

struct wsscreen_list mfb_screenlist = {
	sizeof(_mfb_scrlist) / sizeof(struct wsscreen_descr *), _mfb_scrlist
};

int	mfbioctl __P((void *, u_long, caddr_t, int, struct proc *));
int	mfbmmap __P((void *, off_t, int));

int	mfb_alloc_screen __P((void *, const struct wsscreen_descr *,
				      void **, int *, int *, long *));
void	mfb_free_screen __P((void *, void *));
void	mfb_show_screen __P((void *, void *));

struct wsdisplay_accessops mfb_accessops = {
	mfbioctl,
	mfbmmap,
	mfb_alloc_screen,
	mfb_free_screen,
	mfb_show_screen,
	0 /* load_font */
};

int  mfb_cnattach __P((tc_addr_t));
int  mfbintr __P((void *));
void mfbinit __P((struct fb_devconfig *));

static int  set_cursor __P((struct mfb_softc *, struct wsdisplay_cursor *));
static int  get_cursor __P((struct mfb_softc *, struct wsdisplay_cursor *));
static void set_curpos __P((struct mfb_softc *, struct wsdisplay_curpos *));
void bt431_set_curpos __P((struct mfb_softc *));

/* bit order reverse */
const static u_int8_t flip[256] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

int
mfbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct tc_attach_args *ta = aux;

	if (strncmp("PMAG-AA ", ta->ta_modname, TC_ROM_LLEN) != 0)
		return (0);

	return (1);
}

void
mfb_getdevconfig(dense_addr, dc)
	tc_addr_t dense_addr;
	struct fb_devconfig *dc;
{
	struct raster *rap;
	struct rcons *rcp;
	int i;

	dc->dc_vaddr = dense_addr;
	dc->dc_paddr = MACHINE_KSEG0_TO_PHYS(dc->dc_vaddr + MX_FB_OFFSET);

	dc->dc_wid = 1280;
	dc->dc_ht = 1024;
	dc->dc_depth = 8;
	dc->dc_rowbytes = 2048;
	dc->dc_videobase = dc->dc_vaddr + MX_FB_OFFSET;
	dc->dc_blanked = 0;

	/* initialize colormap and cursor resource */
	mfbinit(dc);

	/* clear the screen */
	for (i = 0; i < dc->dc_ht * dc->dc_rowbytes; i += sizeof(u_int32_t))
		*(u_int32_t *)(dc->dc_videobase + i) = 0;

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

	mfb_stdscreen.nrows = dc->dc_rcons.rc_maxrow;
	mfb_stdscreen.ncols = dc->dc_rcons.rc_maxcol;
}

void
mfbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mfb_softc *sc = (struct mfb_softc *)self;
	struct tc_attach_args *ta = aux;
	struct wsemuldisplaydev_attach_args waa;
	caddr_t mfbbase;
	int console;
	volatile register int junk;

	console = (ta->ta_addr == mfb_consaddr);
	if (console) {
		sc->sc_dc = &mfb_console_dc;
		sc->nscreens = 1;
	}
	else {
		sc->sc_dc = (struct fb_devconfig *)
		    malloc(sizeof(struct fb_devconfig), M_DEVBUF, M_WAITOK);
		mfb_getdevconfig(ta->ta_addr, sc->sc_dc);
	}
	printf(": %d x %d, %dbpp\n", sc->sc_dc->dc_wid, sc->sc_dc->dc_ht,
	    sc->sc_dc->dc_depth);

	sc->sc_cursor.cc_magic.x = MX_MAGIC_X;
	sc->sc_cursor.cc_magic.y = MX_MAGIC_Y;

        tc_intr_establish(parent, ta->ta_cookie, TC_IPL_TTY, mfbintr, sc);

	mfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	*(u_int8_t *)(mfbbase + MX_IREQ_OFFSET) = 0;
	junk = *(u_int8_t *)(mfbbase + MX_IREQ_OFFSET);
	*(u_int8_t *)(mfbbase + MX_IREQ_OFFSET) = 1;

	waa.console = console;
	waa.scrdata = &mfb_screenlist;
	waa.accessops = &mfb_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}

int
mfbioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct mfb_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;
	int turnoff;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_MFB;
		return (0);

	case WSDISPLAYIO_GINFO:
#define	wsd_fbip ((struct wsdisplay_fbinfo *)data)
		wsd_fbip->height = sc->sc_dc->dc_ht;
		wsd_fbip->width = sc->sc_dc->dc_wid;
		wsd_fbip->depth = sc->sc_dc->dc_depth;
		wsd_fbip->cmsize = 0;
#undef fbt
		return (0);

	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		return (ENOTTY);

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
		bt431_set_curpos(sc);
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
	return (ENOTTY);
}

int
mfbmmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct mfb_softc *sc = v;

	if (offset >= MX_FB_SIZE || offset < 0)
		return (-1);
	return machine_btop(sc->sc_dc->dc_paddr + offset);
}

int
mfb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct mfb_softc *sc = v;
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
mfb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct mfb_softc *sc = v;

	if (sc->sc_dc == &mfb_console_dc)
		panic("mfb_free_screen: console");

	sc->nscreens--;
}

void
mfb_show_screen(v, cookie)
	void *v;
	void *cookie;
{
}

int
mfb_cnattach(addr)
        tc_addr_t addr;
{
        struct fb_devconfig *dcp = &mfb_console_dc;
        long defattr;

        mfb_getdevconfig(addr, dcp);
 
        rcons_alloc_attr(&dcp->dc_rcons, 0, 0, 0, &defattr);

        wsdisplay_cnattach(&mfb_stdscreen, &dcp->dc_rcons,
                           0, 0, defattr);
        mfb_consaddr = addr;
        return (0);
}


int
mfbintr(arg)
	void *arg;
{
	struct mfb_softc *sc = arg;
	caddr_t mfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	void *vdac;
	void *curs;
	int v;
	volatile register int junk;
	
	junk = *(u_int8_t *)(mfbbase + MX_IREQ_OFFSET);
	/* *(u_int8_t *)(mfbbase + MX_IREQ_OFFSET) = 1; */

	if (sc->sc_changed == 0)
		return (1);

	vdac = (void *)(mfbbase + MX_BT455_OFFSET);
	curs = (void *)(mfbbase + MX_BT431_OFFSET);

	v = sc->sc_changed;
	sc->sc_changed = 0;	
	if (v & DATA_ENB_CHANGED) {
		BT431_SELECT(curs, BT431_REG_COMMAND);
		HALF(curs, bt_ctl) = (sc->sc_curenb) ? 0x4444 : 0x0404;
	}
	if (v & DATA_CURCMAP_CHANGED) {
		u_int8_t *cp = sc->sc_cursor.cc_color;

		BT455_SELECT(vdac, 8);
		BYTE(vdac, bt_cmap) = 0;	tc_wmb();
		BYTE(vdac, bt_cmap) = cp[1];	tc_wmb();
		BYTE(vdac, bt_cmap) = 0;	tc_wmb();

		BYTE(vdac, bt_cmap) = 0;	tc_wmb();
		BYTE(vdac, bt_cmap) = cp[1];	tc_wmb();
		BYTE(vdac, bt_cmap) = 0;	tc_wmb();

		BYTE(vdac, bt_ovly) = 0;	tc_wmb();
		BYTE(vdac, bt_ovly) = cp[0];	tc_wmb();
		BYTE(vdac, bt_ovly) = 0;	tc_wmb();
	}
	if (v & DATA_CURSHAPE_CHANGED) {
		u_int8_t *ip, *mp, img, msk;
		int bcnt;

		ip = (u_int8_t *)sc->sc_cursor.cc_image;
		mp = (u_int8_t *)(sc->sc_cursor.cc_image + CURSOR_MAX_SIZE);
		bcnt = 0;
		BT431_SELECT(curs, BT431_REG_CRAM_BASE);

		/* 64 pixel scan line is consisted with 16 byte cursor ram */
		while (bcnt < sc->sc_cursor.cc_size.y * 16) {
			/* pad right half 32 pixel when smaller than 33 */
			if ((bcnt & 0x8) && sc->sc_cursor.cc_size.x < 33) {
				HALF(curs, bt_ram) = 0;
				tc_wmb();
			}
			else {
				img = *ip++;
				msk = *mp++;
				img &= msk;	/* cookie off image */
				HALF(curs, bt_ram)
				    = (flip[msk] << 8) | flip[img];
				tc_wmb();
			}
			bcnt += 2;
		}
		/* pad unoccupied scan lines */
		while (bcnt < CURSOR_MAX_SIZE * 16) {
			HALF(curs, bt_ram) = 0;
			tc_wmb();
			bcnt += 2;
		}
	}
	return (1);
}

void
mfbinit(dc)
	struct fb_devconfig *dc;
{
	caddr_t mfbbase = (caddr_t)dc->dc_vaddr;
	void *curs = (void *)(mfbbase + MX_BT431_OFFSET);
	void *vdac = (void *)(mfbbase + MX_BT455_OFFSET);
	int i;

	BT431_SELECT(curs, BT431_REG_COMMAND);
	HALF(curs, bt_ctl) = 0x0404;		tc_wmb();
	HALF(curs, bt_ctl) = 0; /* XLO */	tc_wmb();
	HALF(curs, bt_ctl) = 0; /* XHI */	tc_wmb();
	HALF(curs, bt_ctl) = 0; /* YLO */	tc_wmb();
	HALF(curs, bt_ctl) = 0; /* YHI */	tc_wmb();
	HALF(curs, bt_ctl) = 0; /* XWLO */	tc_wmb();
	HALF(curs, bt_ctl) = 0; /* XWHI */	tc_wmb();
	HALF(curs, bt_ctl) = 0; /* WYLO */	tc_wmb();
	HALF(curs, bt_ctl) = 0; /* WYLO */	tc_wmb();
	HALF(curs, bt_ctl) = 0; /* WWLO */	tc_wmb();
	HALF(curs, bt_ctl) = 0; /* WWHI */	tc_wmb();
	HALF(curs, bt_ctl) = 0; /* WHLO */	tc_wmb();
	HALF(curs, bt_ctl) = 0; /* WHHI */	tc_wmb();

	/* 0: black, 1: white, 8,9: cursor mask, ovly: cursor image */
	BT455_SELECT(vdac, 0);
	BYTE(vdac, bt_cmap) = 0; 		tc_wmb();
	BYTE(vdac, bt_cmap) = 0; 		tc_wmb();
	BYTE(vdac, bt_cmap) = 0;		tc_wmb();
	BYTE(vdac, bt_cmap) = 0;		tc_wmb();
	BYTE(vdac, bt_cmap) = 0xff;		tc_wmb();
	BYTE(vdac, bt_cmap) = 0;		tc_wmb();
	for (i = 2; i < 16; i++) {
		BYTE(vdac, bt_cmap) = 0;	tc_wmb();
		BYTE(vdac, bt_cmap) = 0;	tc_wmb();
		BYTE(vdac, bt_cmap) = 0;	tc_wmb();
	}
	BYTE(vdac, bt_ovly) = 0;	tc_wmb();
	BYTE(vdac, bt_ovly) = 0xff;	tc_wmb();
	BYTE(vdac, bt_ovly) = 0;	tc_wmb();

	BT431_SELECT(curs, BT431_REG_CRAM_BASE);
	for (i = 0; i < 512; i++) {
		HALF(curs, bt_ram) = 0;	tc_wmb();
	}
}

static int
set_cursor(sc, p)
	struct mfb_softc *sc;
	struct wsdisplay_cursor *p;
{
#define	cc (&sc->sc_cursor)
	int v, count, index;

	v = p->which;
	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		index = p->cmap.index;
		count = p->cmap.count;
		if (index >= 2 || (index + count) > 2)
			return (EINVAL);
		if (!uvm_useracc(p->cmap.red, count, B_READ))
			return (EFAULT);
	}
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		if (p->size.x > CURSOR_MAX_SIZE || p->size.y > CURSOR_MAX_SIZE)
			return (EINVAL);
		count = ((p->size.x < 33) ? 4 : 8) * p->size.y;
		if (!uvm_useracc(p->image, count, B_READ) ||
		    !uvm_useracc(p->mask, count, B_READ))
			return (EFAULT);
	}
	if (v & (WSDISPLAY_CURSOR_DOPOS | WSDISPLAY_CURSOR_DOCUR)) {
		if (v & WSDISPLAY_CURSOR_DOCUR)
			cc->cc_hot = p->hot;
		if (v & WSDISPLAY_CURSOR_DOPOS)
			set_curpos(sc, &p->pos);
		bt431_set_curpos(sc);
	}

	sc->sc_changed = 0;
	if (v & WSDISPLAY_CURSOR_DOCUR) {
		sc->sc_curenb = p->enable;
		sc->sc_changed |= DATA_ENB_CHANGED;
	}
	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		copyin(p->cmap.red, &cc->cc_color[index], count);
		sc->sc_changed |= DATA_CURCMAP_CHANGED;
	}
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		cc->cc_size = p->size;
		memset(cc->cc_image, 0, sizeof cc->cc_image);
		copyin(p->image, cc->cc_image, count);
		copyin(p->mask, cc->cc_image+CURSOR_MAX_SIZE, count);
		sc->sc_changed |= DATA_CURSHAPE_CHANGED;
	}

	return (0);
#undef cc
}

static int
get_cursor(sc, p)
	struct mfb_softc *sc;
	struct wsdisplay_cursor *p;
{
	return (ENOTTY); /* XXX */
}

static void
set_curpos(sc, curpos)
	struct mfb_softc *sc;
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
bt431_set_curpos(sc)
	struct mfb_softc *sc;
{
	caddr_t mfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	void *curs = (void *)(mfbbase + MX_BT431_OFFSET);
	u_int16_t twin;
	int x, y, s;

	x = sc->sc_cursor.cc_pos.x - sc->sc_cursor.cc_hot.x;
	y = sc->sc_cursor.cc_pos.y - sc->sc_cursor.cc_hot.y;

	x += sc->sc_cursor.cc_magic.x;
	y += sc->sc_cursor.cc_magic.y;

	s = spltty();

	BT431_SELECT(curs, BT431_REG_CURSOR_X_LOW);
	HALF(curs, bt_ctl) = TWIN_LO(x);	tc_wmb();
	HALF(curs, bt_ctl) = TWIN_HI(x);	tc_wmb();
	HALF(curs, bt_ctl) = TWIN_LO(y);	tc_wmb();
	HALF(curs, bt_ctl) = TWIN_HI(y);	tc_wmb();

	splx(s);
}
