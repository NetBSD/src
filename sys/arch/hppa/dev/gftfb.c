/*	$NetBSD: gftfb.c,v 1.13 2024/04/01 09:48:58 macallan Exp $	*/

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
 * a native driver for HP Visualize EG PCI graphics cards
 * STI portions are from Miodrag Vallat's sti_pci.c
 */

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
#include <dev/ic/stivar.h>

#ifdef STIDEBUG
#define	DPRINTF(s)	do {	\
	if (stidebug)		\
		printf s;	\
} while(0)

extern int stidebug;
#else
#define	DPRINTF(s)	/* */
#endif

int	gftfb_match(device_t, cfdata_t, void *);
void	gftfb_attach(device_t, device_t, void *);

struct	gftfb_softc {
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
	void (*sc_putchar)(void *, int, int, u_int, long);
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
	kmutex_t sc_hwlock;
	uint32_t sc_hwmode;
#define HW_FB	0
#define HW_FILL	1
#define HW_BLIT	2
	uint32_t sc_rect_colour, sc_rect_height;
	/* cursor stuff */
	int sc_cursor_x, sc_cursor_y;
	int sc_hot_x, sc_hot_y, sc_enabled;
	int sc_video_on;
	glyphcache sc_gc;
};

CFATTACH_DECL_NEW(gftfb, sizeof(struct gftfb_softc),
    gftfb_match, gftfb_attach, NULL, NULL);

int	gftfb_readbar(struct sti_softc *, struct pci_attach_args *, u_int, int);
int	gftfb_check_rom(struct gftfb_softc *, struct pci_attach_args *);
void	gftfb_enable_rom(struct sti_softc *);
void	gftfb_disable_rom(struct sti_softc *);
void	gftfb_enable_rom_internal(struct gftfb_softc *);
void	gftfb_disable_rom_internal(struct gftfb_softc *);

void 	gftfb_setup(struct gftfb_softc *);

#define	ngle_bt458_write(memt, memh, r, v) \
	bus_space_write_stream_4(memt, memh, NGLE_REG_RAMDAC + ((r) << 2), (v) << 24)


/* XXX these really need to go into their own header */
int	sti_pci_is_console(struct pci_attach_args *, bus_addr_t *);
int	sti_rom_setup(struct sti_rom *, bus_space_tag_t, bus_space_tag_t,
	    bus_space_handle_t, bus_addr_t *, u_int);
int	sti_screen_setup(struct sti_screen *, int);
void	sti_describe_screen(struct sti_softc *, struct sti_screen *);

#define PCI_ROM_SIZE(mr)                                                \
            (PCI_MAPREG_ROM_ADDR(mr) & -PCI_MAPREG_ROM_ADDR(mr))

/* wsdisplay stuff */
static int	gftfb_ioctl(void *, void *, u_long, void *, int,
			     struct lwp *);
static paddr_t	gftfb_mmap(void *, void *, off_t, int);
static void	gftfb_init_screen(void *, struct vcons_screen *, int, long *);

static int	gftfb_putcmap(struct gftfb_softc *, struct wsdisplay_cmap *);
static int 	gftfb_getcmap(struct gftfb_softc *, struct wsdisplay_cmap *);
static void	gftfb_restore_palette(struct gftfb_softc *);
static int 	gftfb_putpalreg(struct gftfb_softc *, uint8_t, uint8_t,
			    uint8_t, uint8_t);

static void	gftfb_rectfill(struct gftfb_softc *, int, int, int, int,
			    uint32_t);
static void	gftfb_bitblt(void *, int, int, int, int, int,
			    int, int);

static void	gftfb_cursor(void *, int, int, int);
static void	gftfb_putchar(void *, int, int, u_int, long);
static void	gftfb_copycols(void *, int, int, int, int);
static void	gftfb_erasecols(void *, int, int, int, long);
static void	gftfb_copyrows(void *, int, int, int);
static void	gftfb_eraserows(void *, int, int, long);

static void	gftfb_move_cursor(struct gftfb_softc *, int, int);
static int	gftfb_do_cursor(struct gftfb_softc *, struct wsdisplay_cursor *);

static void	gftfb_set_video(struct gftfb_softc *, int);

struct wsdisplay_accessops gftfb_accessops = {
	gftfb_ioctl,
	gftfb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};

#define BA(F,C,S,A,J,B,I)						\
	(((F)<<31)|((C)<<27)|((S)<<24)|((A)<<21)|((J)<<16)|((B)<<12)|(I))

#define IBOvals(R,M,X,S,D,L,B,F)					\
	(((R)<<8)|((M)<<16)|((X)<<24)|((S)<<29)|((D)<<28)|((L)<<31)|((B)<<1)|(F))

