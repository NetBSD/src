/* $NetBSD: sfb.c,v 1.17 1999/06/11 01:44:47 nisimura Exp $ */

/*
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

__KERNEL_RCSID(0, "$NetBSD: sfb.c,v 1.17 1999/06/11 01:44:47 nisimura Exp $");

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
#include <dev/tc/sfbreg.h>

#include <uvm/uvm_extern.h>

#if defined(pmax)
#define	machine_btop(x) mips_btop(x)
#define	MACHINE_KSEG0_TO_PHYS(x) MIPS_KSEG1_TO_PHYS(x)
#endif

#if defined(__alpha__) || defined(alpha)
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
 *		struct {
 *
 * Although HX has single Bt459, 32bit R/W can be done w/o any trouble.
 *	struct bt459reg {
 *		   u_int32_t	   bt_lo;
 *		   u_int32_t	   bt_hi;
 *		   u_int32_t	   bt_reg;
 *		   u_int32_t	   bt_cmap;
 *	};
 *
 */

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
};

#define	HX_MAGIC_X 368
#define	HX_MAGIC_Y 38

int  sfbmatch __P((struct device *, struct cfdata *, void *));
void sfbattach __P((struct device *, struct device *, void *));

struct cfattach sfb_ca = {
	sizeof(struct sfb_softc), sfbmatch, sfbattach,
};

void sfb_getdevconfig __P((tc_addr_t, struct fb_devconfig *));
struct fb_devconfig sfb_console_dc;
tc_addr_t sfb_consaddr;

void	sfb_cursor __P((void *, int, int, int));
int	sfb_mapchar __P((void *, int, unsigned int *));
void	sfb_putchar __P((void *, int, int, u_int, long));
void	sfb_copycols __P((void *, int, int, int, int));
void	sfb_erasecols __P((void *, int, int, int, long));
void	sfb_copyrows __P((void *, int, int, int));
void	sfb_eraserows __P((void *, int, int, long));
int	sfb_alloc_attr __P((void *, int, int, int, long *));
#define	rcons_alloc_attr sfb_alloc_attr

struct wsdisplay_emulops sfb_emulops = {
	sfb_cursor,			/* could use hardware cursor; punt */
	sfb_mapchar,
	sfb_putchar,
	sfb_copycols,
	sfb_erasecols,
	sfb_copyrows,
	sfb_eraserows,
	sfb_alloc_attr
};

struct wsscreen_descr sfb_stdscreen = {
	"std", 0, 0,
	&sfb_emulops,
	0, 0,
	0
};

const struct wsscreen_descr *_sfb_scrlist[] = {
	&sfb_stdscreen,
};

struct wsscreen_list sfb_screenlist = {
	sizeof(_sfb_scrlist) / sizeof(struct wsscreen_descr *), _sfb_scrlist
};

int	sfbioctl __P((void *, u_long, caddr_t, int, struct proc *));
int	sfbmmap __P((void *, off_t, int));

int	sfb_alloc_screen __P((void *, const struct wsscreen_descr *,
				      void **, int *, int *, long *));
void	sfb_free_screen __P((void *, void *));
void	sfb_show_screen __P((void *, void *));

struct wsdisplay_accessops sfb_accessops = {
	sfbioctl,
	sfbmmap,
	sfb_alloc_screen,
	sfb_free_screen,
	sfb_show_screen,
	0 /* load_font */
};

int  sfb_cnattach __P((tc_addr_t));
int  sfbintr __P((void *));
void sfbinit __P((struct fb_devconfig *));

static int  get_cmap __P((struct sfb_softc *, struct wsdisplay_cmap *));
static int  set_cmap __P((struct sfb_softc *, struct wsdisplay_cmap *));
static int  set_cursor __P((struct sfb_softc *, struct wsdisplay_cursor *));
static int  get_cursor __P((struct sfb_softc *, struct wsdisplay_cursor *));
static void set_curpos __P((struct sfb_softc *, struct wsdisplay_curpos *));
static void bt459_set_curpos __P((struct sfb_softc *));


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
sfbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct tc_attach_args *ta = aux;

	if (strncmp("PMAGB-BA", ta->ta_modname, TC_ROM_LLEN) != 0)
		return (0);

	return (1);
}

