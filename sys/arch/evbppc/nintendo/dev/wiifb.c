/* $NetBSD: wiifb.c,v 1.1 2026/01/09 22:54:30 jmcneill Exp $ */

/*-
 * Copyright (c) 2024-2025 Jared McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wiifb.c,v 1.1 2026/01/09 22:54:30 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/wii.h>
#include <machine/wiiu.h>
#include <powerpc/spr.h>
#include <powerpc/oea/spr.h>
#include <powerpc/oea/hid.h>

#include <dev/videomode/videomode.h>
#include <dev/wsfb/genfbvar.h>

#include "mainbus.h"
#include "vireg.h"
#include "gxreg.h"

#define WIIFB_RGB_WIDTH			640
#define WIIFB_RGB_HEIGHT		480
#define WIIFB_RGB_BPP			32

#define WIIFB_ERROR_BLINK_INTERVAL	1000000

#define WIIFB_TOP_BOTTOM_BORDER		16
#define WIIFB_EFFECTIVE_START(p, w)	\
	((uintptr_t)(p) + WIIFB_TOP_BOTTOM_BORDER * (w) * 2)
#define WIIFB_EFFECTIVE_HEIGHT(h)	\
	((h) - WIIFB_TOP_BOTTOM_BORDER * 2)

#define WIIFB_FIFO_SIZE			(256 * 1024)

#define IBM750CL_SPR_HID2		920
#define  IBM750CL_SPR_HID2_WPE		0x40000000	/* Write pipe enable */
#define IBM750CL_SPR_WPAR		921

struct wiifb_mode {
	const char *		name;
	u_int			width;
	u_int			height;
	u_int			lines;
};

static uint32_t wiifb_devcmap[16] = {
	0x00800080,	/* Black */
	0x1dff1d6b,	/* Blue */
	0x4b554b4a,	/* Green */
	0x80808080,	/* Cyan */
	0x4c544cff,	/* Red */
	0x3aaa34b5,	/* Magenta */
	0x7140718a,	/* Brown */
	0xff80ff80,	/* White */
	0x80808080,	/* Gray */
	0xc399c36a,	/* Bright Blue */
	0xd076d074,	/* Bright Green */
	0x80808080,	/* Bright Cyan */
	0x4c544cff,	/* Bright Red */
	0x3aaa34b5,	/* Bright Magenta */
	0xe100e194,	/* Bright Yellow */
	0xff80ff80	/* Bright White */
};

#define WIIFB_MODE_INDEX(fmt, interlaced)	((fmt << 1) | interlaced)
	
static const struct wiifb_mode wiifb_modes[] = {
	[WIIFB_MODE_INDEX(VI_DCR_FMT_NTSC, 0)] = {
		.name = "NTSC 480p",
		.width = 640,
		.height = 480,
		.lines = 525,
	},
	[WIIFB_MODE_INDEX(VI_DCR_FMT_NTSC, 1)] = {
		.name = "NTSC 480i",
		.width = 640,
		.height = 480,
		.lines = 525,
	},
	[WIIFB_MODE_INDEX(VI_DCR_FMT_PAL, 1)] = {
		.name = "PAL 576i",
		.width = 640,
		.height = 574,
		.lines = 625,
	},

};
#define WIIFB_NMODES	__arraycount(wiifb_modes)

struct wiifb_dma {
	bus_dmamap_t		dma_map;
	bus_dma_tag_t		dma_tag;
	bus_size_t		dma_size;
	bus_dma_segment_t	dma_segs[1];
	int			dma_nsegs;
	void			*dma_addr;
};

struct wiifb_softc {
	struct genfb_softc	sc_gen;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;

	void			*sc_bits;

	uint8_t			sc_format;
	bool			sc_interlaced;

	const struct wiifb_mode	*sc_curmode;

	u_int			sc_wsmode;

	volatile uint32_t	*sc_efb;
	volatile uint16_t	*sc_cp;
	volatile uint16_t	*sc_pe;
	volatile uint32_t	*sc_pi;
	gx_wgpipe_t		*sc_wgpipe;

	struct wiifb_dma	sc_rgb;
	struct wiifb_dma	sc_fifo;

	uint16_t		sc_token;
};

