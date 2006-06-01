/*	$NetBSD: mq200.c,v 1.25.6.1 2006/06/01 22:34:32 kardel Exp $	*/

/*-
 * Copyright (c) 2000, 2001 TAKEMURA Shin
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mq200.c,v 1.25.6.1 2006/06/01 22:34:32 kardel Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <dev/wscons/wsconsio.h>

#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include "opt_mq200.h"
#include <hpcmips/dev/mq200reg.h>
#include <hpcmips/dev/mq200var.h>
#include <hpcmips/dev/mq200priv.h>

#include "bivideo.h"
#if NBIVIDEO > 0
#include <dev/hpc/bivideovar.h>
#endif

/*
 * function prototypes
 */
static void	mq200_power(int, void *);
static int	mq200_hardpower(void *, int, long, void *);
static int	mq200_fbinit(struct hpcfb_fbconf *);
static int	mq200_ioctl(void *, u_long, caddr_t, int, struct lwp *);
static paddr_t	mq200_mmap(void *, off_t offset, int);
static void	mq200_update_powerstate(struct mq200_softc *, int);
void	mq200_init_backlight(struct mq200_softc *, int);
void	mq200_init_brightness(struct mq200_softc *, int);
void	mq200_init_contrast(struct mq200_softc *, int);
void	mq200_set_brightness(struct mq200_softc *, int);
void	mq200_set_contrast(struct mq200_softc *, int);

/*
 * static variables
 */
struct hpcfb_accessops mq200_ha = {
	mq200_ioctl, mq200_mmap
};

#ifdef MQ200_DEBUG
int mq200_debug = MQ200DEBUG_CONF;
#endif

int
mq200_probe(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	unsigned long regval;

#if NBIVIDEO > 0
	if (bivideo_dont_attach) /* some video driver already attached */
		return (0);
#endif /* NBIVIDEO > 0 */

	regval = bus_space_read_4(iot, ioh, MQ200_PC00R);
	VPRINTF("probe: vendor id=%04lx product id=%04lx\n",
	    regval & 0xffff, (regval >> 16) & 0xffff);
	if (regval != ((MQ200_PRODUCT_ID << 16) | MQ200_VENDOR_ID))
		return (0);

	return (1);
}

