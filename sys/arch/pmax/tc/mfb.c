/* $NetBSD: mfb.c,v 1.1.2.1 1998/10/15 02:49:00 nisimura Exp $ */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mfb.c,v 1.1.2.1 1998/10/15 02:49:00 nisimura Exp $");

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
#include <pmax/tc/bt431reg.h>

#define	machine_btop(x) mips_btop(x)
#define	MACHINE_KSEG0_TO_PHYS(x) MIPS_KSEG0_TO_PHYS(x)

struct bt455reg {
	u_int8_t	bt_reg;
	unsigned : 24;
	u_int8_t	bt_cmap;
	unsigned : 24;
	u_int8_t	bt_clr;
	unsigned : 24;
	u_int8_t	bt_ovly;
};

/* it's really painful to manipulate 'twined' registers... */
struct bt431reg {
	u_int16_t	bt_lo;
	unsigned : 16;
	u_int16_t	bt_hi;
	unsigned : 16;
	u_int16_t	bt_ram;
	unsigned : 16;
	u_int16_t	bt_ctl;
};

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

struct hwcursor {
	struct wsdisplay_curpos cc_pos;
	struct wsdisplay_curpos cc_hot;
	struct wsdisplay_curpos cc_size;
#define	CURSOR_MAX_SIZE	64
	u_int8_t cc_color[6];
	u_int64_t cc_image[64 + 64];
};

struct mfb_softc {
	struct device sc_dev;
	struct fb_devconfig *sc_dc;	/* device configuration */
	struct hwcursor sc_cursor;	/* software copy of cursor */
	int sc_curenb;			/* cursor sprite enabled */
	int sc_changed;			/* need update of colormap */
#define	DATA_ENB_CHANGED	0x01	/* cursor enable changed */
#define	DATA_CURCMAP_CHANGED	0x02	/* cursor colormap changed */
#define	DATA_CURSHAPE_CHANGED	0x04	/* cursor size, image, mask changed */
#define	DATA_CMAP_CHANGED	0x08	/* colormap changed */
#define	DATA_ALL_CHANGED	0x0f
	int nscreens;
};

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
int	mfb_load_font __P((void *, void *, int, int, int, void *));

struct wsdisplay_accessops mfb_accessops = {
	mfbioctl,
	mfbmmap,
	mfb_alloc_screen,
	mfb_free_screen,
	mfb_show_screen,
	mfb_load_font
};

int  mfb_cnattach __P((tc_addr_t));
int  mfbintr __P((void *));
void mfbinit __P((struct fb_devconfig *));

static int  set_cursor __P((struct mfb_softc *, struct wsdisplay_cursor *));
static int  get_cursor __P((struct mfb_softc *, struct wsdisplay_cursor *));
static void set_curpos __P((struct mfb_softc *, struct wsdisplay_curpos *));
void bt431_set_curpos __P((struct mfb_softc *));

#define	TWIN_LO(x) (twin = (x) & 0x00ff, twin << 8 | twin)
#define	TWIN_HI(x) (twin = (x) & 0xff00, twin | twin >> 8)

#define	BT431_SELECT(curs, regno) do {	\
	u_int16_t twin;			\
	curs->bt_lo = TWIN_LO(regno);	\
	curs->bt_hi = TWIN_HI(regno);	\
	wbflush();			\
   } while (0)

#define BT455_SELECT(vdac, index) do {	\
	vdac->bt_reg = index;		\
	vdac->bt_clr = 0;		\
	wbflush();			\
   } while (0)

#define	MFB_FB_OFFSET		0x200000
#define	MFB_FB_SIZE		 0x40000
#define	MFB_BT455_OFFSET	0x100000
#define	MFB_BT431_OFFSET	0x180000
#define	MFB_IREQ_OFFSET		0x080000	/* Interrupt req. control */

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
	dc->dc_paddr = MACHINE_KSEG0_TO_PHYS(dc->dc_vaddr + MFB_FB_OFFSET);

	dc->dc_wid = 1280;
	dc->dc_ht = 1024;
	dc->dc_depth = 1;
	dc->dc_rowbytes = 2048 / 8;
	dc->dc_videobase = dc->dc_vaddr + MFB_FB_OFFSET;
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
	volatile register junk;

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

        tc_intr_establish(parent, ta->ta_cookie, TC_IPL_TTY, mfbintr, sc);

	mfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	*(u_int8_t *)(mfbbase + MFB_IREQ_OFFSET) = 0;
	junk = *(u_int8_t *)(mfbbase + MFB_IREQ_OFFSET);
	*(u_int8_t *)(mfbbase + MFB_IREQ_OFFSET) = 1;

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
			/* sc->sc_changed |= DATA_ALL_CHANGED; */
			dc->dc_blanked = turnoff;
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
	return ENOTTY;
}

int
mfbmmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct mfb_softc *sc = v;

	if (offset > MFB_FB_SIZE)
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
mfb_load_font(v, cookie, first, num, stride, data)
	void *v;
	void *cookie;
	int first, num, stride;
	void *data;
{
	return (EINVAL);
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
        return(0);
}


