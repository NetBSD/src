/* $NetBSD: tfb.c,v 1.1.2.1 1998/10/15 02:49:01 nisimura Exp $ */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: tfb.c,v 1.1.2.1 1998/10/15 02:49:01 nisimura Exp $");

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
#include <dev/ic/bt463reg.h>
#include <pmax/tc/bt431reg.h>

#define	machine_btop(x) mips_btop(x)
#define	MACHINE_KSEG0_TO_PHYS(x) MIPS_KSEG0_TO_PHYS(x)

struct bt463reg {
	u_int8_t	bt_lo;
	unsigned : 24;
	u_int8_t	bt_hi;
	unsigned : 24;
	u_int8_t	bt_reg;
	unsigned : 24;
	u_int8_t	bt_cmap;
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

struct hwcmap {
#define	CMAP_SIZE	256	/* R/G/B entries */
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

struct tfb_softc {
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
};

int  tfbmatch __P((struct device *, struct cfdata *, void *));
void tfbattach __P((struct device *, struct device *, void *));

struct cfattach tfb_ca = {
	sizeof(struct tfb_softc), tfbmatch, tfbattach,
};

void tfb_getdevconfig __P((tc_addr_t, struct fb_devconfig *));
struct fb_devconfig tfb_console_dc;
tc_addr_t tfb_consaddr;

struct wsdisplay_emulops tfb_emulops = {
	rcons_cursor,			/* could use hardware cursor; punt */
	rcons_mapchar,
	rcons_putchar,
	rcons_copycols,
	rcons_erasecols,
	rcons_copyrows,
	rcons_eraserows,
	rcons_alloc_attr
};

struct wsscreen_descr tfb_stdscreen = {
	"std", 0, 0,
	&tfb_emulops,
	0, 0,
	0
};

const struct wsscreen_descr *_tfb_scrlist[] = {
	&tfb_stdscreen,
};

struct wsscreen_list tfb_screenlist = {
	sizeof(_tfb_scrlist) / sizeof(struct wsscreen_descr *), _tfb_scrlist
};

int	tfbioctl __P((void *, u_long, caddr_t, int, struct proc *));
int	tfbmmap __P((void *, off_t, int));

int	tfb_alloc_screen __P((void *, const struct wsscreen_descr *,
				      void **, int *, int *, long *));
void	tfb_free_screen __P((void *, void *));
void	tfb_show_screen __P((void *, void *));
int	tfb_load_font __P((void *, void *, int, int, int, void *));

struct wsdisplay_accessops tfb_accessops = {
	tfbioctl,
	tfbmmap,
	tfb_alloc_screen,
	tfb_free_screen,
	tfb_show_screen,
	tfb_load_font
};

int  tfb_cnattach __P((tc_addr_t));
int  tfbintr __P((void *));
void tfbinit __P((struct fb_devconfig *));

static int  get_cmap __P((struct tfb_softc *, struct wsdisplay_cmap *));
static int  set_cmap __P((struct tfb_softc *, struct wsdisplay_cmap *));
static int  set_cursor __P((struct tfb_softc *, struct wsdisplay_cursor *));
static int  get_cursor __P((struct tfb_softc *, struct wsdisplay_cursor *));
static void set_curpos __P((struct tfb_softc *, struct wsdisplay_curpos *));
static void bt431_set_curpos __P((struct tfb_softc *));

#define	TWIN_LO(x) (twin = (x) & 0x00ff, twin << 8 | twin)
#define	TWIN_HI(x) (twin = (x) & 0xff00, twin | twin >> 8)

#define	BT431_SELECT(curs, regno) do {	\
	u_int16_t twin;			\
	curs->bt_lo = TWIN_LO(regno);	\
	curs->bt_hi = TWIN_HI(regno);	\
	wbflush();			\
   } while (0)

#define	BT463_SELECT(vdac, regno) do {		\
	vdac->bt_lo = (u_int8_t)(regno);	\
	vdac->bt_hi = (u_int8_t)((regno) >> 8);	\
	wbflush();				\
   } while (0)

#define	TX_BT463_OFFSET	0x040000
#define	TX_BT431_OFFSET	0x040010
#define	TX_CONTROL	0x040030
#define	TX_MAP_REGISTER	0x040030
#define	TX_PIP_OFFSET	0x0800c0
#define	TX_SELECTION	0x100000
#define	TX_8FB_OFFSET	0x200000
#define	TX_8FB_SIZE	0x100000
#define	TX_24FB_OFFSET	0x400000
#define	TX_24FB_SIZE	0x400000
#define	TX_VIDEO_ENABLE	0xa00000

#define TX_CTL_VIDEO_ON	0x80
#define TX_CTL_INT_ENA	0x40
#define TX_CTL_INT_PEND	0x20
#define TX_CTL_SEG_ENA	0x10
#define TX_CTL_SEG	0x0f


int
tfbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct tc_attach_args *ta = aux;

	if (strncmp("PMAG-RO ", ta->ta_modname, TC_ROM_LLEN) != 0
	    && strncmp("PMAG-JA ", ta->ta_modname, TC_ROM_LLEN) != 0)
		return (0);

	return (1);
}

