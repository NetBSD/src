/*	$NetBSD: summitfb.c,v 1.30 2025/01/27 12:10:19 macallan Exp $	*/

/*	$OpenBSD: sti_pci.c,v 1.7 2009/02/06 22:51:04 miod Exp $	*/

/*
 * Copyright (c) 2006, 2007 Miodrag Vallat.
 ^                     2024 Michael Lorenz
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice, this permission notice, and the disclaimer below
 * appear in all copies.
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
 * a native driver for HP Visualize FX graphics cards, so far tested only on
 * my FX4
 * STI portions are from Miodrag Vallat's sti_pci.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: summitfb.c,v 1.30 2025/01/27 12:10:19 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/mutex.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/pci/wsdisplay_pci.h>
#include <dev/wscons/wsdisplay_glyphcachevar.h>

#include <dev/ic/stireg.h>
#include <dev/ic/summitreg.h>
#include <dev/ic/stivar.h>

#include "opt_summitfb.h"

#ifdef SUMMITFB_DEBUG
#define	DPRINTF(s) printf s
#else
#define	DPRINTF(s) __nothing
#endif

int	summitfb_match(device_t, cfdata_t, void *);
void	summitfb_attach(device_t, device_t, void *);

struct	summitfb_softc {
	device_t		sc_dev;
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_tag;

	/* stuff we need in order to use the STI ROM */
	struct sti_softc	sc_base;
	struct sti_screen 	sc_scr;
	bus_space_handle_t	sc_romh;

	int sc_width, sc_height;
	int sc_locked;
	struct vcons_screen sc_console_screen;
	struct wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list sc_screenlist;
	struct vcons_data vd;
	int sc_mode;
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
	uint32_t sc_write_mode, sc_read_mode;
	/* cursor stuff */
	int sc_cursor_x, sc_cursor_y;
	int sc_hot_x, sc_hot_y, sc_enabled;
	/* font-in-vram */
	struct wsdisplay_font *sc_font;
	int sc_font_start;	/* x of font area */
	int sc_cols;		/* chars per line in font area */
	uint32_t sc_palette[16];
	int sc_video_on;
	glyphcache sc_gc;
};

CFATTACH_DECL_NEW(summitfb, sizeof(struct summitfb_softc),
    summitfb_match, summitfb_attach, NULL, NULL);

int	summitfb_readbar(struct sti_softc *, struct pci_attach_args *, u_int,
	    int);
int	summitfb_check_rom(struct summitfb_softc *, struct pci_attach_args *);
void	summitfb_enable_rom(struct sti_softc *);
void	summitfb_disable_rom(struct sti_softc *);
void	summitfb_enable_rom_internal(struct summitfb_softc *);
void	summitfb_disable_rom_internal(struct summitfb_softc *);

void 	summitfb_setup(struct summitfb_softc *);

/* XXX these really need to go into their own header */
int	sti_pci_is_console(struct pci_attach_args *, bus_addr_t *);
int	sti_rom_setup(struct sti_rom *, bus_space_tag_t, bus_space_tag_t,
	    bus_space_handle_t, bus_addr_t *, u_int);
int	sti_screen_setup(struct sti_screen *, int);
void	sti_describe_screen(struct sti_softc *, struct sti_screen *);

#define PCI_ROM_SIZE(mr)						      \
	(PCI_MAPREG_ROM_ADDR(mr) & -PCI_MAPREG_ROM_ADDR(mr))

/* wsdisplay stuff */
static int	summitfb_ioctl(void *, void *, u_long, void *, int,
		    struct lwp *);
static paddr_t	summitfb_mmap(void *, void *, off_t, int);
static void	summitfb_init_screen(void *, struct vcons_screen *, int,
		    long *);

static int	summitfb_putcmap(struct summitfb_softc *,
		    struct wsdisplay_cmap *);
static int 	summitfb_getcmap(struct summitfb_softc *,
		    struct wsdisplay_cmap *);
static void	summitfb_restore_palette(struct summitfb_softc *);
static int 	summitfb_putpalreg(struct summitfb_softc *, uint8_t, uint8_t,
		    uint8_t, uint8_t);

static inline void summitfb_setup_fb(struct summitfb_softc *);
static void 	summitfb_clearfb(struct summitfb_softc *);
static void	summitfb_rectfill(struct summitfb_softc *, int, int, int, int,
		    uint32_t);
static void	summitfb_bitblt(void *, int, int, int, int, int,
		    int, int);

static void	summitfb_cursor(void *, int, int, int);
static void	summitfb_putchar(void *, int, int, u_int, long);
static void	summitfb_putchar_fast(void *, int, int, u_int, long);
static void	summitfb_loadfont(struct summitfb_softc *);
static void	summitfb_putchar_aa(void *, int, int, u_int, long);
static void	summitfb_copycols(void *, int, int, int, int);
static void	summitfb_erasecols(void *, int, int, int, long);
static void	summitfb_copyrows(void *, int, int, int);
static void	summitfb_eraserows(void *, int, int, long);

static void	summitfb_move_cursor(struct summitfb_softc *, int, int);
static int	summitfb_do_cursor(struct summitfb_softc *,
		    struct wsdisplay_cursor *);

static void	summitfb_set_video(struct summitfb_softc *, int);

static void	summitfb_copyfont(struct summitfb_softc *);

struct wsdisplay_accessops summitfb_accessops = {
	.ioctl = summitfb_ioctl,
	.mmap = summitfb_mmap,
	.alloc_screen = NULL,
	.free_screen = NULL,
	.show_screen = NULL,
	.load_font = NULL,
	.pollc = NULL,
	.scroll = NULL,
};

static inline void summitfb_wait_fifo(struct summitfb_softc *, uint32_t);
static inline void summitfb_wait(struct summitfb_softc *);

int	sti_fetchfonts(struct sti_screen *, struct sti_inqconfout *, uint32_t,
	    u_int);

int
summitfb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args *paa = aux;

	if (PCI_VENDOR(paa->pa_id) != PCI_VENDOR_HP)
		return 0;

	if (PCI_PRODUCT(paa->pa_id) == PCI_PRODUCT_HP_VISUALIZE_FX4)
		return 10;	/* beat out sti at pci */

	return 0;
}

static inline uint32_t
summitfb_read4(struct summitfb_softc *sc, uint32_t offset)
{
	struct sti_rom *rom = sc->sc_base.sc_rom;
	bus_space_tag_t memt = rom->memt;
	bus_space_handle_t memh = rom->regh[2];

	return bus_space_read_stream_4(memt, memh, offset - 0x400000);
}

static inline void
summitfb_write4(struct summitfb_softc *sc, uint32_t offset, uint32_t val)
{
	struct sti_rom *rom = sc->sc_base.sc_rom;
	bus_space_tag_t memt = rom->memt;
	bus_space_handle_t memh = rom->regh[2];

	bus_space_write_stream_4(memt, memh, offset - 0x400000, val);
}

static inline void
summitfb_write_mode(struct summitfb_softc *sc, uint32_t mode)
{
	if (sc->sc_write_mode == mode)
		return;
	summitfb_wait(sc);
	summitfb_write4(sc, VISFX_VRAM_WRITE_MODE, mode);
	sc->sc_write_mode = mode;
}

