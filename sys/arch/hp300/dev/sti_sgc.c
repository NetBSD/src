/*	$NetBSD: sti_sgc.c,v 1.3 2020/05/04 06:52:53 tsutsui Exp $	*/
/*	$OpenBSD: sti_sgc.c,v 1.14 2007/05/26 00:36:03 krw Exp $	*/

/*
 * Copyright (c) 2005, Miodrag Vallat
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sti_sgc.c,v 1.3 2020/05/04 06:52:53 tsutsui Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

#include <dev/ic/stireg.h>
#include <dev/ic/stivar.h>

#include <hp300/dev/sgcvar.h>
#include <hp300/dev/sti_sgcvar.h>
#include <machine/autoconf.h>

struct sti_sgc_softc {
	struct sti_softc sc_sti;

	paddr_t sc_bitmap;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_ramdach;
};

/*
 * 425e EVRX specific hardware
 */
#define STI_EVRX_RAMDACOFFSET	0x060000
#define STI_EVRX_RAMDACSIZE	0x000800
#define STI_EVRX_FBOFFSET	0x200000

#define EVRX_BT458_ADDR		(0x200 + 2)
#define EVRX_BT458_CMAP		(0x204 + 2)
#define EVRX_BT458_CTRL		(0x208 + 2)
#define EVRX_BT458_OMAP		(0x20C + 2)

/* from HP-UX /usr/lib/libddevrx.a */
#define EVRX_MAGIC00		0x600
#define EVRX_MAGIC04		0x604
#define EVRX_MAGIC08		0x608
#define EVRX_MAGIC0C		0x60c
#define EVRX_MAGIC10		0x610
#define EVRX_MAGIC10_BSY	0x00010000
#define EVRX_MAGIC18		0x618
#define EVRX_MAGIC1C		0x61c

static int sticonslot = -1;
static struct sti_rom sticn_rom;
static struct sti_screen sticn_scr;
static bus_addr_t sticn_bases[STI_REGION_MAX];

static int sti_sgc_match(device_t, struct cfdata *, void *);
static void sti_sgc_attach(device_t, device_t, void *);

static int sti_sgc_probe(bus_space_tag_t, int);

CFATTACH_DECL_NEW(sti_sgc, sizeof(struct sti_sgc_softc),
    sti_sgc_match, sti_sgc_attach, NULL, NULL);

/* 425e EVRX specific access functions */
static int sti_evrx_setcmap(struct sti_sgc_softc *, struct wsdisplay_cmap *);
static void sti_evrx_resetramdac(struct sti_sgc_softc *);
static void sti_evrx_resetcmap(struct sti_sgc_softc *);
static int sti_evrx_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t sti_evrx_mmap(void *, void *, off_t, int);

static const struct wsdisplay_accessops sti_evrx_accessops = {
	sti_evrx_ioctl,
	sti_evrx_mmap,
	sti_alloc_screen,
	sti_free_screen,
	sti_show_screen,
	sti_load_font
};

static int
sti_sgc_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct sgc_attach_args *saa = aux;

	/*
	 * If we already probed it successfully as a console device, go ahead,
	 * since we will not be able to bus_space_map() again.
	 */
	if (saa->saa_slot == sticonslot)
		return 1;

	return sti_sgc_probe(saa->saa_iot, saa->saa_slot);
}

