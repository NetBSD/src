/* $NetBSD: sfb.c,v 1.1.2.1 1998/10/15 02:49:00 nisimura Exp $ */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: sfb.c,v 1.1.2.1 1998/10/15 02:49:00 nisimura Exp $");

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
#include <pmax/tc/bt459reg.h>

#define	machine_btop(x) mips_btop(x)
#define	MACHINE_KSEG0_TO_PHYS(x) MIPS_KSEG1_TO_PHYS(x)

struct bt459reg {
	u_int8_t	bt_lo;
	unsigned : 24;
	u_int8_t	bt_hi;
	unsigned : 24;
	u_int8_t	bt_reg;
	unsigned : 24;
	u_int8_t	bt_cmap;
};

struct fb_devconfig {
	vm_offset_t dc_vaddr;		/* memory space virtual base address */
	vm_offset_t dc_paddr;		/* memory space physical base address */
	vm_offset_t dc_size;		/* size of slot memory */
	int	    dc_wid;		/* width of frame buffer */
	int	    dc_ht;		/* height of frame buffer */
	int	    dc_depth;		/* depth, bits per pixel */
	int	    dc_rowbytes;	/* bytes in a FB scan line */
	vm_offset_t dc_videobase;	/* base of flat frame buffer */
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

struct sfb_softc {
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

int  sfbmatch __P((struct device *, struct cfdata *, void *));
void sfbattach __P((struct device *, struct device *, void *));

struct cfattach sfb_ca = {
	sizeof(struct sfb_softc), sfbmatch, sfbattach,
};

void sfb_getdevconfig __P((tc_addr_t, struct fb_devconfig *));
struct fb_devconfig sfb_console_dc;
tc_addr_t sfb_consaddr;

struct wsdisplay_emulops sfb_emulops = {
	rcons_cursor,			/* could use hardware cursor; punt */
	rcons_mapchar,
	rcons_putchar,
	rcons_copycols,
	rcons_erasecols,
	rcons_copyrows,
	rcons_eraserows,
	rcons_alloc_attr
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
int	sfb_load_font __P((void *, void *, int, int, int, void *));

struct wsdisplay_accessops sfb_accessops = {
	sfbioctl,
	sfbmmap,
	sfb_alloc_screen,
	sfb_free_screen,
	sfb_show_screen,
	sfb_load_font
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

#define	BT459_SELECT(vdac, regno) do {		\
	vdac->bt_lo = (u_int8_t)(regno);	\
	vdac->bt_hi = (u_int8_t)((regno) >> 8);	\
	wbflush();				\
   } while (0)

#define	SFB_BT459_OFFSET	0x1c0000
#define	SFB_FB_OFFSET		0x200000
#define	SFB_FB_SIZE		0x200000
#define	SFB_ASIC_OFFSET		0x100000
#define	SFB_ASIC_PLANEMASK	  0x0028
#define	SFB_ASIC_PIXELMASK	  0x002c
#define	SFB_ASIC_MODE		  0x0030
#define	SFB_ASIC_CLEAR_INTR	  0x0058
#define	SFB_ASIC_VIDEO_HSETUP	  0x0064
#define	SFB_ASIC_VIDEO_VSETUP	  0x0068
#define	SFB_ASIC_VIDEO_BASE	  0x006c
#define	SFB_ASIC_VIDEO_VALID	  0x0070
#define	SFB_ASIC_ENABLE_INTR	  0x0074

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
	struct hwcmap *cm;
	caddr_t sfbasic;
	int console, i;

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
	cm->r[0] = cm->g[0] = cm->b[0] = 0;
	for (i = 1; i < CMAP_SIZE; i++) {
		cm->r[i] = cm->g[i] = cm->b[i] = 0xff;
	}

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

	if (offset > 0x1000000) /* XXX */
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
sfb_load_font(v, cookie, first, num, stride, data)
	void *v;
	void *cookie;
	int first, num, stride;
	void *data;
{
	return (EINVAL);
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
	struct bt459reg *vdac;
	int v;
	
	*(u_int32_t *)(sfbasic + SFB_ASIC_CLEAR_INTR) = 0;
	/* *(u_int32_t *)(sfbasic + SFB_ASIC_ENABLE_INTR) = 1; */

	if (sc->sc_changed == 0)
		return (1);

	vdac = (void *)(sfbbase + SFB_BT459_OFFSET);
	v = sc->sc_changed;
	sc->sc_changed = 0;

	if (v & DATA_ENB_CHANGED) {
		BT459_SELECT(vdac, BT459_REG_CCR);
		vdac->bt_reg = (sc->sc_curenb) ? 0xc0 : 0x00; wbflush();
	}
	if (v & DATA_CURCMAP_CHANGED) {
		u_int8_t *cp = sc->sc_cursor.cc_color;

		BT459_SELECT(vdac, BT459_REG_CCOLOR_2);
		vdac->bt_reg = cp[1];	wbflush();
		vdac->bt_reg = cp[3];	wbflush();
		vdac->bt_reg = cp[5];	wbflush();

		BT459_SELECT(vdac, BT459_REG_CCOLOR_3);
		vdac->bt_reg = cp[0];	wbflush();
		vdac->bt_reg = cp[2];	wbflush();
		vdac->bt_reg = cp[4];	wbflush();
	}
	if (v & DATA_CURSHAPE_CHANGED) {
		u_int8_t *bp;
		int i;

		bp = (u_int8_t *)&sc->sc_cursor.cc_image;
		BT459_SELECT(vdac, BT459_REG_CRAM_BASE);
		for (i = 0; i < sizeof(sc->sc_cursor.cc_image); i++) {
			vdac->bt_reg = *bp++;
			wbflush();
		}
	}
	if (v & DATA_CMAP_CHANGED) {
		struct hwcmap *cm = &sc->sc_cmap;
		int index;

		BT459_SELECT(vdac, 0);
		for (index = 0; index < CMAP_SIZE; index++) {
			vdac->bt_cmap = cm->r[index];	wbflush();
			vdac->bt_cmap = cm->g[index];	wbflush();
			vdac->bt_cmap = cm->b[index];	wbflush();
		}
	}
	return (1);
}

void
sfbinit(dc)
	struct fb_devconfig *dc;
{
	caddr_t sfbasic = (caddr_t)(dc->dc_vaddr + SFB_ASIC_OFFSET);
	struct bt459reg *vdac = (void *)(dc->dc_vaddr + SFB_BT459_OFFSET);
	int i;