int
mfbintr(arg)
	void *arg;
{
	struct mfb_softc *sc = arg;
	caddr_t mfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	struct bt455reg *vdac;
	struct bt431reg *curs;
	int v;
	volatile register junk;
	
	junk = *(u_int8_t *)(mfbbase + MFB_IREQ_OFFSET);
	/* *(u_int8_t *)(mfbbase + MFB_IREQ_OFFSET) = 1; */

	if (sc->sc_changed == 0)
		return (1);

	vdac = (void *)(mfbbase + MFB_BT455_OFFSET);
	curs = (void *)(mfbbase + MFB_BT431_OFFSET);

	v = sc->sc_changed;
	sc->sc_changed = 0;	
	if (v & DATA_ENB_CHANGED) {
		BT431_SELECT(curs, BT431_REG_COMMAND);
		curs->bt_ctl = (sc->sc_curenb) ? 0x4444 : 0x0404;
	}
	if (v & DATA_CURSHAPE_CHANGED) {
		u_int8_t *bp1, *bp2;
		u_int16_t twin;
		int index;

		bp1 = (u_int8_t *)&sc->sc_cursor.cc_image;
		bp2 = (u_int8_t *)(&sc->sc_cursor.cc_image + CURSOR_MAX_SIZE);

		BT431_SELECT(curs, BT431_REG_CRAM_BASE+0);
		for (index = 0;
		    index < sizeof(sc->sc_cursor.cc_image)/sizeof(u_int16_t);
		    index++) {	
			twin = *bp1++ | (*bp2++ << 8);
			curs->bt_ram = twin;
			wbflush();
		}
	}
	return (1);
}

void
mfbinit(dc)
	struct fb_devconfig *dc;
{
	caddr_t mfbbase = (caddr_t)dc->dc_vaddr;
	struct bt431reg *curs = (void *)(mfbbase + MFB_BT431_OFFSET);
	struct bt455reg *vdac = (void *)(mfbbase + MFB_BT455_OFFSET);
	int i;

	BT455_SELECT(vdac, 0);
	vdac->bt_cmap = 0;	wbflush();
	vdac->bt_cmap = 0;	wbflush();
	vdac->bt_cmap = 0;	wbflush();

	vdac->bt_cmap = 0;	wbflush();
	vdac->bt_cmap = 0xff;	wbflush();
	vdac->bt_cmap = 0;	wbflush();

	for (i = 2; i < 16; i++) {
		vdac->bt_cmap = 0;	wbflush();
		vdac->bt_cmap = 0;	wbflush();
		vdac->bt_cmap = 0;	wbflush();
	}

	BT455_SELECT(vdac, 8);
	vdac->bt_cmap = 0;	wbflush();
	vdac->bt_cmap = 0;	wbflush();
	vdac->bt_cmap = 0;	wbflush();

	vdac->bt_cmap = 0;	wbflush();
	vdac->bt_cmap = 0;	wbflush();
	vdac->bt_cmap = 0;	wbflush();

	vdac->bt_ovly = 0;	wbflush();
	vdac->bt_ovly = 0xff;	wbflush();
	vdac->bt_ovly = 0;	wbflush();

	BT431_SELECT(curs, BT431_REG_COMMAND);
	curs->bt_ctl = 0x0404;	wbflush();
	curs->bt_ctl = 0;	wbflush();
	curs->bt_ctl = 0;	wbflush();
	curs->bt_ctl = 0;	wbflush();
	curs->bt_ctl = 0;	wbflush();
	curs->bt_ctl = 0;	wbflush();
	curs->bt_ctl = 0;	wbflush();
	curs->bt_ctl = 0;	wbflush();
	curs->bt_ctl = 0;	wbflush();
	curs->bt_ctl = 0;	wbflush();
	curs->bt_ctl = 0;	wbflush();
	curs->bt_ctl = 0;	wbflush();
	curs->bt_ctl = 0;	wbflush();
}

static int
set_cursor(sc, p)
	struct mfb_softc *sc;
	struct wsdisplay_cursor *p;
{
#define	cc (&sc->sc_cursor)
	int v, count;

	v = p->which;
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		if (p->size.x > CURSOR_MAX_SIZE || p->size.y > CURSOR_MAX_SIZE)
			return (EINVAL);
		count = (CURSOR_MAX_SIZE / NBBY) * p->size.y;
		if (!useracc(p->image, count, B_READ) ||
		    !useracc(p->mask, count, B_READ))
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
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		cc->cc_size = p->size;
		memset(cc->cc_image, 0, sizeof cc->cc_image);
		copyin(p->image, (caddr_t)cc->cc_image, count);
		copyin(p->mask, (caddr_t)(cc->cc_image+CURSOR_MAX_SIZE), count);
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
	else if (y > dc->dc_ht - sc->sc_cursor.cc_size.y - 1)
		y = dc->dc_ht - sc->sc_cursor.cc_size.y - 1;	
	if (x < 0)
		x = 0;
	else if (x > dc->dc_wid - sc->sc_cursor.cc_size.x - 1)
		x = dc->dc_wid - sc->sc_cursor.cc_size.x - 1;
	sc->sc_cursor.cc_pos.x = x;
	sc->sc_cursor.cc_pos.y = y;
}

void
bt431_set_curpos(sc)
	struct mfb_softc *sc;
{
	caddr_t mfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	struct bt431reg *curs = (void *)(mfbbase + MFB_BT431_OFFSET);
	int x = 360, y = 36; /* magic offset of MX coordinate */
	u_int16_t twin;
	int s;

	x += sc->sc_cursor.cc_pos.x;
	y += sc->sc_cursor.cc_pos.y;

#if 0 /* MACH legend tells */
	/*
	 * Cx = x + D + H - P
	 *  P = 37 if 1:1, 52 if 4:1, 57 if 5:1
	 *  D = pixel skew between outdata and external data
	 *  H = pixels between HSYNCH falling and active video
	 *
	 * Cy = y + V - 32
	 *  V = scanlines between HSYNCH falling, two or more
	 *	clocks after VSYNCH falling, and active video
	 */
#endif
	s = spltty();

	BT431_SELECT(curs, BT431_REG_CURSOR_X_LOW);
	curs->bt_ctl = TWIN_LO(x);
	curs->bt_ctl = TWIN_HI(x);
	curs->bt_ctl = TWIN_LO(y);
	curs->bt_ctl = TWIN_HI(y);

	splx(s);
}
