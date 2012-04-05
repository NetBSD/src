/* $NetBSD: vga_ofbus.c,v 1.15.32.1 2012/04/05 21:33:18 mrg Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: vga_ofbus.c,v 1.15.32.1 2012/04/05 21:33:18 mrg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/kauth.h>

#include <dev/isa/isavar.h>

#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

#include <dev/ofw/openfirm.h>

#include <shark/ofw/vga_ofbusvar.h>

struct vga_ofbus_softc {
	struct vga_softc sc_vga;

	int sc_phandle;
};

#if (NIGSFB_OFBUS > 0) || (NVGA_OFBUS > 0)
extern int console_ihandle;
#endif

int	vga_ofbus_match (device_t, cfdata_t, void *);
void	vga_ofbus_attach (device_t, device_t, void *);

CFATTACH_DECL_NEW(vga_ofbus, sizeof(struct vga_ofbus_softc),
    vga_ofbus_match, vga_ofbus_attach, NULL, NULL);

static const char *compat_strings[] = { "pnpPNP,900", 0 };

static	int vga_ofbus_ioctl(void *, u_long, void *, int, struct lwp *);
static	paddr_t vga_ofbus_mmap(void *, off_t, int);

static const struct vga_funcs vga_ofbus_funcs = {
	vga_ofbus_ioctl,
	vga_ofbus_mmap
};
static uint32_t vga_reg[12];
extern paddr_t isa_io_physaddr, isa_mem_physaddr;

int
vga_ofbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct ofbus_attach_args *oba = aux;

	if (of_compatible(oba->oba_phandle, compat_strings) == -1)
		return (0);

	if (!vga_is_console(&isa_io_bs_tag, WSDISPLAY_TYPE_ISAVGA) &&
	    !vga_common_probe(&isa_io_bs_tag, &isa_mem_bs_tag))
		return (0);

	return (2);	/* more than generic pcdisplay */
}

void
vga_ofbus_attach(device_t parent, device_t self, void *aux)
{
	struct vga_ofbus_softc *osc = device_private(self);
	struct vga_softc *sc = &osc->sc_vga;
	struct ofbus_attach_args *oba = aux;
	int vga_handle, i;

	sc->sc_dev = self;
	aprint_normal("\n");
	osc->sc_phandle = oba->oba_phandle;

	vga_handle = OF_finddevice("/vlbus/display");
	OF_getprop(vga_handle, "reg", vga_reg, sizeof(vga_reg));

	/* for some idiotic reason we get this in the wrong byte order */
	for (i = 0; i < 12; i++) {
		vga_reg[i] = be32toh(vga_reg[i]);
		aprint_debug_dev(self, "vga_reg[%2d] = 0x%08x\n",
		    i, vga_reg[i]);
	}

	vga_common_attach(sc, &isa_io_bs_tag, &isa_mem_bs_tag,
	    WSDISPLAY_TYPE_ISAVGA, 0, &vga_ofbus_funcs);
	if (vga_reg[10] > 0) {
		aprint_normal_dev(self, "aperture at 0x%08x\n", vga_reg[10]);
	}
}

int
vga_ofbus_cnattach(bus_space_tag_t iot, bus_space_tag_t memt)
{
	int chosen_phandle;
	int stdout_ihandle, stdout_phandle, ret;
	char buf[128];

	stdout_phandle = 0;

	/*
	 * Find out whether the firmware's chosen stdout is
	 * a display.  If so, use the existing ihandle so the firmware
	 * doesn't become Unhappy.  If not, just open it.
	 */
	if ((chosen_phandle = OF_finddevice("/chosen")) == -1 ||
	    OF_getprop(chosen_phandle, "stdout", &stdout_ihandle, 
	    sizeof(stdout_ihandle)) != sizeof(stdout_ihandle)) {
		return ENXIO;
	}
	stdout_ihandle = of_decode_int((unsigned char *)&stdout_ihandle);
	if ((stdout_phandle = OF_instance_to_package(stdout_ihandle)) == -1 ||
	    OF_getprop(stdout_phandle, "device_type", buf, sizeof(buf)) <= 0) {
		return ENXIO;
	}

	if (strcmp(buf, "display") != 0) {
		/* The display is not stdout device. */
		return ENXIO;
	}
		
	if (OF_call_method("text-mode3", stdout_ihandle, 0, 0) != 0) {
		printf("vga_ofbus_match: text-mode3 method invocation on VGA "
		       "screen device failed\n");
	}

	ret = vga_cnattach(iot, memt, WSDISPLAY_TYPE_ISAVGA, 1);
#if (NIGSFB_OFBUS > 0) || (NVGA_OFBUS > 0)
	if (ret == 0)
		console_ihandle = stdout_ihandle;
#endif

	return ret;
}

static int
vga_ofbus_ioctl(void *cookie, u_long cmd, void *data, int flag, struct lwp *l)
{
	/* we should catch WSDISPLAYIO_SMODE here */
	return 0;
}

static paddr_t
vga_ofbus_mmap(void *cookie, off_t offset, int prot)
{

	/* only the superuser may mmap IO and aperture */
	if (curlwp != NULL) {
		if (kauth_authorize_machdep(kauth_cred_get(),
		    KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL) != 0) {
			return -1;
		}
	}

	/*
	 * XXX
	 * we should really use bus_space_mmap here but for some reason
	 * the ISA tags contain kernel virtual addresses and translating them
	 * back to physical addresses doesn't really work
	 */
	/* access to IO ports */
	if ((offset >= PCI_MAGIC_IO_RANGE) &&
	    (offset < (PCI_MAGIC_IO_RANGE + 0x10000))) {
		paddr_t pa;

		pa = isa_io_physaddr + offset - PCI_MAGIC_IO_RANGE;
		return arm_btop(pa);
	}

	/* legacy VGA aperture */
	if ((offset >= 0xa0000) && (offset < 0xc0000)) {

		return arm_btop(isa_mem_physaddr + offset);
	}

	/* SVGA aperture, we get the address from OpenFirmware */
	if (vga_reg[10] == 0)
		return -1;

	if ((offset >= vga_reg[10]) &&
	    (offset < (vga_reg[10] + vga_reg[11]))) {

		return arm_btop(isa_mem_physaddr + offset);
	}

	return -1;
}