	*(u_int32_t *)(sfbasic + SFB_ASIC_MODE) = 0;
	*(u_int32_t *)(sfbasic + SFB_ASIC_VIDEO_VALID) = 1;
	*(u_int32_t *)(sfbasic + SFB_ASIC_PLANEMASK) = ~0;
	*(u_int32_t *)(sfbasic + SFB_ASIC_PIXELMASK) = ~0;
	
	*(u_int32_t *)(sfbasic + 0x180000) = 0; /* Bt459 reset */

	BT459_SELECT(vdac, 0);
	vdac->bt_cmap = 0;	wbflush();
	vdac->bt_cmap = 0;	wbflush();
	vdac->bt_cmap = 0;	wbflush();
	for (i = 1; i < CMAP_SIZE; i++) {
		vdac->bt_cmap = 0xff;	wbflush();
		vdac->bt_cmap = 0xff;	wbflush();
		vdac->bt_cmap = 0xff;	wbflush();
	}

	BT459_SELECT(vdac, BT459_REG_COMMAND_0);
	vdac->bt_reg = 0x40; /* CMD0 */	wbflush();
	vdac->bt_reg = 0x0;  /* CMD1 */	wbflush();
	vdac->bt_reg = 0xc0; /* CMD2 */	wbflush();
	vdac->bt_reg = 0xff; /* PRM */	wbflush();
	vdac->bt_reg = 0;    /* 205 */	wbflush();
	vdac->bt_reg = 0x0;  /* PBM */	wbflush();
	vdac->bt_reg = 0;    /* 207 */	wbflush();
	vdac->bt_reg = 0x0;  /* ORM */	wbflush();
	vdac->bt_reg = 0x0;  /* OBM */	wbflush();
	vdac->bt_reg = 0x0;  /* ILV */	wbflush();
	vdac->bt_reg = 0x0;  /* TEST */	wbflush();

	BT459_SELECT(vdac, BT459_REG_CCR);
	vdac->bt_reg = 0x0;	wbflush();
	vdac->bt_reg = 0x0;	wbflush();
	vdac->bt_reg = 0x0;	wbflush();
	vdac->bt_reg = 0x0;	wbflush();
	vdac->bt_reg = 0x0;	wbflush();
	vdac->bt_reg = 0x0;	wbflush();
	vdac->bt_reg = 0x0;	wbflush();
	vdac->bt_reg = 0x0;	wbflush();
	vdac->bt_reg = 0x0;	wbflush();
	vdac->bt_reg = 0x0;	wbflush();
	vdac->bt_reg = 0x0;	wbflush();
	vdac->bt_reg = 0x0;	wbflush();
	vdac->bt_reg = 0x0;	wbflush();
}

static int
get_cmap(sc, p)
	struct sfb_softc *sc;
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
	struct sfb_softc *sc;
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
	struct sfb_softc *sc;
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
		bt459_set_curpos(sc);
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
		copyin(p->image, cc->cc_image, count);
		copyin(p->mask, &cc->cc_image[CURSOR_MAX_SIZE], count);
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
bt459_set_curpos(sc)
	struct sfb_softc *sc;
{
	caddr_t sfbbase = (caddr_t)sc->sc_dc->dc_vaddr;
	struct bt459reg *vdac = (void *)(sfbbase + SFB_BT459_OFFSET);
	int x = 219, y = 34; /* magic offset of HX coordinate */
	int s;

	x += sc->sc_cursor.cc_pos.x;
	y += sc->sc_cursor.cc_pos.y;
	
	s = spltty();

	BT459_SELECT(vdac, BT459_REG_CURSOR_X_LOW);
	vdac->bt_reg = x;	wbflush();
	vdac->bt_reg = x >> 8;	wbflush();
	vdac->bt_reg = y;	wbflush();
	vdac->bt_reg = y >> 8;	wbflush();

	splx(s);
}
