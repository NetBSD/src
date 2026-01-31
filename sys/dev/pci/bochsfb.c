/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
 * Generic driver for Bochs Display Interface (DISPI) based devices:
 * - Bochs VBE/VGA interface
 * - QEMU standard VGA
 * - QEMU virtio-vga series GPU
 *
 * This driver supports both MMIO and I/O port access methods.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bochsfb.c,v 1.1 2026/01/31 12:12:58 nia Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/endian.h>
#include <sys/kauth.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>

#include <dev/pci/bochsfbreg.h>
#include <dev/pci/bochsfbvar.h>

#include <dev/videomode/videomode.h>
#include <dev/pci/wsdisplay_pci.h>

#include "opt_wsemul.h"

static int	bochsfb_match(device_t, cfdata_t, void *);
static void	bochsfb_attach(device_t, device_t, void *);

static void	bochsfb_write_dispi(struct bochsfb_softc *sc, uint16_t reg,
		uint16_t val);
static uint16_t	bochsfb_read_dispi(struct bochsfb_softc *sc, uint16_t reg);
static void	bochsfb_write_vga(struct bochsfb_softc *sc, uint16_t reg,
		uint8_t val);
static uint8_t	bochsfb_read_vga(struct bochsfb_softc *sc, uint16_t reg);
static void 	bochsfb_set_blanking(struct bochsfb_softc *sc, int blank);

static bool	bochsfb_identify(struct bochsfb_softc *sc);
static int      bochsfb_edid_mode(struct bochsfb_softc *sc);
static bool	bochsfb_set_videomode(struct bochsfb_softc *sc);

static paddr_t	bochsfb_mmap(void *v, void *vs, off_t offset, int prot);
static int	bochsfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
		struct lwp *l);
static void	bochsfb_identify_screen(void *cookie, struct vcons_screen *scr,
		int existing, long *defattr);

CFATTACH_DECL_NEW(bochsfb, sizeof(struct bochsfb_softc),
bochsfb_match, bochsfb_attach, NULL, NULL);

struct wsdisplay_accessops bochsfb_accessops = {
	bochsfb_ioctl,
	bochsfb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};

static int
bochsfb_match(device_t parent, cfdata_t match, void *aux)
{
	const struct pci_attach_args *pa = (const struct pci_attach_args *)aux;

	/* This is a unauthorized PCI ID */
	if ((PCI_VENDOR(pa->pa_id) == 0x1234) &&
	    (PCI_PRODUCT(pa->pa_id) == 0x1111))
		return 100;

	return 0;
}

