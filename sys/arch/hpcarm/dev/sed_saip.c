/*	$NetBSD: sed_saip.c,v 1.7 2001/07/16 22:01:38 hpeyerl Exp $	*/

/*-
 * Copyright (c) 1999-2001
 *         Shin Takemura and PocketBSD Project. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/bootinfo.h>
#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

#include <dev/rasops/rasops.h>

#include <dev/hpc/hpcfbvar.h>
#include <dev/hpc/hpcfbio.h>
#include <dev/hpc/hpccmapvar.h>

#include <arch/hpcarm/dev/sed1356var.h>

#define VPRINTF(arg)	do { if (bootverbose) printf arg; } while(0);

/*
 *  function prototypes
 */
int	sed1356match(struct device *, struct cfdata *, void *);
void	sed1356attach(struct device *, struct device *, void *);
int	sed1356_ioctl(void *, u_long, caddr_t, int, struct proc *);
paddr_t	sed1356_mmap(void *, off_t, int);


extern	struct bus_space sa11x0_bs_tag;
extern	int j720lcdpower(void *, int, long, void *);	/* XXX */

static int sed1356_init(struct hpcfb_fbconf *);
static void sed1356_power(int, void *);
static void sed1356_update_powerstate(struct sed1356_softc *, int);
void	sed1356_init_backlight(struct sed1356_softc *, int);
void	sed1356_init_brightness(struct sed1356_softc *, int);
void	sed1356_init_contrast(struct sed1356_softc *, int);
void	sed1356_set_brightness(struct sed1356_softc *, int);
void	sed1356_set_contrast(struct sed1356_softc *, int);

#if defined __mips__ || defined __sh__ || defined __arm__
#define __BTOP(x)		((paddr_t)(x) >> PGSHIFT)
#define __PTOB(x)		((paddr_t)(x) << PGSHIFT)
#else
#error "define btop, ptob."
#endif

/*
 *  static variables
 */
struct cfattach sed_ca = {
	sizeof(struct sed1356_softc), sed1356match, sed1356attach,
};
struct hpcfb_accessops sed1356_ha = {
	sed1356_ioctl, sed1356_mmap
};

static int console_flag = 0;
static int attach_flag = 0;

/*
 *  function bodies
 */
int
sed1356match(struct device *parent, struct cfdata *match, void *aux)
{

	/* XXX check version register */
	return 1;
}

void
sed1356attach(struct device *parent, struct device *self, void *aux)
{
	struct sed1356_softc *sc = (struct sed1356_softc *)self;
	struct hpcfb_attach_args ha;

	printf("\n");

	if (attach_flag) {
		panic("%s(%d): sed1356 attached twice", __FILE__, __LINE__);
	}
	attach_flag = 1;

	if (sed1356_init(&sc->sc_fbconf) != 0) {
		/* just return so that hpcfb will not be attached */
		return;
	}

	sc->sc_iot = &sa11x0_bs_tag;
	sc->sc_parent = (struct sa11x0_softc *)parent;
	if (bus_space_map(sc->sc_iot, (bus_addr_t)bootinfo->fb_addr & ~0x3fffff,
			  0x200, 0, &sc->sc_regh)) {
		printf("%s: unable to map register\n", sc->sc_dev.dv_xname);
		return;
	}

	printf("%s: Epson SED1356", sc->sc_dev.dv_xname);
	if (console_flag) {
		printf(", console");
	}
	printf("\n");
	printf("%s: framebuffer address: 0x%08lx\n", 
		sc->sc_dev.dv_xname, (u_long)bootinfo->fb_addr);

	/* Add a suspend hook to power saving */
	sc->sc_powerstate = 0;
	sc->sc_powerhook = powerhook_establish(sed1356_power, sc);
	if (sc->sc_powerhook == NULL)
		printf("%s: WARNING: unable to establish power hook\n",
			sc->sc_dev.dv_xname);

	/* initialize backlight brightness and lcd contrast */
	sc->sc_lcd_inited = 0;
	sed1356_init_brightness(sc, 1);
	sed1356_init_contrast(sc, 1);
	sed1356_init_backlight(sc, 1);

	ha.ha_console = console_flag;
	ha.ha_accessops = &sed1356_ha;
	ha.ha_accessctx = sc;
	ha.ha_curfbconf = 0;
	ha.ha_nfbconf = 1;
	ha.ha_fbconflist = &sc->sc_fbconf;
	ha.ha_curdspconf = 0;
	ha.ha_ndspconf = 1;
	ha.ha_dspconflist = &sc->sc_dspconf;

	/* XXX */
	if (platid_match(&platid, &platid_mask_MACH_HP_JORNADA_7XX)) {
		config_hook(CONFIG_HOOK_POWERCONTROL,
			    CONFIG_HOOK_POWERCONTROL_LCDLIGHT,
			    CONFIG_HOOK_SHARE, j720lcdpower, sc);
	}

	config_found(self, &ha, hpcfbprint);
}

