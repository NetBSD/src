/*	$NetBSD: ofb.c,v 1.21 2001/06/10 10:34:27 tsubai Exp $	*/

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

#include <uvm/uvm_extern.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <powerpc/mpc6xx/bat.h>
#include <machine/grfioctl.h>

#include <macppc/dev/ofbvar.h>

#if OFB_ENABLE_CACHE
int ofb_enable_cache = 1;
#else
int ofb_enable_cache = 0;
#endif

int	ofbmatch __P((struct device *, struct cfdata *, void *));
void	ofbattach __P((struct device *, struct device *, void *));
int	ofbprint __P((void *, const char *));

struct cfattach ofb_ca = {
	sizeof(struct offb_softc), ofbmatch, ofbattach,
};

struct offb_devconfig ofb_console_dc;

struct wsscreen_descr ofb_stdscreen = {
	"std",
	0, 0,	/* will be filled in -- XXX shouldn't, it's global */
	0,
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
static paddr_t ofb_mmap __P((void *, off_t, int));
static int ofb_alloc_screen __P((void *, const struct wsscreen_descr *,
				void **, int *, int *, long *));
static void ofb_free_screen __P((void *, void *));
static int ofb_show_screen __P((void *, void *, int,
				void (*) (void *, int, int), void *));
static int copy_rom_font __P((void));

struct wsdisplay_accessops ofb_accessops = {
	ofb_ioctl,
	ofb_mmap,
	ofb_alloc_screen,
	ofb_free_screen,
	ofb_show_screen,
	0 /* load_font */
};

static struct wsdisplay_font openfirm6x11;

static void ofb_common_init __P((int, struct offb_devconfig *));
static int ofb_getcmap __P((struct offb_softc *, struct wsdisplay_cmap *));
static int ofb_putcmap __P((struct offb_softc *, struct wsdisplay_cmap *));

int
ofbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_CONTROL)
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
	struct offb_softc *sc = (struct offb_softc *)self;
	struct pci_attach_args *pa = aux;
	struct wsemuldisplaydev_attach_args a;
	int console, node;
	struct offb_devconfig *dc;
	char devinfo[256];

	console = ofb_is_console();

	if (console) {
		dc = &ofb_console_dc;
		node = dc->dc_node;
		sc->nscreens = 1;
	} else {
		int i, len, screenbytes;

		dc = malloc(sizeof(struct offb_devconfig), M_DEVBUF, M_WAITOK);
		bzero(dc, sizeof(struct offb_devconfig));
		node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
		if (node == 0) {
			printf(": ofdev not found\n");
			return;
		}

		/* XXX There are two child screens on PowerBook. */
		bzero(devinfo, sizeof(devinfo));
		OF_getprop(node, "device_type", devinfo, sizeof(devinfo));
		len = strlen(devinfo);
		if (strcmp(devinfo + len - 7, "-parent") == 0)
			node = OF_child(node);

		ofb_common_init(node, dc);

		screenbytes = dc->dc_ri.ri_stride * dc->dc_ri.ri_height;
		for (i = 0; i < screenbytes; i += sizeof(u_int32_t))
			*(u_int32_t *)(dc->dc_paddr + i) = 0xffffffff;
	}
	sc->sc_dc = dc;

	/* XXX */
	if (OF_getprop(node, "assigned-addresses", sc->sc_addrs,
	    sizeof(sc->sc_addrs)) == -1) {
		node = OF_parent(node);
		OF_getprop(node, "assigned-addresses", sc->sc_addrs,
		    sizeof(sc->sc_addrs));
	}

	if (dc->dc_paddr == 0) {
		printf(": cannot map framebuffer\n");
		return;
	}

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s\n", devinfo);
	printf("%s: %d x %d, %dbpp\n", self->dv_xname,
	       dc->dc_ri.ri_width, dc->dc_ri.ri_height, dc->dc_ri.ri_depth);

	sc->sc_cmap_red[0] = sc->sc_cmap_green[0] = sc->sc_cmap_blue[0] = 0;
	sc->sc_cmap_red[15] = sc->sc_cmap_red[255] = 0xff;
	sc->sc_cmap_green[15] = sc->sc_cmap_green[255] = 0xff;
	sc->sc_cmap_blue[15] = sc->sc_cmap_blue[255] = 0xff;

	a.console = console;
	a.scrdata = &ofb_screenlist;
	a.accessops = &ofb_accessops;
	a.accesscookie = sc;

	config_found(self, &a, wsemuldisplaydevprint);
}

void
ofb_common_init(node, dc)
	int node;
	struct offb_devconfig *dc;
{
	struct rasops_info *ri = &dc->dc_ri;
	int32_t addr, width, height, linebytes, depth;