static inline void
summitfb_read_mode(struct summitfb_softc *sc, uint32_t mode)
{
	if (sc->sc_read_mode == mode)
		return;
	summitfb_wait(sc);
	summitfb_write4(sc, VISFX_VRAM_READ_MODE, mode);
	sc->sc_read_mode = mode;
}

void
summitfb_attach(device_t parent, device_t self, void *aux)
{
	struct summitfb_softc *sc = device_private(self);
	struct pci_attach_args *paa = aux;
	struct sti_rom *rom;
	struct rasops_info *ri;
	struct wsemuldisplaydev_attach_args aa;
	struct sti_dd *dd;
	unsigned long defattr = 0;
	int ret, is_console = 0;

	sc->sc_dev = self;

	sc->sc_pc = paa->pa_pc;
	sc->sc_tag = paa->pa_tag;
	sc->sc_base.sc_dev = self;
	sc->sc_base.sc_enable_rom = summitfb_enable_rom;
	sc->sc_base.sc_disable_rom = summitfb_disable_rom;

	aprint_normal("\n");

	if (summitfb_check_rom(sc, paa) != 0)
		return;

	ret = sti_pci_is_console(paa, sc->sc_base. bases);
	if (ret != 0) {
		sc->sc_base.sc_flags |= STI_CONSOLE;
		is_console = 1;
	}
	rom = kmem_zalloc(sizeof(*rom), KM_SLEEP);
	rom->rom_softc = &sc->sc_base;
	ret = sti_rom_setup(rom, paa->pa_iot, paa->pa_memt, sc->sc_romh,
	    sc->sc_base.bases, STI_CODEBASE_MAIN);
	if (ret != 0) {
		kmem_free(rom, sizeof(*rom));
		return;
	}

	sc->sc_base.sc_rom = rom;
	dd = &rom->rom_dd;

	sc->sc_scr.scr_rom = sc->sc_base.sc_rom;
	ret = sti_screen_setup(&sc->sc_scr, STI_FBMODE);

	sti_fetchfonts(&sc->sc_scr, NULL, dd->dd_fntaddr, 0);
	wsfont_init();
	summitfb_copyfont(sc);

	sc->sc_width = sc->sc_scr.scr_cfg.scr_width;
	sc->sc_height = sc->sc_scr.scr_cfg.scr_height;
	sc->sc_write_mode = 0xffffffff;
	sc->sc_read_mode = 0xffffffff;

#ifdef SUMMITFB_DEBUG
	sc->sc_height -= 200;
#endif

	sc->sc_defaultscreen_descr = (struct wsscreen_descr){
		.name = "default",
		.ncols = 0, .nrows = 0,
		.textops = NULL,
		.fontwidth = 8, .fontheight = 16,
		.capabilities = WSSCREEN_WSCOLORS | WSSCREEN_HILIT |
		    WSSCREEN_UNDERLINE | WSSCREEN_RESIZE,
		.modecookie = NULL,
	};

	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_locked = 0;

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
	    &summitfb_accessops);
	sc->vd.init_screen = summitfb_init_screen;
	sc->vd.show_screen_cookie = &sc->sc_gc;
	sc->vd.show_screen_cb = glyphcache_adapt;
	ri = &sc->sc_console_screen.scr_ri;

	sc->sc_gc.gc_bitblt = summitfb_bitblt;
	sc->sc_gc.gc_blitcookie = sc;
	sc->sc_gc.gc_rop = RopSrc;

	summitfb_setup(sc);

	vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1, &defattr);
	sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

	sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
	sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
	sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
	sc->sc_defaultscreen_descr.ncols = ri->ri_cols;

	/*
	 * STI lies to us - it reports a 2048x2048 framebuffer but blitter
	 * ops wrap around below 1024 and we seem to have only about 250
	 * usable columns to the right. Should still be enough to cache
	 * a font or four.
	 * So, the framebuffer seems to be 1536x1024, which is odd since the
	 * FX4 is supposed to support resolutions higher than 1280x1024.
	 * I guess video memory is allocated in 512x512 chunks
	 */
	glyphcache_init(&sc->sc_gc,
	    sc->sc_height,
	    sc->sc_height,
	    (sc->sc_width + 511) & (~511),
	    ri->ri_font->fontwidth,
	    ri->ri_font->fontheight,
	    defattr);

	summitfb_restore_palette(sc);
	summitfb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height,
	    ri->ri_devcmap[(defattr >> 16) & 0xff]);
	summitfb_setup_fb(sc);

	if (is_console) {
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);

		vcons_replay_msgbuf(&sc->sc_console_screen);
	}

	aprint_normal_dev(sc->sc_dev, "%s at %dx%d\n", sc->sc_scr.name,
	    sc->sc_width, sc->sc_height);

	/* no suspend/resume support yet */
	pmf_device_register(sc->sc_dev, NULL, NULL);

	aa.console = is_console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &summitfb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint, CFARGS_NONE);
#ifdef DEBUG
	{
		int i;

		summitfb_rectfill(sc, 0, 824, 1280, 200, 0x00);
		summitfb_rectfill(sc, 5, 830, 100, 190, 0xe0);	/* red */

		summitfb_write_mode(sc, OTC01 | BIN332F | BUFovl);
		summitfb_read_mode(sc, OTC01 | BIN332F | BUFovl);
		summitfb_write4(sc, VISFX_CBR, 0x3f0000ff);
		summitfb_write4(sc, VISFX_FG_COLOUR, 0x7f00ff00);

		for (i = 0; i < 16; i++) {
			summitfb_wait_fifo(sc, 10);
			summitfb_write4(sc, VISFX_IBO, 0x200 | (14 << 4) | i);
			summitfb_write4(sc, VISFX_COPY_SRC, (5 << 16) | 830);
			summitfb_write4(sc, VISFX_COPY_WH, (30 << 16) | 90);
			summitfb_write4(sc, VISFX_COPY_DST,
			    ((i * 35 + 200) << 16) | 830);
		}
		summitfb_write_mode(sc, OTC01 | BIN332F | BUFovl | 0x8c0);
		summitfb_write4(sc, VISFX_FG_COLOUR, 0x7f00ff00);
		for (i = 0; i < 16; i++) {
			summitfb_wait_fifo(sc, 10);
			summitfb_write4(sc, VISFX_IBO, 0x200 | (i << 4) | 5);
			summitfb_write4(sc, VISFX_START,
			    ((i * 35 + 200) << 16) | 930);
			summitfb_write4(sc, VISFX_SIZE, (30 << 16) | 90);
		}
	}
#endif	
}

/*
 * Grovel the STI ROM image.
 */