int
sed1356_getcnfb(struct hpcfb_fbconf *fb)
{
	console_flag = 1;

	return sed1356_init(fb);
}

static int
sed1356_init(struct hpcfb_fbconf *fb)
{
	/*
	 * get fb settings from bootinfo
	 */
	if (bootinfo == NULL ||
	    bootinfo->fb_addr == 0 ||
	    bootinfo->fb_line_bytes == 0 ||
	    bootinfo->fb_width == 0 ||
	    bootinfo->fb_height == 0) {
		printf("no frame buffer information.\n");
		return (-1);
	}

	/* zero fill */
	memset(fb, 0, sizeof(*fb));

	fb->hf_conf_index	= 0;	/* configuration index		*/
	fb->hf_nconfs		= 1;   	/* how many configurations	*/
	strcpy(fb->hf_name, "built-in video");
					/* frame buffer name		*/
	strcpy(fb->hf_conf_name, "default");
					/* configuration name		*/
	fb->hf_height		= bootinfo->fb_height;
	fb->hf_width		= bootinfo->fb_width;

	if (bus_space_map(&sa11x0_bs_tag, (bus_addr_t)bootinfo->fb_addr,
			   bootinfo->fb_height * bootinfo->fb_line_bytes,
			   0, &fb->hf_baseaddr)) {
		printf("unable to map framebuffer\n");
		return (-1);
	}
	fb->hf_offset		= (u_long)bootinfo->fb_addr -
				      __PTOB(__BTOP(bootinfo->fb_addr));
					/* frame buffer start offset   	*/
	fb->hf_bytes_per_line	= bootinfo->fb_line_bytes;
	fb->hf_nplanes		= 1;
	fb->hf_bytes_per_plane	= bootinfo->fb_height *
					bootinfo->fb_line_bytes;

	fb->hf_access_flags |= HPCFB_ACCESS_BYTE;
	fb->hf_access_flags |= HPCFB_ACCESS_WORD;
	fb->hf_access_flags |= HPCFB_ACCESS_DWORD;

	switch (bootinfo->fb_type) {
		/*
		 * gray scale
		 */
	case BIFB_D4_M2L_F:
	case BIFB_D4_M2L_Fx2:
		fb->hf_access_flags |= HPCFB_ACCESS_REVERSE;
		/* fall through */
	case BIFB_D4_M2L_0:
	case BIFB_D4_M2L_0x2:
		fb->hf_class = HPCFB_CLASS_GRAYSCALE;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
		fb->hf_pack_width = 8;
		fb->hf_pixels_per_pack = 2;
		fb->hf_pixel_width = 4;
		fb->hf_class_data_length = sizeof(struct hf_gray_tag);
		fb->hf_u.hf_gray.hf_flags = 0;	/* reserved for future use */
		break;

		/*
		 * indexed color
		 */
	case BIFB_D8_FF:
		fb->hf_access_flags |= HPCFB_ACCESS_REVERSE;
		/* fall through */
	case BIFB_D8_00:
		fb->hf_class = HPCFB_CLASS_INDEXCOLOR;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
		fb->hf_pack_width = 8;
		fb->hf_pixels_per_pack = 1;
		fb->hf_pixel_width = 8;
		fb->hf_class_data_length = sizeof(struct hf_indexed_tag);
		fb->hf_u.hf_indexed.hf_flags = 0; /* reserved for future use */
		break;

		/*
		 * RGB color
		 */
	case BIFB_D16_FFFF:
		fb->hf_access_flags |= HPCFB_ACCESS_REVERSE;
		/* fall through */
	case BIFB_D16_0000:
		fb->hf_class = HPCFB_CLASS_RGBCOLOR;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
#if BYTE_ORDER == BIG_ENDIAN
		fb->hf_swap_flags = HPCFB_SWAP_BYTE;
#endif
		fb->hf_pack_width = 16;
		fb->hf_pixels_per_pack = 1;
		fb->hf_pixel_width = 16;

		fb->hf_class_data_length = sizeof(struct hf_rgb_tag);
		fb->hf_u.hf_rgb.hf_flags = 0;	/* reserved for future use */

		fb->hf_u.hf_rgb.hf_red_width = 5;
		fb->hf_u.hf_rgb.hf_red_shift = 11;
		fb->hf_u.hf_rgb.hf_green_width = 6;
		fb->hf_u.hf_rgb.hf_green_shift = 5;
		fb->hf_u.hf_rgb.hf_blue_width = 5;
		fb->hf_u.hf_rgb.hf_blue_shift = 0;
		fb->hf_u.hf_rgb.hf_alpha_width = 0;
		fb->hf_u.hf_rgb.hf_alpha_shift = 0;
		break;

	default:
		printf("unsupported type %d.\n", bootinfo->fb_type);
		return (-1);
		break;
	}

	return (0); /* no error */
}