#define	    IndexedDcd	0	/* Pixel data is indexed (pseudo) color */
#define	    Otc04	2	/* Pixels in each longword transfer (4) */
#define	    Otc32	5	/* Pixels in each longword transfer (32) */
#define	    Ots08	3	/* Each pixel is size (8)d transfer (1) */
#define	    OtsIndirect	6	/* Each bit goes through FG/BG color(8) */
#define	    AddrLong	5	/* FB address is Long aligned (pixel) */
#define	    BINovly	0x2	/* 8 bit overlay */
#define	    BINapp0I	0x0	/* Application Buffer 0, Indexed */
#define	    BINapp1I	0x1	/* Application Buffer 1, Indexed */
#define	    BINapp0F8	0xa	/* Application Buffer 0, Fractional 8-8-8 */
#define	    BINattr	0xd	/* Attribute Bitmap */
#define	    RopSrc 	0x3
#define	    RopInv 	0xc
#define	    BitmapExtent08  3	/* Each write hits ( 8) bits in depth */
#define	    BitmapExtent32  5	/* Each write hits (32) bits in depth */
#define	    DataDynamic	    0	/* Data register reloaded by direct access */
#define	    MaskDynamic	    1	/* Mask register reloaded by direct access */
#define	    MaskOtc	    0	/* Mask contains Object Count valid bits */

static inline void gftfb_wait_fifo(struct gftfb_softc *, uint32_t);

int
gftfb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args *paa = aux;

	if (PCI_VENDOR(paa->pa_id) != PCI_VENDOR_HP)
		return 0;

	if (PCI_PRODUCT(paa->pa_id) == PCI_PRODUCT_HP_VISUALIZE_EG)
		return 10;	/* beat out sti at pci */

	return 0;
}

void
gftfb_attach(device_t parent, device_t self, void *aux)
{
	struct gftfb_softc *sc = device_private(self);
	struct pci_attach_args *paa = aux;
	struct sti_rom *rom;
	struct rasops_info *ri;
	struct wsemuldisplaydev_attach_args aa;
	unsigned long defattr = 0;
	int ret, is_console = 0;

	sc->sc_dev = self;

	sc->sc_pc = paa->pa_pc;
	sc->sc_tag = paa->pa_tag;
	sc->sc_base.sc_dev = self;
	sc->sc_base.sc_enable_rom = gftfb_enable_rom;
	sc->sc_base.sc_disable_rom = gftfb_disable_rom;

	/* we can *not* be interrupted when doing colour map accesses */
	mutex_init(&sc->sc_hwlock, MUTEX_DEFAULT, IPL_HIGH);

	aprint_normal("\n");

	if (gftfb_check_rom(sc, paa) != 0)
		return;

	ret = sti_pci_is_console(paa, sc->sc_base. bases);
	if (ret != 0) {
		sc->sc_base.sc_flags |= STI_CONSOLE;
		is_console = 1;
	}
	rom = (struct sti_rom *)kmem_zalloc(sizeof(*rom), KM_SLEEP);
	rom->rom_softc = &sc->sc_base;
	ret = sti_rom_setup(rom, paa->pa_iot, paa->pa_memt, sc->sc_romh, sc->sc_base.bases, STI_CODEBASE_MAIN);
	if (ret != 0) {
		kmem_free(rom, sizeof(*rom));
		return;
	}

	sc->sc_base.sc_rom = rom;

	sc->sc_scr.scr_rom = sc->sc_base.sc_rom;
	ret = sti_screen_setup(&sc->sc_scr, STI_FBMODE);

	sc->sc_width = sc->sc_scr.scr_cfg.scr_width;
	sc->sc_height = sc->sc_scr.scr_cfg.scr_height;
	sc->sc_rect_colour = 0xf0000000;
	sc->sc_rect_height = 0;

	aprint_normal_dev(sc->sc_dev, "%s at %dx%d\n", sc->sc_scr.name, 
	    sc->sc_width, sc->sc_height);
	gftfb_setup(sc);

	sc->sc_defaultscreen_descr = (struct wsscreen_descr){
		"default",
		0, 0,
		NULL,
		8, 16,
		WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_UNDERLINE |
		      WSSCREEN_RESIZE,
		NULL
	};

	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_locked = 0;

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
	    &gftfb_accessops);
	sc->vd.init_screen = gftfb_init_screen;
	sc->vd.show_screen_cookie = &sc->sc_gc;
	sc->vd.show_screen_cb = glyphcache_adapt;

	ri = &sc->sc_console_screen.scr_ri;

	sc->sc_gc.gc_bitblt = gftfb_bitblt;
	sc->sc_gc.gc_blitcookie = sc;
	sc->sc_gc.gc_rop = RopSrc;

	if (is_console) {
		vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
		    &defattr);
		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

		sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
		sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
		sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
		sc->sc_defaultscreen_descr.ncols = ri->ri_cols;

		glyphcache_init(&sc->sc_gc, sc->sc_height + 5,
				sc->sc_scr.fbheight - sc->sc_height - 5,
				sc->sc_scr.fbwidth,
				ri->ri_font->fontwidth,
				ri->ri_font->fontheight,
				defattr);

		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);

		gftfb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height,
		    ri->ri_devcmap[(defattr >> 16) & 0xff]);

		vcons_replay_msgbuf(&sc->sc_console_screen);
	} else {
		/*
		 * since we're not the console we can postpone the rest
		 * until someone actually allocates a screen for us
		 */
		if (sc->sc_console_screen.scr_ri.ri_rows == 0) {
			/* do some minimal setup to avoid weirdnesses later */
			vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
			    &defattr);
		} else
			(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);

		glyphcache_init(&sc->sc_gc, sc->sc_height + 5,
				sc->sc_scr.fbheight - sc->sc_height - 5,
				sc->sc_scr.fbwidth,
				ri->ri_font->fontwidth,
				ri->ri_font->fontheight,
				defattr);
	}

	gftfb_restore_palette(sc);

	/* no suspend/resume support yet */
	if (!pmf_device_register(sc->sc_dev, NULL, NULL))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");

	aa.console = is_console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &gftfb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint, CFARGS_NONE);
}

