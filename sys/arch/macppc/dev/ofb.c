/*	$NetBSD: ofb.c,v 1.43.6.1 2006/04/22 11:37:41 simonb Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofb.c,v 1.43.6.1 2006/04/22 11:37:41 simonb Exp $");

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
#include <dev/pci/pciio.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>

#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/grfioctl.h>

#include <powerpc/oea/bat.h>

#include <dev/wscons/wsdisplay_vconsvar.h>
#include <macppc/dev/ofbvar.h>

#if OFB_ENABLE_CACHE
int ofb_enable_cache = 1;
#else
int ofb_enable_cache = 0;
#endif

static int	ofbmatch(struct device *, struct cfdata *, void *);
static void	ofbattach(struct device *, struct device *, void *);

CFATTACH_DECL(ofb, sizeof(struct ofb_softc),
    ofbmatch, ofbattach, NULL, NULL);

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

static int	ofb_ioctl(void *, void *, u_long, caddr_t, int, struct lwp *);
static paddr_t	ofb_mmap(void *, void *, off_t, int);
static int	copy_rom_font(void);

static int	ofb_init_rasops(int, struct rasops_info *);
static void	ofb_init_screen(void *, struct vcons_screen *, int, long *);

struct wsdisplay_accessops ofb_accessops = {
	ofb_ioctl,
	ofb_mmap,
	NULL,		/* vcons_alloc_screen */
	NULL,		/* vcons_free_screen */
	NULL,		/* vcons_show_screen */
	NULL		/* load_font */
};

static struct vcons_screen ofb_console_screen;
static struct wsdisplay_font openfirm6x11;
static int    console_node, console_instance;
static vaddr_t fbaddr;
static int romfont_loaded = 0;

static void	ofb_putpalreg(struct ofb_softc *, int, uint8_t, uint8_t,
    uint8_t);

static int	ofb_getcmap(struct ofb_softc *, struct wsdisplay_cmap *);
static int	ofb_putcmap(struct ofb_softc *, struct wsdisplay_cmap *);
static void	ofb_init_cmap(struct ofb_softc *);

extern const u_char rasops_cmap[768];

static int
ofbmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_CONTROL)
		return 1;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY)
		return 1;

	return 0;
}