static void 
sed1356_power(int why, void *arg)
{
	struct sed1356_softc *sc = arg;

	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		sc->sc_powerstate |= PWRSTAT_SUSPEND;
		sed1356_update_powerstate(sc, PWRSTAT_ALL);
		break;
	case PWR_RESUME:
		sc->sc_powerstate &= ~PWRSTAT_SUSPEND;
		sed1356_update_powerstate(sc, PWRSTAT_ALL);
		break;
	}
}

static void
sed1356_update_powerstate(struct sed1356_softc *sc, int updates)
{
	if (updates & PWRSTAT_LCD)
		config_hook_call(CONFIG_HOOK_POWERCONTROL,
		    CONFIG_HOOK_POWERCONTROL_LCD,
		    (void*)!(sc->sc_powerstate & 
				(PWRSTAT_VIDEOOFF|PWRSTAT_SUSPEND)));

	if (updates & PWRSTAT_BACKLIGHT)
		config_hook_call(CONFIG_HOOK_POWERCONTROL,
		    CONFIG_HOOK_POWERCONTROL_LCDLIGHT,
		    (void*)(!(sc->sc_powerstate & 
				(PWRSTAT_VIDEOOFF|PWRSTAT_SUSPEND)) &&
			     (sc->sc_powerstate & PWRSTAT_BACKLIGHT)));
}