static void
sti_sgc_attach(device_t parent, device_t self, void *aux)
{
	struct sti_sgc_softc *sc = device_private(self);
	struct sti_softc *ssc = &sc->sc_sti;
	struct sgc_attach_args *saa = aux;
	struct sti_screen *scr;
	bus_space_handle_t romh;
	bus_addr_t base;
	struct wsemuldisplaydev_attach_args waa;
	u_int romend;
	struct sti_dd *rom_dd;
	uint32_t grid0;
	int i;

	ssc->sc_dev = self;
	base = (bus_addr_t)sgc_slottopa(saa->saa_slot);

	if (saa->saa_slot == sticonslot) {
		ssc->sc_flags |= STI_CONSOLE | STI_ATTACHED;
		ssc->sc_rom = &sticn_rom;
		ssc->sc_rom->rom_softc = ssc;
		ssc->sc_scr = &sticn_scr;
		ssc->sc_scr->scr_rom = ssc->sc_rom;
		memcpy(ssc->bases, sticn_bases, sizeof(ssc->bases));

		sti_describe(ssc);
	} else {
		if (bus_space_map(saa->saa_iot, base, PAGE_SIZE, 0, &romh)) {
			aprint_error(": can't map ROM");
			return;
		}
		/*
		 * Compute real PROM size
		 */
		romend = sti_rom_size(saa->saa_iot, romh);

		bus_space_unmap(saa->saa_iot, romh, PAGE_SIZE);

		if (bus_space_map(saa->saa_iot, base, romend, 0, &romh)) {
			aprint_error(": can't map frame buffer");
			return;
		}

		ssc->bases[0] = romh;
		for (i = 0; i < STI_REGION_MAX; i++)
			ssc->bases[i] = base;

		if (sti_attach_common(ssc, saa->saa_iot, saa->saa_iot, romh,
		    STI_CODEBASE_ALT) != 0)
			return;
	}

	/* Identify the board model by dd_grid */
	rom_dd = &ssc->sc_rom->rom_dd;
	grid0 = rom_dd->dd_grid[0];
	scr = ssc->sc_scr;

	switch (grid0) {
	case STI_DD_EVRX:
		/*
		 * 425e on-board EVRX framebuffer.
		 * bitmap memory can be accessed at offset +0x200000.
		 */
		sc->sc_bitmap = base + STI_EVRX_FBOFFSET;

		/*
		 * Bt458 RAMDAC can be accessed at offset +0x60200 and
		 * unknown control registers are around +0x60600.
		 */
		sc->sc_bst = saa->saa_iot;
		if (bus_space_map(sc->sc_bst, base + STI_EVRX_RAMDACOFFSET,
		    STI_EVRX_RAMDACSIZE, 0, &sc->sc_ramdach)) {
			aprint_error_dev(self, "can't map RAMDAC\n");
			return;
		}

		aprint_normal_dev(self, "Enable mmap support\n");

		/*
		 * initialize Bt458 RAMDAC and preserve initial color map
		 */
		sti_evrx_resetramdac(sc);
		sti_evrx_resetcmap(sc);

		scr->scr_wsmode = WSDISPLAYIO_MODE_EMUL;
		waa.console = ssc->sc_flags & STI_CONSOLE ? 1 : 0;
		waa.scrdata = &scr->scr_screenlist;
		waa.accessops = &sti_evrx_accessops;
		waa.accesscookie = scr;

		config_found(ssc->sc_dev, &waa, wsemuldisplaydevprint);
		break;

	case STI_DD_CRX:
		/*
		 * HP A1659A CRX on some 425t variants.
		 * Not investigated yet; needs to check HP-UX libddgcrx.a etc.
		 */
		/* FALLTHROUGH */
	default:
		/*
		 * Unsupported variants.
		 * Use default common sti(4) attachment (no bitmap support).
		 */
		sti_end_attach(ssc);
		break;
	}
}

static int
sti_sgc_probe(bus_space_tag_t iot, int slot)
{
	bus_space_handle_t ioh;
	int devtype;

	if (bus_space_map(iot, (bus_addr_t)sgc_slottopa(slot),
	    PAGE_SIZE, 0, &ioh))
		return 0;

	devtype = bus_space_read_1(iot, ioh, 3);

	bus_space_unmap(iot, ioh, PAGE_SIZE);

	/*
	 * This might not be reliable enough. On the other hand, non-STI
	 * SGC cards will apparently not initialize in an hp300, to the
	 * point of not even answering bus probes (checked with an
	 * Harmony/FDDI SGC card).
	 */
	if (devtype != STI_DEVTYPE1 && devtype != STI_DEVTYPE4)
		return 0;

	return 1;
}

