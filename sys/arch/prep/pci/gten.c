/*	$NetBSD: gten.c,v 1.18.12.1 2011/06/23 14:19:36 cherry Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gten.c,v 1.18.12.1 2011/06/23 14:19:36 cherry Exp $");

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

#include <machine/bus.h>
#include <machine/gtenvar.h>

static	int	gten_match(device_t, cfdata_t, void *);
static	void	gten_attach(device_t, device_t, void *);
static	int	gten_print(void *, const char *);

CFATTACH_DECL_NEW(gten, sizeof(struct gten_softc),
    gten_match, gten_attach, NULL, NULL);

static struct rasops_info gten_console_ri;
static pcitag_t gten_console_pcitag;

static struct wsscreen_descr gten_stdscreen = {
	"std",
	0, 0,
	0,
	0, 0,
	WSSCREEN_REVERSE
};

static const struct wsscreen_descr *_gten_scrlist[] = {
	&gten_stdscreen,
	/* XXX other formats, graphics screen? */
};

static struct wsscreen_list gten_screenlist = {
	sizeof(_gten_scrlist) / sizeof(struct wsscreen_descr *), _gten_scrlist
};

static int gten_ioctl(void *, void *, u_long, void *, int, struct proc *);
static paddr_t gten_mmap(void *, void *, off_t, int);
static int gten_alloc_screen(void *, const struct wsscreen_descr *,
			     void **, int *, int *, long *);
static void gten_free_screen(void *, void *);
static int gten_show_screen(void *, void *, int,
			    void (*) (void *, int, int), void *);

static struct wsdisplay_accessops gten_accessops = {
	gten_ioctl,
	gten_mmap,
	gten_alloc_screen,
	gten_free_screen,
	gten_show_screen,
	0 /* load_font */
};

static void gten_common_init(struct rasops_info *);
static int gten_getcmap(struct gten_softc *, struct wsdisplay_cmap *);
static int gten_putcmap(struct gten_softc *, struct wsdisplay_cmap *);

#define	GTEN_VRAM_OFFSET	0xf00000

static int
gten_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_WD &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_WD_90C)
		return 2;

	return 0;
}

static void
gten_attach(device_t parent, device_t self, void *aux)
{
	struct gten_softc *gt = device_private(self);
	struct pci_attach_args *pa = aux;
	struct wsemuldisplaydev_attach_args a;
	int console = (pa->pa_tag == gten_console_pcitag);
	int error;
	char devinfo[256], pbuf[10];

	error = pci_mapreg_info(pa->pa_pc, pa->pa_tag, 0x14,
		PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT,
		&gt->gt_memaddr, &gt->gt_memsize, NULL);
	if (error) {
		aprint_error(": can't determine memory size: error=%d\n",
			error);
		return;
	}
	gt->gt_dev = self;
	if (console) {
		gt->gt_ri = &gten_console_ri;
		gt->gt_nscreens = 1;
	} else {
		gt->gt_ri = malloc(sizeof(*gt->gt_ri),
			M_DEVBUF, M_NOWAIT|M_ZERO);
		if (gt->gt_ri == NULL) {
			aprint_error(": can't alloc memory\n");
			return;
		}
#if 0
		error = pci_mapreg_map(pa, 0x14, 
			PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT,
			BUS_SPACE_MAP_LINEAR, NULL,
			(bus_space_handle_t *) &gt->gt_ri->ri_bits,
			NULL, NULL);
#else
		error = bus_space_map(pa->pa_memt, gt->gt_memaddr + GTEN_VRAM_OFFSET,
			960*1024, BUS_SPACE_MAP_LINEAR, 
			(bus_space_handle_t *) &gt->gt_ri->ri_bits);
#endif
		if (error) {
			aprint_error(": can't map frame buffer: error=%d\n",
			    error);
			return;
		}

		gten_common_init(gt->gt_ri);
	}

	gt->gt_paddr = vtophys((vaddr_t)gt->gt_ri->ri_bits);
	if (gt->gt_paddr == 0) {
		aprint_error(": cannot map framebuffer\n");
		return;
	}
	gt->gt_psize = gt->gt_memsize - GTEN_VRAM_OFFSET;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s\n", devinfo);
	format_bytes(pbuf, sizeof(pbuf), gt->gt_psize);
	aprint_normal_dev(self, "%s: %s, %dx%d, %dbpp\n", pbuf,
	       gt->gt_ri->ri_width, gt->gt_ri->ri_height,
	       gt->gt_ri->ri_depth);
#if defined(DEBUG)
	aprint_debug_dev(self, "%s: text %dx%d, =+%d+%d\n",
	       gt->gt_ri->ri_cols, gt->gt_ri->ri_rows,
	       gt->gt_ri->ri_xorigin, gt->gt_ri->ri_yorigin);

	{ int i;
	  struct rasops_info *ri = gt->gt_ri;
	for (i = 0; i < 64; i++) {
		int j = i * ri->ri_stride;
		int k = (ri->ri_height - i - 1) * gt->gt_ri->ri_stride;
		memset(ri->ri_bits + j, 0, 64 - i);
		memset(ri->ri_bits + j + 64 - i, 255, i);

		memset(ri->ri_bits + j + ri->ri_width - 64 + i, 0, 64 - i);
		memset(ri->ri_bits + j + ri->ri_width - 64, 255, i);

		memset(ri->ri_bits + k, 0, 64 - i);
		memset(ri->ri_bits + k + 64 - i, 255, i);

		memset(ri->ri_bits + k + ri->ri_width - 64 + i, 0, 64 - i);
		memset(ri->ri_bits + k + ri->ri_width - 64, 255, i);
	}}
#endif

	gt->gt_cmap_red[0] = gt->gt_cmap_green[0] = gt->gt_cmap_blue[0] = 0;
	gt->gt_cmap_red[15] = gt->gt_cmap_red[255] = 0xff;
	gt->gt_cmap_green[15] = gt->gt_cmap_green[255] = 0xff;
	gt->gt_cmap_blue[15] = gt->gt_cmap_blue[255] = 0xff;

	a.console = console;
	a.scrdata = &gten_screenlist;
	a.accessops = &gten_accessops;
	a.accesscookie = gt;

	config_found(self, &a, wsemuldisplaydevprint);
}

