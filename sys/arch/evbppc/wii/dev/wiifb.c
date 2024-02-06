/* $NetBSD: wiifb.c,v 1.5.2.3 2024/02/06 12:33:17 martin Exp $ */

/*-
 * Copyright (c) 2024 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: wiifb.c,v 1.5.2.3 2024/02/06 12:33:17 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/wii.h>

#include <dev/videomode/videomode.h>
#include <dev/wsfb/genfbvar.h>

#include "mainbus.h"
#include "vireg.h"
#include "viio.h"

#define WIIFB_ERROR_BLINK_INTERVAL	1000000

#define WIIFB_TOP_BOTTOM_BORDER		16
#define WIIFB_EFFECTIVE_START(p, w)	\
	((uintptr_t)(p) + WIIFB_TOP_BOTTOM_BORDER * (w) * 2)
#define WIIFB_EFFECTIVE_HEIGHT(h)	\
	((h) - WIIFB_TOP_BOTTOM_BORDER * 2)


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

struct wiifb_softc {
	struct genfb_softc	sc_gen;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	void			*sc_bits;

	uint8_t			sc_format;
	bool			sc_interlaced;

	const struct wiifb_mode	*sc_curmode;
};

#define	RD2(sc, reg)		\
	bus_space_read_2((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR2(sc, reg, val)	\
	bus_space_write_2((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define	WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	wiifb_match(device_t, cfdata_t, void *);
static void	wiifb_attach(device_t, device_t, void *);

static void	wiifb_init(struct wiifb_softc *);
static void	wiifb_set_mode(struct wiifb_softc *, uint8_t, bool);
static void	wiifb_set_fb(struct wiifb_softc *);

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

	return strcmp(maa->maa_name, "genfb") == 0;
}

static void
wiifb_attach(device_t parent, device_t self, void *aux)
{
	struct wiifb_softc *sc = device_private(self);
	prop_dictionary_t dict = device_properties(self);
	struct mainbus_attach_args *maa = aux;
	u_int offset;
	uint32_t *p;
	int error;

	sc->sc_gen.sc_dev = self;
	sc->sc_bst = maa->maa_bst;
	error = bus_space_map(sc->sc_bst, maa->maa_addr, VI_SIZE, 0,
	    &sc->sc_bsh);
	if (error != 0) {
		panic("couldn't map registers");
	}
	sc->sc_bits = mapiodev(XFB_START, XFB_SIZE, true);

	/*
	 * Paint the entire FB black. Use 4-byte accesses as the Wii will
	 * ignore 1- and 2- byte writes to uncached memory.
	 */
	for (p = sc->sc_bits, offset = 0;
	     offset < XFB_SIZE;
	     offset += 4, p++) {
		*p = 0x00800080;
	}

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
	WR2(sc, VI_DCR, dcr | VI_DCR_RST);
	WR2(sc, VI_DCR, dcr & ~VI_DCR_RST);
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
	} else if (modeidx == WIIFB_MODE_INDEX(VI_DCR_FMT_NTSC, 0)) {
		/* NTSC 480p */
		WR2(sc, VI_VTR, 0x1e0c);
		WR4(sc, VI_HTR0, 0x476901ad);
		WR4(sc, VI_HTR1, 0x030a4940);
		WR4(sc, VI_VTO, 0x00060030);
		WR4(sc, VI_VTE, 0x00060030);
		WR4(sc, VI_BBOI, 0x81d881d8);
		WR4(sc, VI_BBEI, 0x81d881d8);
	} else if (modeidx == WIIFB_MODE_INDEX(VI_DCR_FMT_PAL, 1)) {
		/* PAL 576i */
		WR2(sc, VI_VTR, 0x11f5);
		WR4(sc, VI_HTR0, 0x4b6a01b0);
		WR4(sc, VI_HTR1, 0x02f85640);
		WR4(sc, VI_VTO, 0x00010023);
		WR4(sc, VI_VTE, 0x00000024);
		WR4(sc, VI_BBOI, 0x4d2b4d6d);
		WR4(sc, VI_BBEI, 0x4d8a4d4c);
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
	struct vi_regs *vr;
	u_int video;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_HOLLYWOOD;
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
		fbi->fbi_width = sc->sc_curmode->width;
		fbi->fbi_height =
		    WIIFB_EFFECTIVE_HEIGHT(sc->sc_curmode->height);
		fbi->fbi_stride = fbi->fbi_width * 2;
		fbi->fbi_fbsize = fbi->fbi_height * fbi->fbi_stride;
		fbi->fbi_bitsperpixel = 16;
		fbi->fbi_pixeltype = WSFB_YUY2;
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

	case VIIO_GETREGS:
	case VIIO_SETREGS:
		vr = data;
		switch (vr->bits) {
		case 16:
			if ((vr->reg & 1) != 0) {
				return EINVAL;
			}
			if (cmd == VIIO_GETREGS) {
				vr->val16 = RD2(sc, vr->reg);
			} else {
				WR2(sc, vr->reg, vr->val16);
			}
			return 0;
		case 32:
			if ((vr->reg & 3) != 0) {
				return EINVAL;
			}
			if (cmd == VIIO_GETREGS) {
				vr->val32 = RD4(sc, vr->reg);
			} else {
				WR4(sc, vr->reg, vr->val32);
			}
			return 0;
		default:
			return EINVAL;
		}
		return 0;
	}

	return EPASSTHROUGH;
}

static paddr_t
wiifb_mmap(void *v, void *vs, off_t off, int prot)
{
	struct wiifb_softc *sc = v;
	bus_addr_t start;
	bus_size_t size;

	start = WIIFB_EFFECTIVE_START(XFB_START, sc->sc_curmode->width);
	size = WIIFB_EFFECTIVE_HEIGHT(sc->sc_curmode->height) *
	       sc->sc_curmode->width * 2;

	if (off < 0 || off >= size) {
		return -1;
	}

	return bus_space_mmap(sc->sc_bst, start, off, prot,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE);
}
