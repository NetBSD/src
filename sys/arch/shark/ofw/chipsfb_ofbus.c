/*	$NetBSD: chipsfb_ofbus.c,v 1.3.2.1 2014/08/20 00:03:23 tls Exp $ */

/*
 * Copyright (c) 2011 Michael Lorenz
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * C&T 6555x series.
 * ofbus attachment for chipsfb
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: chipsfb_ofbus.c,v 1.3.2.1 2014/08/20 00:03:23 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <uvm/uvm.h>

#include <machine/intr.h>
#include <machine/ofw.h>
#include <machine/pmap.h>

#include <dev/isa/isavar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/ofw/openfirm.h>

#include <dev/ic/ct65550reg.h>
#include <dev/ic/ct65550var.h>
#include <shark/ofw/igsfb_ofbusvar.h>

static int	chipsfb_ofbus_is_console(int);

static int chipsfb_ofbus_console = 0;
static int chipsfb_ofbus_phandle = 0;



static int	chipsfb_ofbus_match(device_t, struct cfdata *, void *);
static void	chipsfb_ofbus_attach(device_t, device_t, void *);
static paddr_t	chipsfb_ofbus_mmap(void *, void *, off_t, int);
int chipsfb_ofbus_cnattach(bus_space_tag_t, bus_space_tag_t);

CFATTACH_DECL_NEW(chipsfb_ofbus, sizeof(struct chipsfb_softc),
    chipsfb_ofbus_match, chipsfb_ofbus_attach, NULL, NULL);
    
static const char * const compat_strings[] = { "CHPS,ct65550", NULL };

vaddr_t chipsfb_mem_vaddr = 0, chipsfb_mmio_vaddr = 0;
paddr_t chipsfb_mem_paddr;
extern paddr_t isa_io_physaddr;
struct bus_space chipsfb_memt, chipsfb_iot;

#if (NCHIPSFB_OFBUS > 0) || (NVGA_OFBUS > 0)
extern int console_ihandle;
#endif

int
chipsfb_ofbus_cnattach(bus_space_tag_t iot, bus_space_tag_t memt)
{
	int chosen_phandle, ct_node;
	int stdout_ihandle, stdout_phandle;
	uint32_t regs[16];

	stdout_phandle = 0;

	/* first find out if there's a ct65550 at all in this machine */
	ct_node = OF_finddevice("/vlbus/display");
	if (ct_node == -1)
		return ENXIO;
	if (of_compatible(ct_node, compat_strings) < 0) 
		return ENXIO;

	/*
	 * now we know there's a CyberPro in this machine so map it into
	 * kernel space, even if it's not the console
	 */
	if (OF_getprop(ct_node, "reg", regs, sizeof(regs)) <= 0)
		return ENXIO;

	chipsfb_mem_paddr = be32toh(regs[10]);
	/* 2MB RAM aperture, bufferable and not cacheable */
	chipsfb_mem_vaddr = ofw_map(chipsfb_mem_paddr, 0x00200000, L2_B);
	/* 128kB MMIO registers */
	chipsfb_mmio_vaddr = ofw_map(chipsfb_mem_paddr + CT_OFF_BITBLT,
	    0x00020000, 0);

	memcpy(&chipsfb_memt, memt, sizeof(struct bus_space));
	chipsfb_memt.bs_cookie = (void *)chipsfb_mem_vaddr;
	memcpy(&chipsfb_iot, iot, sizeof(struct bus_space));

	/*
	 * check if the firmware output device is indeed the ct65550
	 */
	if ((chosen_phandle = OF_finddevice("/chosen")) == -1 ||
	    OF_getprop(chosen_phandle, "stdout", &stdout_ihandle, 
	    sizeof(stdout_ihandle)) != sizeof(stdout_ihandle)) {
		return ENXIO;
	}
	stdout_ihandle = of_decode_int((void *)&stdout_ihandle);
	stdout_phandle = OF_instance_to_package(stdout_ihandle);

	if (stdout_phandle != ct_node)
		return ENXIO;


	chipsfb_ofbus_console = 1;
	chipsfb_ofbus_phandle = stdout_phandle;