int
summitfb_check_rom(struct summitfb_softc *spc, struct pci_attach_args *pa)
{
	struct sti_softc *sc = &spc->sc_base;
	pcireg_t address, mask;
	bus_space_handle_t romh;
	bus_size_t romsize, subsize, stiromsize;
	bus_addr_t selected, offs, suboffs;
	uint32_t tmp;
	int i;
	int rc;

	/* sort of inline sti_pci_enable_rom(sc) */
	address = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_MAPREG_ROM);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_MAPREG_ROM,
	    ~PCI_MAPREG_ROM_ENABLE);
	mask = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_MAPREG_ROM);
	address |= PCI_MAPREG_ROM_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_MAPREG_ROM, address);
	sc->sc_flags |= STI_ROM_ENABLED;

	/*
	 * Map the complete ROM for now.
	 */
	romsize = PCI_ROM_SIZE(mask);
	DPRINTF(("%s: mapping rom @ %lx for %lx\n", __func__,
	    (long)PCI_MAPREG_ROM_ADDR(address), (long)romsize));

	rc = bus_space_map(pa->pa_memt, PCI_MAPREG_ROM_ADDR(address), romsize,
	    0, &romh);
	if (rc != 0) {
		aprint_error_dev(sc->sc_dev, "can't map PCI ROM (%d)\n", rc);
		goto fail2;
	}

	summitfb_disable_rom_internal(spc);

	/*
	 * Iterate over the ROM images, pick the best candidate.
	 */
	selected = (bus_addr_t)-1;
	for (offs = 0; offs < romsize; offs += subsize) {
		summitfb_enable_rom_internal(spc);
		/*
		 * Check for a valid ROM header.
		 */
		tmp = bus_space_read_4(pa->pa_memt, romh, offs + 0);
		tmp = le32toh(tmp);
		if (tmp != 0x55aa0000) {
			summitfb_disable_rom_internal(spc);
			if (offs == 0) {
				aprint_error_dev(sc->sc_dev,
				    "invalid PCI ROM header signature"
				    " (%08x)\n", tmp);
				rc = EINVAL;
			}
			break;
		}

		/*
		 * Check ROM type.
		 */
		tmp = bus_space_read_4(pa->pa_memt, romh, offs + 4);
		tmp = le32toh(tmp);
		if (tmp != 0x00000001) {	/* 1 == STI ROM */
			summitfb_disable_rom_internal(spc);
			if (offs == 0) {
				aprint_error_dev(sc->sc_dev,
				    "invalid PCI ROM type (%08x)\n", tmp);
				rc = EINVAL;
			}
			break;
		}

		subsize = (bus_addr_t)bus_space_read_2(pa->pa_memt, romh,
		    offs + 0x0c);
		subsize <<= 9;

#ifdef SUMMITFB_DEBUG
		summitfb_disable_rom_internal(spc);
		DPRINTF(("ROM offset %08x size %08x type %08x",
		    (u_int)offs, (u_int)subsize, tmp));
		summitfb_enable_rom_internal(spc);
#endif

		/*
		 * Check for a valid ROM data structure.
		 * We do not need it except to know what architecture the ROM
		 * code is for.
		 */

		suboffs = offs + bus_space_read_2(pa->pa_memt, romh,
		    offs + 0x18);
		tmp = bus_space_read_4(pa->pa_memt, romh, suboffs + 0);
		tmp = le32toh(tmp);
		if (tmp != 0x50434952) {	/* PCIR */
			summitfb_disable_rom_internal(spc);
			if (offs == 0) {
				aprint_error_dev(sc->sc_dev, "invalid PCI data"
				    " signature (%08x)\n", tmp);
				rc = EINVAL;
			} else {
				DPRINTF((" invalid PCI data signature %08x\n",
				    tmp));
				continue;
			}
		}

		tmp = bus_space_read_1(pa->pa_memt, romh, suboffs + 0x14);
		summitfb_disable_rom_internal(spc);
		DPRINTF((" code %02x", tmp));

		switch (tmp) {
#ifdef __hppa__
		case 0x10:
			if (selected == (bus_addr_t)-1)
				selected = offs;
			break;
#endif
#ifdef __i386__
		case 0x00:
			if (selected == (bus_addr_t)-1)
				selected = offs;
			break;
#endif
		default:
			DPRINTF((" (wrong architecture)"));
			break;
		}
		DPRINTF(("%s\n", selected == offs ? " -> SELECTED" : ""));
	}

	if (selected == (bus_addr_t)-1) {
		if (rc == 0) {
			aprint_error_dev(sc->sc_dev, "found no ROM with "
			    "correct microcode architecture\n");
			rc = ENOEXEC;
		}
		goto fail;
	}

	/*
	 * Read the STI region BAR assignments.
	 */

	summitfb_enable_rom_internal(spc);
	offs = selected + bus_space_read_2(pa->pa_memt, romh, selected + 0x0e);
	for (i = 0; i < STI_REGION_MAX; i++) {
		rc = summitfb_readbar(sc, pa, i,
		    bus_space_read_1(pa->pa_memt, romh, offs + i));
		if (rc != 0)
			goto fail;
	}

	/*
	 * Find out where the STI ROM itself lies, and its size.
	 */

	offs = selected +
	    bus_space_read_4(pa->pa_memt, romh, selected + 0x08);
	stiromsize = bus_space_read_4(pa->pa_memt, romh, offs + 0x18);
	stiromsize = le32toh(stiromsize);
	summitfb_disable_rom_internal(spc);

	/*
	 * Replace our mapping with a smaller mapping of only the area
	 * we are interested in.
	 */

	DPRINTF(("remapping rom @ %lx for %lx\n",
	    (long)(PCI_MAPREG_ROM_ADDR(address) + offs), (long)stiromsize));
	bus_space_unmap(pa->pa_memt, romh, romsize);
	rc = bus_space_map(pa->pa_memt, PCI_MAPREG_ROM_ADDR(address) + offs,
	    stiromsize, 0, &spc->sc_romh);
	if (rc != 0) {
		aprint_error_dev(sc->sc_dev, "can't map STI ROM (%d)\n",
		    rc);
		goto fail2;
	}
 	summitfb_disable_rom_internal(spc);
	sc->sc_flags &= ~STI_ROM_ENABLED;

	return 0;

fail:
	bus_space_unmap(pa->pa_memt, romh, romsize);
fail2:
	summitfb_disable_rom_internal(spc);

	return rc;
}

/*
 * Decode a BAR register.
 */
int
summitfb_readbar(struct sti_softc *sc, struct pci_attach_args *pa,
    u_int region, int bar)
{
	bus_addr_t addr;
	bus_size_t size;
	uint32_t cf;
	int rc;

	if (bar == 0) {
		sc->bases[region] = 0;
		return 0;
	}

#ifdef DIAGNOSTIC
	if (bar < PCI_MAPREG_START || bar > PCI_MAPREG_PPB_END) {
		summitfb_disable_rom(sc);
		printf("%s: unexpected bar %02x for region %d\n",
		    device_xname(sc->sc_dev), bar, region);
		summitfb_enable_rom(sc);
	}
#endif

	cf = pci_conf_read(pa->pa_pc, pa->pa_tag, bar);

	rc = pci_mapreg_info(pa->pa_pc, pa->pa_tag, bar, PCI_MAPREG_TYPE(cf),
	    &addr, &size, NULL);

	if (rc != 0) {
		summitfb_disable_rom(sc);
		aprint_error_dev(sc->sc_dev, "invalid bar %02x"
		    " for region %d\n",
		    bar, region);
		summitfb_enable_rom(sc);
		return rc;
	}

	sc->bases[region] = addr;
	return 0;
}

/*
 * Enable PCI ROM.
 */
void
summitfb_enable_rom_internal(struct summitfb_softc *spc)
{
	pcireg_t address;

	KASSERT(spc != NULL);

	address = pci_conf_read(spc->sc_pc, spc->sc_tag, PCI_MAPREG_ROM);
	address |= PCI_MAPREG_ROM_ENABLE;
	pci_conf_write(spc->sc_pc, spc->sc_tag, PCI_MAPREG_ROM, address);
}