static void
ofbattach(struct device *parent, struct device *self, void *aux)
{
	struct ofb_softc *sc = (struct ofb_softc *)self;
	struct pci_attach_args *pa = aux;
	struct wsemuldisplaydev_attach_args a;
	struct rasops_info *ri = &ofb_console_screen.scr_ri;
	long defattr;
	int console, len, node, sub;
	char devinfo[256];

	node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
	console = (node == console_node);
	if (!console) {
		/* check if any of the childs matches */
		sub = OF_child(node);
		while ((sub != 0) && (sub != console_node)) {
			sub = OF_peer(sub);
		}
		if (sub == console_node) {
			console = TRUE;
		}
	}
	
	sc->sc_memt = pa->pa_memt;
	sc->sc_iot = pa->pa_iot;	
	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf(": %s\n", devinfo);

	if (!console)
		return;
	
	vcons_init(&sc->vd, sc, &ofb_stdscreen, &ofb_accessops);
	sc->vd.init_screen = ofb_init_screen;

	console = ofb_is_console();

	sc->sc_node = node;

	if (console) {
		sc->sc_ih = console_instance;
		vcons_init_screen(&sc->vd, &ofb_console_screen, 1, &defattr);
		ofb_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
		printf("%s: %d x %d, %dbpp\n", self->dv_xname,
		       ri->ri_width, ri->ri_height, ri->ri_depth);
	} else {
		char name[64];
		if (sc->sc_node == 0) {
			printf(": ofdev not found\n");
			return;
		}

		/* XXX There are two child screens on PowerBook. */
		memset(devinfo, 0, sizeof(devinfo));
		OF_getprop(sc->sc_node, "device_type", devinfo, sizeof(devinfo));
		len = strlen(devinfo);
		if (strcmp(devinfo + len - 7, "-parent") == 0)
			sc->sc_node = OF_child(sc->sc_node);

		memset(name, 0, 64);
		OF_package_to_path(sc->sc_node, name, sizeof(name));
		sc->sc_ih = OF_open(name);

	}
	
	sc->sc_fbaddr = 0;
	if (OF_getprop(sc->sc_node, "address", &sc->sc_fbaddr, 4) != 4)
		OF_interpret("frame-buffer-adr", 1, &sc->sc_fbaddr);
	if (sc->sc_fbaddr == 0) {
		printf("%s: Unable to find the framebuffer address.\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	
	/* XXX */
	if (OF_getprop(sc->sc_node, "assigned-addresses", sc->sc_addrs,
	    sizeof(sc->sc_addrs)) == -1) {
		sc->sc_node = OF_parent(sc->sc_node);
		OF_getprop(sc->sc_node, "assigned-addresses", sc->sc_addrs,
		    sizeof(sc->sc_addrs));
	}

	ofb_init_cmap(sc);		

	a.console = console;
	a.scrdata = &ofb_screenlist;
	a.accessops = &ofb_accessops;
	a.accesscookie = &sc->vd;

	config_found(self, &a, wsemuldisplaydevprint);
}

static void
ofb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct ofb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;
	
	if (scr != &ofb_console_screen) {
		ofb_init_rasops(sc->sc_node, ri); 
	}
}

static int
ofb_init_rasops(int node, struct rasops_info *ri)
{
	int32_t width, height, linebytes, depth;

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
	if (OF_getprop(node, "address", &fbaddr, 4) != 4)
		OF_interpret("frame-buffer-adr", 1, &fbaddr);

	if (width == -1 || height == -1 || fbaddr == 0 || fbaddr == -1)
		return FALSE;

	/* Enable write-through cache. */
	if (ofb_enable_cache) {
		vaddr_t va;
		/*
		 * Let's try to find an empty BAT to use 
		 */
		for (va = SEGMENT_LENGTH; va < (USER_SR << ADDR_SR_SHFT);
		     va += SEGMENT_LENGTH) {
			if (battable[va >> ADDR_SR_SHFT].batu == 0) {
				battable[va >> ADDR_SR_SHFT].batl =
				    BATL(fbaddr & 0xf0000000,
					 BAT_G | BAT_W | BAT_M, BAT_PP_RW);
				battable[va >> ADDR_SR_SHFT].batu =
				    BATL(va, BAT_BL_256M, BAT_Vs);
				fbaddr &= 0x0fffffff;
				fbaddr |= va;
				break;
			}
		}
	}

	/* initialize rasops */
	ri->ri_width = width;
	ri->ri_height = height;
	ri->ri_depth = depth;
	ri->ri_stride = linebytes;
	ri->ri_bits = (char *)fbaddr;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;

	/* If screen is smaller than 1024x768, use small font. */
	if ((width < 1024 || height < 768) && (romfont_loaded)) {
		int cols, rows;

		/* 
		 * XXX this assumes we're the console which may or may not 
		 * be the case 
		 */
		OF_interpret("#lines", 1, &rows);
		OF_interpret("#columns", 1, &cols);
		ri->ri_font = &openfirm6x11;
		ri->ri_wsfcookie = -1;		/* not using wsfont */
		rasops_init(ri, rows, cols);

		ri->ri_xorigin = (width - cols * ri->ri_font->fontwidth) >> 1;
		ri->ri_yorigin = (height - rows * ri->ri_font->fontheight) 
		    >> 1;
		ri->ri_bits = (char *)fbaddr + ri->ri_xorigin +
			      ri->ri_stride * ri->ri_yorigin;
	} else {
		/* use as much of the screen as the font permits */
		rasops_init(ri, height/8, width/8);
		ri->ri_caps = WSSCREEN_WSCOLORS;
		rasops_reconfig(ri, height / ri->ri_font->fontheight,
		    width / ri->ri_font->fontwidth);
	}

	return TRUE;
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

static int
ofb_ioctl(void *v, void *vs, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct vcons_data *vd = v;
	struct ofb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;
	struct grfinfo *gm;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;	/* XXX ? */
		return 0;

	case WSDISPLAYIO_GINFO:
		/* we won't get here without any screen anyway */
		if (ms != NULL) {
			wdf = (void *)data;
			wdf->height = ms->scr_ri.ri_width;
			wdf->width = ms->scr_ri.ri_height;
			wdf->depth = ms->scr_ri.ri_depth;
			wdf->cmsize = 256;
			return 0;
		} else
			return ENODEV;

	case WSDISPLAYIO_GETCMAP:
		return ofb_getcmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return ofb_putcmap(sc, (struct wsdisplay_cmap *)data);

	/* XXX There are no way to know framebuffer pa from a user program. */
	case GRFIOCGINFO:
		if (ms != NULL) {
			gm = (void *)data;
			memset(gm, 0, sizeof(struct grfinfo));
			gm->gd_fbaddr = (caddr_t)sc->sc_fbaddr;
			gm->gd_fbrowbytes = ms->scr_ri.ri_stride;
			return 0;
		} else
			return ENODEV;
	case WSDISPLAYIO_SMODE:
		{
			int new_mode = *(int*)data;
			if (new_mode != sc->sc_mode)
			{
				sc->sc_mode = new_mode;
				if (new_mode == WSDISPLAYIO_MODE_EMUL)
				{
					ofb_init_cmap(sc);
					vcons_redraw_screen(ms);
				}
			}
		}
		return 0;
	/* PCI config read/write passthrough. */
	case PCI_IOC_CFGREAD:
	case PCI_IOC_CFGWRITE:
		return (pci_devioctl(sc->sc_pc, sc->sc_pcitag,
		    cmd, data, flag, l));
	}
	return EPASSTHROUGH;
}

static paddr_t
ofb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct ofb_softc *sc = vd->cookie;
	struct rasops_info *ri;
	u_int32_t *ap = sc->sc_addrs;
	struct proc *me;
	int i;

	if (vd->active == NULL) {
		printf("%s: no active screen.\n", sc->sc_dev.dv_xname);
		return -1;
	}
	
	ri = &vd->active->scr_ri;
	
	/* framebuffer at offset 0 */
	if (offset >=0 && offset < (ri->ri_stride * ri->ri_height))
		return sc->sc_fbaddr + offset;

	/*
	 * restrict all other mappings to processes with superuser privileges
	 * or the kernel itself
	 */
	me = __curproc();
	if (me != NULL) {
		if (suser(me->p_ucred, NULL) != 0) {
			printf("%s: mmap() rejected.\n", sc->sc_dev.dv_xname);
			return -1;
		}
	}

	/* let them mmap() 0xa0000 - 0xbffff if it's not covered above */
#ifdef OFB_FAKE_VGA_FB
	if (offset >=0xa0000 && offset < 0xbffff)
		return sc->sc_fbaddr + offset - 0xa0000;
#endif

	/* allow to map our IO space */
	if ((offset >= 0xf2000000) && (offset < 0xf2800000)) {
		return bus_space_mmap(sc->sc_iot, offset-0xf2000000, 0, prot, 
		    BUS_SPACE_MAP_LINEAR);	
	}
	
	for (i = 0; i < 6; i++) {
		switch (ap[0] & OFW_PCI_PHYS_HI_SPACEMASK) {
		case OFW_PCI_PHYS_HI_SPACE_MEM32:
			if (offset >= ap[2] && offset < ap[2] + ap[4])
				return bus_space_mmap(sc->sc_memt, offset, 0, 
				    prot, BUS_SPACE_MAP_LINEAR);
		}
		ap += 5;
	}

	return -1;
}