/*
 * Grovel the STI ROM image.
 */
int
gftfb_check_rom(struct gftfb_softc *spc, struct pci_attach_args *pa)
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

	gftfb_disable_rom_internal(spc);
	/*
	 * Iterate over the ROM images, pick the best candidate.
	 */

	selected = (bus_addr_t)-1;
	for (offs = 0; offs < romsize; offs += subsize) {
		gftfb_enable_rom_internal(spc);
		/*
		 * Check for a valid ROM header.
		 */
		tmp = bus_space_read_4(pa->pa_memt, romh, offs + 0);
		tmp = le32toh(tmp);
		if (tmp != 0x55aa0000) {
			gftfb_disable_rom_internal(spc);
			if (offs == 0) {
				aprint_error_dev(sc->sc_dev,
				    "invalid PCI ROM header signature (%08x)\n",
				     tmp);
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
			gftfb_disable_rom_internal(spc);
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

#ifdef STIDEBUG
		gftfb_disable_rom_internal(spc);
		DPRINTF(("ROM offset %08x size %08x type %08x",
		    (u_int)offs, (u_int)subsize, tmp));
		gftfb_enable_rom_internal(spc);
#endif

		/*
		 * Check for a valid ROM data structure.
		 * We do not need it except to know what architecture the ROM
		 * code is for.
		 */

		suboffs = offs +(bus_addr_t)bus_space_read_2(pa->pa_memt, romh,
		    offs + 0x18);
		tmp = bus_space_read_4(pa->pa_memt, romh, suboffs + 0);
		tmp = le32toh(tmp);
		if (tmp != 0x50434952) {	/* PCIR */
			gftfb_disable_rom_internal(spc);
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
		gftfb_disable_rom_internal(spc);
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
#ifdef STIDEBUG
			DPRINTF((" (wrong architecture)"));
#endif
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

	gftfb_enable_rom_internal(spc);
	offs = selected +
	    (bus_addr_t)bus_space_read_2(pa->pa_memt, romh, selected + 0x0e);
	for (i = 0; i < STI_REGION_MAX; i++) {
		rc = gftfb_readbar(sc, pa, i,
		    bus_space_read_1(pa->pa_memt, romh, offs + i));
		if (rc != 0)
			goto fail;
	}

	/*
	 * Find out where the STI ROM itself lies, and its size.
	 */

	offs = selected +
	    (bus_addr_t)bus_space_read_4(pa->pa_memt, romh, selected + 0x08);
	stiromsize = (bus_addr_t)bus_space_read_4(pa->pa_memt, romh,
	    offs + 0x18);
	stiromsize = le32toh(stiromsize);
	gftfb_disable_rom_internal(spc);

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
 	gftfb_disable_rom_internal(spc);
	sc->sc_flags &= ~STI_ROM_ENABLED;

	return 0;

fail:
	bus_space_unmap(pa->pa_memt, romh, romsize);
fail2:
	gftfb_disable_rom_internal(spc);

	return rc;
}

/*
 * Decode a BAR register.
 */
int
gftfb_readbar(struct sti_softc *sc, struct pci_attach_args *pa, u_int region,
    int bar)
{
	bus_addr_t addr;
	bus_size_t size;
	uint32_t cf;
	int rc;

	if (bar == 0) {
		sc->bases[region] = 0;
		return (0);
	}

#ifdef DIAGNOSTIC
	if (bar < PCI_MAPREG_START || bar > PCI_MAPREG_PPB_END) {
		gftfb_disable_rom(sc);
		printf("%s: unexpected bar %02x for region %d\n",
		    device_xname(sc->sc_dev), bar, region);
		gftfb_enable_rom(sc);
	}
#endif

	cf = pci_conf_read(pa->pa_pc, pa->pa_tag, bar);

	rc = pci_mapreg_info(pa->pa_pc, pa->pa_tag, bar, PCI_MAPREG_TYPE(cf),
	    &addr, &size, NULL);

	if (rc != 0) {
		gftfb_disable_rom(sc);
		aprint_error_dev(sc->sc_dev, "invalid bar %02x for region %d\n",
		    bar, region);
		gftfb_enable_rom(sc);
		return (rc);
	}

	sc->bases[region] = addr;
	return (0);
}

/*
 * Enable PCI ROM.
 */
void
gftfb_enable_rom_internal(struct gftfb_softc *spc)
{
	pcireg_t address;

	KASSERT(spc != NULL);

	address = pci_conf_read(spc->sc_pc, spc->sc_tag, PCI_MAPREG_ROM);
	address |= PCI_MAPREG_ROM_ENABLE;
	pci_conf_write(spc->sc_pc, spc->sc_tag, PCI_MAPREG_ROM, address);
}

void
gftfb_enable_rom(struct sti_softc *sc)
{
	struct gftfb_softc *spc = device_private(sc->sc_dev);

	if (!ISSET(sc->sc_flags, STI_ROM_ENABLED)) {
		gftfb_enable_rom_internal(spc);
	}
	SET(sc->sc_flags, STI_ROM_ENABLED);
}

/*
 * Disable PCI ROM.
 */
void
gftfb_disable_rom_internal(struct gftfb_softc *spc)
{
	pcireg_t address;

	KASSERT(spc != NULL);

	address = pci_conf_read(spc->sc_pc, spc->sc_tag, PCI_MAPREG_ROM);
	address &= ~PCI_MAPREG_ROM_ENABLE;
	pci_conf_write(spc->sc_pc, spc->sc_tag, PCI_MAPREG_ROM, address);
}

void
gftfb_disable_rom(struct sti_softc *sc)
{
	struct gftfb_softc *spc = device_private(sc->sc_dev);

	if (ISSET(sc->sc_flags, STI_ROM_ENABLED)) {
		gftfb_disable_rom_internal(spc);
	}
	CLR(sc->sc_flags, STI_ROM_ENABLED);
}

static inline void
gftfb_wait(struct gftfb_softc *sc)
{
	struct sti_rom *rom = sc->sc_base.sc_rom;
	bus_space_tag_t memt = rom->memt;
	bus_space_handle_t memh = rom->regh[2];
	uint8_t stat;

	do {
		stat = bus_space_read_1(memt, memh, NGLE_REG_15b0);
		if (stat == 0)
			stat = bus_space_read_1(memt, memh, NGLE_REG_15b0);
	} while (stat != 0);
}

static inline void
gftfb_setup_fb(struct gftfb_softc *sc)
{
	struct sti_rom *rom = sc->sc_base.sc_rom;
	bus_space_tag_t memt = rom->memt;
	bus_space_handle_t memh = rom->regh[2];

	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_10, 0x13601000);
	bus_space_write_stream_4(memt, memh, NGLE_REG_14, 0x83000300);
	gftfb_wait(sc);
	bus_space_write_1(memt, memh, NGLE_REG_16b1, 1);
	sc->sc_hwmode = HW_FB;
}

void
gftfb_setup(struct gftfb_softc *sc)
{
	struct sti_rom *rom = sc->sc_base.sc_rom;
	bus_space_tag_t memt = rom->memt;
	bus_space_handle_t memh = rom->regh[2];
	int i;	

	sc->sc_hwmode = HW_FB;
	sc->sc_hot_x = 0;
	sc->sc_hot_y = 0;
	sc->sc_enabled = 0;
	sc->sc_video_on = 1;

	sc->sc_rect_colour = 0xf0000000;
	sc->sc_rect_height = 0;

	/* set Bt458 read mask register to all planes */
	gftfb_wait(sc);
	ngle_bt458_write(memt, memh, 0x08, 0x04);
	ngle_bt458_write(memt, memh, 0x0a, 0xff);

	gftfb_setup_fb(sc);

	/* attr. planes */
	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_11, 0x2ea0d000);
	bus_space_write_stream_4(memt, memh, NGLE_REG_14, 0x23000302);
	bus_space_write_stream_4(memt, memh, NGLE_REG_12, NGLE_ARTIST_CMAP0);
	bus_space_write_stream_4(memt, memh, NGLE_REG_8, 0xffffffff);

	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_6, 0x00000000);
	bus_space_write_stream_4(memt, memh, NGLE_REG_9,
	    (sc->sc_scr.scr_cfg.scr_width << 16) | sc->sc_scr.scr_cfg.scr_height);
	/*
	 * blit into offscreen memory to force flush previous - apparently 
	 * some chips have a bug this works around
	 */
	bus_space_write_stream_4(memt, memh, NGLE_REG_6, 0x05000000);
	bus_space_write_stream_4(memt, memh, NGLE_REG_9, 0x00040001);

	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_12, 0x00000000);

	gftfb_setup_fb(sc);

	/* make sure video output is enabled */
	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_21,
	    bus_space_read_stream_4(memt, memh, NGLE_REG_21) | 0x0a000000);
	bus_space_write_stream_4(memt, memh, NGLE_REG_27,
	    bus_space_read_stream_4(memt, memh, NGLE_REG_27) | 0x00800000);

	/* initialize cursor sprite */
	gftfb_wait(sc);
	
	/* cursor mask */
	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_14, 0x300);
	bus_space_write_stream_4(memt, memh, NGLE_REG_13, 0xffffffff);
	bus_space_write_stream_4(memt, memh, NGLE_REG_11, 0x28A07000);
	bus_space_write_stream_4(memt, memh, NGLE_REG_3, 0);
	for (i = 0; i < 64; i++) {
		bus_space_write_stream_4(memt, memh, NGLE_REG_4, 0xffffffff);
		bus_space_write_stream_4(memt, memh, NGLE_REG_5, 0xffffffff);
	}

	/* cursor image */
	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_14, 0x300);
	bus_space_write_stream_4(memt, memh, NGLE_REG_13, 0xffffffff);
	bus_space_write_stream_4(memt, memh, NGLE_REG_11, 0x28A06000);
	bus_space_write_stream_4(memt, memh, NGLE_REG_3, 0);
	for (i = 0; i < 64; i++) {
		bus_space_write_stream_4(memt, memh, NGLE_REG_4, 0xff00ff00);
		bus_space_write_stream_4(memt, memh, NGLE_REG_5, 0xff00ff00);
	}

	/* colour map */
	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_10, 0xBBE0F000);
	bus_space_write_stream_4(memt, memh, NGLE_REG_14, 0x03000300);
	bus_space_write_stream_4(memt, memh, NGLE_REG_13, 0xffffffff);
	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_3, 0);
	bus_space_write_stream_4(memt, memh, NGLE_REG_4, 0);
	bus_space_write_stream_4(memt, memh, NGLE_REG_4, 0);
	bus_space_write_stream_4(memt, memh, NGLE_REG_4, 0x000000ff);	/* BG */
	bus_space_write_stream_4(memt, memh, NGLE_REG_4, 0x00ff0000);	/* FG */
	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_2, 0);
	bus_space_write_stream_4(memt, memh, NGLE_REG_26, 0x80008004);
	gftfb_setup_fb(sc);	

	gftfb_move_cursor(sc, 100, 100);

}