static int
sti_evrx_setcmap(struct sti_sgc_softc *sc, struct wsdisplay_cmap *p)
{
	struct sti_softc *ssc = &sc->sc_sti;
	struct sti_screen *scr = ssc->sc_scr;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_ramdach;
	uint8_t r[STI_NCMAP], g[STI_NCMAP], b[STI_NCMAP];
	u_int index, count;
	int i, error;

	index = p->index;
	count = p->count;
	if (index >= STI_NCMAP || count > STI_NCMAP - index)
		return EINVAL;

	error = copyin(p->red, &r[index], count);
	if (error)
		return error;
	error = copyin(p->green, &g[index], count);
	if (error)
		return error;
	error = copyin(p->blue, &b[index], count);
	if (error)
		return error;

	memcpy(&scr->scr_rcmap[index], &r[index], count);
	memcpy(&scr->scr_gcmap[index], &g[index], count);
	memcpy(&scr->scr_bcmap[index], &b[index], count);

	/* magic setup from HP-UX */
	bus_space_write_4(bst, bsh, EVRX_MAGIC08, 0x00000001);
	bus_space_write_4(bst, bsh, EVRX_MAGIC00, 0x00000001);
	for (i = index; i < index + count; i++) {
		/* this is what HP-UX woodDownloadCmap() does */
		while ((bus_space_read_4(bst, bsh, EVRX_MAGIC10) &
		    EVRX_MAGIC10_BSY) != 0)
			continue;
		bus_space_write_1(bst, bsh, EVRX_BT458_ADDR, i);
		bus_space_write_1(bst, bsh, EVRX_BT458_CMAP, scr->scr_rcmap[i]);
		bus_space_write_1(bst, bsh, EVRX_BT458_CMAP, scr->scr_gcmap[i]);
		bus_space_write_4(bst, bsh, EVRX_MAGIC10,   scr->scr_bcmap[i]);
	}
	return 0;
}

static void
sti_evrx_resetramdac(struct sti_sgc_softc *sc)
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_ramdach;
#if 0
	int i;
#endif

	/*
	 * Initialize the Bt458.  When we write to control registers,
	 * the address is not incremented automatically. So we specify
	 * it ourselves for each control register.
	 */

	/* all planes will be read */
	bus_space_write_1(bst, bsh, EVRX_BT458_ADDR, 0x04);
	bus_space_write_1(bst, bsh, EVRX_BT458_CTRL, 0xff);

	/* all planes have non-blink */
	bus_space_write_1(bst, bsh, EVRX_BT458_ADDR, 0x05);
	bus_space_write_1(bst, bsh, EVRX_BT458_CTRL, 0x00);

	/* pallete enabled, ovly plane disabled */
	bus_space_write_1(bst, bsh, EVRX_BT458_ADDR, 0x06);
	bus_space_write_1(bst, bsh, EVRX_BT458_CTRL, 0x40);

	/* no test mode */
	bus_space_write_1(bst, bsh, EVRX_BT458_ADDR, 0x07);
	bus_space_write_1(bst, bsh, EVRX_BT458_CTRL, 0x00);

	/* magic initialization from HP-UX woodInitializeHardware() */
	bus_space_write_4(bst, bsh, EVRX_MAGIC00, 0x00000001);
	bus_space_write_4(bst, bsh, EVRX_MAGIC04, 0x00000001);
	bus_space_write_4(bst, bsh, EVRX_MAGIC08, 0x00000001);
	bus_space_write_4(bst, bsh, EVRX_MAGIC0C, 0x00000001);
	bus_space_write_4(bst, bsh, EVRX_MAGIC18, 0xFFFFFFFF);
	bus_space_write_4(bst, bsh, EVRX_MAGIC1C, 0x00000000);

#if 0
	bus_space_write_1(bst, bsh, EVRX_BT458_ADDR, 0x00);
	for (i = 0; i < 4; i++) {
		bus_space_write_1(bst, bsh, EVRX_BT458_OMAP, 0x00);
		bus_space_write_1(bst, bsh, EVRX_BT458_OMAP, 0x00);
		bus_space_write_1(bst, bsh, EVRX_BT458_OMAP, 0x00);
	}
#endif
}

static void
sti_evrx_resetcmap(struct sti_sgc_softc *sc)
{
	struct sti_softc *ssc = &sc->sc_sti;
	struct sti_screen *scr = ssc->sc_scr;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_ramdach;
	int i;

	/* magic setup from HP-UX */
	bus_space_write_4(bst, bsh, EVRX_MAGIC08, 0x00000001);
	bus_space_write_4(bst, bsh, EVRX_MAGIC00, 0x00000001);

	/* preserve palette values initialized by STI firmware */
	for (i = 0; i < STI_NCMAP; i++) {
		/* this is what HP-UX woodUploadCmap() does */
		while ((bus_space_read_4(bst, bsh, EVRX_MAGIC10) &
		    EVRX_MAGIC10_BSY) != 0)
			continue;
		bus_space_write_1(bst, bsh, EVRX_BT458_ADDR, i);
		scr->scr_rcmap[i] = bus_space_read_1(bst, bsh, EVRX_BT458_CMAP);
		scr->scr_gcmap[i] = bus_space_read_1(bst, bsh, EVRX_BT458_CMAP);
		scr->scr_bcmap[i] = bus_space_read_1(bst, bsh, EVRX_BT458_CMAP);
	}
}