void
sfb_getdevconfig(dense_addr, dc)
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
	vbase  = *(u_int32_t *)(sfbasic + SFB_ASIC_VIDEO_BASE) & 0x1ff;

	dc->dc_wid = (hsetup & 0x1ff) << 2;
	dc->dc_ht = (vsetup & 0x7ff);
	dc->dc_depth = 8;
	dc->dc_rowbytes = dc->dc_wid * (dc->dc_depth / 8);
	dc->dc_videobase = dc->dc_vaddr + SFB_FB_OFFSET + vbase * 4096;
	dc->dc_blanked = 0;

	/* initialize colormap and cursor resource */
	sfbinit(dc);

	/* clear the screen */
	for (i = 0; i < dc->dc_ht * dc->dc_rowbytes; i += sizeof(u_int32_t))
		*(u_int32_t *)(dc->dc_videobase + i) = 0x0;

	*(u_int32_t *)(sfbasic + SFB_ASIC_VIDEO_VALID) = 1;

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
}

void
sfbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sfb_softc *sc = (struct sfb_softc *)self;
	struct tc_attach_args *ta = aux;
	struct wsemuldisplaydev_attach_args waa;
	struct hwcmap256 *cm;
	caddr_t sfbasic;
	int console;

	console = (ta->ta_addr == sfb_consaddr);
	if (console) {
		sc->sc_dc = &sfb_console_dc;
		sc->nscreens = 1;
	}
	else {
		sc->sc_dc = (struct fb_devconfig *)
		    malloc(sizeof(struct fb_devconfig), M_DEVBUF, M_WAITOK);
		sfb_getdevconfig(ta->ta_addr, sc->sc_dc);
	}
	printf(": %d x %d, %dbpp\n", sc->sc_dc->dc_wid, sc->sc_dc->dc_ht,
	    sc->sc_dc->dc_depth);

	cm = &sc->sc_cmap;
	memset(cm, 255, sizeof(struct hwcmap256));	/* XXX */
	cm->r[0] = cm->g[0] = cm->b[0] = 0;		/* XXX */

	sc->sc_cursor.cc_magic.x = HX_MAGIC_X;
	sc->sc_cursor.cc_magic.y = HX_MAGIC_Y;

        tc_intr_establish(parent, ta->ta_cookie, TC_IPL_TTY, sfbintr, sc);

	sfbasic = (caddr_t)(sc->sc_dc->dc_vaddr + SFB_ASIC_OFFSET);
	*(u_int32_t *)(sfbasic + SFB_ASIC_CLEAR_INTR) = 0;
	*(u_int32_t *)(sfbasic + SFB_ASIC_ENABLE_INTR) = 1;

	waa.console = console;
	waa.scrdata = &sfb_screenlist;
	waa.accessops = &sfb_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}

int
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
		*(u_int *)data = WSDISPLAY_TYPE_SFB;
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
sfbmmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct sfb_softc *sc = v;

	if (offset >= SFB_SIZE || offset < 0)
		return (-1);
	return machine_btop(sc->sc_dc->dc_paddr + offset);
}

int
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
	rcons_alloc_attr(&sc->sc_dc->dc_rcons, 0, 0, 0, &defattr);
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

	if (sc->sc_dc == &sfb_console_dc)
		panic("sfb_free_screen: console");

	sc->nscreens--;
}

void
sfb_show_screen(v, cookie)
	void *v;
	void *cookie;
{
}

int
sfb_cnattach(addr)
        tc_addr_t addr;
{
        struct fb_devconfig *dcp = &sfb_console_dc;
        long defattr;

        sfb_getdevconfig(addr, dcp);
 
        rcons_alloc_attr(&dcp->dc_rcons, 0, 0, 0, &defattr);

        wsdisplay_cnattach(&sfb_stdscreen, &dcp->dc_rcons,
                           0, 0, defattr);
        sfb_consaddr = addr;
        return(0);
}