int
sed1356_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct sed1356_softc *sc = (struct sed1356_softc *)v;
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_dspconf *dspconf;
	struct wsdisplay_param *dispparam;

	switch (cmd) {
	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		/*
		 * XXX should be able to handle color map in 4/8 bpp mode.
		 */
		return (EINVAL);
	
	case WSDISPLAYIO_SVIDEO:
		if (*(int *)data == WSDISPLAYIO_VIDEO_OFF)
			sc->sc_powerstate |= PWRSTAT_VIDEOOFF;
		else
			sc->sc_powerstate &= ~PWRSTAT_VIDEOOFF;
		sed1356_update_powerstate(sc, PWRSTAT_ALL);
		return 0;

	case WSDISPLAYIO_GVIDEO:
		*(int *)data = (sc->sc_powerstate&PWRSTAT_VIDEOOFF) ? 
				WSDISPLAYIO_VIDEO_OFF:WSDISPLAYIO_VIDEO_ON;
		return 0;


	case WSDISPLAYIO_GETPARAM:
		dispparam = (struct wsdisplay_param*)data;
		switch (dispparam->param) {
		case WSDISPLAYIO_PARAM_BACKLIGHT:
			VPRINTF(("sed1356_ioctl: GET:BACKLIGHT\n"));
			sed1356_init_brightness(sc, 0);
			sed1356_init_backlight(sc, 0);
			VPRINTF(("sed1356_ioctl: GET:(real)BACKLIGHT %d\n",
				 (sc->sc_powerstate&PWRSTAT_BACKLIGHT)? 1: 0));
			dispparam->min = 0;
			dispparam->max = 1;
			if (sc->sc_max_brightness > 0)
				dispparam->curval = sc->sc_brightness > 0? 1: 0;
			else
				dispparam->curval =
				    (sc->sc_powerstate&PWRSTAT_BACKLIGHT) ? 1: 0;
			VPRINTF(("sed1356_ioctl: GET:BACKLIGHT:%d(%s)\n",
				dispparam->curval,
				sc->sc_max_brightness > 0? "brightness": "light"));
			return 0;
			break;
		case WSDISPLAYIO_PARAM_CONTRAST:
			VPRINTF(("sed1356_ioctl: GET:CONTRAST\n"));
			sed1356_init_contrast(sc, 0);
			if (sc->sc_max_contrast > 0) {
				dispparam->min = 0;
				dispparam->max = sc->sc_max_contrast;
				dispparam->curval = sc->sc_contrast;
				VPRINTF(("sed1356_ioctl: GET:CONTRAST max=%d, current=%d\n", sc->sc_max_contrast, sc->sc_contrast));
				return 0;
			} else {
				VPRINTF(("sed1356_ioctl: GET:CONTRAST EINVAL\n"));
				return (EINVAL);
			}
			break;	
		case WSDISPLAYIO_PARAM_BRIGHTNESS:
			VPRINTF(("sed1356_ioctl: GET:BRIGHTNESS\n"));
			sed1356_init_brightness(sc, 0);
			if (sc->sc_max_brightness > 0) {
				dispparam->min = 0;
				dispparam->max = sc->sc_max_brightness;
				dispparam->curval = sc->sc_brightness;
				VPRINTF(("sed1356_ioctl: GET:BRIGHTNESS max=%d, current=%d\n", sc->sc_max_brightness, sc->sc_brightness));
				return 0;
			} else {
				VPRINTF(("sed1356_ioctl: GET:BRIGHTNESS EINVAL\n"));
				return (EINVAL);
			}
			return (EINVAL);
		default:
			return (EINVAL);
		}
		return (0);

	case WSDISPLAYIO_SETPARAM:
		dispparam = (struct wsdisplay_param*)data;
		switch (dispparam->param) {
		case WSDISPLAYIO_PARAM_BACKLIGHT:
			VPRINTF(("sed1356_ioctl: SET:BACKLIGHT\n"));
			if (dispparam->curval < 0 ||
			    1 < dispparam->curval)
				return (EINVAL);
			sed1356_init_brightness(sc, 0);
			VPRINTF(("sed1356_ioctl: SET:max brightness=%d\n", sc->sc_max_brightness));
			if (sc->sc_max_brightness > 0) { /* dimmer */
				if (dispparam->curval == 0){
					sc->sc_brightness_save = sc->sc_brightness;
					sed1356_set_brightness(sc, 0);	/* min */
				} else {
					if (sc->sc_brightness_save == 0)
						sc->sc_brightness_save = sc->sc_max_brightness;
					sed1356_set_brightness(sc, sc->sc_brightness_save);
				}
				VPRINTF(("sed1356_ioctl: SET:BACKLIGHT:brightness=%d\n", sc->sc_brightness));
			} else { /* off */
				if (dispparam->curval == 0)
					sc->sc_powerstate &= ~PWRSTAT_BACKLIGHT;
				else
					sc->sc_powerstate |= PWRSTAT_BACKLIGHT;
				VPRINTF(("sed1356_ioctl: SET:BACKLIGHT:powerstate %d\n",
						(sc->sc_powerstate & PWRSTAT_BACKLIGHT)?1:0));
				sed1356_update_powerstate(sc, PWRSTAT_BACKLIGHT);
				VPRINTF(("sed1356_ioctl: SET:BACKLIGHT:%d\n",
					(sc->sc_powerstate & PWRSTAT_BACKLIGHT)?1:0));
			}
			return 0;
			break;
		case WSDISPLAYIO_PARAM_CONTRAST:
			VPRINTF(("sed1356_ioctl: SET:CONTRAST\n"));
			sed1356_init_contrast(sc, 0);
			if (dispparam->curval < 0 ||
			    sc->sc_max_contrast < dispparam->curval)
				return (EINVAL);
			if (sc->sc_max_contrast > 0) {
				int org = sc->sc_contrast;
				sed1356_set_contrast(sc, dispparam->curval);	
				VPRINTF(("sed1356_ioctl: SET:CONTRAST org=%d, current=%d\n", org, sc->sc_contrast));
				return 0;
			} else {
				VPRINTF(("sed1356_ioctl: SET:CONTRAST EINVAL\n"));
				return (EINVAL);
			}
			break;
		case WSDISPLAYIO_PARAM_BRIGHTNESS:
			VPRINTF(("sed1356_ioctl: SET:BRIGHTNESS\n"));
			sed1356_init_brightness(sc, 0);
			if (dispparam->curval < 0 ||
			    sc->sc_max_brightness < dispparam->curval)
				return (EINVAL);
			if (sc->sc_max_brightness > 0) {
				int org = sc->sc_brightness;
				sed1356_set_brightness(sc, dispparam->curval);	
				VPRINTF(("sed1356_ioctl: SET:BRIGHTNESS org=%d, current=%d\n", org, sc->sc_brightness));
				return 0;
			} else {
				VPRINTF(("sed1356_ioctl: SET:BRIGHTNESS EINVAL\n"));
				return (EINVAL);
			}
			break;
		default:
			return (EINVAL);
		}
		return (0);

	case HPCFBIO_GCONF:
		fbconf = (struct hpcfb_fbconf *)data;
		if (fbconf->hf_conf_index != 0 &&
		    fbconf->hf_conf_index != HPCFB_CURRENT_CONFIG) {
			return (EINVAL);
		}
		*fbconf = sc->sc_fbconf;	/* structure assignment */
		return (0);
	case HPCFBIO_SCONF:
		fbconf = (struct hpcfb_fbconf *)data;
		if (fbconf->hf_conf_index != 0 &&
		    fbconf->hf_conf_index != HPCFB_CURRENT_CONFIG) {
			return (EINVAL);
		}
		/*
		 * nothing to do because we have only one configration
		 */
		return (0);
	case HPCFBIO_GDSPCONF:
		dspconf = (struct hpcfb_dspconf *)data;
		if ((dspconf->hd_unit_index != 0 &&
		     dspconf->hd_unit_index != HPCFB_CURRENT_UNIT) ||
		    (dspconf->hd_conf_index != 0 &&
		     dspconf->hd_conf_index != HPCFB_CURRENT_CONFIG)) {
			return (EINVAL);
		}
		*dspconf = sc->sc_dspconf;	/* structure assignment */
		return (0);
	case HPCFBIO_SDSPCONF:
		dspconf = (struct hpcfb_dspconf *)data;
		if ((dspconf->hd_unit_index != 0 &&
		     dspconf->hd_unit_index != HPCFB_CURRENT_UNIT) ||
		    (dspconf->hd_conf_index != 0 &&
		     dspconf->hd_conf_index != HPCFB_CURRENT_CONFIG)) {
			return (EINVAL);
		}
		/*
		 * nothing to do
		 * because we have only one unit and one configration
		 */
		return (0);
	case HPCFBIO_GOP:
	case HPCFBIO_SOP:
		/*
		 * curently not implemented...
		 */
		return (EINVAL);
	}

	return (ENOTTY);
}

