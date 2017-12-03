/*
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

/* A console driver for Apple's Platinum onboard video controller,
 * found in (all?) Catalyst logic boards including the Powermac 7200.
 *
 * Used valkyriefb.c from NetBSD, and platinumfb.c/platinumfb.h from
 * Linux sources as templates.
 *
 * Platinum is broken regarding openfirmware video variables.  In OF,
 * for a powermac 7200, doing "dev /platinum .properties" results in:
 *
 *	name                    platinum
 *	device_type             display
 *	model                   AAPL,343S1184
 *	AAPL,connector          monitor
 *	reg                     F8000000  00000800
 *	                        F1000000  01000000
 *	AAPL,interrupts         0000001E
 *
 * The first reg is the register set, and the second is for the
 * framebuffer.  There is also a set of colormap registers hardcoded
 * in platinumfbreg.h that (I think) aren't in openfirmware.
 *
 * powermac 7200 VRAM min and max limits are 1 and 4 Mb respectively.
 * OF claims 16M so we don't use that value.  If other machines can
 * can have more or less VRAM this code will need to be modified
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: platinumfb.c,v 1.3.14.2 2017/12/03 11:36:25 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <uvm/uvm_param.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>
#include <machine/pio.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/videomode/videomode.h>

#include <arch/macppc/dev/platinumfbreg.h>

#include <sys/sysctl.h>

#include "opt_wsemul.h"

/*
 * here is a link of supported modes and resolutions:
 *  https://support.apple.com/kb/SP343?locale=en_US
 *
 * default console and X bpp/depth for built-in X config file,
 * select 8 or 16 or 32. 
 *
 */
#define PLATINUM_CONSOLE_DEPTH 8
#define PLATINUM_FB_DEPTH      16

/*
 * XXX 16 bit console not working without this patch to rasops15.c,
 * http://mail-index.netbsd.org/current-users/2016/09/14/msg030132.html
 * see thread, but rasops may need separate flags for host byte order
 * and video hw byte order
 */

/*
 * resolution, from one of platinumfb_setting vmode_name's.
 */
#define PLATINUM_FB_VMODE "1024x768x60"

struct platinumfb_setting {
	char vmode_name[24];
	int32_t width;
	int32_t height;
    	uint8_t freq;
	uint8_t macmode;

	int32_t	pitch[3];
	uint32_t regs[26];
	uint8_t offset[3];
	uint8_t mode[3];
	uint8_t dacula_ctrl[3];
	uint8_t clock_params[2][2];
};

struct platinumfb_softc {
	device_t sc_dev;
	int sc_node;

        uint8_t *sc_reg;
	uint32_t sc_reg_size;

    	uint8_t *sc_cmap;
	uint32_t sc_cmap_size;

	uint8_t *sc_fb;
	uint32_t sc_fb_size;

 	int sc_depth;
	int sc_width, sc_height, sc_linebytes;
	const struct videomode *sc_videomode;
	uint8_t sc_modereg;

        int sc_mode;

	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];

	struct vcons_data vd;

        uint8_t sc_cmode;
        uint8_t sc_dac_type;
        uint32_t sc_vram;
        int sc_on;
        struct platinumfb_setting *sc_pfs;
};

#define DIV2	0x20
#define DIV4	0x40
#define DIV8	0x60
#define DIV16	0x80

static struct platinumfb_setting platinum_5 = {
    "640x480x60",
    640, 480, 60, 5,
    { 672, 1312, 2592 },
    { 0xff0, 4, 0, 0, 0, 0, 0x320, 0,
      0, 0x15e, 0xc8, 0x18, 0x18f, 0x2f, 0x35, 0x3e,
      0x42, 0x182, 0x18e, 0x41a, 0x418, 2, 7, 0x44,
      0x404, 0x408 }, { 0x34, 0x3c, 0x41 },
    { 2, 0, 0xff }, { 0x11, 0x15, 0x19 },
    {{ 26, 0 + DIV8 }, { 14, 2 + DIV4 }}
};