int
sfbintr(arg)
	void *arg;
{
	struct sfb_softc *sc = arg;
	caddr_t sfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	caddr_t sfbasic = sfbbase + SFB_ASIC_OFFSET;
	caddr_t vdac;
	int v;
	
	*(u_int32_t *)(sfbasic + SFB_ASIC_CLEAR_INTR) = 0;
	/* *(u_int32_t *)(sfbasic + SFB_ASIC_ENABLE_INTR) = 1; */

	if (sc->sc_changed == 0)
		return (1);

	vdac = (void *)(sfbbase + SFB_RAMDAC_OFFSET);
	v = sc->sc_changed;
	sc->sc_changed = 0;

	if (v & DATA_ENB_CHANGED) {
		SELECT(vdac, BT459_REG_CCR);
		REG(vdac, bt_reg) = (sc->sc_curenb) ? 0xc0 : 0x00;
	}
	if (v & DATA_CURCMAP_CHANGED) {
		u_int8_t *cp = sc->sc_cursor.cc_color;

		SELECT(vdac, BT459_REG_CCOLOR_2);
		REG(vdac, bt_reg) = cp[1];	tc_wmb();
		REG(vdac, bt_reg) = cp[3];	tc_wmb();
		REG(vdac, bt_reg) = cp[5];	tc_wmb();

		REG(vdac, bt_reg) = cp[0];	tc_wmb();
		REG(vdac, bt_reg) = cp[2];	tc_wmb();
		REG(vdac, bt_reg) = cp[4];	tc_wmb();
	}
	if (v & DATA_CURSHAPE_CHANGED) {
		u_int8_t *ip, *mp, img, msk;
		u_int8_t u;
		int bcnt;

		ip = (u_int8_t *)sc->sc_cursor.cc_image;
		mp = (u_int8_t *)(sc->sc_cursor.cc_image + CURSOR_MAX_SIZE);

		bcnt = 0;
		SELECT(vdac, BT459_REG_CRAM_BASE+0);
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
		while (bcnt < CURSOR_MAX_SIZE * 16) {
			REG(vdac, bt_reg) = 0; tc_wmb();
			REG(vdac, bt_reg) = 0; tc_wmb();
			bcnt += 2;
		}
	}
	if (v & DATA_CMAP_CHANGED) {
		struct hwcmap256 *cm = &sc->sc_cmap;
		int index;

		SELECT(vdac, 0);
		for (index = 0; index < CMAP_SIZE; index++) {
			REG(vdac, bt_cmap) = cm->r[index];	tc_wmb();
			REG(vdac, bt_cmap) = cm->g[index];	tc_wmb();
			REG(vdac, bt_cmap) = cm->b[index];	tc_wmb();
		}
	}
	return (1);
}

void
sfbinit(dc)
	struct fb_devconfig *dc;
{
	caddr_t sfbasic = (caddr_t)(dc->dc_vaddr + SFB_ASIC_OFFSET);
	caddr_t vdac = (void *)(dc->dc_vaddr + SFB_RAMDAC_OFFSET);
	int i;

	*(u_int32_t *)(sfbasic + SFB_ASIC_VIDEO_VALID) = 0;
	*(u_int32_t *)(sfbasic + SFB_ASIC_PLANEMASK) = ~0;
	*(u_int32_t *)(sfbasic + SFB_ASIC_PIXELMASK) = ~0;
	*(u_int32_t *)(sfbasic + SFB_ASIC_MODE) = 0;
	*(u_int32_t *)(sfbasic + SFB_ASIC_ROP) = 3;
	
	*(u_int32_t *)(sfbasic + 0x180000) = 0; /* Bt459 reset */

	SELECT(vdac, BT459_REG_COMMAND_0);
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