#define	RD2(sc, reg)		\
	bus_space_read_2((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR2(sc, reg, val)	\
	bus_space_write_2((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define	WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

#define CP_WRITE(sc, off, val)	(sc)->sc_cp[(off)] = (val)
#define CP_READ(sc, off)	(sc)->sc_cp[(off)]

#define PE_WRITE(sc, off, val)	(sc)->sc_pe[(off)] = (val)
#define PE_READ(sc, off)	(sc)->sc_pe[(off)]

#define PI_WRITE(sc, off, val)	(sc)->sc_pi[(off)] = (val)
#define PI_READ(sc, off)	(sc)->sc_pi[(off)]

static int	wiifb_match(device_t, cfdata_t, void *);
static void	wiifb_attach(device_t, device_t, void *);

static void	wiifb_accel_init(struct wiifb_softc *);
static int	wiifb_vi_intr(void *);
static void	wiifb_vi_refresh(void *);

static void	wiifb_init(struct wiifb_softc *);
static void	wiifb_set_mode(struct wiifb_softc *, uint8_t, bool);
static void	wiifb_set_fb(struct wiifb_softc *);
static void	wiifb_clear_xfb(struct wiifb_softc *);

static int	wiifb_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t	wiifb_mmap(void *, void *, off_t, int);

static struct genfb_ops wiifb_ops = {
	.genfb_ioctl = wiifb_ioctl,
	.genfb_mmap = wiifb_mmap,
};

CFATTACH_DECL_NEW(wiifb, sizeof(struct wiifb_softc),
	wiifb_match, wiifb_attach, NULL, NULL);

static int
wiifb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	return !wiiu_native && strcmp(maa->maa_name, "genfb") == 0;
}

static void
wiifb_attach(device_t parent, device_t self, void *aux)
{
	struct wiifb_softc *sc = device_private(self);
	prop_dictionary_t dict = device_properties(self);
	struct mainbus_attach_args *maa = aux;
	int error;

	sc->sc_gen.sc_dev = self;
	sc->sc_bst = maa->maa_bst;
	error = bus_space_map(sc->sc_bst, VI_BASE, VI_SIZE, 0, &sc->sc_bsh);
	if (error != 0) {
		panic("couldn't map registers");
	}
	sc->sc_bits = mapiodev(XFB_START, XFB_SIZE, true);
	sc->sc_wsmode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_dmat = maa->maa_dmat;

	wiifb_clear_xfb(sc);

	wiifb_init(sc);
	wiifb_set_mode(sc, sc->sc_format, sc->sc_interlaced);

	prop_dictionary_set_uint32(dict, "width", sc->sc_curmode->width);
	prop_dictionary_set_uint32(dict, "height",
	    WIIFB_EFFECTIVE_HEIGHT(sc->sc_curmode->height));
	prop_dictionary_set_uint8(dict, "depth", 16);
	prop_dictionary_set_uint32(dict, "address", XFB_START);
	prop_dictionary_set_uint32(dict, "virtual_address",
	    WIIFB_EFFECTIVE_START(sc->sc_bits, sc->sc_curmode->width));
	prop_dictionary_set_uint64(dict, "devcmap", (uintptr_t)wiifb_devcmap);

	genfb_init(&sc->sc_gen);

	aprint_naive("\n");
	aprint_normal(": %s\n", sc->sc_curmode->name);

	genfb_cnattach();
	prop_dictionary_set_bool(dict, "is_console", true);
	genfb_attach(&sc->sc_gen, &wiifb_ops);

	wiifb_accel_init(sc);
}

static void
wiifb_clear_xfb(struct wiifb_softc *sc)
{
	u_int offset;
	uint32_t *p;

	/*
	 * Paint the entire XFB black. Use 4-byte accesses as the Wii will
	 * ignore 1- and 2- byte writes to uncached memory.
	 */
	for (p = sc->sc_bits, offset = 0;
	     offset < XFB_SIZE;
	     offset += 4, p++) {
		*p = wiifb_devcmap[0];
	}
}

static int
wiifb_vi_init(struct wiifb_softc *sc)
{
	device_t dev = sc->sc_gen.sc_dev;
	void *ih;

	WR4(sc, VI_DI0, VI_DI_ENB |
			__SHIFTIN(1, VI_DI_VCT) |
			__SHIFTIN(1, VI_DI_HCT));
	WR4(sc, VI_DI1, 0);
	WR4(sc, VI_DI2, 0);
	WR4(sc, VI_DI3, 0);

	ih = intr_establish_xname(PI_IRQ_VI, IST_LEVEL, IPL_TTY,
	    wiifb_vi_intr, sc, device_xname(dev));
	if (ih == NULL) {
		aprint_error_dev(dev, "failed to install VI intr handler\n");
		return EIO;
	}

	return 0;
}

static int
wiifb_dma_alloc(struct wiifb_softc *sc, bus_size_t size, bus_size_t align,
    int flags, struct wiifb_dma *dma)
{
	bus_dma_tag_t dmat = sc->sc_dmat; // &wii_mem1_bus_dma_tag;
	int error;

	error = bus_dmamem_alloc(dmat, size, align, 0,
	    dma->dma_segs, 1, &dma->dma_nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;

	error = bus_dmamem_map(dmat, dma->dma_segs, dma->dma_nsegs,
	    size, &dma->dma_addr, BUS_DMA_WAITOK | flags);
	if (error)
		goto free;

	error = bus_dmamap_create(dmat, size, dma->dma_nsegs,
	    size, 0, BUS_DMA_WAITOK, &dma->dma_map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(dmat, dma->dma_map, dma->dma_addr,
	    size, NULL, BUS_DMA_WAITOK);
	if (error)
	    goto destroy;

	dma->dma_size = size;
	dma->dma_tag = dmat;

	memset(dma->dma_addr, 0, dma->dma_size);

	return 0;

destroy:
	bus_dmamap_destroy(dmat, dma->dma_map);
unmap:
	bus_dmamem_unmap(dmat, dma->dma_addr, dma->dma_size);
free:
	bus_dmamem_free(dmat, dma->dma_segs, dma->dma_nsegs);

	return error;
}

static void *
wiifb_mapreg(struct wiifb_softc *sc, bus_addr_t base, bus_size_t size)
{
	bus_space_handle_t bsh;
	int error;

	error = bus_space_map(sc->sc_bst, base, size,
	    BUS_SPACE_MAP_LINEAR, &bsh);
	if (error != 0) {
		panic("couldn't map 0x%x", base);
	}
	return bus_space_vaddr(sc->sc_bst, bsh);
}

static void
wiifb_set_wgpipe(bus_addr_t base)
{
	uint32_t value;

	if (base) {
		mtspr(IBM750CL_SPR_WPAR, base);
	}
	value = mfspr(IBM750CL_SPR_HID2);
	if (base) {
		value |= IBM750CL_SPR_HID2_WPE;
	} else {
		value &= ~IBM750CL_SPR_HID2_WPE;
	}
	mtspr(IBM750CL_SPR_HID2, value);
}

static void
wiifb_ppcsync(void)
{
	uint32_t value;

	value = mfspr(SPR_HID0);
	mtspr(SPR_HID0, value | 0x8);
	asm volatile("isync" ::: "memory");
	asm volatile("sync" ::: "memory");
	mtspr(SPR_HID0, value);
}

static void
wiifb_gx_bp_load(struct wiifb_softc *sc, uint32_t data)
{
	GX_STRICT_ORDER(sc->sc_wgpipe->u8 = 0x61);
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 = data);
}

static void
wiifb_gx_cp_load(struct wiifb_softc *sc, uint8_t addr, uint32_t data)
{
	GX_STRICT_ORDER(sc->sc_wgpipe->u8 = 0x08);
	GX_STRICT_ORDER(sc->sc_wgpipe->u8 = addr);
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 = data);
}

static void
wiifb_gx_xf_load(struct wiifb_softc *sc, uint16_t addr, uint32_t data)
{
	GX_STRICT_ORDER(sc->sc_wgpipe->u8 = 0x10);
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 = addr);
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 = data);
}

static void
wiifb_gx_xf_load_multi(struct wiifb_softc *sc, uint16_t addr,
    uint16_t count, uint32_t *data)
{
	uint16_t n;

	GX_STRICT_ORDER(sc->sc_wgpipe->u8 = 0x10);
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 =
			(((uint32_t)count - 1) << 16) | addr);
	for (n = 0; n < count; n++) {
		GX_STRICT_ORDER(sc->sc_wgpipe->u32 = data[n]);
	}
}