static struct platinumfb_setting platinum_12 = {
    "800x600x75",
    800, 600, 75, 12,
    { 832, 1632, 3232 },
    { 0xff0, 4, 0, 0, 0, 0, 0x320, 0,
      0, 0x1ce, 0x108, 0x14, 0x20f, 0x27, 0x30, 0x39,
      0x72, 0x202, 0x20e, 0x4e2, 0x4e0, 4, 9, 0x2e,
      0x4de, 0x4df }, { 0x64, 0x6c, 0x71 },
    { 2, 0, 0xff }, { 0x11, 0x15, 0x19 },
    {{ 122, 7 + DIV4 }, { 62, 9 + DIV2 }}
};

static struct platinumfb_setting platinum_14 = {
    "1024x768x60",
    1024, 768, 60, 14,
    { 1056, 2080, 4128 },
    { 0xff0, 4, 0, 0, 0, 0, 0x320, 0,
      0, 0x25a, 0x14f, 0x22, 0x29f, 0x43, 0x49, 0x5b,
      0x8e, 0x28e, 0x29e, 0x64c, 0x64a, 0xa, 0xf, 0x44,
      0x644, 0x646 }, { 0x80, 0x88, 0x8d },
    { 2, 0, 0xff }, { 0x11, 0x15, 0x19 },
    {{ 71, 6 + DIV2 }, { 118, 13 + DIV2 }}
};

static struct platinumfb_setting platinum_20 = {
    "1280x1024x75",
    1280, 1024, 75, 20,
    { 1312, 2592, 2592 },
    { 0xffc, 4, 0, 0, 0, 0, 0x428, 0,
      0, 0xb3, 0xd3, 0x12, 0x1a5, 0x23, 0x28, 0x2d,
      0x5e, 0x19e, 0x1a4, 0x854, 0x852, 4, 9, 0x50,
      0x850, 0x851 }, { 0x58, 0x5d, 0x5d },
    { 0, 0xff, 0xff }, { 0x51, 0x55, 0x55 },
    {{ 45, 3 }, { 66, 7 }}
};

static struct platinumfb_setting *pfb_setting[] = {
    &platinum_5,
    &platinum_12,
    &platinum_14,
    &platinum_20
};

static struct vcons_screen platinumfb_console_screen;

static int	platinumfb_match(device_t, cfdata_t, void *);
static void	platinumfb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(platinumfb, sizeof(struct platinumfb_softc),
    platinumfb_match, platinumfb_attach, NULL, NULL);

static int	platinumfb_init(device_t);
static int 	platinumfb_set_mode(struct platinumfb_softc *,
				    const struct videomode *, int);
static void     platinumfb_set_rasops(struct platinumfb_softc *,
				      struct rasops_info *, int);
static int	platinumfb_ioctl(void *, void *, u_long, void *, int,
				 struct lwp *);
static paddr_t	platinumfb_mmap(void *, void *, off_t, int);
static void	platinumfb_init_screen(void *, struct vcons_screen *, int,
				       long *);
static int	platinumfb_putcmap(struct platinumfb_softc *,
				   struct wsdisplay_cmap *);
static int 	platinumfb_getcmap(struct platinumfb_softc *,
				   struct wsdisplay_cmap *);
static void	platinumfb_init_cmap(struct platinumfb_softc *);
static void	platinumfb_restore_palette(struct platinumfb_softc *);
static void	platinumfb_putpalreg(struct platinumfb_softc *,
				     uint8_t, uint8_t, uint8_t, uint8_t);
static uint32_t platinumfb_line_tweak(struct platinumfb_softc *);
static paddr_t	platinumfb_page_align_up(struct platinumfb_softc *);
static void 	platinumfb_set_clock(struct platinumfb_softc *);
static void	platinumfb_dac_type(struct platinumfb_softc *);
static void	platinumfb_memory_size(struct platinumfb_softc *);
static void	platinumfb_set_hardware(struct platinumfb_softc *);

static inline void platinumfb_write_reg(struct platinumfb_softc *,
 					int, uint32_t);
 					
#ifdef notyet
static inline uint32_t platinumfb_read_reg(struct platinumfb_softc *, int);
#endif
static inline void platinumfb_write_cmap_reg(struct platinumfb_softc *, 
 					     int, uint8_t);
static inline uint8_t platinumfb_read_cmap_reg(struct platinumfb_softc *, int);
static inline void platinumfb_store_d2(struct platinumfb_softc *, 
				       uint8_t, uint8_t);

