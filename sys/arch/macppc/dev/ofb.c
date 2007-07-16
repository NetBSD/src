/*	$NetBSD: ofb.c,v 1.58 2007/07/16 22:17:03 macallan Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ofb.c,v 1.58 2007/07/16 22:17:03 macallan Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/kauth.h>

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

#include <dev/wscons/wsdisplay_vconsvar.h>

#include <macppc/dev/ofbvar.h>

struct ofb_softc {
	struct	device sc_dev;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
	bus_space_tag_t sc_memt;
	bus_space_tag_t sc_iot;

	u_int32_t sc_addrs[30];		/* "assigned-addresses" storage */
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
	
	int sc_node, sc_ih, sc_mode;
	paddr_t sc_fbaddr;
	
	struct vcons_data vd;
};

static int	ofbmatch(struct device *, struct cfdata *, void *);
static void	ofbattach(struct device *, struct device *, void *);

CFATTACH_DECL(ofb, sizeof(struct ofb_softc),
    ofbmatch, ofbattach, NULL, NULL);

const struct wsscreen_descr *_ofb_scrlist[] = {
	&ofb_stdscreen,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list ofb_screenlist = {
	sizeof(_ofb_scrlist) / sizeof(struct wsscreen_descr *), _ofb_scrlist
};

static int	ofb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	ofb_mmap(void *, void *, off_t, int);

static void	ofb_init_screen(void *, struct vcons_screen *, int, long *);

struct wsdisplay_accessops ofb_accessops = {
	ofb_ioctl,
	ofb_mmap,
	NULL,		/* vcons_alloc_screen */
	NULL,		/* vcons_free_screen */
	NULL,		/* vcons_show_screen */
	NULL		/* load_font */
};

static void	ofb_putpalreg(struct ofb_softc *, int, uint8_t, uint8_t,
    uint8_t);

static int	ofb_getcmap(struct ofb_softc *, struct wsdisplay_cmap *);
static int	ofb_putcmap(struct ofb_softc *, struct wsdisplay_cmap *);
static void	ofb_init_cmap(struct ofb_softc *);
static int	ofb_drm_print(void *, const char *);

extern const u_char rasops_cmap[768];

extern int console_node;
extern int console_instance;

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
	int console, node, sub;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf(": %s\n", devinfo);

	if (console_node == 0)
		return;

	node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
	console = (node == console_node);
	if (!console) {
		/* check if any of the childs matches */
		sub = OF_child(node);
		while ((sub != 0) && (sub != console_node)) {
			sub = OF_peer(sub);
		}
		if (sub == console_node) {
			console = true;
		}
	}
	
	sc->sc_memt = pa->pa_memt;
	sc->sc_iot = pa->pa_iot;	
	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;

	if (!console)
		return;
	
	vcons_init(&sc->vd, sc, &ofb_stdscreen, &ofb_accessops);
	sc->vd.init_screen = ofb_init_screen;

	sc->sc_node = console_node;

	sc->sc_ih = console_instance;
	vcons_init_screen(&sc->vd, &ofb_console_screen, 1, &defattr);
	ofb_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
	
	printf("%s: %d x %d, %dbpp\n", self->dv_xname,
	       ri->ri_width, ri->ri_height, ri->ri_depth);
	
	sc->sc_fbaddr = 0;
	if (OF_getprop(sc->sc_node, "address", &sc->sc_fbaddr, 4) != 4)
		OF_interpret("frame-buffer-adr", 0, 1, &sc->sc_fbaddr);
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

	config_found_ia(self, "drm", aux, ofb_drm_print);
}

static int
ofb_drm_print(void *aux, const char *pnp)
{
	if (pnp)
		aprint_normal("direct rendering for %s", pnp);
	return (UNSUPP);
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
ofb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
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
			gm->gd_fbaddr = (void *)sc->sc_fbaddr;
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
	struct lwp *me;
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
	me = curlwp;
	if (me != NULL) {
		if (kauth_authorize_generic(me->l_cred, KAUTH_GENERIC_ISSUSER,
		    NULL) != 0) {
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
	if (sc != NULL) {
		OF_call_method_1("color!", sc->sc_ih, 4, r, g, b, idx);
		sc->sc_cmap_red[idx] = r;
		sc->sc_cmap_green[idx] = g;
		sc->sc_cmap_blue[idx] = b;
	} else {
		OF_call_method_1("color!", console_instance, 4, r, g, b, idx);
	}	
}

static void
ofb_init_cmap(struct ofb_softc *sc)
{
	int idx, i;
	/* mess with the palette only when we're running in 8 bit */
	if (ofb_console_screen.scr_ri.ri_depth == 8) {
		idx = 0;
		for (i = 0; i < 256; i++) {
			ofb_putpalreg(sc, i, rasops_cmap[(i * 3) + 0],
			     rasops_cmap[(i * 3) + 1],
			     rasops_cmap[(i * 3) + 2]);
		}
	}
}