static int
sti_evrx_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct sti_screen *scr = (struct sti_screen *)v;
	struct sti_rom *rom = scr->scr_rom;
	struct sti_softc *ssc = rom->rom_softc;
	struct sti_sgc_softc *sc = device_private(ssc->sc_dev);
	struct wsdisplay_fbinfo *wdf;
	struct wsdisplay_cmap *cmapp;
	u_int idx, count;
	int error, new_mode;

	switch (cmd) {
	case WSDISPLAYIO_GINFO:
		wdf = (struct wsdisplay_fbinfo *)data;
		wdf->height = scr->scr_cfg.scr_height;
		wdf->width  = scr->scr_cfg.scr_width;
		wdf->depth  = scr->scr_bpp;
		wdf->cmsize = STI_NCMAP;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		cmapp = (struct wsdisplay_cmap *)data;
		idx = cmapp->index;
		count = cmapp->count;
		if (idx >= STI_NCMAP || count > STI_NCMAP - idx)
			return EINVAL;
		error = copyout(&scr->scr_rcmap[idx], cmapp->red, count);
		if (error != 0)
			return error;
		error = copyout(&scr->scr_gcmap[idx], cmapp->green, count);
		if (error != 0)
			return error;
		error = copyout(&scr->scr_bcmap[idx], cmapp->blue, count);
		if (error != 0)
			return error;
		return 0;
		
	case WSDISPLAYIO_PUTCMAP:
		cmapp = (struct wsdisplay_cmap *)data;
		return sti_evrx_setcmap(sc, cmapp);

	case WSDISPLAYIO_SMODE:
		new_mode = *(int *)data;
		if (new_mode != scr->scr_wsmode) {
			scr->scr_wsmode = new_mode;
			if (new_mode == WSDISPLAYIO_MODE_EMUL) {
				sti_init(scr, STI_TEXTMODE);
			} else if (new_mode == WSDISPLAYIO_MODE_DUMBFB) {
				sti_init(scr, 0);
				sti_evrx_resetramdac(sc);
			}
		}
		return 0;

	}
	return sti_ioctl(v, vs, cmd, data, flag, l);
}

static paddr_t
sti_evrx_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct sti_screen *scr = (struct sti_screen *)v;
	struct sti_rom *rom = scr->scr_rom;
	struct sti_softc *ssc = rom->rom_softc;
	struct sti_sgc_softc *sc = device_private(ssc->sc_dev);
	paddr_t cookie = -1;

	if ((offset & PAGE_MASK) != 0)
		return -1;

	switch (scr->scr_wsmode) {
	case WSDISPLAYIO_MODE_MAPPED:
		/* not implemented yet; what should be shown? */
		break;
	case WSDISPLAYIO_MODE_DUMBFB:
		if (offset >= 0 && offset < (scr->fbwidth * scr->fbheight))
			cookie = m68k_btop(sc->sc_bitmap + offset);
		break;
	default:
		break;
	}

	return cookie;
}

int
sti_sgc_cnprobe(bus_space_tag_t bst, bus_addr_t addr, int slot)
{
	void *va;
	bus_space_handle_t romh;
	int devtype, rv = 0;

	if (bus_space_map(bst, addr, PAGE_SIZE, 0, &romh))
		return 0;

	va = bus_space_vaddr(bst, romh);
	if (badaddr(va))
		goto out;

	devtype = bus_space_read_1(bst, romh, 3);
	if (devtype == STI_DEVTYPE1 || devtype == STI_DEVTYPE4)
		rv = 1;

 out:
	bus_space_unmap(bst, romh, PAGE_SIZE);
	return rv;
}

void
sti_sgc_cnattach(bus_space_tag_t bst, bus_addr_t addr, int slot)
{
	int i;

	/* sticn_bases[0] will be fixed in sti_cnattach() */
	for (i = 0; i < STI_REGION_MAX; i++)
		sticn_bases[i] = addr;

	sti_cnattach(&sticn_rom, &sticn_scr, bst, sticn_bases,
	    STI_CODEBASE_ALT);

	sticonslot = slot;
}