struct wsscreen_descr platinumfb_defaultscreen = {
	"default",
	0, 0,
	NULL,
	8, 16,
	WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
	NULL,
};

const struct wsscreen_descr *_platinumfb_scrlist[] = {
	&platinumfb_defaultscreen,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list platinumfb_screenlist = {
	sizeof(_platinumfb_scrlist) / sizeof(struct wsscreen_descr *),
	_platinumfb_scrlist
};

struct wsdisplay_accessops platinumfb_accessops = {
	platinumfb_ioctl,
	platinumfb_mmap,
	NULL,
	NULL,
	NULL,
	NULL,	/* load_font */
	NULL,	/* polls */
	NULL,	/* scroll */
};

static inline void
platinumfb_write_reg(struct platinumfb_softc *sc, int reg, uint32_t val)
{
	out32(sc->sc_reg + PLATINUM_REG_OFFSET_ADDR(reg), val);
}

#ifdef notyet
static inline uint32_t
platinumfb_read_reg(struct platinumfb_softc *sc, int reg)
{
	return in32(sc->sc_reg + PLATINUM_REG_OFFSET_ADDR(reg));
}
#endif

static inline void
platinumfb_write_cmap_reg(struct platinumfb_softc *sc, 
			  int reg_offset, uint8_t val)
{
	out8(sc->sc_cmap + reg_offset, val);
}

static inline uint8_t
platinumfb_read_cmap_reg(struct platinumfb_softc *sc, int reg_offset)
{
	return in8(sc->sc_cmap + reg_offset);
}

static inline void
platinumfb_store_d2(struct platinumfb_softc *sc, 
			  uint8_t a, uint8_t d)
{
	platinumfb_write_cmap_reg(sc, PLATINUM_CMAP_ADDR_OFFSET, a + 32);
	platinumfb_write_cmap_reg(sc, PLATINUM_CMAP_D2_OFFSET, d);
}

static void
platinumfb_putpalreg(struct platinumfb_softc *sc,
    uint8_t reg, uint8_t r, uint8_t g, uint8_t b)
{
	platinumfb_write_cmap_reg(sc, PLATINUM_CMAP_ADDR_OFFSET, reg);
	platinumfb_write_cmap_reg(sc, PLATINUM_CMAP_LUT_OFFSET, r);
	platinumfb_write_cmap_reg(sc, PLATINUM_CMAP_LUT_OFFSET, g);
	platinumfb_write_cmap_reg(sc, PLATINUM_CMAP_LUT_OFFSET, b);
}

static uint32_t
platinumfb_line_tweak(struct platinumfb_softc *sc)
{
	/* bytes per line adjustment depending on resolution and depth */
	if (sc->sc_cmode > PLATINUM_CMODE_8 &&
	    strcmp(sc->sc_pfs->vmode_name, "832x624x75") == 0)
		return 0x10;
	else
		return 0x20;
}

static paddr_t
platinumfb_page_align_up(struct platinumfb_softc *sc)
{
	/* round up framebuffer address to the next highest page */
	paddr_t addr = (paddr_t)sc->sc_fb;
	paddr_t ret = round_page(addr);

	if (ret == addr)
		ret = round_page(addr + 1);

	return ret;
}

/* 2 versions of platinum clock, older one uses clock[1] and
 *     freq = 14.3Mhz * c0 / (c1 & 0x1f) / (1 << (c1 >> 5))
 * newer one uses clock[0] and
 *     freq = 15Mhz * c0 / ((c1 & 0x1f) + 2) / (1 << (c1 >> 5))
 */
static void
platinumfb_set_clock(struct platinumfb_softc *sc)
{
	uint8_t clk_idx = sc->sc_dac_type == PLATINUM_DAC_1 ? 1 : 0;
	uint8_t d2;

	platinumfb_store_d2(sc, 6, 0xc6);

	platinumfb_write_cmap_reg(sc, PLATINUM_CMAP_ADDR_OFFSET, 3+32);
	d2 = platinumfb_read_cmap_reg(sc, PLATINUM_CMAP_D2_OFFSET);

	if (d2 == 2) {
		platinumfb_store_d2(sc, 7, sc->sc_pfs->clock_params[clk_idx][0]);
		platinumfb_store_d2(sc, 8, sc->sc_pfs->clock_params[clk_idx][1]);
		platinumfb_store_d2(sc, 3, 3);
	} else {
		platinumfb_store_d2(sc, 4, sc->sc_pfs->clock_params[clk_idx][0]);
		platinumfb_store_d2(sc, 5, sc->sc_pfs->clock_params[clk_idx][1]);
		platinumfb_store_d2(sc, 3, 2);
	}

	delay(5000);
	platinumfb_store_d2(sc, 9, 0xa6);
}

static void
platinumfb_dac_type(struct platinumfb_softc *sc)
{
	uint8_t dtype = 0;

	platinumfb_write_cmap_reg(sc, PLATINUM_CMAP_ADDR_OFFSET, 0x40);
	dtype = platinumfb_read_cmap_reg(sc, PLATINUM_CMAP_D2_OFFSET);

	switch (dtype) {
		case PLATINUM_DAC_0:
		case PLATINUM_DAC_1:
	 	/* do nothing */
	 	break;
	default:
		aprint_error_dev(sc->sc_dev, "unknown dac 0x%x, using 0x%x\n",
				 dtype, PLATINUM_DAC_0);
		dtype = PLATINUM_DAC_0;
		break;
	}

	/* save type */
	sc->sc_dac_type = dtype;
}

static void
platinumfb_memory_size(struct platinumfb_softc *sc)
{
	int i;
	off_t offset = PLATINUM_FB_BANK_SIZE;
	paddr_t total_vram = PLATINUM_FB_MIN_SIZE;

	uint8_t test_val[] = {0x34, 0x56, 0x78};
	uint8_t bank[] = {0, 0, 0};
	uint8_t num_elems = sizeof(test_val)/sizeof(test_val[0]);

	volatile uint8_t *fbuffer = mapiodev((paddr_t)sc->sc_fb, sc->sc_fb_size, false);

	if (fbuffer == NULL)
		panic("platinumfb could not mapiodev");

	/* turn on all banks of RAM */
	platinumfb_write_reg(sc, 16, (paddr_t)sc->sc_fb);
	platinumfb_write_reg(sc, 20, 0x1011);
	platinumfb_write_reg(sc, 24, 0);

	/*
	 * write "unique" value to each bank of memory and read value
	 * back. if match assumes VRAM bank exists.  On the powermac 7200,
	 * bank0 is always there and soldered to motherboard, don't know
	 * if that is the case for others
	 */
	for (i = 0 ; i < num_elems; i++) {
		out8(fbuffer + offset, test_val[i]);
		out8(fbuffer + offset + 0x8, 0x0);

		__asm volatile ("eieio; dcbf 0,%0"::"r"(&fbuffer[offset]):"memory");

		bank[i] = fbuffer[offset] == test_val[i];
		total_vram += bank[i] * PLATINUM_FB_BANK_SIZE;
		offset += PLATINUM_FB_BANK_SIZE;
	}

	/* save total vram or minimum */
	if (total_vram >= PLATINUM_FB_MIN_SIZE && total_vram <= PLATINUM_FB_MAX_SIZE) {
		sc->sc_vram = total_vram;
	} else {
		aprint_error_dev(sc->sc_dev,
				 "invalid VRAM size 0x%lx, using min 0x%x\n",
				 total_vram, PLATINUM_FB_MIN_SIZE);
		sc->sc_vram = PLATINUM_FB_MIN_SIZE;
	}

	unmapiodev((paddr_t)fbuffer, sc->sc_fb_size);
}

static int
platinumfb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	return (strcmp(ca->ca_name, "platinum") == 0);
}