	SELECT(vdac, BT459_REG_CCR);
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
	SELECT(vdac, BT459_REG_CRAM_BASE);
	for (i = 0; i < 1024; i++)
		REG(vdac, bt_reg) = 0xff;	tc_wmb();

	/*
	 * 2 bit/pixel cursor.  Assign MSB for cursor mask and LSB for
	 * cursor image.  CCOLOR_2 for mask color, while CCOLOR_3 for
	 * image color.  CCOLOR_1 will be never used.
	 */
	SELECT(vdac, BT459_REG_CCOLOR_1);
	REG(vdac, bt_reg) = 0xff;	tc_wmb();
	REG(vdac, bt_reg) = 0xff;	tc_wmb();
	REG(vdac, bt_reg) = 0xff;	tc_wmb();

	REG(vdac, bt_reg) = 0;	tc_wmb();
	REG(vdac, bt_reg) = 0;	tc_wmb();
	REG(vdac, bt_reg) = 0;	tc_wmb();

	REG(vdac, bt_reg) = 0xff;	tc_wmb();
	REG(vdac, bt_reg) = 0xff;	tc_wmb();
	REG(vdac, bt_reg) = 0xff;	tc_wmb();
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
	int v, index, count, icount;

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
	struct sfb_softc *sc;
	struct wsdisplay_cursor *p;
{
	return (ENOTTY); /* XXX */
}

static void
set_curpos(sc, curpos)
	struct sfb_softc *sc;
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

static void
bt459_set_curpos(sc)
	struct sfb_softc *sc;
{
	caddr_t sfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	caddr_t vdac = (void *)(sfbbase + SFB_RAMDAC_OFFSET);
	int x, y, s;

	x = sc->sc_cursor.cc_pos.x - sc->sc_cursor.cc_hot.x;
	y = sc->sc_cursor.cc_pos.y - sc->sc_cursor.cc_hot.y;

	x += sc->sc_cursor.cc_magic.x;
	y += sc->sc_cursor.cc_magic.y;
	
	s = spltty();

	SELECT(vdac, BT459_REG_CURSOR_X_LOW);
	REG(vdac, bt_reg) = x;		tc_wmb();
	REG(vdac, bt_reg) = x >> 8;	tc_wmb();
	REG(vdac, bt_reg) = y;		tc_wmb();
	REG(vdac, bt_reg) = y >> 8;	tc_wmb();