static void
bochsfb_attach(device_t parent, device_t self, void *aux)
{
	struct bochsfb_softc *sc = device_private(self);
	prop_dictionary_t dict = device_properties(self);
	struct wsemuldisplaydev_attach_args ws_aa;
	struct rasops_info *ri;
	const struct pci_attach_args *pa = aux;
	pcireg_t screg;
	bool is_console = false;
	long defattr;

	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_dev = self;
	sc->sc_pci_id = pa->pa_id;

	pci_aprint_devinfo(pa, NULL);
	prop_dictionary_get_bool(dict, "is_console", &is_console);

	/*
	 * Map VGA I/O and memory space.
	 * First try to map framebuffer memory
	 */
	if (pci_mapreg_map(pa, PCI_MAPREG_START, PCI_MAPREG_TYPE_MEM,
			   BUS_SPACE_MAP_LINEAR, &sc->sc_memt,
			   &sc->sc_fb_handle, &sc->sc_fb_addr,
			   &sc->sc_fb_size) != 0) {
		aprint_error_dev(sc->sc_dev, "failed to map framebuffer memory\n");
		return;
	}

	/* Try to map MMIO region for the DISPI interface */
	if (pci_mapreg_map(pa, PCI_MAPREG_START + 8, PCI_MAPREG_TYPE_MEM,
			0, &sc->sc_mmiot, &sc->sc_mmioh, &sc->sc_mmio_addr,
			&sc->sc_mmio_size) != 0) {

		aprint_normal_dev(sc->sc_dev, "MMIO BAR not available, using I/O ports\n");
		sc->sc_has_mmio = false;

		/* I/O ports only exist if it's a VGA device*/
		if (!(PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY &&
	     	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_DISPLAY_VGA)) {
			aprint_error_dev(sc->sc_dev,
				"DISPI I/O port not available\n");
			return;
		}

		/* Map DISPI I/O ports as fallback */
		if (bus_space_map(pa->pa_iot, VBE_DISPI_IOPORT_INDEX,
			4, 0, &sc->sc_ioh_dispi) != 0) {
			aprint_error_dev(sc->sc_dev,
				"couldn't map DISPI I/O ports\n");
			return;
		}

		/* Map I/O space for VGA and Bochs DISPI interface */
		if (bus_space_map(pa->pa_iot, VGA_IO_START, VGA_IO_SIZE, 0,
			&sc->sc_ioh_vga) != 0) {
			aprint_error_dev(sc->sc_dev, "couldn't map VGA I/O space\n");
			return;
		}
		sc->sc_iot = pa->pa_iot;
	} else {
		aprint_normal_dev(sc->sc_dev, "using MMIO for DISPI interface\n");
		sc->sc_has_mmio = true;
	}

	/* Enable memory and I/O space */
	screg = pci_conf_read(sc->sc_pc, sc->sc_pcitag,
			      PCI_COMMAND_STATUS_REG);
	screg |= PCI_COMMAND_MEM_ENABLE;
	/* Avoid mess with legacy IO on secondary display */
	if (!sc->sc_has_mmio)
		screg |= PCI_COMMAND_IO_ENABLE;
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PCI_COMMAND_STATUS_REG, screg);


	aprint_normal_dev(sc->sc_dev, "framebuffer at 0x%08lx, size %ld MB\n",
			(long)sc->sc_fb_addr, (long)sc->sc_fb_size / (1024 * 1024));

	/* Initialize the display */
	if (!bochsfb_identify(sc)) {
		aprint_error_dev(sc->sc_dev, "initialization failed\n");
		return;
	}

	if (bochsfb_edid_mode(sc)) {
		/* No EDID data, use default resolution */
		sc->sc_width = BOCHSFB_DEFAULT_WIDTH;
		sc->sc_height = BOCHSFB_DEFAULT_HEIGHT;
	}

	sc->sc_bpp = 32; /* 32 bbp */
	sc->sc_linebytes = sc->sc_width * (sc->sc_bpp / 8);

	aprint_normal_dev(sc->sc_dev, "setting %dx%d %d bpp resolution\n",
				sc->sc_width, sc->sc_height, sc->sc_bpp);

	if (!bochsfb_set_videomode(sc)) {
		aprint_error_dev(sc->sc_dev, "couldn't set video mode\n");
		return;
	}
	bochsfb_set_blanking(sc, WSDISPLAYIO_VIDEO_ON);

	sc->sc_defaultscreen_descr = (struct wsscreen_descr){
		"default",
		0, 0,
		NULL,
		8, 16,
		WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
		NULL
	};
	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
		  &bochsfb_accessops);
	sc->vd.init_screen = bochsfb_identify_screen;

	ri = &sc->sc_console_screen.scr_ri;

	if (is_console) {
		vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
				  &defattr);

		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
		vcons_redraw_screen(&sc->sc_console_screen);

		sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
		sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
		sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
		sc->sc_defaultscreen_descr.ncols = ri->ri_cols;

		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
				   defattr);
		vcons_replay_msgbuf(&sc->sc_console_screen);
	} else {
		if (sc->sc_console_screen.scr_ri.ri_rows == 0) {
			vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
					  &defattr);
		} else
			(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	}

	ws_aa.console = is_console;
	ws_aa.scrdata = &sc->sc_screenlist;
	ws_aa.accessops = &bochsfb_accessops;
	ws_aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &ws_aa, wsemuldisplaydevprint, CFARGS_NONE);
}