static void
platinumfb_attach(device_t parent, device_t self, void *aux)
{
	struct platinumfb_softc *sc = device_private(self);
	struct confargs *ca = aux;
	u_int *reg = ca->ca_reg;

	sc->sc_dev = self;
	sc->sc_node = ca->ca_node;

	sc->sc_reg = (uint8_t *)reg[0];
	sc->sc_reg_size = reg[1];

	sc->sc_fb = (uint8_t *)reg[2];
	sc->sc_fb_size = PLATINUM_FB_MAX_SIZE;

	sc->sc_cmap = (uint8_t *)PLATINUM_CMAP_BASE_ADDR;
	sc->sc_cmap_size = PLATINUM_CMAP_SIZE;

	aprint_normal(" reg-addr 0x%08lx fb-addr 0x%08lx cmap-addr 0x%08lx\n",
		      (paddr_t)sc->sc_reg, 
		      (paddr_t)sc->sc_fb,
		      (paddr_t)sc->sc_cmap);

	config_finalize_register(sc->sc_dev, platinumfb_init);
}

static void
platinumfb_init_cmap(struct platinumfb_softc *sc)
{
	int i;
	uint8_t tmp;

	switch (sc->sc_cmode) {
	case PLATINUM_CMODE_8:
	default:
		/* R3G3B2 colormap */
		for (i = 0; i < 256; i++) {
			tmp = i & 0xe0;
			/*
			 * replicate bits so 0xe0 maps to a red value of 0xff
			 * in order to make white look actually white
			 */
			tmp |= (tmp >> 3) | (tmp >> 6);
			sc->sc_cmap_red[i] = tmp;

			tmp = (i & 0x1c) << 3;
			tmp |= (tmp >> 3) | (tmp >> 6);
			sc->sc_cmap_green[i] = tmp;

			tmp = (i & 0x03) << 6;
			tmp |= tmp >> 2;
			tmp |= tmp >> 4;
			sc->sc_cmap_blue[i] = tmp;
		}
		break;

	case PLATINUM_CMODE_16:
		for (i = 0; i < 32; i++) {
			tmp = 255 * i / 32;
			sc->sc_cmap_red[i] = tmp;
			sc->sc_cmap_green[i] = tmp;
			sc->sc_cmap_blue[i] = tmp;
		}
		for (i = 32; i < 256; i++) {
			sc->sc_cmap_red[i] =  0;
			sc->sc_cmap_blue[i] = 0;
			sc->sc_cmap_green[i] = 0;
		}
		break;

	case PLATINUM_CMODE_32:
		for (i = 0; i < 256; i++) {
			sc->sc_cmap_red[i] = i;
			sc->sc_cmap_green[i] = i;
			sc->sc_cmap_blue[i] = i;
		}
		break;
	}
}