#if (NCHIPSFB_OFBUS > 0) || (NVGA_OFBUS > 0)
	console_ihandle = stdout_ihandle;
#endif
	/* we're all set, now let's wait for chipsfb to attach */

	return 0;
}

static int
chipsfb_ofbus_is_console(int phandle)
{

	return chipsfb_ofbus_console && (phandle == chipsfb_ofbus_phandle);
}


static int
chipsfb_ofbus_match(device_t parent, struct cfdata *match, void *aux)
{
	struct ofbus_attach_args *oba = aux;

	if (of_compatible(oba->oba_phandle, compat_strings) < 0)
		return 0;

	return 10;	/* beat vga etc. */
}

static void
chipsfb_ofbus_attach(device_t parent, device_t self, void *aux)
{
	struct chipsfb_softc *sc = device_private(self);
	struct ofbus_attach_args *oba = aux;
	prop_dictionary_t dict;
	int isconsole, width, height, linebytes, depth;
	
	printf(": Chips & Technologies 65550 at 0x%08x\n", 
	    (uint32_t)chipsfb_mem_paddr);

	sc->sc_dev = self;
	sc->sc_memt = &chipsfb_memt;
	sc->sc_iot = &chipsfb_iot;
	sc->sc_fb = chipsfb_mem_paddr;
	sc->sc_fbsize = 0x00800000;	/* 8MB aperture */
	sc->sc_fbh = chipsfb_mem_vaddr;
	sc->sc_mmregh = chipsfb_mmio_vaddr;
	sc->sc_ioregh = isa_io_data_vaddr();
	sc->sc_mmap = chipsfb_ofbus_mmap;
	sc->sc_ioctl = NULL;
	sc->memsize = 0x00200000;

	dict = device_properties(sc->sc_dev);
	if (OF_getprop(oba->oba_phandle, "width", &width, sizeof(width)) == 4) {
		width = be32toh(width);
	} else
		width = 640;
	if (OF_getprop(oba->oba_phandle, "height", &height, sizeof(height))
	    == 4) {
		height = be32toh(height);
	} else
		height = 480;
	if (OF_getprop(oba->oba_phandle, "depth", &depth, sizeof(depth)) == 4) {
		depth = be32toh(depth);
	} else
		depth = 8;
	if (OF_getprop(oba->oba_phandle, "linebytes", &linebytes,
	    sizeof(linebytes)) == 4) {
		linebytes = be32toh(linebytes);
	} else
		linebytes = width * (depth >> 3);
	isconsole = chipsfb_ofbus_is_console(oba->oba_phandle);

	prop_dictionary_set_uint32(dict, "width", width);
	prop_dictionary_set_uint32(dict, "height", height);
	prop_dictionary_set_uint32(dict, "linebytes", linebytes);
	prop_dictionary_set_uint32(dict, "depth", depth);
	prop_dictionary_set_bool(dict, "is_console", isconsole);

	chipsfb_do_attach(sc);
}

static paddr_t
chipsfb_ofbus_mmap(void *v, void *vs, off_t offset, int prot)
{

#ifdef PCI_MAGIC_IO_RANGE
	/* access to IO ports */
	if ((offset >= PCI_MAGIC_IO_RANGE) &&
	    (offset < (PCI_MAGIC_IO_RANGE + 0x10000))) {
		paddr_t pa;

		pa = isa_io_physaddr + offset - PCI_MAGIC_IO_RANGE;
		return arm_btop(pa);
	}
#endif

	if ((offset >= chipsfb_mem_paddr) && 
	    (offset < (chipsfb_mem_paddr + CT_OFF_BITBLT))) {
		return (arm_btop(offset) | ARM32_MMAP_WRITECOMBINE);
	} else if ((offset >= (chipsfb_mem_paddr + CT_OFF_BITBLT)) && 
	    (offset < (chipsfb_mem_paddr + 0x00800000))) {
		return arm_btop(offset);
	} else if ((offset >= 0xa0000) && 
	    (offset < 0xbffff)) {
		return (arm_btop(offset) | ARM32_MMAP_WRITECOMBINE);
	}

	return -1;
}