int
ofb_cnattach()
{
	struct rasops_info *ri = &ofb_console_screen.scr_ri;
	long defattr;
	int crow = 0;
	int chosen, stdout, node;

	chosen = OF_finddevice("/chosen");
	OF_getprop(chosen, "stdout", &stdout, sizeof(stdout));
	node = OF_instance_to_package(stdout);
	console_node = node;
	console_instance = stdout;

	/* get current cursor position */
	OF_interpret("line#", 1, &crow);

	/* move (rom monitor) cursor to the lowest line - 1 */
	OF_interpret("#lines 2 - to line#", 0);
	
	wsfont_init();
	if (copy_rom_font() == 0) {
		romfont_loaded = 1;
	}
	
	/* set up rasops */
	ofb_init_rasops(console_node, ri);

	/*
	 * no need to clear the screen here when we're mimicing firmware
	 * output anyway
	 */
#if 0
	if (ri->ri_width >= 1024 && ri->ri_height >= 768) {
		int i, screenbytes = ri->ri_stride * ri->ri_height;

		for (i = 0; i < screenbytes; i += sizeof(u_int32_t))
			*(u_int32_t *)(fbaddr + i) = 0xffffffff;
		crow = 0;
	}
#endif
	
	ofb_stdscreen.nrows = ri->ri_rows;
	ofb_stdscreen.ncols = ri->ri_cols;
	ofb_stdscreen.textops = &ri->ri_ops;
	ofb_stdscreen.capabilities = ri->ri_caps;

	ri->ri_ops.allocattr(ri, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&ofb_stdscreen, ri, 0, crow, defattr);

	return 0;
}

static int
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

static int
ofb_getcmap(struct ofb_softc *sc, struct wsdisplay_cmap *cm)
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
ofb_putcmap(struct ofb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];
	u_char *r, *g, *b;

	if (cm->index >= 256 || cm->count > 256 ||
	    (cm->index + cm->count) > 256)
		return EINVAL;
	error = copyin(cm->red, &rbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->green, &gbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->blue, &bbuf[index], count);
	if (error)
		return error;

	memcpy(&sc->sc_cmap_red[index], &rbuf[index], count);
	memcpy(&sc->sc_cmap_green[index], &gbuf[index], count);
	memcpy(&sc->sc_cmap_blue[index], &bbuf[index], count);

	r = &sc->sc_cmap_red[index];
	g = &sc->sc_cmap_green[index];
	b = &sc->sc_cmap_blue[index];
	for (i = 0; i < count; i++) {
		OF_call_method_1("color!", sc->sc_ih, 4, *r, *g, *b, index);
		r++, g++, b++, index++;
	}
	return 0;
}

static void
ofb_putpalreg(struct ofb_softc *sc, int idx, uint8_t r, uint8_t g, uint8_t b)
{
	if ((idx<0) || (idx > 255))
		return;
	OF_call_method_1("color!", sc->sc_ih, 4, r, g, b, idx);
	sc->sc_cmap_red[idx] = r;
	sc->sc_cmap_green[idx] = g;
	sc->sc_cmap_blue[idx] = b;
}

static void
ofb_init_cmap(struct ofb_softc *sc)
{
	int idx, i;
	/* mess with the palette only when we're running in 8 bit */
	if (ofb_console_screen.scr_ri.ri_depth == 8) {
		idx = 0;
		for (i = 0; i < 256; i++) {
			ofb_putpalreg(sc, i, rasops_cmap[idx],
			    rasops_cmap[idx + 1], rasops_cmap[idx + 2]);
			idx += 3;
		}
	}
}