static void
platinumfb_restore_palette(struct platinumfb_softc *sc)
{
	int i;

	for (i = 0; i < 256; i++) {
		platinumfb_putpalreg(sc, i, sc->sc_cmap_red[i],
 			     sc->sc_cmap_green[i], sc->sc_cmap_blue[i]);
	}
}

static int
platinumfb_init(device_t self)
{
	struct platinumfb_softc *sc = device_private(self);
	const struct videomode *mode = NULL;
	struct rasops_info *ri;

	struct wsemuldisplaydev_attach_args aa;
	bool is_console = FALSE;
	long defattr;

	int i;

	/*
	 * become console if OF variable "output-device" is "screen" or
	 * contains "platinum", since normal OF video variables are unavailable
	 */
	int options;
	char output_device[128];
	options = OF_finddevice("/options");
	if (options == 0 || 
	    options == -1 ||
	    OF_getprop(options, "output-device", output_device,
		 sizeof(output_device)) == 0 ) {
		aprint_error_dev(sc->sc_dev,
		    "could not get output-device prop, assuming not console\n");
	} else {
		if (strstr(output_device,"platinum") ||
		    strcmp(output_device,"screen") == 0 ) {
			is_console = TRUE;
		}
	}	

	sc->sc_pfs = NULL;
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_on = WSDISPLAYIO_VIDEO_ON;

	/* determine vram memory and dac clock type */
	platinumfb_memory_size(sc);
	platinumfb_dac_type(sc);

	aprint_normal_dev(sc->sc_dev,"is_console %d dac 0x%x vram 0x%x\n",
			  is_console, sc->sc_dac_type, sc->sc_vram);

	for (i=0; i < sizeof(pfb_setting)/sizeof(pfb_setting[0]); i++) {
		if (strcmp(PLATINUM_FB_VMODE, pfb_setting[i]->vmode_name)==0) {
			mode = pick_mode_by_ref(pfb_setting[i]->width, 
						pfb_setting[i]->height,
 						pfb_setting[i]->freq);
			break;
		}
	}

	if (!mode) {
		aprint_error_dev(sc->sc_dev,
		    "pick_mode_by_ref failed, using default\n");
		mode = pick_mode_by_ref(800, 600, 75);
	}

	if (platinumfb_set_mode(sc, mode, PLATINUM_CONSOLE_DEPTH) != 0) {
		aprint_error_dev(sc->sc_dev, "platinumfb_set_mode failed\n");
		return 0;
	}

	vcons_init(&sc->vd, sc, &platinumfb_defaultscreen,
		   &platinumfb_accessops);
	sc->vd.init_screen = platinumfb_init_screen;

	ri = &platinumfb_console_screen.scr_ri;
	vcons_init_screen(&sc->vd, &platinumfb_console_screen, 1, &defattr);

	platinumfb_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

	platinumfb_defaultscreen.textops = &ri->ri_ops;
	platinumfb_defaultscreen.capabilities = ri->ri_caps;
	platinumfb_defaultscreen.nrows = ri->ri_rows;
	platinumfb_defaultscreen.ncols = ri->ri_cols;

	if (is_console) {
		wsdisplay_cnattach(&platinumfb_defaultscreen, ri, 0, 0,
		    defattr);
		vcons_replay_msgbuf(&platinumfb_console_screen);
	}

	aa.console = is_console;
	aa.scrdata = &platinumfb_screenlist;
	aa.accessops = &platinumfb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(self, &aa, wsemuldisplaydevprint);

	return 0;
}