void
mq200_attach(struct mq200_softc *sc)
{
	unsigned long regval;
	struct hpcfb_attach_args ha;
	int console = (bootinfo->bi_cnuse & BI_CNUSE_SERIAL) ? 0 : 1;

	printf(": ");
	if (mq200_fbinit(&sc->sc_fbconf) != 0) {
		/* just return so that hpcfb will not be attached */
		return;
	}

	sc->sc_fbconf.hf_baseaddr = (u_long)bootinfo->fb_addr;
	sc->sc_fbconf.hf_offset	= (u_long)sc->sc_fbconf.hf_baseaddr -
	    MIPS_PHYS_TO_KSEG1(mips_ptob(mips_btop(sc->sc_baseaddr)));
	DPRINTF("hf_baseaddr=%lx\n", sc->sc_fbconf.hf_baseaddr);
	DPRINTF("hf_offset=%lx\n", sc->sc_fbconf.hf_offset);

	regval = mq200_read(sc, MQ200_PC08R);
	printf("MQ200 Rev.%02lx video controller", regval & 0xff);
	if (console) {
		printf(", console");
	}
	printf("\n");
        printf("%s: framebuffer address: 0x%08lx\n",
	    sc->sc_dev.dv_xname, (u_long)bootinfo->fb_addr);

	/*
	 * setup registers
	 */
	sc->sc_flags = 0;
	sc->sc_baseclock = 12288;	/* 12.288 MHz */
#ifdef MQ200_DEBUG
	if (bootverbose) {
		/* dump current setting	*/
		mq200_dump_all(sc);
		mq200_dump_pll(sc);
	}
#endif
	mq200_setup_regctx(sc);
	mq200_mdsetup(sc);
	if (sc->sc_md) {
		int mode;

		switch (sc->sc_fbconf.hf_pixel_width) {
		case  1:	mode = MQ200_GCC_1BPP;		break;
		case  2:	mode = MQ200_GCC_2BPP;		break;
		case  4:	mode = MQ200_GCC_4BPP;		break;
		case  8:	mode = MQ200_GCC_8BPP;		break;
		case 16:	mode = MQ200_GCC_16BPP_DIRECT;	break;
		default:
			printf("%s: %dbpp isn't supported\n",
			    sc->sc_dev.dv_xname, sc->sc_fbconf.hf_pixel_width);
			return;
		}

		if (sc->sc_md->md_flags & MQ200_MD_HAVEFP) {
			sc->sc_flags |= MQ200_SC_GC2_ENABLE;	/* FP	*/
		}
#if MQ200_USECRT
		if (sc->sc_md->md_flags & MQ200_MD_HAVECRT) {
			int i;
			sc->sc_flags |= MQ200_SC_GC1_ENABLE;	/* CRT	*/
			for (i = 0; i < mq200_crt_nparams; i++) {
				sc->sc_crt = &mq200_crt_params[i];
				if (sc->sc_md->md_fp_width <=
				    mq200_crt_params[i].width &&
				    sc->sc_md->md_fp_height <=
				    mq200_crt_params[i].height)
					break;
			}
		}
#endif
		mq200_setup(sc);

		if (sc->sc_flags & MQ200_SC_GC2_ENABLE)	/* FP	*/
			mq200_win_enable(sc, MQ200_GC2, mode,
			    sc->sc_fbconf.hf_baseaddr,
			    sc->sc_fbconf.hf_width, sc->sc_fbconf.hf_height,
			    sc->sc_fbconf.hf_bytes_per_plane);
		if (sc->sc_flags & MQ200_SC_GC1_ENABLE)	/* CRT	*/
			mq200_win_enable(sc, MQ200_GC1, mode,
			    sc->sc_fbconf.hf_baseaddr,
			    sc->sc_fbconf.hf_width, sc->sc_fbconf.hf_height,
			    sc->sc_fbconf.hf_bytes_per_plane);
	}
#ifdef MQ200_DEBUG
	if (sc->sc_md == NULL || bootverbose) {
		mq200_dump_pll(sc);
	}
#endif

	/* Add a power hook to power saving */
	sc->sc_mq200pwstate = MQ200_POWERSTATE_D0;
	sc->sc_powerhook = powerhook_establish(mq200_power, sc);
	if (sc->sc_powerhook == NULL)
		printf("%s: WARNING: unable to establish power hook\n",
		    sc->sc_dev.dv_xname);

	/* Add a hard power hook to power saving */
	sc->sc_hardpowerhook = config_hook(CONFIG_HOOK_PMEVENT,
	    CONFIG_HOOK_PMEVENT_HARDPOWER,
	    CONFIG_HOOK_SHARE,
	    mq200_hardpower, sc);
	if (sc->sc_hardpowerhook == NULL)
		printf("%s: WARNING: unable to establish hard power hook\n",
		    sc->sc_dev.dv_xname);

	/* initialize backlight brightness and lcd contrast */
	sc->sc_lcd_inited = 0;
	mq200_init_brightness(sc, 1);
	mq200_init_contrast(sc, 1);
	mq200_init_backlight(sc, 1);

	if (console && hpcfb_cnattach(&sc->sc_fbconf) != 0) {
		panic("mq200_attach: can't init fb console");
	}

	ha.ha_console = console;
	ha.ha_accessops = &mq200_ha;
	ha.ha_accessctx = sc;
	ha.ha_curfbconf = 0;
	ha.ha_nfbconf = 1;
	ha.ha_fbconflist = &sc->sc_fbconf;
	ha.ha_curdspconf = 0;
	ha.ha_ndspconf = 1;
	ha.ha_dspconflist = &sc->sc_dspconf;

	config_found(&sc->sc_dev, &ha, hpcfbprint);

#if NBIVIDEO > 0
	/*
	 * bivideo is no longer need
	 */
	bivideo_dont_attach = 1;
#endif /* NBIVIDEO > 0 */
}

static void
mq200_update_powerstate(struct mq200_softc *sc, int updates)
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

static void
mq200_power(int why, void *arg)
{
	struct mq200_softc *sc = arg;

	switch (why) {
	case PWR_SUSPEND:
		sc->sc_powerstate |= PWRSTAT_SUSPEND;
		mq200_update_powerstate(sc, PWRSTAT_ALL);
		break;
	case PWR_STANDBY:
		sc->sc_powerstate |= PWRSTAT_SUSPEND;
		mq200_update_powerstate(sc, PWRSTAT_ALL);
		break;
	case PWR_RESUME:
		sc->sc_powerstate &= ~PWRSTAT_SUSPEND;
		mq200_update_powerstate(sc, PWRSTAT_ALL);
		break;
	}
}

