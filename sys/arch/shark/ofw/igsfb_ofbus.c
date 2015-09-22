/*	$NetBSD: igsfb_ofbus.c,v 1.15.6.1 2015/09/22 12:05:51 skrll Exp $ */

/*
 * Copyright (c) 2006 Michael Lorenz
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
 * Integraphics Systems IGA 168x and CyberPro series.
 * ofbus attachment for Valeriy E. Ushakov's igsfb driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: igsfb_ofbus.c,v 1.15.6.1 2015/09/22 12:05:51 skrll Exp $");

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

#include <dev/ic/igsfbreg.h>
#include <dev/ic/igsfbvar.h>
#include <shark/ofw/igsfb_ofbusvar.h>

static int	igsfb_ofbus_is_console(int);

static int igsfb_ofbus_console = 0;
static int igsfb_ofbus_phandle = 0;



static int	igsfb_ofbus_match(device_t, cfdata_t, void *);
static void	igsfb_ofbus_attach(device_t, device_t, void *);
static int	igsfb_setup_dc(struct igsfb_devconfig *);
static paddr_t	igsfb_ofbus_mmap(void *, void *, off_t, int);

CFATTACH_DECL_NEW(igsfb_ofbus, sizeof(struct igsfb_softc),
    igsfb_ofbus_match, igsfb_ofbus_attach, NULL, NULL);
    
static const char * const compat_strings[] = { "igs,cyperpro2010", NULL };

vaddr_t igsfb_mem_vaddr = 0, igsfb_mmio_vaddr = 0;
paddr_t igsfb_mem_paddr;
extern paddr_t isa_io_physaddr;
struct bus_space igsfb_memt, igsfb_iot;

#if (NIGSFB_OFBUS > 0) || (NVGA_OFBUS > 0)
extern int console_ihandle;
#endif

int
igsfb_ofbus_cnattach(bus_space_tag_t iot, bus_space_tag_t memt)
{
	struct igsfb_devconfig *dc;
	int ret;
	int chosen_phandle, igs_node;
	int stdout_ihandle, stdout_phandle;
	uint32_t regs[16];
	char mode_buffer[64];

	stdout_phandle = 0;

	/* first find out if there's a CyberPro at all in this machine */
	igs_node = OF_finddevice("/vlbus/display");
	if (igs_node == -1)
		return ENXIO;
	if (of_compatible(igs_node, compat_strings) < 0) 
		return ENXIO;

	/*
	 * now we know there's a CyberPro in this machine so map it into
	 * kernel space, even if it's not the console
	 */
	if (OF_getprop(igs_node, "reg", regs, sizeof(regs)) <= 0)
		return ENXIO;

	igsfb_mem_paddr = be32toh(regs[13]);
	/* 4MB VRAM aperture, bufferable and cacheable */
	igsfb_mem_vaddr = ofw_map(igsfb_mem_paddr, 0x00400000, L2_B);
	/* MMIO registers */
	igsfb_mmio_vaddr = ofw_map(igsfb_mem_paddr + IGS_MEM_MMIO_SELECT,
	    0x00100000, 0);

	memcpy(&igsfb_memt, memt, sizeof(struct bus_space));
	igsfb_memt.bs_cookie = (void *)igsfb_mem_vaddr;
	memcpy(&igsfb_iot, memt, sizeof(struct bus_space));
	igsfb_iot.bs_cookie = (void *)igsfb_mmio_vaddr;

	/*
	 * check if the firmware output device is indeed the CyberPro
	 */
	if ((chosen_phandle = OF_finddevice("/chosen")) == -1 ||
	    OF_getprop(chosen_phandle, "stdout", &stdout_ihandle, 
	    sizeof(stdout_ihandle)) != sizeof(stdout_ihandle)) {
		return ENXIO;
	}
	stdout_ihandle = of_decode_int((void *)&stdout_ihandle);
	stdout_phandle = OF_instance_to_package(stdout_ihandle);

	if (stdout_phandle != igs_node)
		return ENXIO;

	/* ok, now setup and attach the console */
	dc = &igsfb_console_dc;
	ret = igsfb_setup_dc(dc);
	if (ret)
		return ret;

	if (of_get_mode_string(mode_buffer, sizeof(mode_buffer))) {
		strcpy(dc->dc_modestring, mode_buffer);
	}	

	ret = igsfb_cnattach_subr(dc);
	if (ret)
		return ret;

	igsfb_ofbus_console = 1;
	igsfb_ofbus_phandle = stdout_phandle;
