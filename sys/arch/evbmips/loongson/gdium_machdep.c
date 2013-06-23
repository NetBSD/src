/*	$OpenBSD: gdium_machdep.c,v 1.6 2010/05/08 21:59:56 miod Exp $	*/

/*
 * Copyright (c) 2010 Miodrag Vallat.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Gdium Liberty specific code and configuration data.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/types.h>

#include <evbmips/loongson/autoconf.h>
#include <evbmips/loongson/loongson_intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <mips/bonito/bonitoreg.h>
#include <mips/bonito/bonitovar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

int	gdium_revision = 0;
static pcireg_t fb_addr = 0;

void	gdium_attach_hook(device_t, device_t, struct pcibus_attach_args *);
void	gdium_device_register(device_t, void *);
int	gdium_intr_map(int, int, int, pci_intr_handle_t *);
void	gdium_powerdown(void);
void	gdium_reset(void);

const struct bonito_config gdium_bonito = {
	.bc_adbase = 11,

	.bc_gpioIE = LOONGSON_INTRMASK_GPIO,
	.bc_intEdge = LOONGSON_INTRMASK_PCI_SYSERR |
	    LOONGSON_INTRMASK_PCI_PARERR,
	.bc_intSteer = 0,
	.bc_intPol = LOONGSON_INTRMASK_DRAM_PARERR |
	    LOONGSON_INTRMASK_PCI_SYSERR | LOONGSON_INTRMASK_PCI_PARERR,

	.bc_attach_hook = gdium_attach_hook,
};


const struct platform gdium_platform = {
	.system_type = LOONGSON_GDIUM,
	.vendor = "EMTEC",
	.product = "Gdium",

	.bonito_config = &gdium_bonito,
	.isa_chipset = NULL,
	.legacy_io_ranges = NULL,
	.bonito_mips_intr = MIPS_INT_MASK_4,
	.isa_mips_intr = 0,
	.isa_intr = NULL,
	.p_pci_intr_map = gdium_intr_map,
	.irq_map = loongson2f_irqmap,

	.setup = NULL,
	.device_register = gdium_device_register,

	.powerdown = gdium_powerdown,
	.reset = gdium_reset
};

static struct vcons_screen gdium_console_screen;

static struct wsscreen_descr gdium_stdscreen = {
	.name = "std",
};

void
gdium_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
	pci_chipset_tag_t pc = pba->pba_pc;
	pcireg_t id;
	pcitag_t tag;
#ifdef notyet
	int bar;
#endif
#if 0
	pcireg_t reg;
	int dev, func;
#endif

	if (pba->pba_bus != 0)
		return;

#ifdef notyet
	/*
	 * Clear all BAR of the mini PCI slot; PMON did not initialize
	 * it, and we do not want it to conflict with anything.
	 */
	tag = pci_make_tag(pc, 0, 13, 0);
	for (bar = PCI_MAPREG_START; bar < PCI_MAPREG_END; bar += 4)
		pci_conf_write(pc, tag, bar, 0);
#else
	/*
	 * Force a non conflicting BAR for the wireless controller,
	 * until proper resource configuration code is added to
	 * bonito (work in progress).
	 */
	tag = pci_make_tag(pc, 0, 13, 0);
	pci_conf_write(pc, tag, PCI_MAPREG_START, 0x06228000);
#endif

	/*
	 * Figure out which motherboard we are running on.
	 * Might not be good enough...
	 */
	tag = pci_make_tag(pc, 0, 17, 0);
	id = pci_conf_read(pc, tag, PCI_ID_REG);
	if (id == PCI_ID_CODE(PCI_VENDOR_NEC, PCI_PRODUCT_NEC_USB))
		gdium_revision = 1;
	
#if 0
	/*
	 * Tweak the usb controller capabilities.
	 */
	for (dev = pci_bus_maxdevs(pc, 0); dev >= 0; dev--) {
		tag = pci_make_tag(pc, 0, dev, 0);
		id = pci_conf_read(pc, tag, PCI_ID_REG);
		if (id != PCI_ID_CODE(PCI_VENDOR_NEC, PCI_PRODUCT_NEC_USB))
			continue;
		if (gdium_revision != 0) {
			reg = pci_conf_read(pc, tag, 0xe0);
			/* enable ports 1 and 2 */
			reg |= 0x00000003;
			pci_conf_write(pc, tag, 0xe0, reg);
		} else {
			for (func = 0; func < 2; func++) {
				tag = pci_make_tag(pc, 0, dev, func);
				id = pci_conf_read(pc, tag, PCI_ID_REG);
				if (PCI_VENDOR(id) != PCI_VENDOR_NEC)
					continue;
				if (PCI_PRODUCT(id) != PCI_PRODUCT_NEC_USB &&
				    PCI_PRODUCT(id) != PCI_PRODUCT_NEC_USB2)
					continue;

				reg = pci_conf_read(pc, tag, 0xe0);
				/* enable ports 1 and 3, disable port 2 */
				reg &= ~0x00000007;
				reg |= 0x00000005;
				pci_conf_write(pc, tag, 0xe0, reg);
				pci_conf_write(pc, tag, 0xe4, 0x00000020);
			}
		}
	}
#endif
}