paddr_t
sed1356_mmap(void *ctx, off_t offset, int prot)
{
	struct sed1356_softc *sc = (struct sed1356_softc *)ctx;

	if (offset < 0 ||
	    (sc->sc_fbconf.hf_bytes_per_plane +
		sc->sc_fbconf.hf_offset) <  offset)
		return -1;

	return __BTOP((u_long)bootinfo->fb_addr + offset);
}


void
sed1356_init_backlight(struct sed1356_softc *sc, int inattach)
{
	int val = -1;

	if (sc->sc_lcd_inited&BACKLIGHT_INITED)
		return;

	if (config_hook_call(CONFIG_HOOK_GET, 
	     CONFIG_HOOK_POWER_LCDLIGHT, &val) != -1) {
		/* we can get real light state */
		VPRINTF(("sed1356_init_backlight: real backlight=%d\n", val));
		if (val == 0)
			sc->sc_powerstate &= ~PWRSTAT_BACKLIGHT;
		else
			sc->sc_powerstate |= PWRSTAT_BACKLIGHT;
		sc->sc_lcd_inited |= BACKLIGHT_INITED;
	} else if (inattach) {
		/* 
		   we cannot get real light state in attach time
		   because light device not yet attached.
		   we will retry in !inattach.
		   temporary assume light is on.
		 */
		sc->sc_powerstate |= PWRSTAT_BACKLIGHT;
	} else {
		/* we cannot get real light state, so work by myself state */
		sc->sc_lcd_inited |= BACKLIGHT_INITED;
	}
}