void
summitfb_enable_rom(struct sti_softc *sc)
{
	struct summitfb_softc *spc = device_private(sc->sc_dev);

	if (!ISSET(sc->sc_flags, STI_ROM_ENABLED)) {
		summitfb_enable_rom_internal(spc);
	}
	SET(sc->sc_flags, STI_ROM_ENABLED);
}

/*
 * Disable PCI ROM.
 */
void
summitfb_disable_rom_internal(struct summitfb_softc *spc)
{
	pcireg_t address;

	KASSERT(spc != NULL);

	address = pci_conf_read(spc->sc_pc, spc->sc_tag, PCI_MAPREG_ROM);
	address &= ~PCI_MAPREG_ROM_ENABLE;
	pci_conf_write(spc->sc_pc, spc->sc_tag, PCI_MAPREG_ROM, address);
}

void
summitfb_disable_rom(struct sti_softc *sc)
{
	struct summitfb_softc *spc = device_private(sc->sc_dev);

	if (ISSET(sc->sc_flags, STI_ROM_ENABLED)) {
		summitfb_disable_rom_internal(spc);
	}
	CLR(sc->sc_flags, STI_ROM_ENABLED);
}

static inline void
summitfb_wait(struct summitfb_softc *sc)
{

	while (summitfb_read4(sc, VISFX_STATUS) != 0)
		continue;
}

static inline void
summitfb_setup_fb(struct summitfb_softc *sc)
{

	summitfb_wait(sc);
	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		summitfb_write_mode(sc, VISFX_WRITE_MODE_PLAIN);
		summitfb_read_mode(sc, VISFX_WRITE_MODE_PLAIN);
		summitfb_write4(sc, VISFX_APERTURE_ACCESS, VISFX_DEPTH_8);
		/* make overlay opaque */
		summitfb_write4(sc, VISFX_OTR, OTR_T | OTR_L1 | OTR_L0);
	} else {
		summitfb_write_mode(sc, OTC01 | BIN8F | BUFFL);
		summitfb_read_mode(sc, OTC01 | BIN8F | BUFFL);
		summitfb_write4(sc, VISFX_APERTURE_ACCESS, VISFX_DEPTH_32);
		/* make overlay transparent */
		summitfb_write4(sc, VISFX_OTR, OTR_A);
	}
	summitfb_write4(sc, VISFX_IBO, RopSrc);
}

void
summitfb_setup(struct summitfb_softc *sc)
{
	int i;

	sc->sc_hot_x = 0;
	sc->sc_hot_y = 0;
	sc->sc_enabled = 0;
	sc->sc_video_on = 1;

	summitfb_wait(sc);
#if 1
	/* these control byte swapping */
	summitfb_write4(sc, 0xb08044, 0x1b);	/* MFU_BSCTD */
	summitfb_write4(sc, 0xb08048, 0x1b);	/* MFU_BSCCTL */

	summitfb_write4(sc, 0x920860, 0xe4);	/* FBC_RBS */
	summitfb_write4(sc, 0x921114, 0);	/* CPE, clip plane enable */
	summitfb_write4(sc, 0x9211d8, 0);	/* FCDA */

	summitfb_write4(sc, 0xa00818, 0);	/* WORG window origin */
	summitfb_write4(sc, 0xa0081c, 0);	/* FBS front buffer select*/
	summitfb_write4(sc, 0xa00850, 0);	/* MISC_CTL */
	summitfb_write4(sc, 0xa0086c, 0);	/* WCE window clipping enable */
#endif
	/* initialize drawiing engine */
	summitfb_wait(sc);
	summitfb_write4(sc, VISFX_CONTROL, 0);	// clear WFC
	summitfb_write4(sc, VISFX_APERTURE_ACCESS, VISFX_DEPTH_8);
	summitfb_write4(sc, VISFX_PIXEL_MASK, 0xffffffff);
	summitfb_write4(sc, VISFX_PLANE_MASK, 0xffffffff);
	summitfb_write4(sc, VISFX_FOE, FOE_BLEND_ROP);
	summitfb_write4(sc, VISFX_IBO, RopSrc);
	summitfb_write_mode(sc, VISFX_WRITE_MODE_PLAIN);
	summitfb_read_mode(sc, OTC04 | BIN8I | BUFovl);
	summitfb_write4(sc, VISFX_CLIP_TL, 0);
	summitfb_write4(sc, VISFX_CLIP_WH,
	    ((sc->sc_scr.fbwidth) << 16) | (sc->sc_scr.fbheight));
	/* turn off the cursor sprite */
	summitfb_write4(sc, VISFX_CURSOR_POS, 0);
	/* disable throttling by moving the throttle window way off screen */
	summitfb_write4(sc, VISFX_TCR, 0x10001000);

	/* make sure the overlay is opaque */
	summitfb_write4(sc, VISFX_OTR, OTR_T | OTR_L1 | OTR_L0);

	/*
	 * initialize XLUT, I mean attribute table
	 * set all to 24bit, CFS1
	 */
	for (i = 0; i < 16; i++)
		summitfb_write4(sc, VISFX_IAA(i), IAA_8F | IAA_CFS1);
	/* RGB8, no LUT */
	summitfb_write4(sc, VISFX_CFS(1), CFS_8F | CFS_BYPASS);
	/* overlay is 8bit, uses LUT 0 */
	summitfb_write4(sc, VISFX_CFS(16), CFS_8I | CFS_LUT0);
	summitfb_write4(sc, VISFX_CFS(17), CFS_8I | CFS_LUT0);

	/* zero the attribute plane */
	summitfb_write_mode(sc, OTC04 | BINapln);
	summitfb_wait_fifo(sc, 12);
	summitfb_write4(sc, VISFX_PLANE_MASK, 0xff);
	summitfb_write4(sc, VISFX_IBO, 0);	/* GXclear */
	summitfb_write4(sc, VISFX_FG_COLOUR, 0);
	summitfb_write4(sc, VISFX_START, 0);
	summitfb_write4(sc, VISFX_SIZE, (sc->sc_width << 16) | sc->sc_height);
	summitfb_wait(sc);
	summitfb_write4(sc, VISFX_PLANE_MASK, 0xffffffff);

	/* turn off force attr so the above takes effect */
	summitfb_write4(sc, VISFX_FATTR, 0);

	summitfb_setup_fb(sc);
}