static void
wiifb_gx_set_viewport(struct wiifb_softc *sc, u_int w, u_int h)
{
	uint32_t data[6];

	KASSERT(w == 640);
	KASSERT(h == 480);

	data[0] = 0x00000140;
	data[1] = 0xffffff10;
	data[2] = 0x00ffffff;
	data[3] = 0x00000296;
	data[4] = 0x00000246;
	data[5] = 0x00ffffff;

	wiifb_gx_xf_load_multi(sc, GX_XF_VIEWPORT_X0,
	    __arraycount(data), data);
}

static void
wiifb_gx_set_scissor(struct wiifb_softc *sc, u_int x, u_int y,
    u_int w, u_int h)
{
	uint32_t xo = x + 0x156;
	uint32_t yo = y + 0x156;
	uint32_t wo = xo + w - 1;
	uint32_t ho = yo + h - 1;

	wiifb_gx_bp_load(sc, 0x20000000 | yo | (xo << 12));
	wiifb_gx_bp_load(sc, 0x21000000 | ho | (wo << 12));
	wiifb_gx_bp_load(sc, 0x59000000 | GX_XY(xo >> 1, yo >> 1));
}

static void
wiifb_gx_init(struct wiifb_softc *sc)
{
	const uint32_t fifo_start = sc->sc_fifo.dma_segs[0].ds_addr;
	const uint32_t fifo_end = fifo_start + sc->sc_fifo.dma_size - 4;
	const uint32_t fifo_hiwat = GX_FIFO_HIWAT(sc->sc_fifo.dma_size);
	const uint32_t fifo_lowat = GX_FIFO_LOWAT(sc->sc_fifo.dma_size);

	/* Disable WGPIPE and unlink CP FIFO before changing settings. */
	wiifb_set_wgpipe(0);
	CP_WRITE(sc, CP_CR, 0);
	CP_WRITE(sc, CP_CLEAR, CP_CLEAR_UNDERFLOW | CP_CLEAR_OVERFLOW);

	/* Setup GP FIFO */
	CP_WRITE(sc, CP_FIFO_BASE_LO, LOWER_16_BITS(fifo_start));               
	CP_WRITE(sc, CP_FIFO_BASE_HI, UPPER_16_BITS(fifo_start));               
	CP_WRITE(sc, CP_FIFO_END_LO, LOWER_16_BITS(fifo_end));                  
	CP_WRITE(sc, CP_FIFO_END_HI, UPPER_16_BITS(fifo_end));                  
	CP_WRITE(sc, CP_FIFO_HIWAT_LO, LOWER_16_BITS(fifo_hiwat));              
	CP_WRITE(sc, CP_FIFO_HIWAT_HI, UPPER_16_BITS(fifo_hiwat));              
	CP_WRITE(sc, CP_FIFO_LOWAT_LO, LOWER_16_BITS(fifo_lowat));              
	CP_WRITE(sc, CP_FIFO_LOWAT_HI, UPPER_16_BITS(fifo_lowat));              
	CP_WRITE(sc, CP_FIFO_RW_DIST_LO, 0);                                    
	CP_WRITE(sc, CP_FIFO_RW_DIST_HI, 0);                                    
	CP_WRITE(sc, CP_FIFO_WRITE_PTR_LO, LOWER_16_BITS(fifo_start));          
	CP_WRITE(sc, CP_FIFO_WRITE_PTR_HI, UPPER_16_BITS(fifo_start));          
	CP_WRITE(sc, CP_FIFO_READ_PTR_LO, LOWER_16_BITS(fifo_start));           
	CP_WRITE(sc, CP_FIFO_READ_PTR_HI, UPPER_16_BITS(fifo_start));           
	wiifb_ppcsync();

	/* Setup CPU FIFO */
	PI_WRITE(sc, PI_FIFO_BASE_START, fifo_start);
	PI_WRITE(sc, PI_FIFO_BASE_END, fifo_end);
	PI_WRITE(sc, PI_FIFO_WRITE_PTR, fifo_start);
	wiifb_ppcsync();

	/* Link CP/PE FIFO and enable GP FIFO */
	CP_WRITE(sc, CP_CR, CP_CR_GP_LINK_ENABLE);
	CP_WRITE(sc, CP_CR, CP_READ(sc, CP_CR) | CP_CR_READ_ENABLE);            

	/* Init pixel engine */
	PE_WRITE(sc, PE_ZCONF,                                                  
	    PE_ZCONF_UPD_ENABLE |                                               
	    PE_ZCONF_FUNC_ALWAYS |                                              
	    PE_ZCONF_COMP_ENABLE);                                              
	PE_WRITE(sc, PE_ALPHA_CONF,                                             
	    PE_ALPHA_CONF_OP_SET |                                              
	    PE_ALPHA_CONF_SRC_1 |                                               
	    PE_ALPHA_CONF_DST_0 |                                               
	    PE_ALPHA_CONF_UPD_A |                                               
	    PE_ALPHA_CONF_UPD_C);                                               
	PE_WRITE(sc, PE_ALPHA_DEST, 0);                                         
	PE_WRITE(sc, PE_ALPHA_MODE, PE_ALPHA_MODE_ALWAYS);                      
	PE_WRITE(sc, PE_ALPHA_READ,                                             
	    PE_ALPHA_READ_UNK | PE_ALPHA_READ_FF);                              

	/* Enable WG pipe */
	wiifb_set_wgpipe(WGPIPE_BASE);

	/* Sanitize command processor registers */
	for (int n = 0; n < 8; n++) {
		wiifb_gx_cp_load(sc, 0x80 + n, 0x80000000);
	}
        wiifb_gx_cp_load(sc, 0x20, 0);

	/* Sanitize transform unit registers */
        wiifb_gx_xf_load(sc, 0x1000, 0x3f);
        wiifb_gx_xf_load(sc, 0x1005, 0x01);
        wiifb_gx_xf_load(sc, 0x1012, 0x01);
        wiifb_gx_xf_load(sc, 0x1006, 0);

	/* Initialize blitting processor */
        wiifb_gx_bp_load(sc, 0x00000001);
        wiifb_gx_bp_load(sc, 0x01666666);
        wiifb_gx_bp_load(sc, 0x02666666);
        wiifb_gx_bp_load(sc, 0x03666666);
        wiifb_gx_bp_load(sc, 0x04666666);
        wiifb_gx_bp_load(sc, 0x22000606);
        wiifb_gx_bp_load(sc, 0x23000000);
        wiifb_gx_bp_load(sc, 0x24000000);
        wiifb_gx_bp_load(sc, 0x42000000);
        wiifb_gx_bp_load(sc, 0x44000003);
        wiifb_gx_bp_load(sc, 0x43000000);
        wiifb_gx_bp_load(sc, 0x53595000);
        wiifb_gx_bp_load(sc, 0x54000015);
        wiifb_gx_bp_load(sc, 0x550003ff);
        wiifb_gx_bp_load(sc, 0x560003ff);
        wiifb_gx_bp_load(sc, 0x5800000f);
        wiifb_gx_bp_load(sc, 0x67000000);

	/* Set viewport and scissor parameters */
	wiifb_gx_set_viewport(sc, WIIFB_RGB_WIDTH, WIIFB_RGB_HEIGHT);
	wiifb_gx_set_scissor(sc, 0, 0, WIIFB_RGB_WIDTH, WIIFB_RGB_HEIGHT);

	wiifb_gx_bp_load(sc, 0x4000001f);

	if (sc->sc_curmode->height == 574) {
		/* Scale 480 lines to PAL display height */
		wiifb_gx_bp_load(sc, 0x4e000127);
	}

	/* Copy mode parameters */
	wiifb_gx_bp_load(sc, 0x410004bc);

	/* Copy source */
	wiifb_gx_bp_load(sc, 0x49000000 | GX_XY(0, 0));
	wiifb_gx_bp_load(sc, 0x4a000000 |
	    GX_XY(WIIFB_RGB_WIDTH - 1, WIIFB_RGB_HEIGHT - 1));

	/* Copy destination */
	wiifb_gx_bp_load(sc, 0x4d000000 | (WIIFB_RGB_WIDTH >> 4));

	/* XFB address */
	wiifb_gx_bp_load(sc, 0x4b000000 | (XFB_START >> 5));

	/* Copy clear settings */
	wiifb_gx_bp_load(sc, 0x4f000000);
	wiifb_gx_bp_load(sc, 0x50000000);
	wiifb_gx_bp_load(sc, 0x5100ffff);
}