void
sed1356_init_brightness(struct sed1356_softc *sc, int inattach)
{
	int val = -1;

	if (sc->sc_lcd_inited&BRIGHTNESS_INITED)
		return;

	VPRINTF(("sed1356_init_brightness\n"));
	if (config_hook_call(CONFIG_HOOK_GET, 
	     CONFIG_HOOK_BRIGHTNESS_MAX, &val) != -1) {
		/* we can get real brightness max */
		VPRINTF(("sed1356_init_brightness: real brightness max=%d\n", val));
		sc->sc_max_brightness = val;
		val = -1;
		if (config_hook_call(CONFIG_HOOK_GET, 
		     CONFIG_HOOK_BRIGHTNESS, &val) != -1) {
			/* we can get real brightness */
			VPRINTF(("sed1356_init_brightness: real brightness=%d\n", val));
			sc->sc_brightness_save = sc->sc_brightness = val;
		} else {
			sc->sc_brightness_save =
			sc->sc_brightness = sc->sc_max_brightness;
		}
		sc->sc_lcd_inited |= BRIGHTNESS_INITED;
	} else if (inattach) {
		/* 
		   we cannot get real brightness in attach time
		   because brightness device not yet attached.
		   we will retry in !inattach.
		 */
		sc->sc_max_brightness = -1;
		sc->sc_brightness = -1;
		sc->sc_brightness_save = -1;
	} else {
		/* we cannot get real brightness */
		sc->sc_lcd_inited |= BRIGHTNESS_INITED;
	}

	return;
}

void
sed1356_init_contrast(struct sed1356_softc *sc, int inattach)
{
	int val = -1;

	if (sc->sc_lcd_inited&CONTRAST_INITED)
		return;

	VPRINTF(("sed1356_init_contrast\n"));
	if (config_hook_call(CONFIG_HOOK_GET, 
	     CONFIG_HOOK_CONTRAST_MAX, &val) != -1) {
		/* we can get real contrast max */
		VPRINTF(("sed1356_init_contrast: real contrast max=%d\n", val));
		sc->sc_max_contrast = val;
		val = -1;
		if (config_hook_call(CONFIG_HOOK_GET, 
		     CONFIG_HOOK_CONTRAST, &val) != -1) {
			/* we can get real contrast */
			VPRINTF(("sed1356_init_contrast: real contrast=%d\n", val));
			sc->sc_contrast = val;
		} else {
			sc->sc_contrast = sc->sc_max_contrast;
		}
		sc->sc_lcd_inited |= CONTRAST_INITED;
	} else if (inattach) {
		/* 
		   we cannot get real contrast in attach time
		   because contrast device not yet attached.
		   we will retry in !inattach.
		 */
		sc->sc_max_contrast = -1;
		sc->sc_contrast = -1;
	} else {
		/* we cannot get real contrast */
		sc->sc_lcd_inited |= CONTRAST_INITED;
	}

	return;
}

void
sed1356_set_brightness(struct sed1356_softc *sc, int val)
{
	sc->sc_brightness = val;

	config_hook_call(CONFIG_HOOK_SET, CONFIG_HOOK_BRIGHTNESS, &val);
	if (config_hook_call(CONFIG_HOOK_GET, 
	     CONFIG_HOOK_BRIGHTNESS, &val) != -1) {
		sc->sc_brightness = val;
	}
}

void
sed1356_set_contrast(struct sed1356_softc *sc, int val)
{
	sc->sc_contrast = val;

	config_hook_call(CONFIG_HOOK_SET, CONFIG_HOOK_CONTRAST, &val);
	if (config_hook_call(CONFIG_HOOK_GET, 
	     CONFIG_HOOK_CONTRAST, &val) != -1) {
		sc->sc_contrast = val;
	}
}