static int
summitfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct vcons_data *vd = v;
	struct summitfb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_STI;
		return 0;

	case GCID:
		*(u_int *)data = sc->sc_scr.scr_rom->rom_dd.dd_grid[0];
		return 0;

	/* PCI config read/write passthrough. */
	case PCI_IOC_CFGREAD:
	case PCI_IOC_CFGWRITE:
		return pci_devioctl(sc->sc_pc, sc->sc_tag,
		    cmd, data, flag, l);

	case WSDISPLAYIO_GET_BUSID:
		return wsdisplayio_busid_pci(sc->sc_dev, sc->sc_pc,
		    sc->sc_tag, data);

	case WSDISPLAYIO_GINFO:
		if (ms == NULL)
			return ENODEV;
		wdf = data;
		wdf->height = ms->scr_ri.ri_height;
		wdf->width = ms->scr_ri.ri_width;
		wdf->depth = ms->scr_ri.ri_depth;
		wdf->cmsize = 256;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return summitfb_getcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return summitfb_putcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = 2048;
		return 0;

	case WSDISPLAYIO_SMODE: {
		int new_mode = *(int *)data;

		if (new_mode != sc->sc_mode) {
			sc->sc_mode = new_mode;
			if(new_mode == WSDISPLAYIO_MODE_EMUL) {
				summitfb_setup(sc);
				summitfb_restore_palette(sc);
				glyphcache_wipe(&sc->sc_gc);
				summitfb_loadfont(sc);
				summitfb_rectfill(sc, 0, 0, sc->sc_width,
				    sc->sc_height, ms->scr_ri.ri_devcmap[
				    (ms->scr_defattr >> 16) & 0xff]);
				vcons_redraw_screen(ms);
				summitfb_set_video(sc, 1);
			} else
				summitfb_clearfb(sc);
			summitfb_setup_fb(sc);
		}
		return 0;
	}

	case WSDISPLAYIO_GET_FBINFO: {
		struct wsdisplayio_fbinfo *fbi = data;
		int ret;

		ret = wsdisplayio_get_fbinfo(&ms->scr_ri, fbi);
		//fbi->fbi_fbsize = sc->sc_height * 2048;
		fbi->fbi_stride = 8192;
		fbi->fbi_bitsperpixel = 32;
		fbi->fbi_pixeltype = WSFB_RGB;
		fbi->fbi_subtype.fbi_rgbmasks.red_offset = 16;
		fbi->fbi_subtype.fbi_rgbmasks.red_size = 8;
		fbi->fbi_subtype.fbi_rgbmasks.green_offset = 8;
		fbi->fbi_subtype.fbi_rgbmasks.green_size = 8;
		fbi->fbi_subtype.fbi_rgbmasks.blue_offset = 0;
		fbi->fbi_subtype.fbi_rgbmasks.blue_size = 8;
		fbi->fbi_subtype.fbi_rgbmasks.alpha_size = 0;
		fbi->fbi_fbsize = sc->sc_scr.fbheight * 8192;
		return ret;
	}

	case WSDISPLAYIO_GCURPOS: {
		struct wsdisplay_curpos *cp = data;

		cp->x = sc->sc_cursor_x;
		cp->y = sc->sc_cursor_y;
		return 0;
	}

	case WSDISPLAYIO_SCURPOS: {
		struct wsdisplay_curpos *cp = data;

		summitfb_move_cursor(sc, cp->x, cp->y);
		return 0;
	}

	case WSDISPLAYIO_GCURMAX: {
		struct wsdisplay_curpos *cp = data;

		cp->x = 64;
		cp->y = 64;
		return 0;
	}

	case WSDISPLAYIO_SCURSOR: {
		struct wsdisplay_cursor *cursor = data;

		return summitfb_do_cursor(sc, cursor);
	}

	case WSDISPLAYIO_SVIDEO:
		summitfb_set_video(sc, *(int *)data);
		return 0;
	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = sc->sc_video_on ?
		    WSDISPLAYIO_VIDEO_ON : WSDISPLAYIO_VIDEO_OFF;
		return 0;
	}
	return EPASSTHROUGH;
}

static paddr_t
summitfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct summitfb_softc *sc = vd->cookie;
	struct sti_rom *rom = sc->sc_base.sc_rom;
	paddr_t pa = -1;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)
		return -1;

	if (offset >= 0 && offset < 0x01000000) {
		/* framebuffer */
		pa = bus_space_mmap(rom->memt, sc->sc_scr.fbaddr, offset,
		    prot, BUS_SPACE_MAP_LINEAR);
	} else if (offset >= 0x80000000 && offset < 0x81000000) {
		/* blitter registers etc. */
		pa = bus_space_mmap(rom->memt, rom->regh[0],
		    offset - 0x80000000, prot, BUS_SPACE_MAP_LINEAR);
	}

	return pa;
}

static void
summitfb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct summitfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = 8;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = 2048;
	ri->ri_flg = RI_CENTER | RI_8BIT_IS_RGB
	    | RI_ENABLE_ALPHA | RI_PREFER_ALPHA
	    ;

	ri->ri_bits = (void *)sc->sc_scr.fbaddr;
	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_UNDERLINE |
	    WSSCREEN_RESIZE;
	scr->scr_flags |= VCONS_LOADFONT;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
	    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;

	ri->ri_ops.copyrows = summitfb_copyrows;
	ri->ri_ops.copycols = summitfb_copycols;
	ri->ri_ops.eraserows = summitfb_eraserows;
	ri->ri_ops.erasecols = summitfb_erasecols;
	ri->ri_ops.cursor = summitfb_cursor;
	sc->sc_font = NULL;
	if (FONT_IS_ALPHA(ri->ri_font)) {
		ri->ri_ops.putchar = summitfb_putchar_aa;
	} else
	{
		int fbwidth = (sc->sc_width + 511) & ~511;
		int fcols = (fbwidth - sc->sc_width - 2) / ri->ri_font->fontwidth;
		int frows = sc->sc_height / ri->ri_font->fontheight;
		sc->sc_font_start = sc->sc_width + 2;
		if ((fcols * frows) >= ri->ri_font->numchars) {
			/* ok, we can do this */
			sc->sc_cols = fcols;
			sc->sc_font = ri->ri_font;
			summitfb_loadfont(sc);
			ri->ri_ops.putchar = summitfb_putchar_fast;
		} else
			ri->ri_ops.putchar = summitfb_putchar;
	}
}