static void
bochsfb_identify_screen(void *cookie, struct vcons_screen *scr, int existing,
		    long *defattr)
{
	struct bochsfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	wsfont_init();

	ri->ri_depth = sc->sc_bpp;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_linebytes;
	ri->ri_flg = RI_CENTER;

	ri->ri_bits = bus_space_vaddr(sc->sc_memt, sc->sc_fb_handle);

	ri->ri_rnum = 8;
	ri->ri_gnum = 8;
	ri->ri_bnum = 8;
	ri->ri_rpos = 16;
	ri->ri_gpos = 8;
	ri->ri_bpos = 0;

	scr->scr_flags |= VCONS_DONT_READ;

	rasops_init(ri,
		    ri->ri_height / 8,
		    ri->ri_width / 8);

	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
			ri->ri_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
}

/*
 * Write to the VBE DISPI interface
 */
static void
bochsfb_write_dispi(struct bochsfb_softc *sc, uint16_t reg, uint16_t val)
{
	if (sc->sc_has_mmio) {
		/* Use memory mapped I/O */
		bus_space_write_2(sc->sc_mmiot, sc->sc_mmioh,
				  BOCHSFB_MMIO_DISPI_OFFSET + reg * 2, val);
	} else {
		/* Use I/O ports */
		bus_space_write_2(sc->sc_iot, sc->sc_ioh_dispi,
			VBE_DISPI_IOPORT_INDEX - VBE_DISPI_IOPORT_INDEX, reg);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh_dispi,
			VBE_DISPI_IOPORT_DATA - VBE_DISPI_IOPORT_INDEX, val);
	}
}

/*
* Read from the VBE DISPI interface
*/
static uint16_t
bochsfb_read_dispi(struct bochsfb_softc *sc, uint16_t reg)
{
	if (sc->sc_has_mmio) {
		/* Use memory mapped I/O */
		return bus_space_read_2(sc->sc_mmiot, sc->sc_mmioh,
					BOCHSFB_MMIO_DISPI_OFFSET + reg * 2);
	} else {
		/* Use I/O ports */
		bus_space_write_2(sc->sc_iot, sc->sc_ioh_dispi,
				  VBE_DISPI_IOPORT_INDEX - VBE_DISPI_IOPORT_INDEX, reg);
		return bus_space_read_2(sc->sc_iot, sc->sc_ioh_dispi,
					VBE_DISPI_IOPORT_DATA - VBE_DISPI_IOPORT_INDEX);
	}
}

/*
* Write to the VGA IO Ports
*/
static void
bochsfb_write_vga(struct bochsfb_softc *sc, uint16_t reg, uint8_t val)
{
	if (sc->sc_has_mmio) {
		/* Use memory mapped I/O */
		bus_space_write_1(sc->sc_mmiot, sc->sc_mmioh,
				  reg - VGA_IO_START + BOCHSFB_MMIO_VGA_OFFSET,
				  val);
		return;
	}

	bus_space_write_1(sc->sc_iot, sc->sc_ioh_vga, reg, val);
}

/*
* Read from the VGA IO Ports
*/
static uint8_t
bochsfb_read_vga(struct bochsfb_softc *sc, uint16_t reg)
{
	if (sc->sc_has_mmio) {
		/* Use memory mapped I/O */
		return bus_space_read_1(sc->sc_mmiot, sc->sc_mmioh,
					reg - VGA_IO_START + BOCHSFB_MMIO_VGA_OFFSET);
	}

	return bus_space_read_1(sc->sc_iot, sc->sc_ioh_vga, reg);
}