static void
wiifb_gx_flush(struct wiifb_softc *sc)
{
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 = 0);
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 = 0);
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 = 0);
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 = 0);
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 = 0);
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 = 0);
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 = 0);
	GX_STRICT_ORDER(sc->sc_wgpipe->u32 = 0);
	wiifb_ppcsync();
}

static void
wiifb_accel_init(struct wiifb_softc *sc)
{
	bus_size_t rgb_size;
	bus_size_t fifo_size;

	if (wiifb_vi_init(sc) != 0) {
		panic("couldn't init VI");
	}

	rgb_size = WIIFB_RGB_WIDTH * WIIFB_RGB_HEIGHT;
	rgb_size = rgb_size * WIIFB_RGB_BPP / NBBY;
	rgb_size = roundup(rgb_size, PAGE_SIZE);
	if (wiifb_dma_alloc(sc, rgb_size, PAGE_SIZE, BUS_DMA_PREFETCHABLE,
			    &sc->sc_rgb) != 0) {
		panic("couldn't alloc rgb fb");
	}

	fifo_size = WIIFB_FIFO_SIZE;
	if (wiifb_dma_alloc(sc, fifo_size, GX_FIFO_ALIGN, BUS_DMA_NOCACHE,
	    		    &sc->sc_fifo) != 0) {
		panic("couldn't alloc gx fifo");
	}

	sc->sc_efb = wiifb_mapreg(sc, EFB_BASE, EFB_SIZE);
	sc->sc_cp = wiifb_mapreg(sc, CP_BASE, CP_SIZE);
	sc->sc_pe = wiifb_mapreg(sc, PE_BASE, PE_SIZE);
	sc->sc_pi = wiifb_mapreg(sc, PI_BASE, PI_SIZE);
	sc->sc_wgpipe = wiifb_mapreg(sc, WGPIPE_BASE, WGPIPE_SIZE);

	wiifb_gx_init(sc);
};