static void
platinumfb_set_hardware(struct platinumfb_softc *sc)
{
	int i;
	bool one_bank = sc->sc_vram == PLATINUM_FB_BANK_SIZE;

	/* now start programming the chip */
	platinumfb_write_reg(sc, 24, 7);    /* turn off display */

	for (i = 0; i < 26; ++i)
		platinumfb_write_reg(sc, i+32, sc->sc_pfs->regs[i]);

	platinumfb_write_reg(sc, 26+32, one_bank ? 
			 sc->sc_pfs->offset[sc->sc_cmode] + 4 - sc->sc_cmode :
			 sc->sc_pfs->offset[sc->sc_cmode]);

	/*
	 * reg 16 apparently needs an address 0x10 less the where frame
	 * buffer/ri_bits start for console text to be aligned.  In
	 * addition, X memory maps (mmap) the frame buffer starting on
	 * page boundaries.  So to get both X and console text aligned we
	 * start at the first page up from sc_fb[0]. Starting at sc_fb[0]
	 * did work on my machine but not sure if this negative offset
	 * would be problematic elsewhere.
	 *
	 * Not sure why linux used different fb offsets for each mode, as
	 * any addresses seemed to work as long as relative difference was
	 * 0x10.
	 */
	platinumfb_write_reg(sc, 16, platinumfb_page_align_up(sc) - 0x10);

	platinumfb_write_reg(sc, 18, sc->sc_pfs->pitch[sc->sc_cmode]);

	/* 
	 * XXX register 19 setting looks wrong for 1 bank & 32 bpp.
	 * 512x384 is only resolution that would use such a setting, but
	 * that is not currently in videomodes.c
	 */
	if (sc->sc_cmode == PLATINUM_CMODE_32 &&
	   (sc->sc_pfs->macmode == 1 || sc->sc_pfs->macmode == 2))
		aprint_error_dev(sc->sc_dev,
		    "platinumfb reg19 array out-of-bounds");

	platinumfb_write_reg(sc, 19, one_bank ? 
	    sc->sc_pfs->mode[sc->sc_cmode+1] : /* XXX fix this for 32 bpp */
	    sc->sc_pfs->mode[sc->sc_cmode]);

	platinumfb_write_reg(sc, 20, one_bank ? 0x11 : 0x1011);
	platinumfb_write_reg(sc, 21, 0x100);
	platinumfb_write_reg(sc, 22, 1);
	platinumfb_write_reg(sc, 23, 1);
	platinumfb_write_reg(sc, 26, 0xc00);
	platinumfb_write_reg(sc, 27, 0x235);
	/* platinumfb_write_reg(sc, 27, 0x2aa); */

	platinumfb_store_d2(sc, 0,
	    one_bank ? sc->sc_pfs->dacula_ctrl[sc->sc_cmode] & 0xf :
	               sc->sc_pfs->dacula_ctrl[sc->sc_cmode]);
	platinumfb_store_d2(sc, 1, 4);
	platinumfb_store_d2(sc, 2, 0);

	platinumfb_set_clock(sc);

	platinumfb_write_reg(sc, 24, 0);  /* turn display on */
}