void
tfb_getdevconfig(dense_addr, dc)
	tc_addr_t dense_addr;
	struct fb_devconfig *dc;
{
	struct raster *rap;
	struct rcons *rcp;
	int i;

	dc->dc_vaddr = dense_addr;
	dc->dc_paddr = MACHINE_KSEG0_TO_PHYS(dc->dc_vaddr);

	dc->dc_wid = 1280;
	dc->dc_ht = 1024;
	dc->dc_depth = 8;
	dc->dc_rowbytes = 1280;
	dc->dc_videobase = dc->dc_vaddr + TX_8FB_OFFSET;
	dc->dc_blanked = 0;

	/* initialize colormap and cursor resource */
	tfbinit(dc);

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

	tfb_stdscreen.nrows = dc->dc_rcons.rc_maxrow;
	tfb_stdscreen.ncols = dc->dc_rcons.rc_maxcol;
}

void
tfbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct tfb_softc *sc = (struct tfb_softc *)self;
	struct tc_attach_args *ta = aux;
	struct wsemuldisplaydev_attach_args waa;
	struct hwcmap *cm;
	int console, i;

	console = (ta->ta_addr == tfb_consaddr);
	if (console) {
		sc->sc_dc = &tfb_console_dc;
		sc->nscreens = 1;
	}
	else {
		sc->sc_dc = (struct fb_devconfig *)
		    malloc(sizeof(struct fb_devconfig), M_DEVBUF, M_WAITOK);
		tfb_getdevconfig(ta->ta_addr, sc->sc_dc);
	}
	printf(": %d x %d, %dbpp\n", sc->sc_dc->dc_wid, sc->sc_dc->dc_ht,
	    sc->sc_dc->dc_depth);

	cm = &sc->sc_cmap;
	cm->r[0] = cm->g[0] = cm->b[0] = 0;
	for (i = 1; i < CMAP_SIZE; i++) {
		cm->r[i] = cm->g[i] = cm->b[i] = 0xff;
	}

        tc_intr_establish(parent, ta->ta_cookie, TC_IPL_TTY, tfbintr, sc);
	*(u_int8_t *)(sc->sc_dc->dc_vaddr + TX_CONTROL) &= ~0x40;
	*(u_int8_t *)(sc->sc_dc->dc_vaddr + TX_CONTROL) |= 0x40;

	waa.console = console;
	waa.scrdata = &tfb_screenlist;
	waa.accessops = &tfb_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}

int
tfbioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct tfb_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;
	int turnoff;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = /* WSDISPLAY_TYPE_TX */ 0x19980910;
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
tfbmmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct tfb_softc *sc = v;

	if (offset > TX_8FB_SIZE) /* XXX */
		return (-1);
	return machine_btop(sc->sc_dc->dc_paddr + TX_8FB_OFFSET + offset);
}

int
tfb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct tfb_softc *sc = v;
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
tfb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct tfb_softc *sc = v;

	if (sc->sc_dc == &tfb_console_dc)
		panic("tfb_free_screen: console");

	sc->nscreens--;
}

void
tfb_show_screen(v, cookie)
	void *v;
	void *cookie;
{
}

int
tfb_load_font(v, cookie, first, num, stride, data)
	void *v;
	void *cookie;
	int first, num, stride;
	void *data;
{
	return (EINVAL);
}

int
tfb_cnattach(addr)
        tc_addr_t addr;
{
        struct fb_devconfig *dcp = &tfb_console_dc;
        long defattr;

        tfb_getdevconfig(addr, dcp);
 
        rcons_alloc_attr(&dcp->dc_rcons, 0, 0, 0, &defattr);

        wsdisplay_cnattach(&tfb_stdscreen, &dcp->dc_rcons,
                           0, 0, defattr);
        tfb_consaddr = addr;
        return(0);
}