static void
wiifb_rgb_to_efb(struct wiifb_softc *sc)
{
	u_int y;
	uint32_t *src = sc->sc_rgb.dma_addr;
	uint32_t *dst = __UNVOLATILE(sc->sc_efb);
	register_t hid0 = mfspr(SPR_HID0);

	KASSERT(src != NULL);
	KASSERT(dst != NULL);

	/* Disable store gathering while writing to EFB. */
	mtspr(SPR_HID0, hid0 & ~HID0_SGE);

	for (y = 0; y < WIIFB_RGB_HEIGHT; y++) {
		memcpy(dst, src, WIIFB_RGB_WIDTH * 4);
		src += WIIFB_RGB_WIDTH;
		dst += 1024;
	}

	/* Re-enable store gathering. */
	mtspr(SPR_HID0, hid0);
}

static void
wiifb_efb_to_xfb(struct wiifb_softc *sc)
{
	const uint32_t copy_mask = sc->sc_curmode->height == 574 ? 0x400 : 0x0;

	/* Execute copy to XFB */
	wiifb_gx_bp_load(sc, 0x52004803 | copy_mask);
}

static void
wiifb_gx_draw_done(struct wiifb_softc *sc, uint16_t token)
{
	/* Draw done */
	wiifb_gx_bp_load(sc, 0x45000002);
	/* Write tokens */
        wiifb_gx_bp_load(sc, 0x48000000 | token);
        wiifb_gx_bp_load(sc, 0x47000000 | token);
	/* Flush WG pipe */
	wiifb_gx_flush(sc);
}