#if (NIGSFB_OFBUS > 0) || (NVGA_OFBUS > 0)
	console_ihandle = stdout_ihandle;
#endif
	return 0;
}

static int
igsfb_setup_dc(struct igsfb_devconfig *dc)
{
	int ret;

	dc->dc_id = 0x2010;
	dc->dc_memt = &igsfb_memt;
	dc->dc_memaddr = 0;
	dc->dc_memsz= 0x00400000;
	dc->dc_memflags = 0;

	dc->dc_iot = &igsfb_iot;
	dc->dc_iobase = 0;
	dc->dc_ioflags = 0;
	dc->dc_mmap = igsfb_ofbus_mmap;
	if (bus_space_map(dc->dc_iot,
			  dc->dc_iobase + IGS_REG_BASE, IGS_REG_SIZE,
			  dc->dc_ioflags,
			  &dc->dc_ioh) != 0)
	{
		printf("unable to map I/O registers\n");
		return 1;
	}
	ret = igsfb_enable(dc->dc_iot, dc->dc_iobase, dc->dc_ioflags);
	if (ret)
		return ret;
	return 0;
}

static int
igsfb_ofbus_is_console(int phandle)
{

	return igsfb_ofbus_console && (phandle == igsfb_ofbus_phandle);
}


static int
igsfb_ofbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct ofbus_attach_args *oba = aux;

	if (of_compatible(oba->oba_phandle, compat_strings) < 0)
		return 0;

	return 10;	/* beat vga etc. */
}

static void
igsfb_ofbus_attach(device_t parent, device_t self, void *aux)
{
	struct igsfb_softc *sc = device_private(self);
	struct ofbus_attach_args *oba = aux;
	uint32_t regs[16];
	int isconsole, ret;
	
	sc->sc_dev = self;
	if (igsfb_ofbus_is_console(oba->oba_phandle)) {
		isconsole = 1;
		sc->sc_dc = &igsfb_console_dc;
	} else {
		isconsole = 0;
		sc->sc_dc = malloc(sizeof(struct igsfb_devconfig),
				   M_DEVBUF, M_NOWAIT | M_ZERO);
		if (sc->sc_dc == NULL)
			panic("unable to allocate igsfb_devconfig");
		if (OF_getprop(oba->oba_phandle, "reg",
			       regs, sizeof(regs)) <= 0)
		{
			printf(": unable to read 'reg' property\n");
			return;
		}
		ret = igsfb_setup_dc(sc->sc_dc);
		if (ret)
			return;
	}
	printf(": IGS CyberPro 2010 at 0x%08x\n", (uint32_t)igsfb_mem_paddr);

	igsfb_attach_subr(sc, isconsole);
}

static paddr_t
igsfb_ofbus_mmap(void *v, void *vs, off_t offset, int prot)
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
	/*
	 * we also need to allow mapping of the whole aperture, including MMIO 	
	 * registers on CyberPro at its physical address
	 */
	if ((offset >= igsfb_mem_paddr) && 
	    (offset < (igsfb_mem_paddr + 0x00800000))) {
		return (arm_btop(offset) | ARM32_MMAP_WRITECOMBINE);
	}
	if ((offset >= (igsfb_mem_paddr + 0x00800000)) && 
	    (offset < (igsfb_mem_paddr + 0x01000000)))
		return arm_btop(offset);

	return -1;
}