static int
platinumfb_set_mode(struct platinumfb_softc *sc,
    const struct videomode *mode, int depth)
{
	int i;

	/* first find the parameter for the mode register */
	i = 0;
	while((i < __arraycount(pfb_setting)) &&
	      (strcmp(mode->name, pfb_setting[i]->vmode_name) != 0))
	    i++;

	if (i >= __arraycount(pfb_setting)) {
		aprint_error_dev(sc->sc_dev,
		    "Can't find a mode register value for %s\n", 
				 mode->name);
		return EINVAL;
	}

	/* found a mode */
	sc->sc_pfs = pfb_setting[i];

	/* determine depth settings */
	switch (depth) {
	case 8:
	default:
		sc->sc_depth = 8;
		sc->sc_cmode = PLATINUM_CMODE_8;
		break;
	case 15:
	case 16:
		/* 15 bpp but use 16 so X/wsfb works */
		sc->sc_depth = 16;
		sc->sc_cmode = PLATINUM_CMODE_16;
		break;
	case 24:
	case 32:
		/* 24 bpp but use 32 so X/wsfb works */
		sc->sc_depth = 32;
		sc->sc_cmode = PLATINUM_CMODE_32;
		break;
	}

	sc->sc_modereg = sc->sc_pfs->macmode;
	sc->sc_videomode = mode;
	sc->sc_height = mode->vdisplay;
	sc->sc_width = mode->hdisplay;
	sc->sc_linebytes = sc->sc_width * (1 << sc->sc_cmode) + platinumfb_line_tweak(sc);

	/* check if we have enough video memory */
	if (sc->sc_height * sc->sc_linebytes > sc->sc_vram - PAGE_SIZE) {
		aprint_error_dev(sc->sc_dev, "Not enough video RAM for %s\n",
 			     mode->name);
		return EINVAL;
	}

	/* set up and write colormap */
	platinumfb_init_cmap(sc);
	platinumfb_restore_palette(sc);

	/* set hardware registers */
	platinumfb_set_hardware(sc);

	aprint_normal_dev(sc->sc_dev, "switched to %s in %d bit color\n",
			  sc->sc_pfs->vmode_name, sc->sc_depth);
	return 0;
}

static int
platinumfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct platinumfb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;
	int i;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PLATINUM;
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = ms->scr_ri.ri_height;
		wdf->width = ms->scr_ri.ri_width;
		wdf->depth = ms->scr_ri.ri_depth;
		wdf->cmsize = 256;
		return 0;

	case WSDISPLAYIO_GVIDEO:
		*(int *)data = sc->sc_on;
		return 0;

	case WSDISPLAYIO_SVIDEO:
		/* 
		 * poor man's screen blanking, just write zeros to colormap
		 * registers but don't save in softc.  
		 */
		if (*(int *)data != sc->sc_on) {
			sc->sc_on = (sc->sc_on == WSDISPLAYIO_VIDEO_ON ?
				 WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON);

			/* XXX need to lock colormap? */
			if (sc->sc_on == WSDISPLAYIO_VIDEO_OFF) {
				for (i=0; i < 256; i++)
					platinumfb_putpalreg(sc, i, 0, 0, 0);
			} else {
				platinumfb_restore_palette(sc);
			}
		}
		return 0;

	case WSDISPLAYIO_GETCMAP:
		if (sc->sc_cmode == PLATINUM_CMODE_8) {
			return platinumfb_getcmap(sc,
			     (struct wsdisplay_cmap *)data);
		} else
			return 0;

	case WSDISPLAYIO_PUTCMAP:
		if (sc->sc_cmode == PLATINUM_CMODE_8) {
			return platinumfb_putcmap(sc,
			    (struct wsdisplay_cmap *)data);
		} else
			return 0;

	case WSDISPLAYIO_SMODE: {
		int new_mode = *(int*)data;

		if (new_mode != sc->sc_mode) {
			int new_depth = new_mode == WSDISPLAYIO_MODE_EMUL ?
			    PLATINUM_CONSOLE_DEPTH : PLATINUM_FB_DEPTH;

			switch(new_mode) {
			/*
			 * XXX - not sure how this is supposed to work for
			 * switching bpp between console and X, but cases with
			 * (EMUL MAPPED) or (EMUL MAPPED DUMBFB) work, but 
			 * (EMUL DUMBFB) garbles screen for some reason.
			 */
			case WSDISPLAYIO_MODE_EMUL:
			case WSDISPLAYIO_MODE_MAPPED:
			/* case WSDISPLAYIO_MODE_DUMBFB: XXX */
	
				/* in case screen is "blanked" */
				platinumfb_restore_palette(sc);
	
				sc->sc_mode = new_mode;
	
				platinumfb_set_mode(sc, sc->sc_videomode, new_depth);
				platinumfb_set_rasops(sc, &ms->scr_ri, true);
	
				if (new_mode == WSDISPLAYIO_MODE_EMUL)
					vcons_redraw_screen(ms);
			}
		}
	
		return 0;
	}

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_linebytes;
		return 0;

	case WSDISPLAYIO_GET_FBINFO: {
		struct wsdisplayio_fbinfo *fbi = data;
		return wsdisplayio_get_fbinfo(&ms->scr_ri, fbi);
	}

	}
	return EPASSTHROUGH;
}