/*
* Identify the Bochs/QEMU display
*/
static bool
bochsfb_identify(struct bochsfb_softc *sc)
{
	/* Check for the Bochs display ID */
	sc->sc_id = bochsfb_read_dispi(sc, VBE_DISPI_INDEX_ID);

	if ((sc->sc_id & 0xFFF0) != VBE_DISPI_ID0) {
		aprint_error_dev(sc->sc_dev,
				 "invalid display ID 0x%04x\n", sc->sc_id);
		return false;
	}

	aprint_normal_dev(sc->sc_dev, "Bochs display ID 0x%04x found\n",
			  sc->sc_id);

	return true;
}

static int bochsfb_edid_mode(struct bochsfb_softc *sc)
{
	int ret;

	if (!sc->sc_has_mmio)
		return -1;

	/* VirtIO VGA is not coming with EDID support */
	if (PCI_VENDOR(sc->sc_pci_id) == PCI_VENDOR_QUMRANET)
		return -1;

	/* Read EDID data */
	bus_space_read_region_1(sc->sc_mmiot, sc->sc_mmioh,
				BOCHSFB_MMIO_EDID_OFFSET,
				sc->edid_buf,
				BOCHSFB_MMIO_EDID_SIZE);

	/* Parse EDID data */
	ret = edid_parse(sc->edid_buf, &sc->sc_ei);

	if (ret != 0) {
		aprint_normal_dev(sc->sc_dev,
			"failed to parse EDID data\n");
		return ret;
	}

	/* Get the preferred mode */
	if (!sc->sc_ei.edid_preferred_mode) {
		aprint_normal_dev(sc->sc_dev,
			"no preferred mode found in EDID data\n");
		return -1;
	}

	/* Set the preferred mode */
	sc->sc_width = sc->sc_ei.edid_preferred_mode->hdisplay;
	sc->sc_height = sc->sc_ei.edid_preferred_mode->vdisplay;

	return 0;
}

/*
* Set video mode using the Bochs interface
*/
static bool
bochsfb_set_videomode(struct bochsfb_softc *sc)
{
	bochsfb_write_dispi(sc, VBE_DISPI_INDEX_ENABLE, 0);

	/* Set resolution and bit depth */
	bochsfb_write_dispi(sc, VBE_DISPI_INDEX_BPP, sc->sc_bpp);
	bochsfb_write_dispi(sc, VBE_DISPI_INDEX_XRES, sc->sc_width);
	bochsfb_write_dispi(sc, VBE_DISPI_INDEX_YRES, sc->sc_height);
	bochsfb_write_dispi(sc, VBE_DISPI_INDEX_BANK, 0);
	bochsfb_write_dispi(sc, VBE_DISPI_INDEX_VIRT_WIDTH, sc->sc_width);
	bochsfb_write_dispi(sc, VBE_DISPI_INDEX_VIRT_HEIGHT, sc->sc_height);
	bochsfb_write_dispi(sc, VBE_DISPI_INDEX_X_OFFSET, 0);
	bochsfb_write_dispi(sc, VBE_DISPI_INDEX_Y_OFFSET, 0);

	/* Re-enable with linear frame buffer */
	bochsfb_write_dispi(sc, VBE_DISPI_INDEX_ENABLE,
			    	VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

	return true;
}

static void
bochsfb_set_blanking(struct bochsfb_softc *sc, int blank)
{
	bochsfb_write_vga(sc, 0x3C2, 0x01);
	(void)bochsfb_read_vga(sc, 0x3DA);

	if (blank == WSDISPLAYIO_VIDEO_OFF) {
		bochsfb_write_vga(sc, 0x3C0, 0x00);
	} else {
		bochsfb_write_vga(sc, 0x3C0, 0x20);
	}
	sc->sc_blank = blank;
}

static int
bochsfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct vcons_data *vd;
	struct bochsfb_softc *sc;
	struct wsdisplay_fbinfo *wsfbi;
	struct vcons_screen *ms;

	vd = v;
	sc = vd->cookie;
	ms = vd->active;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
		return 0;

	case PCI_IOC_CFGREAD:
	case PCI_IOC_CFGWRITE:
		return pci_devioctl(sc->sc_pc, sc->sc_pcitag,
				    cmd, data, flag, l);

	case WSDISPLAYIO_GET_BUSID:
		return wsdisplayio_busid_pci(sc->sc_dev, sc->sc_pc,
					     sc->sc_pcitag, data);

	case WSDISPLAYIO_GINFO:
		if (ms == NULL)
			return ENODEV;

		wsfbi = (void *)data;
		wsfbi->height = ms->scr_ri.ri_height;
		wsfbi->width = ms->scr_ri.ri_width;
		wsfbi->depth = ms->scr_ri.ri_depth;
		wsfbi->cmsize = 0; /* No color map */
		return 0;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_linebytes;
		return 0;
	
	case WSDISPLAYIO_SMODE:
		{
			int new_mode = *(int *)data;
			if (new_mode != sc->sc_mode) {
				sc->sc_mode = new_mode;
				if (new_mode == WSDISPLAYIO_MODE_EMUL) {
					vcons_redraw_screen(ms);
				}
			}
			return 0;
		}
	case WSDISPLAYIO_GET_FBINFO:
		{
			struct wsdisplayio_fbinfo *fbi = data;
			struct rasops_info *ri;
			int ret;

			ri = &sc->vd.active->scr_ri;
			ret = wsdisplayio_get_fbinfo(ri, fbi);
			return ret;
		}
	case WSDISPLAYIO_GVIDEO:
		*(int *)data = sc->sc_blank;
		return 0;
	case WSDISPLAYIO_SVIDEO:
		bochsfb_set_blanking(sc, *(int *)data);
		return 0;
	case WSDISPLAYIO_GET_EDID:
		{
			struct wsdisplayio_edid_info *d = data;
			return wsdisplayio_get_edid(sc->sc_dev, d);
		}
	}


	return EPASSTHROUGH;
}