static void
wiifb_vi_refresh(void *priv)
{
	struct wiifb_softc *sc = priv;

	wiifb_rgb_to_efb(sc);
	wiifb_efb_to_xfb(sc);
	wiifb_gx_draw_done(sc, sc->sc_token++);
}

static int
wiifb_vi_intr(void *priv)
{
	struct wiifb_softc *sc = priv;
	uint32_t di0;
	int ret = 0;

	di0 = RD4(sc, VI_DI0);

	WR4(sc, VI_DI0, RD4(sc, VI_DI0) & ~VI_DI_INT);
	WR4(sc, VI_DI1, RD4(sc, VI_DI1) & ~VI_DI_INT);
	WR4(sc, VI_DI2, RD4(sc, VI_DI2) & ~VI_DI_INT);
	WR4(sc, VI_DI3, RD4(sc, VI_DI3) & ~VI_DI_INT);

	if ((di0 & VI_DI_INT) != 0 &&
	    sc->sc_wsmode != WSDISPLAYIO_MODE_EMUL) {
		wiifb_vi_refresh(sc);
		ret = 1;
	}

	return ret;
}

static void
wiifb_init(struct wiifb_softc *sc)
{
	uint16_t dcr;
	uint16_t visel;

	/* Read current display format and interlaced settings. */
	dcr = RD2(sc, VI_DCR);
	if ((dcr & VI_DCR_ENB) != 0) {
		sc->sc_format = __SHIFTOUT(dcr, VI_DCR_FMT);
		sc->sc_interlaced = (dcr & VI_DCR_NIN) == 0;
	} else {
		visel = RD2(sc, VI_VISEL);
		sc->sc_format = VI_DCR_FMT_NTSC;
		sc->sc_interlaced = (visel & VI_VISEL_COMPONENT_CABLE) == 0;
	}

	/* Reset video interface. */
	WR2(sc, VI_DCR, VI_DCR_RST);
	delay(1000);

	/* Initialize video format and interlace selector. */
	dcr = __SHIFTIN(sc->sc_format, VI_DCR_FMT) |
	      (sc->sc_interlaced ? 0 : VI_DCR_NIN);
	WR2(sc, VI_DCR, dcr);
}