static paddr_t
platinumfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct platinumfb_softc *sc = vd->cookie;
	paddr_t pa = -1;
	paddr_t fb_aligned = platinumfb_page_align_up(sc);
	paddr_t fb_vram = sc->sc_vram - (fb_aligned - (paddr_t)sc->sc_fb);

	/* XXX need to worry about superuser or mapping other registers? */

	if (offset >= 0 && offset < fb_vram)
		pa = fb_aligned + offset;

	return pa;
}

static void platinumfb_set_rasops(struct platinumfb_softc *sc,
				  struct rasops_info *ri,
				  int existing)
{
	memset(ri, 0, sizeof(struct rasops_info));

	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_linebytes;
	ri->ri_bits = (u_char*)platinumfb_page_align_up(sc);
	ri->ri_flg = RI_FULLCLEAR;

	if (existing)
		ri->ri_flg |= RI_CLEAR;

	switch (sc->sc_cmode) {
	case PLATINUM_CMODE_8:
	default:
		ri->ri_flg |= RI_ENABLE_ALPHA | RI_8BIT_IS_RGB;

		break;
	case PLATINUM_CMODE_16:
		if (strcmp(sc->sc_pfs->vmode_name, "640x480x60") == 0 ||
		    strcmp(sc->sc_pfs->vmode_name, "800x600x75") == 0 )
			ri->ri_flg |= RI_ENABLE_ALPHA;

		ri->ri_rnum = 5;
		ri->ri_rpos = 10;
		ri->ri_gnum = 5;
		ri->ri_gpos = 5;
		ri->ri_bnum = 5;
		ri->ri_bpos = 0;

		break;
	case PLATINUM_CMODE_32:
		if (strcmp(sc->sc_pfs->vmode_name, "640x480x60") == 0 ||
		    strcmp(sc->sc_pfs->vmode_name, "800x600x75") == 0 )
			ri->ri_flg |= RI_ENABLE_ALPHA;

		ri->ri_rnum = 8;
		ri->ri_rpos = 16;
		ri->ri_gnum = 8;
		ri->ri_gpos = 8;
		ri->ri_bnum = 8;
		ri->ri_bpos = 0;

		break;
	}

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

}

static void
platinumfb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct platinumfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	scr->scr_flags |= VCONS_DONT_READ;

	platinumfb_set_rasops(sc, ri, existing);

	ri->ri_hw = scr;
}

static int
platinumfb_putcmap(struct platinumfb_softc *sc, struct wsdisplay_cmap *cm)
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

	/* write colormap registers if not currently blanked */
	if (sc->sc_on == WSDISPLAYIO_VIDEO_ON) {
		r = &sc->sc_cmap_red[index];
		g = &sc->sc_cmap_green[index];
		b = &sc->sc_cmap_blue[index];

		for (i = 0; i < count; i++) {
			platinumfb_putpalreg(sc, index, *r, *g, *b);
			index++;
			r++, g++, b++;
		}
	}

	return 0;
}

static int
platinumfb_getcmap(struct platinumfb_softc *sc, struct wsdisplay_cmap *cm)
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