static void
gten_common_init(struct rasops_info *ri)
{
	int32_t addr, width, height, linebytes, depth;
	int i, screenbytes;

	/* initialize rasops */
	ri->ri_width = 640;
	ri->ri_height = 480;
	ri->ri_depth = 8;
	ri->ri_stride = 640;
	ri->ri_flg = RI_FORCEMONO | RI_FULLCLEAR;

	rasops_init(ri, 30, 80);
	/* black on white */
	ri->ri_devcmap[0] = 0xffffffff;			/* bg */
	ri->ri_devcmap[1] = 0;				/* fg */

	memset(ri->ri_bits, 0xff, ri->ri_stride * ri->ri_height);

	gten_stdscreen.nrows = ri->ri_rows;
	gten_stdscreen.ncols = ri->ri_cols;
	gten_stdscreen.textops = &ri->ri_ops;
	gten_stdscreen.capabilities = ri->ri_caps;
}

static int
gten_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct proc *p)
{
	struct gten_softc *gt = v;
	struct wsdisplay_fbinfo *wdf;
	struct grfinfo *gm;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;	/* XXX ? */
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = gt->gt_ri->ri_height;
		wdf->width = gt->gt_ri->ri_width;
		wdf->depth = gt->gt_ri->ri_depth;
		wdf->cmsize = 256;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return gten_getcmap(gt, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return gten_putcmap(gt, (struct wsdisplay_cmap *)data);
	}
	return EPASSTHROUGH;
}

static paddr_t
gten_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct gten_softc *gt = v;

	if (offset >= 0 && offset < gt->gt_psize)
		return gt->gt_paddr + offset;
	if (offset >= 0x1000000 && offset < gt->gt_memsize)
		return gt->gt_memaddr + offset - 0x1000000;

	return -1;
}

static int
gten_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
    int *curxp, int *curyp, long *attrp)
{
	struct gten_softc *gt = v;
	struct rasops_info *ri = gt->gt_ri;
	long defattr;

	if (gt->gt_nscreens > 0)
		return (ENOMEM);

	*cookiep = ri;			/* one and only for now */
	*curxp = 0;
	*curyp = 0;
	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	*attrp = defattr;
	gt->gt_nscreens++;
	return 0;
}

static void
gten_free_screen(void *v, void *cookie)
{
	struct gten_softc *gt = v;

	if (gt->gt_ri == &gten_console_ri)
		panic("gten_free_screen: console");

	gt->gt_nscreens--;
}

static int
gten_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{
	return (0);
}

int
gten_cnattach(pci_chipset_tag_t pc, bus_space_tag_t memt)
{
	struct rasops_info *ri = &gten_console_ri;
	u_int32_t mapreg, id, mask, mapsize;
	long defattr;
	pcitag_t tag;
	int s, error;
	bus_size_t bussize;
	bus_addr_t busaddr;

	tag = pci_make_tag(pc, 0, 14, 0);

	id = pci_conf_read(pc, tag, PCI_ID_REG);
	if (PCI_VENDOR(id) != PCI_VENDOR_WD ||
	    PCI_PRODUCT(id) != PCI_PRODUCT_WD_90C)
		return ENXIO;

	mapreg = pci_conf_read(pc, tag, 0x14);
	if (PCI_MAPREG_TYPE(mapreg) != PCI_MAPREG_TYPE_MEM ||
	    PCI_MAPREG_MEM_TYPE(mapreg) != PCI_MAPREG_MEM_TYPE_32BIT)
		return ENXIO;

        s = splhigh();
        pci_conf_write(pc, tag, 0x14, 0xffffffff);
        mask = pci_conf_read(pc, tag, 0x14);
        pci_conf_write(pc, tag, 0x14, mapreg);
	splx(s);
	bussize = PCI_MAPREG_MEM_SIZE(mask);
	busaddr = PCI_MAPREG_MEM_ADDR(mapreg);

	error = bus_space_map(memt, busaddr + GTEN_VRAM_OFFSET, 960*1024,
		BUS_SPACE_MAP_LINEAR, (bus_space_handle_t *) &ri->ri_bits);
	if (error)
		return error;

	gten_common_init(ri);

	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&gten_stdscreen, ri, 0, 0, defattr);

	gten_console_pcitag = tag;

	return 0;
}

static int
gten_getcmap(struct gten_softc *gt, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int error;

	if (index >= 256 || count > 256 || index + count > 256)
		return EINVAL;

	error = copyout(&gt->gt_cmap_red[index],   cm->red,   count);
	if (error)
		return error;
	error = copyout(&gt->gt_cmap_green[index], cm->green, count);
	if (error)
		return error;
	error = copyout(&gt->gt_cmap_blue[index],  cm->blue,  count);
	if (error)
		return error;

	return 0;
}

static int
gten_putcmap(struct gten_softc *gt, struct wsdisplay_cmap *cm)
{
	int index = cm->index;
	int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];

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

	memcpy(&gt->gt_cmap_red[index], &rbuf[index], count);
	memcpy(&gt->gt_cmap_green[index], &gbuf[index], count);
	memcpy(&gt->gt_cmap_blue[index], &bbuf[index], count);
	return 0;
}