int
tfbintr(arg)
	void *arg;
{
	struct tfb_softc *sc = arg;
	caddr_t tfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	struct bt463reg *vdac;
	struct bt431reg *curs;
	int v;
	
	*(u_int8_t *)(tfbbase + TX_CONTROL) &= ~0x40;
	*(u_int8_t *)(tfbbase + TX_CONTROL) |= 0x40;
	if (sc->sc_changed == 0)
		return (1);

	vdac = (void *)(tfbbase + TX_BT463_OFFSET);
	curs = (void *)(tfbbase + TX_BT431_OFFSET);
	v = sc->sc_changed;
	sc->sc_changed = 0;
	if (v & DATA_ENB_CHANGED) {
		BT431_SELECT(curs, BT431_REG_COMMAND);
		curs->bt_ctl = (sc->sc_curenb) ? 0x4444 : 0x0404;
	}
	if (v & DATA_CURCMAP_CHANGED) {
		u_int8_t *cp = sc->sc_cursor.cc_color;

		BT463_SELECT(vdac, BT463_IREG_CURSOR_COLOR_0);
		vdac->bt_reg = cp[1]; wbflush();
		vdac->bt_reg = cp[3]; wbflush();
		vdac->bt_reg = cp[5]; wbflush();

		vdac->bt_reg = cp[0]; wbflush();
		vdac->bt_reg = cp[2]; wbflush();
		vdac->bt_reg = cp[4]; wbflush();

		vdac->bt_reg = cp[1]; wbflush();
		vdac->bt_reg = cp[3]; wbflush();
		vdac->bt_reg = cp[5]; wbflush();

		vdac->bt_reg = cp[1]; wbflush();
		vdac->bt_reg = cp[3]; wbflush();
		vdac->bt_reg = cp[5]; wbflush();
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
	if (v & DATA_CMAP_CHANGED) {
		struct hwcmap *cm = &sc->sc_cmap;
		int index;

		BT463_SELECT(vdac, BT463_IREG_CPALETTE_RAM);
		for (index = 0; index < CMAP_SIZE; index++) {
			vdac->bt_cmap = cm->r[index];
			vdac->bt_cmap = cm->g[index];
			vdac->bt_cmap = cm->b[index];
		}
	}
	return (1);
}

void
tfbinit(dc)
	struct fb_devconfig *dc;
{
	caddr_t tfbbase = (caddr_t)dc->dc_vaddr;
	struct bt463reg *vdac = (void *)(tfbbase + TX_BT463_OFFSET);
	struct bt431reg *curs = (void *)(tfbbase + TX_BT431_OFFSET);
	int i;

	BT463_SELECT(vdac, BT463_IREG_COMMAND_0);
	vdac->bt_reg = 0x40;	wbflush();
	vdac->bt_reg = 0x46;	wbflush();
	vdac->bt_reg = 0xc0;	wbflush();
	vdac->bt_reg = 0;	wbflush();	/* !? 204 !? */
	vdac->bt_reg = 0xff;	wbflush();	/* plane  0:7  */
	vdac->bt_reg = 0xff;	wbflush();	/* plane  8:15 */
	vdac->bt_reg = 0xff;	wbflush();	/* plane 16:23 */
	vdac->bt_reg = 0xff;	wbflush();	/* plane 24:27 */
	vdac->bt_reg = 0x00;	wbflush();	/* blink  0:7  */
	vdac->bt_reg = 0x00;	wbflush();	/* blink  8:15 */
	vdac->bt_reg = 0x00;	wbflush();	/* blink 16:23 */
	vdac->bt_reg = 0x00;	wbflush();	/* blink 24:27 */
	vdac->bt_reg = 0x00;	wbflush();

	/* bt463_load_wid(vdac, 0, 16, -1); */

	BT463_SELECT(vdac, BT463_IREG_CPALETTE_RAM);
	vdac->bt_cmap = 0;	wbflush();
	vdac->bt_cmap = 0;	wbflush();
	vdac->bt_cmap = 0;	wbflush();
	for (i = 1; i < CMAP_SIZE; i++) {
		vdac->bt_cmap = 0xff;	wbflush();
		vdac->bt_cmap = 0xff;	wbflush();
		vdac->bt_cmap = 0xff;	wbflush();
	}

	BT463_SELECT(vdac, BT463_IREG_CPALETTE_RAM+0x100);
	for (i = 0; i < CMAP_SIZE; i++) {
		vdac->bt_cmap = 0;	wbflush();
		vdac->bt_cmap = 0;	wbflush();
		vdac->bt_cmap = 0;	wbflush();
	}

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
get_cmap(sc, p)
	struct tfb_softc *sc;
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
	struct tfb_softc *sc;
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
	struct tfb_softc *sc;
	struct wsdisplay_cursor *p;
{
#define	cc (&sc->sc_cursor)
	int v, index, count;

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
	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		count = p->cmap.count;
		copyin(p->cmap.red, &cc->cc_color[index], count);
		copyin(p->cmap.green, &cc->cc_color[index + 2], count);
		copyin(p->cmap.blue, &cc->cc_color[index + 4], count);
		sc->sc_changed |= DATA_CURCMAP_CHANGED;
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
	struct tfb_softc *sc;
	struct wsdisplay_cursor *p;
{
	return (ENOTTY); /* XXX */
}

static void
set_curpos(sc, curpos)
	struct tfb_softc *sc;
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

static void
bt431_set_curpos(sc)
	struct tfb_softc *sc;
{
	caddr_t tfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	struct bt431reg *curs = (void *)(tfbbase + TX_BT431_OFFSET);
	int x = 220, y = 35; /* magic offset of TX coordinate */
	u_int16_t twin;
	int s;

	x += sc->sc_cursor.cc_pos.x;
	y += sc->sc_cursor.cc_pos.y;

	s = spltty();

	BT431_SELECT(curs, BT431_REG_CURSOR_X_LOW);
	curs->bt_ctl = TWIN_LO(x);
	curs->bt_ctl = TWIN_HI(x);
	curs->bt_ctl = TWIN_LO(y);
	curs->bt_ctl = TWIN_HI(y);

	splx(s);
}