static int
gftfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct gftfb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_STI;
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
		wdf = (void *)data;
		wdf->height = ms->scr_ri.ri_height;
		wdf->width = ms->scr_ri.ri_width;
		wdf->depth = ms->scr_ri.ri_depth;
		wdf->cmsize = 256;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return gftfb_getcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return gftfb_putcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = 2048;
		return 0;

	case WSDISPLAYIO_SMODE: {
		int new_mode = *(int*)data;
		if (new_mode != sc->sc_mode) {
			sc->sc_mode = new_mode;
			if(new_mode == WSDISPLAYIO_MODE_EMUL) {
				gftfb_setup(sc);
				gftfb_restore_palette(sc);
				glyphcache_wipe(&sc->sc_gc);
				gftfb_rectfill(sc, 0, 0, sc->sc_width,
				    sc->sc_height, ms->scr_ri.ri_devcmap[
				    (ms->scr_defattr >> 16) & 0xff]);
				vcons_redraw_screen(ms);
				gftfb_set_video(sc, 1);
			}
		}
		}
		return 0;

	case WSDISPLAYIO_GET_FBINFO:
		{
			struct wsdisplayio_fbinfo *fbi = data;
			int ret;

			ret = wsdisplayio_get_fbinfo(&ms->scr_ri, fbi);
			fbi->fbi_fbsize = sc->sc_scr.fbheight * 2048;
			return ret;
		}

	case WSDISPLAYIO_GCURPOS:
		{
			struct wsdisplay_curpos *cp = (void *)data;

			cp->x = sc->sc_cursor_x;
			cp->y = sc->sc_cursor_y;
		}
		return 0;

	case WSDISPLAYIO_SCURPOS:
		{
			struct wsdisplay_curpos *cp = (void *)data;

			gftfb_move_cursor(sc, cp->x, cp->y);
		}
		return 0;

	case WSDISPLAYIO_GCURMAX:
		{
			struct wsdisplay_curpos *cp = (void *)data;

			cp->x = 64;
			cp->y = 64;
		}
		return 0;

	case WSDISPLAYIO_SCURSOR:
		{
			struct wsdisplay_cursor *cursor = (void *)data;

			return gftfb_do_cursor(sc, cursor);
		}

	case WSDISPLAYIO_SVIDEO:
		gftfb_set_video(sc, *(int *)data);
		return 0;
	case WSDISPLAYIO_GVIDEO:
		return sc->sc_video_on ? 
		    WSDISPLAYIO_VIDEO_ON : WSDISPLAYIO_VIDEO_OFF;
	}
	return EPASSTHROUGH;
}