	splx(s);
}

#define	MODE_SIMPLE		0
#define	MODE_OPAQUESTIPPLE	1
#define	MODE_OPAQUELINE		2
#define	MODE_TRANSPARENTSTIPPLE	5
#define	MODE_TRANSPARENTLINE	6
#define	MODE_COPY		7

#define	SFBALIGNMASK		0x7
#define	SFBSTIPPLEALL1		0xffffffff
#define	SFBSTIPPLEBITS		32
#define	SFBSTIPPLEBITMASK	0x1f
#define	SFBSTIPPLEBYTESDONE	32

/*
 * Paint (or unpaint) the cursor.
 */
void
sfb_cursor(id, on, row, col)
	void *id;
	int on, row, col;
{
	struct rcons *rc = id;
	int x, y;

	/* turn the cursor off */
	if (!on) {
		/* make sure it's on */
		if ((rc->rc_bits & RC_CURSOR) == 0)
			return;

		row = *rc->rc_crowp;
		col = *rc->rc_ccolp;
	} else {
		/* unpaint the old copy. */
		*rc->rc_crowp = row;
		*rc->rc_ccolp = col;
	}

	x = col * rc->rc_font->width + rc->rc_xorigin;
	y = row * rc->rc_font->height + rc->rc_yorigin;

	raster_op(rc->rc_sp, x, y,
	    rc->rc_font->width, rc->rc_font->height,
	    RAS_INVERT,
	    (struct raster *) 0, 0, 0);

	rc->rc_bits ^= RC_CURSOR;
}

/*
 * Actually write a string to the frame buffer.
 */
int
sfb_mapchar(id, uni, index)
	void *id;
	int uni;
	unsigned int *index;
{
	if (uni < 128) {
		*index = uni;
		return (5);
	}
	*index = ' ';
	return (0);
}

/*
 * Actually write a string to the frame buffer.
 */
void
sfb_putchar(id, row, col, uc, attr)
	void *id;
	int row, col;
	u_int uc;
	long attr;
{
	struct rcons *rc = id;
	int x, y, op;
	u_char help;

	x = col * rc->rc_font->width + rc->rc_xorigin;
	y = row * rc->rc_font->height + rc->rc_font_ascent + rc->rc_yorigin;

	op = RAS_SRC;
	if ((attr != 0) ^ ((rc->rc_bits & RC_INVERT) != 0))
		op = RAS_NOT(op);
	help = uc & 0xff;
	raster_textn(rc->rc_sp, x, y, op, rc->rc_font, &help, 1);
}

/*
 * Copy characters in a line.
 */
void
sfb_copycols(id, row, srccol, dstcol, ncols)
	void *id;
	int row, srccol, dstcol, ncols;
{
	struct rcons *rc = id;
	struct raster *rap = rc->rc_sp;
	caddr_t sp, dp, basex, sfb;
	int scanspan, height, width, aligns, alignd, w, y;
	u_int32_t lmasks, rmasks, lmaskd, rmaskd;

	scanspan = rap->linelongs * 4;
	y = rc->rc_yorigin + rc->rc_font->height * row;
	basex = (caddr_t)rap->pixels + y * scanspan + rc->rc_xorigin;
	height = rc->rc_font->height;
	w = rc->rc_font->width * ncols;

	dp = basex + rc->rc_font->width * dstcol;
	alignd = (long)dp & SFBALIGNMASK;
	dp -= alignd;
	width = w + alignd;
	lmaskd = SFBSTIPPLEALL1 << alignd;
	rmaskd = SFBSTIPPLEALL1 >> (-width & SFBSTIPPLEBITMASK);

	sp = basex + rc->rc_font->width * srccol;
	aligns = (long)sp & SFBALIGNMASK;
	sp -= aligns;
	width = w + aligns;
	lmasks = SFBSTIPPLEALL1 << aligns;
	rmasks = SFBSTIPPLEALL1 >> (-width & SFBSTIPPLEBITMASK);

	width += (-width & SFBSTIPPLEBITMASK);
	sfb = rap->data;
	*(u_int32_t *)(sfb + SFB_ASIC_MODE) = MODE_COPY;
	*(u_int32_t *)(sfb + SFB_ASIC_PLANEMASK) = ~0;
	*(u_int32_t *)(sfb + SFB_ASIC_PIXELSHIFT) = alignd - aligns;

	if (width <= SFBSTIPPLEBITS) {
		while (height > 0) {
			*(u_int32_t *)sp = lmasks & rmasks;
			*(u_int32_t *)dp = lmaskd & rmaskd;
			sp += scanspan;
			dp += scanspan;
			height--;
		}
	}
	/* copy forward (left-to-right) */
	else if (dstcol < srccol || srccol + ncols < dstcol) {
		caddr_t sq = sp, dq = dp;

		w = width;
		while (height > 0) {
			*(u_int32_t *)sp = lmasks;
			*(u_int32_t *)dp = lmaskd;
			width -= 2 * SFBSTIPPLEBITS;
			while (width > 0) {
				sp += SFBSTIPPLEBYTESDONE;
				dp += SFBSTIPPLEBYTESDONE;
				*(u_int32_t *)sp = SFBSTIPPLEALL1;
				*(u_int32_t *)dp = SFBSTIPPLEALL1;
				width -= SFBSTIPPLEBITS;
			}
			sp += SFBSTIPPLEBYTESDONE;
			dp += SFBSTIPPLEBYTESDONE;
			*(u_int32_t *)sp = rmasks;
			*(u_int32_t *)dp = rmaskd;

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			width = w;
			height--;
		}
	}
	/* copy backward (right-to-left) */
	else {
		caddr_t sq = (sp += width), dq = (dp += width);

		w = width;
		while (height > 0) {
			*(u_int32_t *)sp = rmasks;
			*(u_int32_t *)dp = rmaskd;
			width -= 2 * SFBSTIPPLEBITS;
			while (width > 0) {
				sp -= SFBSTIPPLEBYTESDONE;
				dp -= SFBSTIPPLEBYTESDONE;
				*(u_int32_t *)sp = SFBSTIPPLEALL1;
				*(u_int32_t *)dp = SFBSTIPPLEALL1;
				width -= SFBSTIPPLEBITS;
			}
			sp -= SFBSTIPPLEBYTESDONE;
			dp -= SFBSTIPPLEBYTESDONE;
			*(u_int32_t *)sp = lmasks;
			*(u_int32_t *)dp = lmaskd;

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			width = w;
			height--;
		}
	}
	*(u_int32_t *)(sfb + SFB_ASIC_MODE) = MODE_SIMPLE;
}

/*
 * Clear characters in a line.
 */
void
sfb_erasecols(id, row, startcol, ncols, fillattr)
	void *id;
	int row, startcol, ncols;
	long fillattr;
{
	struct rcons *rc = id;
	struct raster *rap = rc->rc_sp;
	caddr_t sfb, p;
	int scanspan, startx, height, width, align, w, y;
	u_int32_t lmask, rmask;

	scanspan = rap->linelongs * 4;
	y = rc->rc_yorigin + rc->rc_font->height * row;
	startx = rc->rc_xorigin + rc->rc_font->width * startcol;
	height = rc->rc_font->height;
	w = rc->rc_font->width * ncols;

	p = (caddr_t)rap->pixels + y * scanspan + startx;
	align = (long)p & SFBALIGNMASK;
	p -= align;
	width = w + align;
	lmask = SFBSTIPPLEALL1 << align;
	rmask = SFBSTIPPLEALL1 >> (-width & SFBSTIPPLEBITMASK);
	sfb = rap->data;
	*(u_int32_t *)(sfb + SFB_ASIC_MODE) = MODE_TRANSPARENTSTIPPLE;
	*(u_int32_t *)(sfb + SFB_ASIC_PLANEMASK) = ~0;
	*(u_int32_t *)(sfb + SFB_ASIC_FG) = 0;
	if (width <= SFBSTIPPLEBITS) {
		while (height > 0) {
			*(u_int32_t *)(sfb + SFB_ASIC_ADDRESS) = (long)p;
			*(u_int32_t *)(sfb + SFB_ASIC_START) = lmask & rmask;
			p += scanspan;
			height--;
		}
	}
	else {
		caddr_t q = p;
		while (height > 0) {
			*(u_int32_t *)p = lmask;
			width -= 2 * SFBSTIPPLEBITS;
			while (width > 0) {
				p += SFBSTIPPLEBYTESDONE;
				*(u_int32_t *)p = SFBSTIPPLEALL1;
				width -= SFBSTIPPLEBITS;
			}
			p += SFBSTIPPLEBYTESDONE;
			*(u_int32_t *)p = rmask;

			p = (q += scanspan);
			width = w + align;
			height--;
		}
	}
	*(u_int32_t *)(sfb + SFB_ASIC_MODE) = MODE_SIMPLE;
}

/*
 * Copy lines.
 */
void
sfb_copyrows(id, srcrow, dstrow, nrows)
	void *id;
	int srcrow, dstrow, nrows;
{
	struct rcons *rc = id;
	struct raster *rap = rc->rc_sp;
	caddr_t sfb, p;
	int scanspan, offset, srcy, height, width, align, w;
	u_int32_t lmask, rmask;

	scanspan = rap->linelongs * 4;
	height = rc->rc_font->height * nrows;
	offset = (dstrow - srcrow) * scanspan * rc->rc_font->height;
	srcy = rc->rc_yorigin + rc->rc_font->height * srcrow;
	if (srcrow < dstrow && srcrow + nrows > dstrow) {
		scanspan = -scanspan;
		srcy += height;
	}

	p = (caddr_t)(rap->pixels + srcy * rap->linelongs) + rc->rc_xorigin;
	align = (long)p & SFBALIGNMASK;
	p -= align;
	w = rc->rc_font->width * rc->rc_maxcol;
	width = w + align;
	lmask = SFBSTIPPLEALL1 << align;
	rmask = SFBSTIPPLEALL1 >> (-width & SFBSTIPPLEBITMASK);
	sfb = rap->data;
	*(u_int32_t *)(sfb + SFB_ASIC_MODE) = MODE_COPY;
	*(u_int32_t *)(sfb + SFB_ASIC_PLANEMASK) = ~0;
	*(u_int32_t *)(sfb + SFB_ASIC_PIXELSHIFT) = 0;
	if (width <= SFBSTIPPLEBITS) {
		/* never happens */;
	}
	else {
		caddr_t q = p;
		while (height > 0) {
			*(u_int32_t *)p = lmask;
			*(u_int32_t *)(p + offset) = lmask;
			width -= 2 * SFBSTIPPLEBITS;
			while (width > 0) {
				p += SFBSTIPPLEBYTESDONE;
				*(u_int32_t *)p = SFBSTIPPLEALL1;
				*(u_int32_t *)(p + offset) = SFBSTIPPLEALL1;
				width -= SFBSTIPPLEBITS;
			}
			p += SFBSTIPPLEBYTESDONE;
			*(u_int32_t *)p = rmask;
			*(u_int32_t *)(p + offset) = rmask;

			p = (q += scanspan);
			width = w + align;
			height--;
		}
	}
	*(u_int32_t *)(sfb + SFB_ASIC_MODE) = MODE_SIMPLE;
}

/*
 * Erase characters in a line.
 */
void
sfb_eraserows(id, startrow, nrows, fillattr)
	void *id;
	int startrow, nrows;
	long fillattr;
{
	struct rcons *rc = id;
	struct raster *rap = rc->rc_sp;
	caddr_t sfb, p;
	int scanspan, starty, height, width, align, w;
	u_int32_t lmask, rmask;

	scanspan = rap->linelongs * 4;
	starty = rc->rc_yorigin + rc->rc_font->height * startrow;
	height = rc->rc_font->height * nrows;

	p = (caddr_t)rap->pixels + starty * scanspan + rc->rc_xorigin;
	align = (long)p & SFBALIGNMASK;
	p -= align;
	w = rc->rc_font->width * rc->rc_maxcol;
	width = w + align;
	lmask = SFBSTIPPLEALL1 << align;
	rmask = SFBSTIPPLEALL1 >> (-width & SFBSTIPPLEBITMASK);
	sfb = rap->data;
	*(u_int32_t *)(sfb + SFB_ASIC_MODE) = MODE_TRANSPARENTSTIPPLE;
	*(u_int32_t *)(sfb + SFB_ASIC_PLANEMASK) = ~0;
	*(u_int32_t *)(sfb + SFB_ASIC_FG) = 0;
	if (width <= SFBSTIPPLEBITS) {
		/* never happens */;
	}
	else {
		caddr_t q = p;
		while (height > 0) {
			*(u_int32_t *)p = lmask;
			width -= 2 * SFBSTIPPLEBITS;
			while (width > 0) {
				p += SFBSTIPPLEBYTESDONE;
				*(u_int32_t *)p = SFBSTIPPLEALL1;
				width -= SFBSTIPPLEBITS;
			}
			p += SFBSTIPPLEBYTESDONE;
			*(u_int32_t *)p = rmask;

			p = (q += scanspan);
			width = w + align;
			height--;
		}
	}
	*(u_int32_t *)(sfb + SFB_ASIC_MODE) = MODE_SIMPLE;
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
