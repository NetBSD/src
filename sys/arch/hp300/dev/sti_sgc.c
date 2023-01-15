/*	$NetBSD: sti_sgc.c,v 1.8 2023/01/15 06:19:45 tsutsui Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: sti_sgc.c,v 1.8 2023/01/15 06:19:45 tsutsui Exp $");

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
};

/*
 * 425e EVRX specific hardware
 */
/*
 * EVRX RAMDAC (Bt458) is found at offset 0x060000 from SGC bus PA and
 * offset 0x040000 length 0x1c0000 is mapped in MI sti via ROM region 2
 */
#define STI_EVRX_REGNO2OFFSET	0x020000
#define STI_EVRX_FBOFFSET	0x200000

#define EVRX_BT458_ADDR		(STI_EVRX_REGNO2OFFSET + 0x200 + 2)
#define EVRX_BT458_CMAP		(STI_EVRX_REGNO2OFFSET + 0x204 + 2)
#define EVRX_BT458_CTRL		(STI_EVRX_REGNO2OFFSET + 0x208 + 2)
#define EVRX_BT458_OMAP		(STI_EVRX_REGNO2OFFSET + 0x20C + 2)

/* from HP-UX /usr/lib/libddevrx.a */
#define EVRX_MAGIC00		(STI_EVRX_REGNO2OFFSET + 0x600)
#define EVRX_MAGIC04		(STI_EVRX_REGNO2OFFSET + 0x604)
#define EVRX_MAGIC08		(STI_EVRX_REGNO2OFFSET + 0x608)
#define EVRX_MAGIC0C		(STI_EVRX_REGNO2OFFSET + 0x60c)
#define EVRX_MAGIC10		(STI_EVRX_REGNO2OFFSET + 0x610)
#define EVRX_MAGIC10_BSY	0x00010000
#define EVRX_MAGIC18		(STI_EVRX_REGNO2OFFSET + 0x618)
#define EVRX_MAGIC1C		(STI_EVRX_REGNO2OFFSET + 0x61c)

/*
 * HP A1659A CRX specific hardware
 */
#define STI_CRX_FBOFFSET	0x01000000

static int sticonslot = -1;
static struct bus_space_tag sticn_tag;
static struct sti_rom sticn_rom;
static struct sti_screen sticn_scr;
static bus_addr_t sticn_bases[STI_REGION_MAX];

static int sti_sgc_match(device_t, struct cfdata *, void *);
static void sti_sgc_attach(device_t, device_t, void *);

static int sti_sgc_probe(bus_space_tag_t, int);

CFATTACH_DECL_NEW(sti_sgc, sizeof(struct sti_sgc_softc),
    sti_sgc_match, sti_sgc_attach, NULL, NULL);

/* 425e EVRX/CRX specific access functions */
static int sti_evrx_putcmap(struct sti_screen *, u_int, u_int);
static void sti_evrx_resetramdac(struct sti_screen *);
static void sti_evrx_resetcmap(struct sti_screen *);
static void sti_evrx_setupfb(struct sti_screen *);
static paddr_t sti_m68k_mmap(void *, void *, off_t, int);

static const struct wsdisplay_accessops sti_m68k_accessops = {
	sti_ioctl,
	sti_m68k_mmap,
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
	bus_space_tag_t bst;
	bus_space_handle_t romh;
	bus_addr_t base;
	struct wsemuldisplaydev_attach_args waa;
	u_int romend;
	struct sti_dd *rom_dd;
	uint32_t grid0;
	int i;

	ssc->sc_dev = self;
	bst = saa->saa_iot;
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
		if (bus_space_map(bst, base, PAGE_SIZE, 0, &romh)) {
			aprint_error(": can't map ROM");
			return;
		}
		/*
		 * Compute real PROM size
		 */
		romend = sti_rom_size(bst, romh);

		bus_space_unmap(bst, romh, PAGE_SIZE);

		if (bus_space_map(bst, base, romend, 0, &romh)) {
			aprint_error(": can't map frame buffer");
			return;
		}

		ssc->bases[0] = romh;
		for (i = 0; i < STI_REGION_MAX; i++)
			ssc->bases[i] = base;

		if (sti_attach_common(ssc, bst, bst, romh,
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

		aprint_normal_dev(self, "Enable mmap support\n");

		/*
		 * initialize Bt458 RAMDAC and preserve initial color map
		 */
		sti_evrx_resetramdac(scr);
		sti_evrx_resetcmap(scr);

		scr->setupfb = sti_evrx_setupfb;
		scr->putcmap = sti_evrx_putcmap;

		scr->scr_wsmode = WSDISPLAYIO_MODE_EMUL;
		waa.console = ssc->sc_flags & STI_CONSOLE ? 1 : 0;
		waa.scrdata = &scr->scr_screenlist;
		waa.accessops = &sti_m68k_accessops;
		waa.accesscookie = scr;

		config_found(ssc->sc_dev, &waa, wsemuldisplaydevprint,
		    CFARGS_NONE);
		break;

	case STI_DD_CRX:
		/*
		 * HP A1659A CRX on some 425t variants.
		 * bitmap memory can be accessed at offset +0x1000000.
		 */
		sc->sc_bitmap = base + STI_CRX_FBOFFSET;

		aprint_normal_dev(self, "Enable mmap support\n");

		scr->scr_wsmode = WSDISPLAYIO_MODE_EMUL;
		waa.console = ssc->sc_flags & STI_CONSOLE ? 1 : 0;
		waa.scrdata = &scr->scr_screenlist;
		waa.accessops = &sti_m68k_accessops;
		waa.accesscookie = scr;

		config_found(ssc->sc_dev, &waa, wsemuldisplaydevprint,
		    CFARGS_NONE);
		break;
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
	 * SGC cards will apparently not initialize in the hp300, to the
	 * point of not even answering bus probes (checked with an
	 * Harmony/FDDI SGC card).
	 */
	if (devtype != STI_DEVTYPE1 && devtype != STI_DEVTYPE4)
		return 0;

	return 1;
}

static int
sti_evrx_putcmap(struct sti_screen *scr, u_int index, u_int count)
{
	struct sti_rom *rom = scr->scr_rom;
	bus_space_tag_t bst = rom->memt;
	bus_space_handle_t bsh = rom->regh[2];
	int i;

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
sti_evrx_resetramdac(struct sti_screen *scr)
{
	struct sti_rom *rom = scr->scr_rom;
	bus_space_tag_t bst = rom->memt;
	bus_space_handle_t bsh = rom->regh[2];
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

	/* palette enabled, ovly plane disabled */
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
sti_evrx_resetcmap(struct sti_screen *scr)
{
	struct sti_rom *rom = scr->scr_rom;
	bus_space_tag_t bst = rom->memt;
	bus_space_handle_t bsh = rom->regh[2];
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

static void
sti_evrx_setupfb(struct sti_screen *scr)
{

	sti_init(scr, 0);
	sti_evrx_resetramdac(scr);
}

static paddr_t
sti_m68k_mmap(void *v, void *vs, off_t offset, int prot)
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

	sticn_tag = *bst;

	/* sticn_bases[0] will be fixed in sti_cnattach() */
	for (i = 0; i < STI_REGION_MAX; i++)
		sticn_bases[i] = addr;

	sti_cnattach(&sticn_rom, &sticn_scr, &sticn_tag, sticn_bases,
	    STI_CODEBASE_ALT);

	sticonslot = slot;
}