static paddr_t
gftfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct gftfb_softc *sc = vd->cookie;
	struct sti_rom *rom = sc->sc_base.sc_rom;
	paddr_t pa;

	if (offset < 0 || offset >= sc->sc_scr.fblen)
		return -1;

	if (sc->sc_mode != WSDISPLAYIO_MODE_DUMBFB)
		return -1;

	pa = bus_space_mmap(rom->memt, sc->sc_scr.fbaddr, offset, prot,
	    BUS_SPACE_MAP_LINEAR);
	return pa;
}

static void
gftfb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct gftfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = 8;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = 2048;
	ri->ri_flg = RI_CENTER | RI_8BIT_IS_RGB | 
		     RI_ENABLE_ALPHA | RI_PREFER_ALPHA;

	ri->ri_bits = (void *)sc->sc_scr.fbaddr;
	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_UNDERLINE |
		      WSSCREEN_RESIZE;
	scr->scr_flags |= VCONS_LOADFONT;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
	sc->sc_putchar = ri->ri_ops.putchar;
	ri->ri_ops.copyrows = gftfb_copyrows;
	ri->ri_ops.copycols = gftfb_copycols;
	ri->ri_ops.eraserows = gftfb_eraserows;
	ri->ri_ops.erasecols = gftfb_erasecols;
	ri->ri_ops.cursor = gftfb_cursor;
	ri->ri_ops.putchar = gftfb_putchar;
}