static paddr_t
bochsfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd;
	struct bochsfb_softc *sc;
	paddr_t pa;

	vd = v;
	sc = vd->cookie;

	if (sc->sc_mode == WSDISPLAYIO_MODE_DUMBFB) {
		if (offset < sc->sc_fb_size) {
			pa = bus_space_mmap(sc->sc_memt, sc->sc_fb_addr + offset, 0,
					prot, BUS_SPACE_MAP_LINEAR);
			return pa;
		}
	} else if (sc->sc_mode == WSDISPLAYIO_MODE_MAPPED) {
		if (kauth_authorize_machdep(kauth_cred_get(),
		    KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL) != 0) {
			aprint_error_dev(sc->sc_dev, "mmap() rejected.\n");
			return -1;
		}

		if ((offset >= sc->sc_fb_addr) &&
		    (offset < sc->sc_fb_addr + sc->sc_fb_size)) {
			pa = bus_space_mmap(sc->sc_memt, offset, 0, prot,
					    BUS_SPACE_MAP_LINEAR);
			return pa;
		}

		if (sc->sc_has_mmio &&
		    (offset >= sc->sc_mmio_addr) &&
		    (offset < sc->sc_mmio_addr + sc->sc_mmio_size)) {
			pa = bus_space_mmap(sc->sc_mmiot, offset, 0, prot,
					    BUS_SPACE_MAP_LINEAR);
			return pa;
		}

#ifdef PCI_MAGIC_IO_RANGE
		/* allow mapping of IO space */
		if ((offset >= PCI_MAGIC_IO_RANGE) &&
		    (offset < PCI_MAGIC_IO_RANGE + 0x10000)) {
			pa = bus_space_mmap(sc->sc_iot,
			    offset - PCI_MAGIC_IO_RANGE, 0, prot, 0);
			return pa;
		}
#endif
	}

	return -1;
}