static void
wiifb_set_mode(struct wiifb_softc *sc, uint8_t format, bool interlaced)
{
	u_int modeidx;
	u_int strides, reads;

	modeidx = WIIFB_MODE_INDEX(format, interlaced);
	if (modeidx == WIIFB_MODE_INDEX(VI_DCR_FMT_NTSC, 1)) {
		/* NTSC 480i Magic numbers from YAGCD. */
		WR2(sc, VI_VTR, 0x0f06);
		WR4(sc, VI_HTR0, 0x476901AD);
		WR4(sc, VI_HTR1, 0x02EA5140);
		WR4(sc, VI_VTO, 0x00030018);
		WR4(sc, VI_VTE, 0x00020019);
		WR4(sc, VI_BBOI, 0x410C410C);
		WR4(sc, VI_BBEI, 0x40ED40ED);
		WR2(sc, VI_DPV, 0x0000);
		WR2(sc, VI_DPH, 0x0000);
	} else if (modeidx == WIIFB_MODE_INDEX(VI_DCR_FMT_NTSC, 0)) {
		/* NTSC 480p */
		WR2(sc, VI_VTR, 0x1e0c);
		WR4(sc, VI_HTR0, 0x476901ad);
		WR4(sc, VI_HTR1, 0x030a4940);
		WR4(sc, VI_VTO, 0x00060030);
		WR4(sc, VI_VTE, 0x00060030);
		WR4(sc, VI_BBOI, 0x81d881d8);
		WR4(sc, VI_BBEI, 0x81d881d8);
		WR2(sc, VI_DPV, 0x0000);
		WR2(sc, VI_DPH, 0x0000);
	} else if (modeidx == WIIFB_MODE_INDEX(VI_DCR_FMT_PAL, 1)) {
		/* PAL 576i */
		WR2(sc, VI_VTR, 0x11f5);
		WR4(sc, VI_HTR0, 0x4b6a01b0);
		WR4(sc, VI_HTR1, 0x02f85640);
		WR4(sc, VI_VTO, 0x00010023);
		WR4(sc, VI_VTE, 0x00000024);
		WR4(sc, VI_BBOI, 0x4d2b4d6d);
		WR4(sc, VI_BBEI, 0x4d8a4d4c);
		WR2(sc, VI_DPV, 0x013c);
		WR2(sc, VI_DPH, 0x0144);
	} else {
		/*
		 * Display mode is not supported. Blink the slot LED to
		 * indicate failure.
		 */
		wii_slot_led_blink(WIIFB_ERROR_BLINK_INTERVAL);
	}

	if (modeidx >= WIIFB_NMODES || wiifb_modes[modeidx].name == NULL) {
		panic("Unsupported format (0x%x) / interlaced (%d) settings",
		    sc->sc_format, sc->sc_interlaced);
	}
	sc->sc_curmode = &wiifb_modes[modeidx];

	/* Filter coefficient table, values from YAGCD. */
	WR4(sc, VI_FCT0, 0x1ae771f0);
	WR4(sc, VI_FCT1, 0x0db4a574);
	WR4(sc, VI_FCT2, 0x00c1188e);
	WR4(sc, VI_FCT3, 0xc4c0cbe2);
	WR4(sc, VI_FCT4, 0xfcecdecf);
	WR4(sc, VI_FCT5, 0x13130f08);
	WR4(sc, VI_FCT6, 0x00080C0f);

	/* Unknown registers. */
	WR4(sc, VI_UNKNOWN_68H, 0x00ff0000);
	WR2(sc, VI_UNKNOWN_76H, 0x00ff);
	WR4(sc, VI_UNKNOWN_78H, 0x00ff00ff);
	WR4(sc, VI_UNKNOWN_7CH, 0x00ff00ff);

	/* Picture configuration */
	strides = (sc->sc_curmode->width * 2) / (interlaced ? 16 : 32);
	reads = (sc->sc_curmode->width * 2) / 32;
	WR2(sc, VI_PICCONF,
	    __SHIFTIN(strides, VI_PICCONF_STRIDES) |
	    __SHIFTIN(reads, VI_PICCONF_READS));

	/* Horizontal scaler configuration */
	if (interlaced) {
		WR2(sc, VI_HSR, __SHIFTIN(256, VI_HSR_STP));
	} else {
		WR2(sc, VI_HSR, __SHIFTIN(244, VI_HSR_STP) | VI_HSR_HS_EN);
	}

	/* Video clock configuration */
	WR2(sc, VI_VICLK,
	    interlaced ? VI_VICLK_SEL_27MHZ : VI_VICLK_SEL_54MHZ);

	/* Horizontal scaling width */
	WR2(sc, VI_HSCALINGW, sc->sc_curmode->width);

	/* Set framebuffer address */
	wiifb_set_fb(sc);

	/* Finally, enable the framebuffer */
	WR2(sc, VI_DCR, RD2(sc, VI_DCR) | VI_DCR_ENB);
}