static int
gftfb_putcmap(struct gftfb_softc *sc, struct wsdisplay_cmap *cm)
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
		gftfb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
gftfb_getcmap(struct gftfb_softc *sc, struct wsdisplay_cmap *cm)
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
gftfb_restore_palette(struct gftfb_softc *sc)
{
	uint8_t cmap[768];
	int i, j;

	j = 0;
	rasops_get_cmap(&sc->sc_console_screen.scr_ri, cmap, sizeof(cmap));
	for (i = 0; i < 256; i++) {
		sc->sc_cmap_red[i] = cmap[j];
		sc->sc_cmap_green[i] = cmap[j + 1];
		sc->sc_cmap_blue[i] = cmap[j + 2];
		gftfb_putpalreg(sc, i, cmap[j], cmap[j + 1], cmap[j + 2]);
		j += 3;
	}
}

static int
gftfb_putpalreg(struct gftfb_softc *sc, uint8_t idx, uint8_t r, uint8_t g,
    uint8_t b)
{
	struct sti_rom *rom = sc->sc_base.sc_rom;
	bus_space_tag_t memt = rom->memt;
	bus_space_handle_t memh = rom->regh[2];

	mutex_enter(&sc->sc_hwlock);
	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_10, 0xbbe0f000);
	bus_space_write_stream_4(memt, memh, NGLE_REG_14, 0x03000300);
	bus_space_write_stream_4(memt, memh, NGLE_REG_13, 0xffffffff);

	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_3,
	    0x400 | (idx << 2));
	bus_space_write_stream_4(memt, memh, NGLE_REG_4,
	    (r << 16) | (g << 8) | b);

	bus_space_write_stream_4(memt, memh, NGLE_REG_2, 0x400);
	bus_space_write_stream_4(memt, memh, NGLE_REG_26, 0x80000100);
	gftfb_setup_fb(sc);
	mutex_exit(&sc->sc_hwlock);
	return 0;
}

static inline void
gftfb_wait_fifo(struct gftfb_softc *sc, uint32_t slots)
{
	struct sti_rom *rom = sc->sc_base.sc_rom;
	bus_space_tag_t memt = rom->memt;
	bus_space_handle_t memh = rom->regh[2];
	uint32_t reg;

	do {
		reg = bus_space_read_stream_4(memt, memh, NGLE_REG_34);
	} while (reg < slots);
}

static void
gftfb_real_rectfill(struct gftfb_softc *sc, int x, int y, int wi, int he,
		      uint32_t bg)
{
	struct sti_rom *rom = sc->sc_base.sc_rom;
	bus_space_tag_t memt = rom->memt;
	bus_space_handle_t memh = rom->regh[2];

	if (sc->sc_hwmode != HW_FILL) {
		gftfb_wait_fifo(sc, 4);
		/* transfer data */
		bus_space_write_stream_4(memt, memh, NGLE_REG_8, 0xffffffff);
		/* plane mask */
		bus_space_write_stream_4(memt, memh, NGLE_REG_13, 0xff);
		/* bitmap op */
		bus_space_write_stream_4(memt, memh, NGLE_REG_14, 
		    IBOvals(RopSrc, 0, BitmapExtent08, 0, DataDynamic, MaskOtc, 0, 0));
		/* dst bitmap access */
		bus_space_write_stream_4(memt, memh, NGLE_REG_11,
		    BA(IndexedDcd, Otc32, OtsIndirect, AddrLong, 0, BINapp0I, 0));
		sc->sc_hwmode = HW_FILL;
	}
	gftfb_wait_fifo(sc, 3);
	bus_space_write_stream_4(memt, memh, NGLE_REG_35, bg);
	/* dst XY */
	bus_space_write_stream_4(memt, memh, NGLE_REG_6, (x << 16) | y);
	/* len XY start */
	bus_space_write_stream_4(memt, memh, NGLE_REG_9, (wi << 16) | he);

}

static void
gftfb_rectfill(struct gftfb_softc *sc, int x, int y, int wi, int he,
		      uint32_t bg)
{
	/*
	 * For some reason my 4MB VisEG always draws rectangles at least 32
	 * pixels wide - no idea why, the bitblt command doesn't have this
	 * problem.
	 * So, as a workaround, we draw a 50xFontHeight rectangle to the right
	 * of the visible fb, keep track of the colour so we don't need to
	 * redraw every time, and bitblt the portion we need
	 */
	if (wi < 50) {
		if ((bg != sc->sc_rect_colour) ||
		    (he > sc->sc_rect_height)) {
			gftfb_real_rectfill(sc, sc->sc_width + 10, 0, 50, 
			    he, bg);
			sc->sc_rect_colour = bg;
			sc->sc_rect_height = he;
		}
		gftfb_bitblt(sc, sc->sc_width + 10, 0, x, y, wi, he, RopSrc);
	} else
		gftfb_real_rectfill(sc, x, y, wi, he, bg);
}