int
gdium_intr_map(int dev, int fn, int pin, pci_intr_handle_t *ihp)
{
	switch (dev) {
	/* mini-PCI slot */
	case 13:	/* C D A B */
		*ihp = BONITO_DIRECT_IRQ(LOONGSON_INTR_PCIA + (pin + 1) % 4);
		return 0;
	/* Frame buffer */
	case 14:
		*ihp = BONITO_DIRECT_IRQ(LOONGSON_INTR_PCIA);
		return 0;
	/* USB */
	case 15:
		if (gdium_revision == 0)
			*ihp = BONITO_DIRECT_IRQ(LOONGSON_INTR_PCIA +
			    (pin - 1));
		else
			*ihp = BONITO_DIRECT_IRQ(LOONGSON_INTR_PCIB);
		return 0;
	/* Ethernet */
	case 16:
		*ihp = BONITO_DIRECT_IRQ(LOONGSON_INTR_PCID);
		return 0;
	/* USB, not present in old motherboard revision */
	case 17:
		if (gdium_revision != 0) {
			*ihp = BONITO_DIRECT_IRQ(LOONGSON_INTR_PCIC);
			return 0;
		} else
			break;
	default:
		break;
	}

	return 1;
}

/*
 * Due to PMON limitations on the Gdium Liberty, we do not get boot device
 * information from PMON.
 *
 * Because of this, we always pretend the G-Key port is the boot device.
 *
 * Note that, unlike on the Lemote machines, other USB devices gets a fixed
 * numbering (USB0 and USB1).
 */

extern struct cfdriver bonito_cd;
extern struct cfdriver pci_cd;
extern struct cfdriver ehci_cd;
extern struct cfdriver usb_cd;
extern struct cfdriver uhub_cd;
extern struct cfdriver umass_cd;
extern struct cfdriver scsibus_cd;
extern struct cfdriver sd_cd;

#include <dev/pci/pcivar.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>

void
gdium_device_register(device_t dev, void *aux)
{
	prop_dictionary_t dict;
	static int gkey_chain_pos = 0;
	static device_t lastparent = NULL;

	if (device_is_a(dev, "genfb") || device_is_a(dev, "voyagerfb")) {
		dict = device_properties(dev);
		/*
		 * this is a hack
		 * is_console needs to be checked against reality
		 */
		prop_dictionary_set_bool(dict, "is_console", 1);
		prop_dictionary_set_uint32(dict, "width", 1024);
		prop_dictionary_set_uint32(dict, "height", 600);
		prop_dictionary_set_uint32(dict, "depth", 16);
		prop_dictionary_set_uint32(dict, "linebytes", 2048);
		if (fb_addr != 0)
			prop_dictionary_set_uint32(dict, "address", fb_addr);
	}
	if (device_parent(dev) != lastparent && gkey_chain_pos != 0)
		return;

	switch (gkey_chain_pos) {
	case 0:	/* bonito at mainbus */
		if (device_is_a(dev, "bonito"))
			goto advance;
		break;
	case 1:	/* pci at bonito */
		if (device_is_a(dev, "pci"))
			goto advance;
		break;
	case 2:	/* ehci at pci dev 15 */
		if (device_is_a(dev, "ehci")) {
			struct pci_attach_args *paa = aux;
			if (paa->pa_device == 15)
				goto advance;
		}
		break;
	case 3:	/* usb at ehci */
		if (device_is_a(dev, "usb"))
			goto advance;
		break;
	case 4:	/* uhub at usb */
		if (device_is_a(dev, "uhub"))
			goto advance;
		break;
	case 5:	/* umass at uhub port 3 */
		if (device_is_a(dev, "umass")) {
			struct usb_attach_arg *uaa = aux;
			if (uaa->port == 3)
				goto advance;
		}
		break;
	case 6:	/* scsibus at umass */
		if (device_is_a(dev, "scsibus"))
			goto advance;
		break;
	case 7:	/* sd at scsibus */
		if (booted_device == NULL)
			booted_device = dev;
		break;
	}

	return;

advance:
	gkey_chain_pos++;
	lastparent = dev;
}

void
gdium_powerdown(void)
{
	REGVAL(BONITO_GPIODATA) |= 0x00000002;
	REGVAL(BONITO_GPIOIE) &= ~0x00000002;
	printf("Powering down...\n");
	while(1) delay(1000);
}

void
gdium_reset(void)
{
	REGVAL(BONITO_GPIODATA) &= ~0x00000002;
	REGVAL(BONITO_GPIOIE) &= ~0x00000002;
}

/*
 * Early console code
 */

int
gdium_cnattach(bus_space_tag_t memt, bus_space_tag_t iot,
    pci_chipset_tag_t pc, pcitag_t tag, pcireg_t id)
{
	struct rasops_info * const ri = &gdium_console_screen.scr_ri;
	long defattr;
	pcireg_t reg;


	/* filter out unrecognized devices */
	switch (id) {
	default:
		return ENODEV;
	case PCI_ID_CODE(PCI_VENDOR_SILMOTION, PCI_PRODUCT_SILMOTION_SM502):
		break;
	}

	wsfont_init();
	
	/* set up rasops */
	ri->ri_width = 1024;
	ri->ri_height = 600;
	ri->ri_depth = 16;
	ri->ri_stride = 0x800;

	/* read the mapping register for the frame buffer */
	reg = pci_conf_read(pc, tag, PCI_MAPREG_START);
	fb_addr = reg;

	ri->ri_bits = (char *)MIPS_PHYS_TO_KSEG1(BONITO_PCILO_BASE + reg);
	ri->ri_flg = RI_CENTER | RI_NO_AUTO;

	memset(ri->ri_bits, 0, 0x200000);

	/* use as much of the screen as the font permits */
	rasops_init(ri, 30, 80);

	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);

	gdium_stdscreen.nrows = ri->ri_rows;
	gdium_stdscreen.ncols = ri->ri_cols;
	gdium_stdscreen.textops = &ri->ri_ops;
	gdium_stdscreen.capabilities = ri->ri_caps;

	ri->ri_ops.allocattr(ri, 0, ri->ri_rows - 1, 0, &defattr);

	wsdisplay_preattach(&gdium_stdscreen, ri, 0, 0, defattr);
	
	return 0;

}