static int
mq200_hardpower(void *ctx, int type, long id, void *msg)
{
	struct mq200_softc *sc = ctx;
	int why = (int)msg;

	switch (why) {
	case PWR_SUSPEND:
		sc->sc_mq200pwstate = MQ200_POWERSTATE_D2;
		break;
	case PWR_STANDBY:
		sc->sc_mq200pwstate = MQ200_POWERSTATE_D3;
		break;
	case PWR_RESUME:
		sc->sc_mq200pwstate = MQ200_POWERSTATE_D0;
		break;
	}

	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
	    MQ200_PMCSR, sc->sc_mq200pwstate);

	/*
	 * you should wait until the
	 * power state transit sequence will end.
	 */
	{
		unsigned long tmp;
		do {
			tmp = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
			    MQ200_PMCSR);
		} while ((tmp & 0x3) != (sc->sc_mq200pwstate & 0x3));
		delay(100000); /* XXX */
	}

	return (0);
}


static int
mq200_fbinit(struct hpcfb_fbconf *fb)
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
	bzero(fb, sizeof(*fb));

	fb->hf_conf_index	= 0;	/* configuration index		*/
	fb->hf_nconfs		= 1;   	/* how many configurations	*/
	strcpy(fb->hf_name, "built-in video");
					/* frame buffer name		*/
	strcpy(fb->hf_conf_name, "default");
					/* configuration name		*/
	fb->hf_height		= bootinfo->fb_height;
	fb->hf_width		= bootinfo->fb_width;
	fb->hf_baseaddr		= mips_ptob(mips_btop(bootinfo->fb_addr));
	fb->hf_offset		= (u_long)bootinfo->fb_addr - fb->hf_baseaddr;
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
		 * monochrome
		 */
	case BIFB_D1_M2L_1:
		fb->hf_access_flags |= HPCFB_ACCESS_REVERSE;
		/* fall through */
	case BIFB_D1_M2L_0:
		fb->hf_class = HPCFB_CLASS_GRAYSCALE;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
		fb->hf_pack_width = 8;
		fb->hf_pixels_per_pack = 8;
		fb->hf_pixel_width = 1;
		fb->hf_class_data_length = sizeof(struct hf_gray_tag);
		fb->hf_u.hf_gray.hf_flags = 0;	/* reserved for future use */
		break;

		/*
		 * gray scale
		 */
	case BIFB_D2_M2L_3:
	case BIFB_D2_M2L_3x2:
		fb->hf_access_flags |= HPCFB_ACCESS_REVERSE;
		/* fall through */
	case BIFB_D2_M2L_0:
	case BIFB_D2_M2L_0x2:
		fb->hf_class = HPCFB_CLASS_GRAYSCALE;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
		fb->hf_pack_width = 8;
		fb->hf_pixels_per_pack = 4;
		fb->hf_pixel_width = 2;
		fb->hf_class_data_length = sizeof(struct hf_gray_tag);
		fb->hf_u.hf_gray.hf_flags = 0;	/* reserved for future use */
		break;

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
		fb->hf_order_flags = HPCFB_REVORDER_BYTE;
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
		printf("unknown type (=%d).\n", bootinfo->fb_type);
		return (-1);
		break;
	}

	return (0); /* no error */
}