static void
gftfb_bitblt(void *cookie, int xs, int ys, int xd, int yd, int wi,
			    int he, int rop)
{
	struct gftfb_softc *sc = cookie;
	struct sti_rom *rom = sc->sc_base.sc_rom;
	bus_space_tag_t memt = rom->memt;
	bus_space_handle_t memh = rom->regh[2];

	if (sc->sc_hwmode != HW_BLIT) {
		gftfb_wait(sc);
		bus_space_write_stream_4(memt, memh, NGLE_REG_10, 0x13a01000);
		sc->sc_hwmode = HW_BLIT;
	}
	gftfb_wait_fifo(sc, 5);
	bus_space_write_stream_4(memt, memh, NGLE_REG_14, ((rop << 8) & 0xf00) | 0x23000000);
	bus_space_write_stream_4(memt, memh, NGLE_REG_13, 0xff);
	bus_space_write_stream_4(memt, memh, NGLE_REG_24, (xs << 16) | ys);
	bus_space_write_stream_4(memt, memh, NGLE_REG_7, (wi << 16) | he);
	bus_space_write_stream_4(memt, memh, NGLE_REG_25, (xd << 16) | yd);
}

static void
gftfb_nuke_cursor(struct rasops_info *ri)
{
	struct vcons_screen *scr = ri->ri_hw;
	struct gftfb_softc *sc = scr->scr_cookie;
	int wi, he, x, y;
		
	if (ri->ri_flg & RI_CURSOR) {
		wi = ri->ri_font->fontwidth;
		he = ri->ri_font->fontheight;
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		gftfb_bitblt(sc, x, y, x, y, wi, he, RopInv);
		ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
gftfb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct gftfb_softc *sc = scr->scr_cookie;
	int x, y, wi, he;
	
	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;
	
	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		if (on) {
			if (ri->ri_flg & RI_CURSOR) {
				gftfb_nuke_cursor(ri);
			}
			x = col * wi + ri->ri_xorigin;
			y = row * he + ri->ri_yorigin;
			gftfb_bitblt(sc, x, y, x, y, wi, he, RopInv);
			ri->ri_flg |= RI_CURSOR;
		}
		ri->ri_crow = row;
		ri->ri_ccol = col;
	} else
	{
		ri->ri_crow = row;
		ri->ri_ccol = col;
		ri->ri_flg &= ~RI_CURSOR;
	}

}

static void
gftfb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct gftfb_softc *sc = scr->scr_cookie;
	int x, y, wi, he, rv = GC_NOPE;
	uint32_t bg;

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
		gftfb_rectfill(sc, x, y, wi, he, bg);
		return;
	}

	rv = glyphcache_try(&sc->sc_gc, c, x, y, attr);
	if (rv == GC_OK)
		return;

	if (sc->sc_hwmode != HW_FB) gftfb_setup_fb(sc);
	sc->sc_putchar(cookie, row, col, c, attr);

	if (rv == GC_ADD)
		glyphcache_add(&sc->sc_gc, c, x, y);
}

static void
gftfb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct gftfb_softc *sc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;
	
	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		if (ri->ri_crow == row && 
		   (ri->ri_ccol >= srccol && ri->ri_ccol < (srccol + ncols)) &&
		   (ri->ri_flg & RI_CURSOR)) {
			gftfb_nuke_cursor(ri);
		}

		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		gftfb_bitblt(sc, xs, y, xd, y, width, height, RopSrc);
		if (ri->ri_crow == row && 
		   (ri->ri_ccol >= dstcol && ri->ri_ccol < (dstcol + ncols)))
			ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
gftfb_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct gftfb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;
	
	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		gftfb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
		if (ri->ri_crow == row && 
		   (ri->ri_ccol >= startcol && ri->ri_ccol < (startcol + ncols)))
			ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
gftfb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct gftfb_softc *sc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		if ((ri->ri_crow >= srcrow && ri->ri_crow < (srcrow + nrows)) &&
		   (ri->ri_flg & RI_CURSOR)) {
			gftfb_nuke_cursor(ri);
		}
		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		gftfb_bitblt(sc, x, ys, x, yd, width, height, RopSrc);
		if (ri->ri_crow >= dstrow && ri->ri_crow < (dstrow + nrows))
			ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
gftfb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct gftfb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;
	
	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		gftfb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);

		if (ri->ri_crow >= row && ri->ri_crow < (row + nrows))
			ri->ri_flg &= ~RI_CURSOR;
	}
}

/*
 * cursor sprite handling
 * like most hw info, xf86 3.3 -> nglehdw.h was used as documentation
 * problem is, the PCI EG doesn't quite behave like an S9000_ID_ARTIST
 * the cursor position register bahaves like the one on HCRX while using
 * the same address as Artist, incuding the enable bit and weird handling
 * of negative coordinates. The rest of it, colour map, sprite image etc.,
 * behave like Artist.
 */
 
