/*	$NetBSD: gten.c,v 1.1 2000/12/01 21:54:01 matt Exp $	*/

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

int	gten_match (struct device *, struct cfdata *, void *);
void	gten_attach (struct device *, struct device *, void *);
int	gten_print (void *, const char *);

struct cfattach gten_ca = {
	sizeof(struct gten_softc), gten_match, gten_attach,
};

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

static int gten_ioctl (void *, u_long, caddr_t, int, struct proc *);
static paddr_t gten_mmap (void *, off_t, int);
static int gten_alloc_screen (void *, const struct wsscreen_descr *,
			      void **, int *, int *, long *);
static void gten_free_screen (void *, void *);
static int gten_show_screen (void *, void *, int,
			     void (*) (void *, int, int), void *);

struct wsdisplay_accessops gten_accessops = {
	gten_ioctl,
	gten_mmap,
	gten_alloc_screen,
	gten_free_screen,
	gten_show_screen,
	0 /* load_font */
};

static void gten_common_init (struct rasops_info *);
static int gten_getcmap (struct gten_softc *, struct wsdisplay_cmap *);
static int gten_putcmap (struct gten_softc *, struct wsdisplay_cmap *);

int
gten_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_WD &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_WD_90C)
		return 2;

	return 0;
}

void
gten_attach(struct device *parent, struct device *self, void *aux)
{
	struct gten_softc *gt = (struct gten_softc *)self;
	struct pci_attach_args *pa = aux;
	struct wsemuldisplaydev_attach_args a;
	int console = (pa->pa_tag == gten_console_pcitag);
	int error;
	char devinfo[256], pbuf[10];

	error = pci_mapreg_info(pa->pa_pc, pa->pa_tag, 0x14,
		PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT,
		&gt->gt_memaddr, &gt->gt_memsize, NULL);
	if (error) {
		printf(": can't determine memory size: error=%d\n",
			error);
		return;
	}
	if (console) {
		gt->gt_ri = &gten_console_ri;
		gt->gt_nscreens = 1;
	} else {
		MALLOC(gt->gt_ri, struct rasops_info *, sizeof(*gt->gt_ri),
			M_DEVBUF, M_NOWAIT);
		if (gt->gt_ri == NULL) {
			printf(": can't alloc memory\n");
			return;
		}
		memset(gt->gt_ri, 0, sizeof(*gt->gt_ri));
#if 0
		error = pci_mapreg_map(pa, 0x14, 
			PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT,
			BUS_SPACE_MAP_LINEAR, NULL,
			(bus_space_handle_t *) &gt->gt_ri->ri_bits,
			NULL, NULL);
#else
		error = bus_space_map(pa->pa_memt, gt->gt_memaddr + 0xef9000,
			1024*1024, BUS_SPACE_MAP_LINEAR, 
			(bus_space_handle_t *) &gt->gt_ri->ri_bits);
#endif
		if (error) {
			printf(": can't map frame buffer: error=%d\n", error);
			return;
		}

		gten_common_init(gt->gt_ri);

#if 0
		int i, len, screenbytes;
		screenbytes = ri->ri_stride * ri->ri_height;
		for (i = 0; i < screenbytes; i += sizeof(u_int32_t))
			*(u_int32_t *)(gt->gt_ri->ri_bits + i) = 0xffffffff;
#endif
	}

	gt->gt_paddr = vtophys((vaddr_t)gt->gt_ri->ri_bits);
	if (gt->gt_paddr == 0) {
		printf(": cannot map framebuffer\n");
		return;
	}

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s\n", devinfo);
	format_bytes(pbuf, sizeof(pbuf), gt->gt_memsize);
	printf("%s: %s, %d x %d, %dbpp\n", self->dv_xname, pbuf,
	       gt->gt_ri->ri_width, gt->gt_ri->ri_height, gt->gt_ri->ri_depth);

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
	ri->ri_flg = RI_FORCEMONO | RI_FULLCLEAR | RI_CENTER;

	rasops_init(ri, 24, 80);
	/* black on white */
	ri->ri_devcmap[0] = 0xffffffff;			/* bg */
	ri->ri_devcmap[1] = 0;				/* fg */

	gten_stdscreen.nrows = ri->ri_rows;
	gten_stdscreen.ncols = ri->ri_cols;
	gten_stdscreen.textops = &ri->ri_ops;
	gten_stdscreen.capabilities = ri->ri_caps;

	screenbytes = ri->ri_stride * ri->ri_height;
	for (i = 0; i < screenbytes; i += sizeof(u_int32_t))
		*(u_int32_t *)(ri->ri_bits + i) = 0xffffffff;
}