int
mq200_ioctl(v, cmd, data, flag, l)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct lwp *l;
{
	struct mq200_softc *sc = (struct mq200_softc *)v;
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_dspconf *dspconf;
	struct wsdisplay_cmap *cmap;
	struct wsdisplay_param *dispparam;

	switch (cmd) {
	case WSDISPLAYIO_GETCMAP:
		cmap = (struct wsdisplay_cmap *)data;

		if (sc->sc_fbconf.hf_class != HPCFB_CLASS_INDEXCOLOR ||
		    sc->sc_fbconf.hf_pack_width != 8 ||
		    256 <= cmap->index ||
		    256 - cmap->index < cmap->count)
			return (EINVAL);

		/*
		 * This driver can't get color map.
		 */
		return (EINVAL);

	case WSDISPLAYIO_PUTCMAP:
		/*
		 * This driver can't set color map.
		 */
		return (EINVAL);

	case WSDISPLAYIO_SVIDEO:
		if (*(int *)data == WSDISPLAYIO_VIDEO_OFF)
			sc->sc_powerstate |= PWRSTAT_VIDEOOFF;
		else
			sc->sc_powerstate &= ~PWRSTAT_VIDEOOFF;
		mq200_update_powerstate(sc, PWRSTAT_ALL);
		return 0;

	case WSDISPLAYIO_GVIDEO:
		*(int *)data = (sc->sc_powerstate&PWRSTAT_VIDEOOFF) ?
		    WSDISPLAYIO_VIDEO_OFF:WSDISPLAYIO_VIDEO_ON;
		return 0;

	case WSDISPLAYIO_GETPARAM:
		dispparam = (struct wsdisplay_param*)data;
		switch (dispparam->param) {
		case WSDISPLAYIO_PARAM_BACKLIGHT:
			VPRINTF("ioctl: GET:BACKLIGHT\n");
			mq200_init_brightness(sc, 0);
			mq200_init_backlight(sc, 0);
			VPRINTF("ioctl: GET:(real)BACKLIGHT %d\n",
			    (sc->sc_powerstate&PWRSTAT_BACKLIGHT)? 1: 0);
			dispparam->min = 0;
			dispparam->max = 1;
			if (sc->sc_max_brightness > 0)
				dispparam->curval = sc->sc_brightness > 0
				    ? 1: 0;
			else
				dispparam->curval =
				    (sc->sc_powerstate&PWRSTAT_BACKLIGHT)
				    ? 1: 0;
			VPRINTF("ioctl: GET:BACKLIGHT:%d(%s)\n",
			    dispparam->curval,
			    sc->sc_max_brightness > 0? "brightness": "light");
			return 0;
			break;
		case WSDISPLAYIO_PARAM_CONTRAST:
			VPRINTF("ioctl: GET:CONTRAST\n");
			mq200_init_contrast(sc, 0);
			if (sc->sc_max_contrast > 0) {
				dispparam->min = 0;
				dispparam->max = sc->sc_max_contrast;
				dispparam->curval = sc->sc_contrast;
				VPRINTF("ioctl: GET:CONTRAST"
				    " max=%d, current=%d\n",
				    sc->sc_max_contrast, sc->sc_contrast);
				return 0;
			} else {
				VPRINTF("ioctl: GET:CONTRAST EINVAL\n");
				return (EINVAL);
			}
			break;
		case WSDISPLAYIO_PARAM_BRIGHTNESS:
			VPRINTF("ioctl: GET:BRIGHTNESS\n");
			mq200_init_brightness(sc, 0);
			if (sc->sc_max_brightness > 0) {
				dispparam->min = 0;
				dispparam->max = sc->sc_max_brightness;
				dispparam->curval = sc->sc_brightness;
				VPRINTF("ioctl: GET:BRIGHTNESS"
				    " max=%d, current=%d\n",
				    sc->sc_max_brightness, sc->sc_brightness);
				return 0;
			} else {
				VPRINTF("ioctl: GET:BRIGHTNESS EINVAL\n");
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
			VPRINTF("ioctl: SET:BACKLIGHT\n");
			if (dispparam->curval < 0 ||
			    1 < dispparam->curval)
				return (EINVAL);
			mq200_init_brightness(sc, 0);
			VPRINTF("ioctl: SET:max brightness=%d\n",
			    sc->sc_max_brightness);
			if (sc->sc_max_brightness > 0) { /* dimmer */
				if (dispparam->curval == 0){
					sc->sc_brightness_save =
					    sc->sc_brightness;
					mq200_set_brightness(sc, 0); /* min */
				} else {
					if (sc->sc_brightness_save == 0)
						sc->sc_brightness_save =
						    sc->sc_max_brightness;
					mq200_set_brightness(sc,
					    sc->sc_brightness_save);
				}
				VPRINTF("ioctl: SET:BACKLIGHT:"
				    " brightness=%d\n", sc->sc_brightness);
			} else { /* off */
				if (dispparam->curval == 0)
					sc->sc_powerstate &= ~PWRSTAT_BACKLIGHT;
				else
					sc->sc_powerstate |= PWRSTAT_BACKLIGHT;
				VPRINTF("ioctl: SET:BACKLIGHT:"
				    " powerstate %d\n",
				    (sc->sc_powerstate & PWRSTAT_BACKLIGHT)
				    ? 1 : 0);
				mq200_update_powerstate(sc, PWRSTAT_BACKLIGHT);
				VPRINTF("ioctl: SET:BACKLIGHT:%d\n",
				    (sc->sc_powerstate & PWRSTAT_BACKLIGHT)
				    ? 1 : 0);
			}
			return 0;
			break;
		case WSDISPLAYIO_PARAM_CONTRAST:
			VPRINTF("ioctl: SET:CONTRAST\n");
			mq200_init_contrast(sc, 0);
			if (dispparam->curval < 0 ||
			    sc->sc_max_contrast < dispparam->curval)
				return (EINVAL);
			if (sc->sc_max_contrast > 0) {
				int org = sc->sc_contrast;
				mq200_set_contrast(sc, dispparam->curval);
				VPRINTF("ioctl: SET:CONTRAST"
				    " org=%d, current=%d\n", org,
				    sc->sc_contrast);
				VPRINTF("ioctl: SETPARAM:"
				    " CONTRAST org=%d, current=%d\n", org,
				    sc->sc_contrast);
				return 0;
			} else {
				VPRINTF("ioctl: SET:CONTRAST EINVAL\n");
				return (EINVAL);
			}
			break;
		case WSDISPLAYIO_PARAM_BRIGHTNESS:
			VPRINTF("ioctl: SET:BRIGHTNESS\n");
			mq200_init_brightness(sc, 0);
			if (dispparam->curval < 0 ||
			    sc->sc_max_brightness < dispparam->curval)
				return (EINVAL);
			if (sc->sc_max_brightness > 0) {
				int org = sc->sc_brightness;
				mq200_set_brightness(sc, dispparam->curval);
				VPRINTF("ioctl: SET:BRIGHTNESS"
				    " org=%d, current=%d\n", org,
				    sc->sc_brightness);
				return 0;
			} else {
				VPRINTF("ioctl: SET:BRIGHTNESS EINVAL\n");
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
		 * nothing to do because we have only one configuration
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
		 * because we have only one unit and one configuration
		 */
		return (0);
	case HPCFBIO_GOP:
	case HPCFBIO_SOP:
		/*
		 * curently not implemented...
		 */
		return (EINVAL);
	}

	return (EPASSTHROUGH);
}

paddr_t
mq200_mmap(void *ctx, off_t offset, int prot)
{
	struct mq200_softc *sc = (struct mq200_softc *)ctx;

	if (offset < 0 || MQ200_MAPSIZE <= offset)
		return -1;

	return mips_btop(sc->sc_baseaddr + offset);
}


void
mq200_init_backlight(struct mq200_softc *sc, int inattach)
{
	int val = -1;

	if (sc->sc_lcd_inited&BACKLIGHT_INITED)
		return;

	if (config_hook_call(CONFIG_HOOK_GET,
	    CONFIG_HOOK_POWER_LCDLIGHT, &val) != -1) {
		/* we can get real light state */
		VPRINTF("init_backlight: real backlight=%d\n", val);
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
mq200_init_brightness(struct mq200_softc *sc, int inattach)
{
	int val = -1;

	if (sc->sc_lcd_inited&BRIGHTNESS_INITED)
		return;

	VPRINTF("init_brightness\n");
	if (config_hook_call(CONFIG_HOOK_GET,
	    CONFIG_HOOK_BRIGHTNESS_MAX, &val) != -1) {
		/* we can get real brightness max */
		VPRINTF("init_brightness: real brightness max=%d\n", val);
		sc->sc_max_brightness = val;
		val = -1;
		if (config_hook_call(CONFIG_HOOK_GET,
		    CONFIG_HOOK_BRIGHTNESS, &val) != -1) {
			/* we can get real brightness */
			VPRINTF("init_brightness: real brightness=%d\n", val);
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
mq200_init_contrast(struct mq200_softc *sc, int inattach)
{
	int val = -1;

	if (sc->sc_lcd_inited&CONTRAST_INITED)
		return;

	VPRINTF("init_contrast\n");
	if (config_hook_call(CONFIG_HOOK_GET,
	    CONFIG_HOOK_CONTRAST_MAX, &val) != -1) {
		/* we can get real contrast max */
		VPRINTF("init_contrast: real contrast max=%d\n", val);
		sc->sc_max_contrast = val;
		val = -1;
		if (config_hook_call(CONFIG_HOOK_GET,
		    CONFIG_HOOK_CONTRAST, &val) != -1) {
			/* we can get real contrast */
			VPRINTF("init_contrast: real contrast=%d\n", val);
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
mq200_set_brightness(struct mq200_softc *sc, int val)
{
	sc->sc_brightness = val;

	config_hook_call(CONFIG_HOOK_SET, CONFIG_HOOK_BRIGHTNESS, &val);
	if (config_hook_call(CONFIG_HOOK_GET,
	    CONFIG_HOOK_BRIGHTNESS, &val) != -1) {
		sc->sc_brightness = val;
	}
}

void
mq200_set_contrast(struct mq200_softc *sc, int val)
{
	sc->sc_contrast = val;

	config_hook_call(CONFIG_HOOK_SET, CONFIG_HOOK_CONTRAST, &val);
	if (config_hook_call(CONFIG_HOOK_GET,
	    CONFIG_HOOK_CONTRAST, &val) != -1) {
		sc->sc_contrast = val;
	}
}