static void
gftfb_move_cursor(struct gftfb_softc *sc, int x, int y)
{
	struct sti_rom *rom = sc->sc_base.sc_rom;
	bus_space_tag_t memt = rom->memt;
	bus_space_handle_t memh = rom->regh[2];
	uint32_t pos;

	sc->sc_cursor_x = x;
	x -= sc->sc_hot_x;
	sc->sc_cursor_y = y;
	y -= sc->sc_hot_y;

	if (x < 0) x = 0x1000 - x;
	if (y < 0) y = 0x1000 - y;
	pos = (x << 16) | y;
	if (sc->sc_enabled) pos |= 0x80000000;
	gftfb_wait(sc);
	bus_space_write_stream_4(memt, memh, NGLE_REG_17, pos);
	bus_space_write_stream_4(memt, memh, NGLE_REG_18, 0x80);
}

static int
gftfb_do_cursor(struct gftfb_softc *sc, struct wsdisplay_cursor *cur)
{
	struct sti_rom *rom = sc->sc_base.sc_rom;
	bus_space_tag_t memt = rom->memt;
	bus_space_handle_t memh = rom->regh[2];

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

		gftfb_move_cursor(sc, cur->pos.x, cur->pos.y);
	}
	if (cur->which & WSDISPLAY_CURSOR_DOCMAP) {
		uint32_t rgb;
		uint8_t r[2], g[2], b[2];

		copyin(cur->cmap.blue, b, 2);
		copyin(cur->cmap.green, g, 2);
		copyin(cur->cmap.red, r, 2);
		mutex_enter(&sc->sc_hwlock);
		gftfb_wait(sc);
		bus_space_write_stream_4(memt, memh, NGLE_REG_10, 0xBBE0F000);
		bus_space_write_stream_4(memt, memh, NGLE_REG_14, 0x03000300);
		bus_space_write_stream_4(memt, memh, NGLE_REG_13, 0xffffffff);
		gftfb_wait(sc);
		bus_space_write_stream_4(memt, memh, NGLE_REG_3, 0);
		bus_space_write_stream_4(memt, memh, NGLE_REG_4, 0);
		bus_space_write_stream_4(memt, memh, NGLE_REG_4, 0);
		rgb = (r[0] << 16) | (g[0] << 8) | b[0];
		bus_space_write_stream_4(memt, memh, NGLE_REG_4, rgb);	/* BG */
		rgb = (r[1] << 16) | (g[1] << 8) | b[1];
		bus_space_write_stream_4(memt, memh, NGLE_REG_4, rgb);	/* FG */
		bus_space_write_stream_4(memt, memh, NGLE_REG_2, 0);
		bus_space_write_stream_4(memt, memh, NGLE_REG_26, 0x80008004);
		gftfb_setup_fb(sc);	
		mutex_exit(&sc->sc_hwlock);

	}
	if (cur->which & WSDISPLAY_CURSOR_DOSHAPE) {
		uint32_t buffer[128], latch, tmp;
		int i;

		copyin(cur->mask, buffer, 512);
		gftfb_wait(sc);
		bus_space_write_stream_4(memt, memh, NGLE_REG_14, 0x300);
		bus_space_write_stream_4(memt, memh, NGLE_REG_13, 0xffffffff);
		bus_space_write_stream_4(memt, memh, NGLE_REG_11, 0x28A07000);
		bus_space_write_stream_4(memt, memh, NGLE_REG_3, 0);
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
			bus_space_write_stream_4(memt, memh, NGLE_REG_4, latch);
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
			bus_space_write_stream_4(memt, memh, NGLE_REG_5, latch);
		}

		copyin(cur->image, buffer, 512);
		gftfb_wait(sc);
		bus_space_write_stream_4(memt, memh, NGLE_REG_14, 0x300);
		bus_space_write_stream_4(memt, memh, NGLE_REG_13, 0xffffffff);
		bus_space_write_stream_4(memt, memh, NGLE_REG_11, 0x28A06000);
		bus_space_write_stream_4(memt, memh, NGLE_REG_3, 0);
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
			bus_space_write_stream_4(memt, memh, NGLE_REG_4, latch);
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
			bus_space_write_stream_4(memt, memh, NGLE_REG_5, latch);
		}
		gftfb_setup_fb(sc);
	}

	return 0;
}

static void
gftfb_set_video(struct gftfb_softc *sc, int on)
{
	struct sti_rom *rom = sc->sc_base.sc_rom;
	bus_space_tag_t memt = rom->memt;
	bus_space_handle_t memh = rom->regh[2];

	if (sc->sc_video_on == on)
		return;
		
	sc->sc_video_on = on;

	gftfb_wait(sc);
	if (on) {
		bus_space_write_stream_4(memt, memh, NGLE_REG_21,
		    bus_space_read_stream_4(memt, memh, NGLE_REG_21) | 0x0a000000);
		bus_space_write_stream_4(memt, memh, NGLE_REG_27,
		    bus_space_read_stream_4(memt, memh, NGLE_REG_27) | 0x00800000);
	} else {
		bus_space_write_stream_4(memt, memh, NGLE_REG_21,
		    bus_space_read_stream_4(memt, memh, NGLE_REG_21) &  ~0x0a000000);
		bus_space_write_stream_4(memt, memh, NGLE_REG_27,
		    bus_space_read_stream_4(memt, memh, NGLE_REG_27) & ~0x00800000);
	}
}