int
gten_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
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
	return -1;
}

paddr_t
gten_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct gten_softc *gt = v;

	if (offset >=0 && offset < gt->gt_memsize)
		return gt->gt_paddr + offset;

	return -1;
}

int
gten_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct gten_softc *gt = v;
	struct rasops_info *ri = gt->gt_ri;
	long defattr;

	if (gt->gt_nscreens > 0)
		return (ENOMEM);

	*cookiep = ri;			/* one and only for now */
	*curxp = 0;
	*curyp = 0;
	(*ri->ri_ops.alloc_attr)(ri, 0, 0, 0, &defattr);
	*attrp = defattr;
	gt->gt_nscreens++;
	return 0;
}

void
gten_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct gten_softc *gt = v;

	if (gt->gt_ri == &gten_console_ri)
		panic("gten_free_screen: console");

	gt->gt_nscreens--;
}

int
gten_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb) (void *, int, int);
	void *cbarg;
{
	return (0);
}

int
gten_cnattach(bus_space_tag_t memt)
{
	struct rasops_info *ri = &gten_console_ri;
	u_int32_t mapreg, id, mask, mapsize;
	long defattr;
	pcitag_t tag;
	int s, error;
	bus_size_t bussize;
	bus_addr_t busaddr;

	tag = pci_make_tag(NULL, 0, 14, 0);

	id = pci_conf_read(NULL, tag, PCI_ID_REG);
	if (PCI_VENDOR(id) != PCI_VENDOR_WD ||
	    PCI_PRODUCT(id) != PCI_PRODUCT_WD_90C)
		return ENXIO;

	mapreg = pci_conf_read(NULL, tag, 0x14);
	if (PCI_MAPREG_TYPE(mapreg) != PCI_MAPREG_TYPE_MEM ||
	    PCI_MAPREG_MEM_TYPE(mapreg) != PCI_MAPREG_MEM_TYPE_32BIT)
		return ENXIO;

        s = splhigh();
        pci_conf_write(NULL, tag, 0x14, 0xffffffff);
        mask = pci_conf_read(NULL, tag, 0x14);
        pci_conf_write(NULL, tag, 0x14, mapreg);
	splx(s);
	bussize = PCI_MAPREG_MEM_SIZE(mask);
	busaddr = PCI_MAPREG_MEM_ADDR(mapreg);

	error = bus_space_map(memt, busaddr + 0xef9000, bussize,
		BUS_SPACE_MAP_LINEAR, (bus_space_handle_t *) &ri->ri_bits);
	if (error)
		return error;

	gten_common_init(ri);

	(*ri->ri_ops.alloc_attr)(ri, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&gten_stdscreen, ri, 0, 0, defattr);

	gten_console_pcitag = tag;

	return 0;
}

int
gten_getcmap(gt, cm)
	struct gten_softc *gt;
	struct wsdisplay_cmap *cm;
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

int
gten_putcmap(gt, cm)
	struct gten_softc *gt;
	struct wsdisplay_cmap *cm;
{
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
	copyin(cm->red,   &gt->gt_cmap_red[index],   count);
	copyin(cm->green, &gt->gt_cmap_green[index], count);
	copyin(cm->blue,  &gt->gt_cmap_blue[index],  count);

	r = &gt->gt_cmap_red[index];
	g = &gt->gt_cmap_green[index];
	b = &gt->gt_cmap_blue[index];

#if 0
	for (i = 0; i < count; i++) {
		OF_call_method_1("color!", dc->dc_ih, 4, *r, *g, *b, index);
		r++, g++, b++, index++;
	}
#endif

	return 0;
}
