/*	$NetBSD: ofb.c,v 1.7 1999/02/02 16:48:17 tsubai Exp $	*/

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/rcons/raster.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_raster.h>
#include <dev/wscons/wsdisplayvar.h>

#include <machine/bus.h>
#include <machine/grfioctl.h>

#include <macppc/dev/ofbvar.h>

int	ofbmatch __P((struct device *, struct cfdata *, void *));
void	ofbattach __P((struct device *, struct device *, void *));
int	ofbprint __P((void *, const char *));

struct cfattach ofb_ca = {
	sizeof(struct ofb_softc), ofbmatch, ofbattach,
};

struct ofb_devconfig ofb_console_dc;

struct wsdisplay_emulops ofb_emulops = {
	rcons_cursor,			/* could use hardware cursor; punt */
	rcons_mapchar,
	rcons_putchar,
	rcons_copycols,
	rcons_erasecols,
	rcons_copyrows,
	rcons_eraserows,
	rcons_alloc_attr
};

struct wsscreen_descr ofb_stdscreen = {
	"std",
	0, 0,	/* will be filled in -- XXX shouldn't, it's global */
	&ofb_emulops,
	0, 0,
	WSSCREEN_REVERSE
};

const struct wsscreen_descr *_ofb_scrlist[] = {
	&ofb_stdscreen,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list ofb_screenlist = {
	sizeof(_ofb_scrlist) / sizeof(struct wsscreen_descr *), _ofb_scrlist
};

static int ofb_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static int ofb_mmap __P((void *, off_t, int));
static int ofb_alloc_screen __P((void *, const struct wsscreen_descr *,
				void **, int *, int *, long *));
static void ofb_free_screen __P((void *, void *));
static void ofb_show_screen __P((void *, void *));

struct wsdisplay_accessops ofb_accessops = {
	ofb_ioctl,
	ofb_mmap,
	ofb_alloc_screen,
	ofb_free_screen,
	ofb_show_screen,
	0 /* load_font */
};

static void ofb_common_init __P((int, struct ofb_devconfig *));

int
ofbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	/* /chaos/control */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE &&
	    PCI_PRODUCT(pa->pa_id) == 3)
		return 1;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY)
		return 1;

	return 0;
}

void
ofbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ofb_softc *sc = (struct ofb_softc *)self;
	struct pci_attach_args *pa = aux;
	struct wsemuldisplaydev_attach_args a;
	int console;
	struct ofb_devconfig *dc;
	char devinfo[256];

	console = ofb_is_console();

	if (console) {
		dc = &ofb_console_dc;
		sc->nscreens = 1;
	} else {
		int node;

		dc = malloc(sizeof(struct ofb_devconfig), M_DEVBUF, M_WAITOK);
		bzero(dc, sizeof(struct ofb_devconfig));
		node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
		if (node == 0) {
			printf(": ofdev not found\n");
			return;
		}
		ofb_common_init(node, dc);
	}
	sc->sc_dc = dc;

	if (dc->dc_paddr == 0) {
		printf(": cannot map framebuffer\n");
		return;
	}
	dc->dc_raster.pixels = mapiodev(dc->dc_paddr,
				dc->dc_linebytes * dc->dc_height);

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s\n", devinfo);
	printf("%s: %d x %d, %dbpp\n", self->dv_xname,
	    dc->dc_raster.width, dc->dc_raster.height, dc->dc_raster.depth);

	a.console = console;
	a.scrdata = &ofb_screenlist;
	a.accessops = &ofb_accessops;
	a.accesscookie = sc;

	config_found(self, &a, wsemuldisplaydevprint);
}

void
ofb_common_init(node, dc)
	int node;
	struct ofb_devconfig *dc;
{
	struct raster *rap;
	struct rcons *rcp;
	int i;
	int addr, width, height, linebytes, depth;
	u_int regs[5];

	OF_getprop(node, "assigned-addresses", regs, sizeof(regs));
	addr = regs[2] & 0xf0000000;

	if (dc->dc_ih == 0) {
		char name[64];

		bzero(name, 64);
		OF_package_to_path(node, name, sizeof(name));
		dc->dc_ih = OF_open(name);
	}

	/*
	 * /chaos/control don't have "width", "height", ...
	 */
	width = height = -1;
	if (OF_getprop(node, "width", &width, sizeof(width)) != 4)
		OF_interpret("screen-width", 1, &width);
	if (OF_getprop(node, "height", &height, sizeof(height)) != 4)
		OF_interpret("screen-height", 1, &height);
	if (OF_getprop(node, "linebytes", &linebytes, sizeof(linebytes)) != 4)
		linebytes = width;			/* XXX */
	if (OF_getprop(node, "depth", &depth, sizeof(depth)) != 4)
		depth = 8;				/* XXX */
	if (width == -1 || height == -1)
		return;

	OF_interpret("frame-buffer-adr", 1, &addr);
	if (addr == 0)
		return;
	dc->dc_paddr = addr;	/* PA of the frame buffer */

	/* Set colormap to black on white. */
	OF_call_method_1("color!", dc->dc_ih, 4, 0, 0, 0, 0xff);
	OF_call_method_1("color!", dc->dc_ih, 4, 255, 255, 255, 0);
	for (i = 0; i < height * linebytes; i += sizeof(u_int32_t))
		*(u_int32_t *)(addr + i) = 0;