static int
summitfb_putcmap(struct summitfb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_char *r, *g, *b;
	u_int index = cm->index;
	u_int count = cm->count;
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

	memcpy(&sc->sc_cmap_red[index], &rbuf[index], count);
	memcpy(&sc->sc_cmap_green[index], &gbuf[index], count);
	memcpy(&sc->sc_cmap_blue[index], &bbuf[index], count);

	r = &sc->sc_cmap_red[index];
	g = &sc->sc_cmap_green[index];
	b = &sc->sc_cmap_blue[index];

	for (i = 0; i < count; i++) {
		summitfb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
summitfb_getcmap(struct summitfb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int error;

	if (index >= 255 || count > 256 || index + count > 256)
		return EINVAL;

	error = copyout(&sc->sc_cmap_red[index],   cm->red,   count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_green[index], cm->green, count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_blue[index],  cm->blue,  count);
	if (error)
		return error;

	return 0;
}

static void
summitfb_restore_palette(struct summitfb_softc *sc)
{
	uint8_t cmap[768];
	int i, j;

	j = 0;
	rasops_get_cmap(&sc->sc_console_screen.scr_ri, cmap, sizeof(cmap));
	for (i = 0; i < 256; i++) {
		sc->sc_cmap_red[i] = cmap[j];
		sc->sc_cmap_green[i] = cmap[j + 1];
		sc->sc_cmap_blue[i] = cmap[j + 2];
		summitfb_putpalreg(sc, i, cmap[j], cmap[j + 1], cmap[j + 2]);
		j += 3;
	}
	for (i = 0; i < 16; i++) {
		sc->sc_palette[i] = (rasops_cmap[i * 3] << 16) |
				    (rasops_cmap[i * 3 + 1] << 8) | 
				     rasops_cmap[i * 3 + 2];
	}

}

static int
summitfb_putpalreg(struct summitfb_softc *sc, uint8_t idx,
    uint8_t r, uint8_t g, uint8_t b)
{

	summitfb_write4(sc, VISFX_COLOR_INDEX, idx);
	summitfb_write4(sc, VISFX_COLOR_VALUE, (r << 16) | ( g << 8) | b);
	summitfb_write4(sc, VISFX_COLOR_MASK, 0xff);
	return 0;
}

static inline void
summitfb_wait_fifo(struct summitfb_softc *sc, uint32_t slots)
{
	uint32_t reg;

	do {
		reg = summitfb_read4(sc, VISFX_FIFO);
	} while (reg < slots);
}

static void
summitfb_clearfb(struct summitfb_softc *sc)
{
	summitfb_write_mode(sc, OTC32 | BIN8F | BUFBL | BUFFL | 0x8c0);
	summitfb_wait_fifo(sc, 10);
	summitfb_write4(sc, VISFX_IBO, RopSrc);
	summitfb_write4(sc, VISFX_FG_COLOUR, 0);
	summitfb_write4(sc, VISFX_START, 0);
	summitfb_write4(sc, VISFX_SIZE, (sc->sc_width << 16) | sc->sc_height);
}

static void
summitfb_rectfill(struct summitfb_softc *sc, int x, int y, int wi, int he,
    uint32_t bg)
{

	summitfb_write_mode(sc, VISFX_WRITE_MODE_FILL);
	summitfb_wait_fifo(sc, 10);
	summitfb_write4(sc, VISFX_IBO, RopSrc);
	summitfb_write4(sc, VISFX_FG_COLOUR, bg);
	summitfb_write4(sc, VISFX_START, (x << 16) | y);
	summitfb_write4(sc, VISFX_SIZE, (wi << 16) | he);
}

static void
summitfb_bitblt(void *cookie, int xs, int ys, int xd, int yd, int wi,
    int he, int rop)
{
	struct summitfb_softc *sc = cookie;
	uint32_t read_mode, write_mode;

	read_mode = OTC04 | BIN8I;
	write_mode = OTC04 | BIN8I;
	if (ys >= sc->sc_height) {
		read_mode |= BUFBL;
		ys -= sc->sc_height;
	}
	if (yd >= sc->sc_height) {
		write_mode |= BUFBL;
		yd -= sc->sc_height;
	}
	summitfb_write_mode(sc, write_mode);
	summitfb_read_mode(sc, read_mode);
	summitfb_wait_fifo(sc, 10);
	summitfb_write4(sc, VISFX_IBO, rop);
	summitfb_write4(sc, VISFX_COPY_SRC, (xs << 16) | ys);
	summitfb_write4(sc, VISFX_COPY_WH, (wi << 16) | he);
	summitfb_write4(sc, VISFX_COPY_DST, (xd << 16) | yd);

}

static void
summitfb_nuke_cursor(struct rasops_info *ri)
{
	struct vcons_screen *scr = ri->ri_hw;
	struct summitfb_softc *sc = scr->scr_cookie;
	int wi, he, x, y;

	if (ri->ri_flg & RI_CURSOR) {
		wi = ri->ri_font->fontwidth;
		he = ri->ri_font->fontheight;
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		summitfb_bitblt(sc, x, y, x, y, wi, he, RopInv);
		ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
summitfb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct summitfb_softc *sc = scr->scr_cookie;
	int x, y, wi, he;

	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		if (on) {
			if (ri->ri_flg & RI_CURSOR) {
				summitfb_nuke_cursor(ri);
			}
			x = col * wi + ri->ri_xorigin;
			y = row * he + ri->ri_yorigin;
			summitfb_bitblt(sc, x, y, x, y, wi, he, RopInv);
			ri->ri_flg |= RI_CURSOR;
		}
		ri->ri_crow = row;
		ri->ri_ccol = col;
	} else {
		ri->ri_crow = row;
		ri->ri_ccol = col;
		ri->ri_flg &= ~RI_CURSOR;
	}

}

static void
summitfb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct summitfb_softc *sc = scr->scr_cookie;
	void *data;
	int i, x, y, wi, he;
	uint32_t bg, fg, mask;

	if (sc->sc_mode != WSDISPLAYIO_MODE_EMUL)
		return;

	if (!CHAR_IN_FONT(c, font))
		return;

	if (row == ri->ri_crow && col == ri->ri_ccol) {
		ri->ri_flg &= ~RI_CURSOR;
	}

	wi = font->fontwidth;
	he = font->fontheight;

	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	bg = ri->ri_devcmap[(attr >> 16) & 0xf];

	/* if we're drawing a space we're done here */
	if (c == 0x20) {
		summitfb_rectfill(sc, x, y, wi, he, bg);
		return;
	}

	fg = ri->ri_devcmap[(attr >> 24) & 0x0f];

	summitfb_write_mode(sc, VISFX_WRITE_MODE_EXPAND);
	summitfb_wait_fifo(sc, 12);
	summitfb_write4(sc, VISFX_IBO, RopSrc);
	summitfb_write4(sc, VISFX_FG_COLOUR, fg);
	summitfb_write4(sc, VISFX_BG_COLOUR, bg);
	mask = 0xffffffff << (32 - wi);
	summitfb_write4(sc, VISFX_PIXEL_MASK, mask);
	/* not a tpyo, coordinates *are* backwards for this register */
	summitfb_write4(sc, VISFX_VRAM_WRITE_DEST, (y << 16) | x);

	data = WSFONT_GLYPH(c, font);

	if (ri->ri_font->stride == 1) {
		uint8_t *data8 = data;
		for (i = 0; i < he; i++) {
			mask = *data8;
			summitfb_write4(sc, VISFX_VRAM_WRITE_DATA_INCRY,
			    mask << 24);
			data8++;
		}
	} else {
		uint16_t *data16 = data;
		for (i = 0; i < he; i++) {
			mask = *data16;
			summitfb_write4(sc, VISFX_VRAM_WRITE_DATA_INCRY,
			    mask << 16);
			data16++;
		}
	}
}

static void
summitfb_loadfont(struct summitfb_softc *sc)
{
	int i, c, x, y;
	uint8_t *data;
	uint16_t *data16;
	uint32_t mask;

	if (sc->sc_font == NULL)
		return;

	summitfb_write_mode(sc, VISFX_WRITE_MODE_EXPAND);
	summitfb_wait_fifo(sc, 10);
	summitfb_write4(sc, VISFX_IBO, RopSrc);
	summitfb_write4(sc, VISFX_FG_COLOUR, 0xffffffff);
	summitfb_write4(sc, VISFX_BG_COLOUR, 0);

	mask = 0xffffffff << (32 - sc->sc_font->fontwidth);
	summitfb_write4(sc, VISFX_PIXEL_MASK, mask);

	for (c = 0; c < sc->sc_font->numchars; c++) {
		x = sc->sc_font_start + (c % sc->sc_cols) * sc->sc_font->fontwidth;
		y = (c / sc->sc_cols) * sc->sc_font->fontheight;
		data = WSFONT_GLYPH(sc->sc_font->firstchar + c, sc->sc_font);
		summitfb_write4(sc, VISFX_VRAM_WRITE_DEST, (y << 16) | x);
		if (sc->sc_font->stride == 1) {
			for (i = 0; i < sc->sc_font->fontheight; i++) {
				mask = *data;
				summitfb_write4(sc, VISFX_VRAM_WRITE_DATA_INCRY,
				    mask << 24);
				data++;
			}
		} else {
			data16 = (uint16_t *)data;
			for (i = 0; i < sc->sc_font->fontheight; i++) {
				mask = *data16;
				summitfb_write4(sc, VISFX_VRAM_WRITE_DATA_INCRY,
				    mask << 16);
				data16++;
			}
		}
	}
}

static void
summitfb_putchar_fast(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct summitfb_softc *sc = scr->scr_cookie;
	int i, x, y, wi, he, xs, ys;
	uint32_t bg, fg;

	if (sc->sc_mode != WSDISPLAYIO_MODE_EMUL)
		return;

	if (!CHAR_IN_FONT(c, font))
		return;

	/* for autogenerated line drawing characters */
	if (font != sc->sc_font) {
		summitfb_putchar(cookie, row, col, c, attr);
		return;
	}

	if (row == ri->ri_crow && col == ri->ri_ccol) {
		ri->ri_flg &= ~RI_CURSOR;
	}

	wi = font->fontwidth;
	he = font->fontheight;

	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	bg = ri->ri_devcmap[(attr >> 16) & 0xf];

	/* if we're drawing a space we're done here */
	if (c == 0x20) {
		summitfb_rectfill(sc, x, y, wi, he, bg);
		return;
	}

	fg = ri->ri_devcmap[(attr >> 24) & 0x0f];

	summitfb_write_mode(sc, 0x050000c0);
	summitfb_wait_fifo(sc, 8);
	summitfb_write4(sc, VISFX_IBO, RopSrc);
	summitfb_write4(sc, VISFX_FG_COLOUR, fg);
	summitfb_write4(sc, VISFX_BG_COLOUR, bg);

	i = c - font->firstchar;
	xs = sc->sc_font_start + (i % sc->sc_cols) * sc->sc_font->fontwidth;
	ys = (i / sc->sc_cols) * sc->sc_font->fontheight;

	summitfb_wait_fifo(sc, 8);
	summitfb_write4(sc, VISFX_COPY_SRC, (xs << 16) | ys);
	summitfb_write4(sc, VISFX_COPY_WH, (wi << 16) | he);
	summitfb_write4(sc, VISFX_COPY_DST, (x << 16) | y);

}

static void
summitfb_putchar_aa(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct summitfb_softc *sc = scr->scr_cookie;
	int x, y, wi, he, rv = GC_NOPE, i, j;
	uint32_t bg, fg, tmp;
	uint8_t *data;

	if (sc->sc_mode != WSDISPLAYIO_MODE_EMUL)
		return;

	if (!CHAR_IN_FONT(c, font))
		return;

	if (row == ri->ri_crow && col == ri->ri_ccol) {
		ri->ri_flg &= ~RI_CURSOR;
	}

	wi = font->fontwidth;
	he = font->fontheight;

	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	bg = ri->ri_devcmap[(attr >> 16) & 0xf];

	if (c == 0x20) {
		summitfb_rectfill(sc, x, y, wi, he, bg);
		return;
	}

	rv = glyphcache_try(&sc->sc_gc, c, x, y, attr);
	if (rv == GC_OK)
		return;

	/*
	 * first we clear the background - we should be able to use the CBR
	 * register as constant background but so far I couldn't make that work
	 */
	summitfb_rectfill(sc, x, y, wi, he, bg);

	/*
	 * if we ever figure out how to use a constant background colour we can
	 * skip the read mode setting
	 */
	summitfb_read_mode(sc, OTC01 | BIN332F | BUFovl);
	summitfb_write_mode(sc, OTC01 | BIN332F | BUFovl);
	/* we need the foreground colour as full RGB8 */
	fg = sc->sc_palette[(attr >> 24) & 0xf];

	/*
	 * set the blending equation to
	 * src_color * src_alpha + dst_color * (1 - src_alpha)
	 */
	summitfb_write4(sc, VISFX_IBO,
	    IBO_ADD | SRC(IBO_SRC) | DST(IBO_ONE_MINUS_SRC));

	/* get the glyph */
	data = WSFONT_GLYPH(c, font);
	for (i = 0; i < he; i++) {
		/*
		 * make some room in the pipeline
		 * with just plain ROPs we can just hammer the FIFO without
		 * having to worry about overflowing it but I suspect with
		 * alpha blending enabled things may be a little slower
		 */
		summitfb_wait_fifo(sc, wi * 2);
		/* start a new line */
		summitfb_write4(sc, VISFX_VRAM_WRITE_DEST, ((y + i) << 16) | x);
		for (j = 0; j < wi; j++) {
			tmp = *data;
			/* alpha & RGB -> ARGB */
			summitfb_write4(sc, VISFX_VRAM_WRITE_DATA_INCRX,
			    (tmp << 24) | fg);
			data++;
		}
	}
	
	if (rv == GC_ADD)
		glyphcache_add(&sc->sc_gc, c, x, y);
}

static void
summitfb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct summitfb_softc *sc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;

	if (sc->sc_locked == 0 && sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {

		if (ri->ri_crow == row &&
		    ri->ri_ccol >= srccol && ri->ri_ccol < (srccol + ncols) &&
		    (ri->ri_flg & RI_CURSOR)) {
			summitfb_nuke_cursor(ri);
		}

		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		summitfb_bitblt(sc, xs, y, xd, y, width, height, RopSrc);

		if (ri->ri_crow == row &&
		    ri->ri_ccol >= dstcol && ri->ri_ccol < (dstcol + ncols))
			ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
summitfb_erasecols(void *cookie, int row, int startcol, int ncols,
    long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct summitfb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if (sc->sc_locked == 0 && sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		summitfb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);

		if (ri->ri_crow == row &&
		    ri->ri_ccol >= startcol &&
		    ri->ri_ccol < (startcol + ncols))
			ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
summitfb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct summitfb_softc *sc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	if (sc->sc_locked == 0 && sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {

		if (ri->ri_crow >= srcrow && ri->ri_crow < (srcrow + nrows) &&
		    (ri->ri_flg & RI_CURSOR)) {
			summitfb_nuke_cursor(ri);
		}

		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		summitfb_bitblt(sc, x, ys, x, yd, width, height, RopSrc);

		if (ri->ri_crow >= dstrow && ri->ri_crow < (dstrow + nrows))
			ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
summitfb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct summitfb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if (sc->sc_locked == 0 && sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		summitfb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);

		if (ri->ri_crow >= row && ri->ri_crow < (row + nrows))
			ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
summitfb_move_cursor(struct summitfb_softc *sc, int x, int y)
{
	uint32_t pos;

	sc->sc_cursor_x = x;
	x -= sc->sc_hot_x;
	sc->sc_cursor_y = y;
	y -= sc->sc_hot_y;

	if (x < 0)
		x = 0x1000 - x;
	if (y < 0)
		y = 0x1000 - y;
	pos = (x << 16) | y;
	if (sc->sc_enabled)
		pos |= 0x80000000;
	summitfb_write4(sc, VISFX_CURSOR_POS, pos);
}

static int
summitfb_do_cursor(struct summitfb_softc *sc, struct wsdisplay_cursor *cur)
{

	if (cur->which & WSDISPLAY_CURSOR_DOCUR) {
		sc->sc_enabled = cur->enable;
		cur->which |= WSDISPLAY_CURSOR_DOPOS;
	}
	if (cur->which & WSDISPLAY_CURSOR_DOHOT) {
		sc->sc_hot_x = cur->hot.x;
		sc->sc_hot_y = cur->hot.y;
		cur->which |= WSDISPLAY_CURSOR_DOPOS;
	}
	if (cur->which & WSDISPLAY_CURSOR_DOPOS) {
		summitfb_move_cursor(sc, cur->pos.x, cur->pos.y);
	}
	if (cur->which & WSDISPLAY_CURSOR_DOCMAP) {
		uint32_t rgb;
		uint8_t r[2], g[2], b[2];

		copyin(cur->cmap.blue, b, 2);
		copyin(cur->cmap.green, g, 2);
		copyin(cur->cmap.red, r, 2);
		summitfb_write4(sc, VISFX_CURSOR_INDEX, 0);
		rgb = r[0] << 16 | g[0] << 8 | b[0];
		summitfb_write4(sc, VISFX_CURSOR_BG, rgb);
		rgb = r[1] << 16 | g[1] << 8 | b[1];
		summitfb_write4(sc, VISFX_CURSOR_FG, rgb);

	}
	if (cur->which & WSDISPLAY_CURSOR_DOSHAPE) {

		uint32_t buffer[128], latch, tmp;
		int i;

		copyin(cur->mask, buffer, 512);
		summitfb_write4(sc, VISFX_CURSOR_INDEX, 0);
		for (i = 0; i < 128; i += 2) {
			latch = 0;
			tmp = buffer[i] & 0x80808080;
			latch |= tmp >> 7;
			tmp = buffer[i] & 0x40404040;
			latch |= tmp >> 5;
			tmp = buffer[i] & 0x20202020;
			latch |= tmp >> 3;
			tmp = buffer[i] & 0x10101010;
			latch |= tmp >> 1;
			tmp = buffer[i] & 0x08080808;
			latch |= tmp << 1;
			tmp = buffer[i] & 0x04040404;
			latch |= tmp << 3;
			tmp = buffer[i] & 0x02020202;
			latch |= tmp << 5;
			tmp = buffer[i] & 0x01010101;
			latch |= tmp << 7;
			summitfb_write4(sc, VISFX_CURSOR_DATA, latch);
			latch = 0;
			tmp = buffer[i + 1] & 0x80808080;
			latch |= tmp >> 7;
			tmp = buffer[i + 1] & 0x40404040;
			latch |= tmp >> 5;
			tmp = buffer[i + 1] & 0x20202020;
			latch |= tmp >> 3;
			tmp = buffer[i + 1] & 0x10101010;
			latch |= tmp >> 1;
			tmp = buffer[i + 1] & 0x08080808;
			latch |= tmp << 1;
			tmp = buffer[i + 1] & 0x04040404;
			latch |= tmp << 3;
			tmp = buffer[i + 1] & 0x02020202;
			latch |= tmp << 5;
			tmp = buffer[i + 1] & 0x01010101;
			latch |= tmp << 7;
			summitfb_write4(sc, VISFX_CURSOR_DATA, latch);
		}

		summitfb_write4(sc, VISFX_CURSOR_INDEX, 0x80);
		copyin(cur->image, buffer, 512);
		for (i = 0; i < 128; i += 2) {
			latch = 0;
			tmp = buffer[i] & 0x80808080;
			latch |= tmp >> 7;
			tmp = buffer[i] & 0x40404040;
			latch |= tmp >> 5;
			tmp = buffer[i] & 0x20202020;
			latch |= tmp >> 3;
			tmp = buffer[i] & 0x10101010;
			latch |= tmp >> 1;
			tmp = buffer[i] & 0x08080808;
			latch |= tmp << 1;
			tmp = buffer[i] & 0x04040404;
			latch |= tmp << 3;
			tmp = buffer[i] & 0x02020202;
			latch |= tmp << 5;
			tmp = buffer[i] & 0x01010101;
			latch |= tmp << 7;
			summitfb_write4(sc, VISFX_CURSOR_DATA, latch);
			latch = 0;
			tmp = buffer[i + 1] & 0x80808080;
			latch |= tmp >> 7;
			tmp = buffer[i + 1] & 0x40404040;
			latch |= tmp >> 5;
			tmp = buffer[i + 1] & 0x20202020;
			latch |= tmp >> 3;
			tmp = buffer[i + 1] & 0x10101010;
			latch |= tmp >> 1;
			tmp = buffer[i + 1] & 0x08080808;
			latch |= tmp << 1;
			tmp = buffer[i + 1] & 0x04040404;
			latch |= tmp << 3;
			tmp = buffer[i + 1] & 0x02020202;
			latch |= tmp << 5;
			tmp = buffer[i + 1] & 0x01010101;
			latch |= tmp << 7;
			summitfb_write4(sc, VISFX_CURSOR_DATA, latch);
		}
	}

	return 0;
}

static void
summitfb_set_video(struct summitfb_softc *sc, int on)
{

	if (sc->sc_video_on == on)
		return;

	sc->sc_video_on = on;

	summitfb_wait(sc);
	if (on) {
		summitfb_write4(sc, VISFX_MPC, MPC_VIDEO_ON);
	} else {
		summitfb_write4(sc, VISFX_MPC, MPC_VSYNC_OFF | MPC_HSYNC_OFF);
	}
}

static void
summitfb_copyfont(struct summitfb_softc *sc)
{
	struct sti_font *fp = &sc->sc_scr.scr_curfont;
	uint8_t *font = sc->sc_scr.scr_romfont;
	uint8_t *fontbuf, *fontdata, *src, *dst;
	struct wsdisplay_font *f;
	int bufsize, i, si;

	if (font == NULL)
		return;

	bufsize = sizeof(struct wsdisplay_font) + 32 + fp->bpc * (fp->last - fp->first);
	DPRINTF(("%s: %dx%d %d\n", __func__, fp->width, fp->height, bufsize));
	fontbuf = kmem_alloc(bufsize, KM_SLEEP);
	f = (struct wsdisplay_font *)fontbuf;
	f->name = fontbuf + sizeof(struct wsdisplay_font);
	fontdata = fontbuf + sizeof(struct wsdisplay_font) + 32;
	strcpy(fontbuf + sizeof(struct wsdisplay_font), "HP ROM");
	f->firstchar = fp->first;
	f->numchars = (fp->last + 1) - fp->first;
	f->encoding = WSDISPLAY_FONTENC_ISO;
	f->fontwidth = fp->width;
	f->fontheight = fp->height;
	f->stride = (fp->width + 7) >> 3;
	f->bitorder = WSDISPLAY_FONTORDER_L2R;
	f->byteorder = WSDISPLAY_FONTORDER_L2R;
	f->data = fontdata;
	/* skip over font struct */
	font += sizeof(struct sti_font);
	/* now copy and rearrange the glyphs into ISO order */
	/* first, copy the characters up to 0x7f */
	memcpy(fontdata, font, (0x80 - fp->first) * fp->bpc);
	/* zero 0x80 to 0x9f */
	memset(fontdata + 0x80 * fp->bpc, 0, 0x20 * fp->bpc);
	/* rearrange 0xa0 till last */
	for (i = 0xa0; i < (fp->last + 1); i++) {
		dst = fontdata + fp->bpc * i;
		si = sti_unitoroman[i - 0xa0];
		if (si != 0) {
			src = font + fp->bpc * si;
			memcpy(dst, src, fp->bpc);
		} else {
			/* no mapping - zeeo this cell */
			memset(dst, 0, fp->bpc);
		}
	}
	wsfont_add(f, 0);
}