	dc->dc_node = node;
	if (dc->dc_ih == 0) {
		char name[64];

		bzero(name, 64);
		OF_package_to_path(node, name, sizeof(name));
		dc->dc_ih = OF_open(name);
	}

	/* XXX /chaos/control doesn't have "width", "height", ... */
	width = height = -1;
	if (OF_getprop(node, "width", &width, 4) != 4)
		OF_interpret("screen-width", 1, &width);
	if (OF_getprop(node, "height", &height, 4) != 4)
		OF_interpret("screen-height", 1, &height);
	if (OF_getprop(node, "linebytes", &linebytes, 4) != 4)
		linebytes = width;			/* XXX */
	if (OF_getprop(node, "depth", &depth, 4) != 4)
		depth = 8;				/* XXX */
	if (OF_getprop(node, "address", &addr, 4) != 4)
		OF_interpret("frame-buffer-adr", 1, &addr);

	if (width == -1 || height == -1 || addr == 0 || addr == -1)
		return;

	dc->dc_paddr = addr;		/* PA of the frame buffer */

	/* Make sure 0/0/0 is black and 255/255/255 is white. */
	OF_call_method_1("color!", dc->dc_ih, 4, 0, 0, 0, 0);
	OF_call_method_1("color!", dc->dc_ih, 4, 255, 255, 255, 255);

	/* Enable write-through cache. */
	if (ofb_enable_cache && battable[0xc].batu == 0) {
		battable[0xc].batl = BATL(addr & 0xf0000000, BAT_W, BAT_PP_RW);
		battable[0xc].batu = BATL(0xc0000000, BAT_BL_256M, BAT_Vs);
		addr &= 0x0fffffff;
		addr |= 0xc0000000;
	}

	/* initialize rasops */
	ri->ri_width = width;
	ri->ri_height = height;
	ri->ri_depth = depth;
	ri->ri_stride = linebytes;
	ri->ri_bits = (char *)addr;
	ri->ri_flg = RI_FORCEMONO | RI_FULLCLEAR | RI_CENTER;

	/* If screen is smaller than 1024x768, use small font. */
	if ((width < 1024 || height < 768) && copy_rom_font() == 0) {
		int cols, rows;

		OF_interpret("#lines", 1, &rows);
		OF_interpret("#columns", 1, &cols);

		ri->ri_font = &openfirm6x11;
		ri->ri_wsfcookie = -1;		/* not using wsfont */
		rasops_init(ri, rows, cols);

		ri->ri_xorigin = 2;
		ri->ri_yorigin = 3;
		ri->ri_bits = (char *)addr + ri->ri_xorigin +
			      ri->ri_stride * ri->ri_yorigin;
	} else {
		rasops_init(ri, 24, 80);
		rasops_reconfig(ri, (height - 2) / ri->ri_font->fontheight,
		    ((width - 2) / ri->ri_font->fontwidth) & ~7);
	}

	/* black on white */
	ri->ri_devcmap[0] = 0xffffffff;			/* bg */
	ri->ri_devcmap[1] = 0;				/* fg */

	ofb_stdscreen.nrows = ri->ri_rows;
	ofb_stdscreen.ncols = ri->ri_cols;
	ofb_stdscreen.textops = &ri->ri_ops;
	ofb_stdscreen.capabilities = ri->ri_caps;
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
	struct offb_softc *sc = v;
	struct offb_devconfig *dc = sc->sc_dc;
	struct wsdisplay_fbinfo *wdf;
	struct grfinfo *gm;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;	/* XXX ? */
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = dc->dc_ri.ri_height;
		wdf->width = dc->dc_ri.ri_width;
		wdf->depth = dc->dc_ri.ri_depth;
		wdf->cmsize = 256;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return ofb_getcmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return ofb_putcmap(sc, (struct wsdisplay_cmap *)data);

	/* XXX There are no way to know framebuffer pa from a user program. */
	case GRFIOCGINFO:
		gm = (void *)data;
		bzero(gm, sizeof(struct grfinfo));
		gm->gd_fbaddr = (caddr_t)dc->dc_paddr;
		gm->gd_fbrowbytes = dc->dc_ri.ri_stride;
		return 0;
	}
	return -1;
}

paddr_t
ofb_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct offb_softc *sc = v;
	struct offb_devconfig *dc = sc->sc_dc;
	struct rasops_info *ri = &dc->dc_ri;
	u_int32_t *ap = sc->sc_addrs;
	paddr_t pa;
	int i;

	if (offset >=0 && offset < (ri->ri_stride * ri->ri_height))
		return dc->dc_paddr + offset;

	pa = offset;
	for (i = 0; i < 6; i++) {
		switch (ap[0] & OFW_PCI_PHYS_HI_SPACEMASK) {
		case OFW_PCI_PHYS_HI_SPACE_MEM32:
			if (pa >= ap[2] && pa < ap[2] + ap[4])
				return pa;
		/* XXX I/O space? */
		}
		ap += 5;
	}

	return -1;
}