static void
wiifb_set_fb(struct wiifb_softc *sc)
{
	uint32_t taddr = XFB_START;
	uint32_t baddr = taddr + (sc->sc_interlaced ?
				  sc->sc_curmode->width * 2 : 0);

	WR4(sc, VI_TFBL,
	    VI_TFBL_PGOFF |
	    __SHIFTIN((taddr >> 5), VI_TFBL_FBB) |
	    __SHIFTIN((taddr / 2) & 0xf, VI_TFBL_XOF));
	WR4(sc, VI_TFBR, 0);

	WR4(sc, VI_BFBL,
	    VI_BFBL_PGOFF |
	    __SHIFTIN((baddr >> 5), VI_BFBL_FBB) |
	    __SHIFTIN((baddr / 2) & 0xf, VI_BFBL_XOF));
	WR4(sc, VI_BFBR, 0);
}

static int
wiifb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct wiifb_softc *sc = v;
	struct wsdisplayio_bus_id *busid;
	struct wsdisplayio_fbinfo *fbi;
	u_int video;
	u_int wsmode;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_GENFB;
		return 0;
	case WSDISPLAYIO_GET_BUSID:
		busid = data;
		busid->bus_type = WSDISPLAYIO_BUS_SOC;
		return 0;
	case WSDISPLAYIO_GET_FBINFO:
		fbi = data;
		/*
		 * rasops info does not match the pixel encoding due to our
		 * devcmap, so fill out fbinfo manually instead of relying
		 * on wsdisplayio_get_fbinfo.
		 */
		fbi->fbi_fboffset = 0;
		fbi->fbi_width = WIIFB_RGB_WIDTH;
		fbi->fbi_height = WIIFB_RGB_HEIGHT;
		fbi->fbi_bitsperpixel = WIIFB_RGB_BPP;
		fbi->fbi_stride = fbi->fbi_width * fbi->fbi_bitsperpixel / 8;
		fbi->fbi_fbsize = fbi->fbi_height * fbi->fbi_stride;
		fbi->fbi_pixeltype = WSFB_RGB;
		fbi->fbi_flags = WSFB_VRAM_IS_RAM;
		return 0;

	case WSDISPLAYIO_SVIDEO:
		video = *(u_int *)data;
		switch (video) {
		case WSDISPLAYIO_VIDEO_OFF:
			out32(HW_VIDIM, __SHIFTIN(7, VIDIM_Y) |
					__SHIFTIN(7, VIDIM_C) |
					VIDIM_E);
			return 0;
		case WSDISPLAYIO_VIDEO_ON:
			out32(HW_VIDIM, 0);
			return 0;
		default:
			return EINVAL;
		}

	case WSDISPLAYIO_SMODE:
		wsmode = *(u_int *)data;
		if (wsmode != WSDISPLAYIO_MODE_EMUL) {
			/* Blank the RGB FB when leaving text mode */
			memset(sc->sc_rgb.dma_addr, 0, sc->sc_rgb.dma_size);
		}
		if (sc->sc_wsmode != wsmode) {
			sc->sc_wsmode = wsmode;

			if (wsmode == WSDISPLAYIO_MODE_EMUL) {
				wiifb_clear_xfb(sc);
			}
		}
		return EPASSTHROUGH;
	}

	return EPASSTHROUGH;
}

static paddr_t
wiifb_mmap(void *v, void *vs, off_t off, int prot)
{
	struct wiifb_softc *sc = v;

	if (sc->sc_wsmode == WSDISPLAYIO_MODE_EMUL || sc->sc_efb == NULL) {
		return -1;
	}

	if (off < 0 || off >= sc->sc_rgb.dma_size) {
		return -1;
	}

	return bus_dmamem_mmap(sc->sc_rgb.dma_tag,
	    sc->sc_rgb.dma_segs, sc->sc_rgb.dma_nsegs,
	    off, prot, BUS_DMA_PREFETCHABLE);
}