	/* initialize the raster */
	rap = &dc->dc_raster;
	rap->width = width;
	rap->height = height;
	rap->depth = depth;
	rap->linelongs = linebytes / sizeof(u_int32_t);
	rap->pixels = (u_int32_t *)addr;

	/* initialize the raster console blitter */
	rcp = &dc->dc_rcons;
	rcp->rc_sp = rap;
	rcp->rc_crow = rcp->rc_ccol = -1;
	rcp->rc_crowp = &rcp->rc_crow;
	rcp->rc_ccolp = &rcp->rc_ccol;
	rcons_init(rcp, 128, 128);

	ofb_stdscreen.nrows = dc->dc_rcons.rc_maxrow;
	ofb_stdscreen.ncols = dc->dc_rcons.rc_maxcol;
}

int
ofb_is_console()
{
	int chosen, stdout, node;
	char type[16];

	chosen = OF_finddevice("/chosen");
	OF_getprop(chosen, "stdout", &stdout, 4);
	node = OF_instance_to_package(stdout);
	OF_getprop(node, "device_type", type, sizeof(type));
	if (strcmp(type, "display") == 0)
		return 1;
	else
		return 0;
}

int
ofb_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct ofb_softc *sc = v;
	struct ofb_devconfig *dc = sc->sc_dc;
	struct wsdisplay_fbinfo *wdf;
	struct grfinfo *gm;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;	/* XXX ? */
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = sc->sc_dc->dc_raster.height;
		wdf->width = sc->sc_dc->dc_raster.width;
		wdf->depth = sc->sc_dc->dc_raster.depth;
		wdf->cmsize = 256;
		return 0;

	case WSDISPLAYIO_PUTCMAP:
		return putcmap(sc, data);

	/* XXX There are no way to know framebuffer pa from a user program. */
	case GRFIOCGINFO:
		gm = (void *)data;
		bzero(gm, sizeof(struct grfinfo));
		gm->gd_fbaddr = (caddr_t)sc->sc_dc->dc_paddr;
		gm->gd_fbrowbytes = sc->sc_dc->dc_linebytes;
		return 0;
	}
	return -1;
}

int
ofb_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct ofb_softc *sc = v;
	struct ofb_devconfig *dc = sc->sc_dc;

	if (offset >= (dc->dc_linebytes * dc->dc_height) || offset < 0)
		return -1;

	return dc->dc_paddr + offset;
}

int
ofb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct ofb_softc *sc = v;
	long defattr;

	if (sc->nscreens > 0)
		return (ENOMEM);

	*cookiep = &sc->sc_dc->dc_rcons; /* one and only for now */
	*curxp = 0;
	*curyp = 0;
	rcons_alloc_attr(&sc->sc_dc->dc_rcons, 0, 0, 0, &defattr);
	*attrp = defattr;
	sc->nscreens++;
	return 0;
}

void
ofb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct ofb_softc *sc = v;

	if (sc->sc_dc == &ofb_console_dc)
		panic("ofb_free_screen: console");

	sc->nscreens--;
}

void
ofb_show_screen(v, cookie)
	void *v;
	void *cookie;
{
}

int
ofb_cnattach()
{
	struct ofb_devconfig *dc = &ofb_console_dc;
	long defattr;
	int crow;
	int chosen, stdout, node;
	char cmd[32];

	chosen = OF_finddevice("/chosen");
	OF_getprop(chosen, "stdout", &stdout, sizeof(stdout));
	node = OF_instance_to_package(stdout);
	dc->dc_ih = stdout;

	ofb_common_init(node, dc);
#if 0
	/* get current cursor position */
	OF_interpret("line#", 1, &crow);

	/* move (rom monitor) cursor to the lowest line */
	sprintf(cmd, "%x to line#", ofb_stdscreen.nrows - 1);
	OF_interpret(cmd, 0);
#endif
	rcons_alloc_attr(&dc->dc_rcons, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&ofb_stdscreen, &dc->dc_rcons, 0, 0, defattr);

	return 0;
}

int
putcmap(sc, cm)
	struct ofb_softc *sc;
	struct wsdisplay_cmap *cm;
{
	struct ofb_devconfig *dc = sc->sc_dc;
	int index = cm->index;
	int count = cm->count;
	int i;
	u_char *r, *g, *b;

	if (cm->index >= 256 || cm->count > 256 ||
	    (cm->index + cm->count) > 256)
		return EINVAL;
#if defined(UVM)
	if (!uvm_useracc(cm->red, cm->count, B_READ) ||
	    !uvm_useracc(cm->green, cm->count, B_READ) ||
	    !uvm_useracc(cm->blue, cm->count, B_READ))
		return EFAULT;
#else
	if (!useracc(cm->red, cm->count, B_READ) ||
	    !useracc(cm->green, cm->count, B_READ) ||
	    !useracc(cm->blue, cm->count, B_READ))
		return EFAULT;
#endif
	copyin(cm->red,   &sc->sc_cmap_red[index],   count);
	copyin(cm->green, &sc->sc_cmap_green[index], count);
	copyin(cm->blue,  &sc->sc_cmap_blue[index],  count);

	r = &sc->sc_cmap_red[index];
	g = &sc->sc_cmap_green[index];
	b = &sc->sc_cmap_blue[index];

	for (i = 0; i < count; i++) {
		OF_call_method_1("color!", dc->dc_ih, 4, *r, *g, *b, index);
		r++, g++, b++, index++;
	}

	return 0;
}