int
ofb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct offb_softc *sc = v;
	struct rasops_info *ri = &sc->sc_dc->dc_ri;
	long defattr;

	if (sc->nscreens > 0)
		return (ENOMEM);

	*cookiep = ri;			/* one and only for now */
	*curxp = 0;
	*curyp = 0;
	(*ri->ri_ops.alloc_attr)(ri, 0, 0, 0, &defattr);
	*attrp = defattr;
	sc->nscreens++;
	return 0;
}

void
ofb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct offb_softc *sc = v;

	if (sc->sc_dc == &ofb_console_dc)
		panic("ofb_free_screen: console");

	sc->nscreens--;
}

int
ofb_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb) __P((void *, int, int));
	void *cbarg;
{

	return (0);
}

int
ofb_cnattach()
{
	struct offb_devconfig *dc = &ofb_console_dc;
	struct rasops_info *ri = &dc->dc_ri;
	long defattr;
	int crow = 0;
	int chosen, stdout, node;

	chosen = OF_finddevice("/chosen");
	OF_getprop(chosen, "stdout", &stdout, sizeof(stdout));
	node = OF_instance_to_package(stdout);
	dc->dc_ih = stdout;

	/* get current cursor position */
	OF_interpret("line#", 1, &crow);

	/* move (rom monitor) cursor to the lowest line - 1 */
	OF_interpret("#lines 2 - to line#", 0);

	ofb_common_init(node, dc);

	if (ri->ri_width >= 1024 && ri->ri_height >= 768) {
		int i, screenbytes = ri->ri_stride * ri->ri_height;

		for (i = 0; i < screenbytes; i += sizeof(u_int32_t))
			*(u_int32_t *)(dc->dc_paddr + i) = 0xffffffff;
		crow = 0;
	}

	(*ri->ri_ops.alloc_attr)(ri, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&ofb_stdscreen, ri, 0, crow, defattr);

	return 0;
}

int
copy_rom_font()
{
	u_char *romfont;
	int char_width, char_height;
	int chosen, mmu, m, e;

	/* Get ROM FONT address. */
	OF_interpret("font-adr", 1, &romfont);
	if (romfont == NULL)
		return -1;

	chosen = OF_finddevice("/chosen");
	OF_getprop(chosen, "mmu", &mmu, 4);

	/*
	 * Convert to physcal address.  We cannot access to Open Firmware's
	 * virtual address space.
	 */
	OF_call_method("translate", mmu, 1, 3, romfont, &romfont, &m, &e);
 
	/* Get character size */
	OF_interpret("char-width", 1, &char_width);
	OF_interpret("char-height", 1, &char_height);

	openfirm6x11.name = "Open Firmware";
	openfirm6x11.firstchar = 32;
	openfirm6x11.numchars = 96;
	openfirm6x11.encoding = WSDISPLAY_FONTENC_ISO;
	openfirm6x11.fontwidth = char_width;
	openfirm6x11.fontheight = char_height;
	openfirm6x11.stride = 1;
	openfirm6x11.bitorder = WSDISPLAY_FONTORDER_L2R;
	openfirm6x11.byteorder = WSDISPLAY_FONTORDER_L2R;
	openfirm6x11.data = romfont;

	return 0;
}

int
ofb_getcmap(sc, cm)
	struct offb_softc *sc;
	struct wsdisplay_cmap *cm;
{
	u_int index = cm->index;
	u_int count = cm->count;
	int error;

	if (index >= 256 || count > 256 || index + count > 256)
		return EINVAL;

	error = copyout(&sc->sc_cmap_red[index],   cm->red,   count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_green[index], cm->green, count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_blue[index],  cm->blue,  count);
	if (error)
		return error;

	return 0;
}

int
ofb_putcmap(sc, cm)
	struct offb_softc *sc;
	struct wsdisplay_cmap *cm;
{
	struct offb_devconfig *dc = sc->sc_dc;
	int index = cm->index;
	int count = cm->count;
	int i;
	u_char *r, *g, *b;

	if (cm->index >= 256 || cm->count > 256 ||
	    (cm->index + cm->count) > 256)
		return EINVAL;
	if (!uvm_useracc(cm->red, cm->count, B_READ) ||
	    !uvm_useracc(cm->green, cm->count, B_READ) ||
	    !uvm_useracc(cm->blue, cm->count, B_READ))
		return EFAULT;
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
