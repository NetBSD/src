#include <sys/param.h>
#include <sys/device.h>

#include <machine/pio.h>
#include <machine/autoconf.h>
#include <machine/grfioctl.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/ofw/openfirm.h>

#include <macppc/dev/grfvar.h>

caddr_t videoaddr;
int videorowbytes;
int videobitdepth;
int videosize;
static struct grfinfo ati_display;

static void grf_ati_attach __P((struct device *, struct device *, void *));
static int grf_ati_match __P((struct device *, struct cfdata *, void *));

static int ati_mode __P((struct grf_softc *, int, void *));

struct grf_ati_softc {
	struct device sc_dev;
};

struct cfattach grfati_ca = {
	sizeof(struct grf_ati_softc), grf_ati_match, grf_ati_attach
};

int
grf_ati_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_DISPLAY_VGA &&
	/*  PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_ATI_MACH64_GX && */
	    PCI_VENDOR(pa->pa_id) == PCI_VENDOR_ATI)
		return 1;

	return 0;
}

void
grf_ati_attach(parent, self, aux)
	struct device *parent, *self;
	void	*aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	int csr;
	int width, height, node;
	u_int reg[5];
	caddr_t regaddr;
	struct grf_attach_args ga;
	char type[32];
	extern int console_node;

	printf("\n");
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);

	csr |= PCI_COMMAND_MEM_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, csr);

	/*
	 * OF_open("/bandit/ATY,mach64") hangs... so we can use
	 * framebuffer when it is specified as a console.
	 */

	node = console_node;
	if (node == -1)
		return;

	bzero(type, sizeof(type));
	OF_getprop(node, "device_type", type, sizeof(type));
	if (strcmp(type, "display") != 0)
		return;

	OF_getprop(node, "assigned-addresses", reg, sizeof(reg));

	regaddr = mapiodev(reg[2] + 0x800000 - 0x400, 0x400);
	videoaddr = mapiodev(reg[2] + 0x400 , reg[4] - 0x400);


	/*
	 * set character color to rgb:ff/ff/ff
	 */
#define DAC_W_INDEX 0xc0
#define DAC_DATA    0xc1

	out8(regaddr + DAC_W_INDEX, 255);
	out8(regaddr + DAC_DATA, 255);
	out8(regaddr + DAC_DATA, 255);
	out8(regaddr + DAC_DATA, 255);

	ga.ga_name = "grf";
	ga.ga_mode = ati_mode;
	ga.ga_display = &ati_display;

	height = videosize >> 16;

	ati_display.gd_regaddr = (caddr_t)reg[2] + 0x800000 - 0x400;
	ati_display.gd_regsize = 0x400;
	ati_display.gd_fbaddr = (caddr_t)reg[2] + 0x400;
	ati_display.gd_fbsize = videorowbytes * height;
	ati_display.gd_colors = 1 << videobitdepth;
	ati_display.gd_planes = videobitdepth;
	ati_display.gd_fbwidth = videorowbytes * 8 / videobitdepth;
	ati_display.gd_fbheight = height;
	ati_display.gd_fbrowbytes = videorowbytes;
	ati_display.gd_devaddr = (caddr_t)reg[2];
	ati_display.gd_devsize = reg[4];

	config_found(self, &ga, NULL);
}

int
ati_mode(sc, cmd, aux)
	struct grf_softc *sc;
	int cmd;
	void *aux;
{
	return 0;
}
