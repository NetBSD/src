/*	$NetBSD: mq200.c,v 1.2.2.6 2001/03/12 13:28:37 bouyer Exp $	*/

/*-
 * Copyright (c) 2000 Takemura Shin
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

#include <hpcmips/dev/mq200reg.h>
#include <hpcmips/dev/mq200var.h>
#include "bivideo.h"
#if NBIVIDEO > 0
#include <dev/hpc/bivideovar.h>     
#endif

#define MQ200DEBUG
#ifdef MQ200DEBUG
#ifndef MQ200DEBUG_CONF
#define MQ200DEBUG_CONF 0
#endif
int	mq200_debug = MQ200DEBUG_CONF;
#define	DPRINTF(arg)     do { if (mq200_debug) printf arg; } while(0);
#define	DPRINTFN(n, arg) do { if (mq200_debug > (n)) printf arg; } while (0);
#define	VPRINTF(arg)     do { if (bootverbose || mq200_debug) printf arg; } while(0);
#define	VPRINTFN(n, arg) do { if (bootverbose || mq200_debug > (n)) printf arg; } while (0);
#else
#define	DPRINTF(arg)     do { } while (0);
#define DPRINTFN(n, arg) do { } while (0);
#define	VPRINTF(arg)     do { if (bootverbose) printf arg; } while(0);
#define	VPRINTFN(n, arg) do { if (bootverbose) printf arg; } while (0);
#endif

/*
 * function prototypes
 */
static void	mq200_power __P((int, void *));
static int	mq200_hardpower __P((void *, int, long, void *));
static int	mq200_fbinit __P((struct hpcfb_fbconf *));
static int	mq200_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static paddr_t	mq200_mmap __P((void *, off_t offset, int));
static void	mq200_update_powerstate __P((struct mq200_softc *, int));
void	mq200_get_backlight __P((struct mq200_softc *));
void	mq200_init_brightness __P((struct mq200_softc *));
void	mq200_init_contrast __P((struct mq200_softc *));
void	mq200_set_brightness __P((struct mq200_softc *, int));
void	mq200_set_contrast __P((struct mq200_softc *, int));

/*
 * static variables
 */
struct hpcfb_accessops mq200_ha = {
	mq200_ioctl, mq200_mmap
};

int
mq200_probe(iot, ioh)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{
	unsigned long regval;

#if NBIVIDEO > 0
	if (bivideo_dont_attach) /* some video driver already attached */
		return (0);
#endif /* NBIVIDEO > 0 */

	regval = bus_space_read_4(iot, ioh, MQ200_PC00R);
	VPRINTF(("mq200 probe: vendor id=%04lx product id=%04lx\n",
		 regval & 0xffff, (regval >> 16) & 0xffff));
	if (regval != ((MQ200_PRODUCT_ID << 16) | MQ200_VENDOR_ID))
		return (0);

	return (1);
}

void
mq200_attach(sc)
	struct mq200_softc *sc;
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
	DPRINTF(("hf_baseaddr=%lx\n", sc->sc_fbconf.hf_baseaddr));
	DPRINTF(("hf_offset=%lx\n", sc->sc_fbconf.hf_offset));

	regval = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MQ200_PC08R);
	printf("MQ200 Rev.%02lx video controller", regval & 0xff);
	if (console) {
		printf(", console");
	}
	printf("\n");
        printf("%s: framebuffer address: 0x%08lx\n",
		sc->sc_dev.dv_xname, (u_long)bootinfo->fb_addr);
	
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
	sc->sc_brightness = sc->sc_contrast =
	sc->sc_max_brightness = sc->sc_max_contrast = -1;
	mq200_init_brightness(sc);
	mq200_init_contrast(sc);
	mq200_get_backlight(sc);

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
mq200_update_powerstate(sc, updates)
	struct mq200_softc *sc;
	int updates;
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
mq200_power(why, arg)
	int why;
	void *arg;
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
mq200_hardpower(ctx, type, id, msg)
	void *ctx;
	int type;
	long id;
	void *msg;
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
mq200_fbinit(fb)
	struct hpcfb_fbconf *fb;
{

	/*
	 * get fb settings from bootinfo
	 */
	if (bootinfo == NULL ||
	    bootinfo->fb_addr == 0 ||
	    bootinfo->fb_line_bytes == 0 ||
	    bootinfo->fb_width == 0 ||
	    bootinfo->fb_height == 0) {
		printf("no frame buffer infomation.\n");
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
#if BYTE_ORDER == LITTLE_ENDIAN
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
		printf("unknown type (=%d).\n", bootinfo->fb_type);
		return (-1);
		break;
	}

	return (0); /* no error */
}

int
mq200_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct mq200_softc *sc = (struct mq200_softc *)v;
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_dspconf *dspconf;
	struct wsdisplay_cmap *cmap;
	struct wsdisplay_param *dispparam;

	switch (cmd) {
	case WSDISPLAYIO_GETCMAP:
		cmap = (struct wsdisplay_cmap*)data;

		if (sc->sc_fbconf.hf_class != HPCFB_CLASS_INDEXCOLOR ||
		    sc->sc_fbconf.hf_pack_width != 8 ||
		    256 <= cmap->index ||
		    256 < (cmap->index + cmap->count))
			return (EINVAL);

#if 0
		if (!uvm_useracc(cmap->red, cmap->count, B_WRITE) ||
		    !uvm_useracc(cmap->green, cmap->count, B_WRITE) ||
		    !uvm_useracc(cmap->blue, cmap->count, B_WRITE))
			return (EFAULT);

		copyout(&bivideo_cmap_r[cmap->index], cmap->red, cmap->count);
		copyout(&bivideo_cmap_g[cmap->index], cmap->green,cmap->count);
		copyout(&bivideo_cmap_b[cmap->index], cmap->blue, cmap->count);
#endif

		return (0);

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
			VPRINTF(("mq200_ioctl: GETPARAM:BACKLIGHT call\n"));
			if (sc->sc_max_brightness == -1)
				mq200_init_brightness(sc);
			mq200_get_backlight(sc);
			dispparam->min = 0;
			dispparam->max = 1;
			if (sc->sc_max_brightness > 0)
				dispparam->curval = sc->sc_brightness > 0? 1: 0;
			else
				dispparam->curval =
				    (sc->sc_powerstate&PWRSTAT_BACKLIGHT) ? 1 : 0;
			VPRINTF(("mq200_ioctl: GETPARAM:BACKLIGHT:%d\n",
				dispparam->curval));
			return 0;
			break;
		case WSDISPLAYIO_PARAM_CONTRAST:
			VPRINTF(("mq200_ioctl: GETPARAM:CONTRAST call\n"));
			if (sc->sc_max_contrast == -1)
				mq200_init_contrast(sc);
			if (sc->sc_max_contrast > 0) {
				dispparam->min = 0;
				dispparam->max = sc->sc_max_contrast;
				dispparam->curval = sc->sc_contrast;
				VPRINTF(("mq200_ioctl: GETPARAM:CONTRAST max=%d, current=%d\n", sc->sc_max_contrast, sc->sc_contrast));
				return 0;
			} else {
				VPRINTF(("mq200_ioctl: GETPARAM:CONTRAST ret\n"));
				return (EINVAL);
			}
			break;	
		case WSDISPLAYIO_PARAM_BRIGHTNESS:
			VPRINTF(("mq200_ioctl: GETPARAM:BRIGHTNESS call\n"));
			if (sc->sc_max_brightness == -1)
				mq200_init_brightness(sc);
			if (sc->sc_max_brightness > 0) {
				dispparam->min = 0;
				dispparam->max = sc->sc_max_brightness;
				dispparam->curval = sc->sc_brightness;
				VPRINTF(("mq200_ioctl: GETPARAM:BRIGHTNESS max=%d, current=%d\n", sc->sc_max_brightness, sc->sc_brightness));
				return 0;
			} else {
				VPRINTF(("mq200_ioctl: GETPARAM:BRIGHTNESS ret\n"));
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
			VPRINTF(("mq200_ioctl: SETPARAM:BACKLIGHT call\n"));
			if (dispparam->curval < 0 ||
			    1 < dispparam->curval)
				return (EINVAL);
			if (sc->sc_max_brightness == -1)
				mq200_init_brightness(sc);
			VPRINTF(("mq200_ioctl: SETPARAM:max brightness=%d\n", sc->sc_max_brightness));
			if (sc->sc_max_brightness > 0) { /* dimmer */
				if (dispparam->curval == 0){
					sc->sc_brightness_save = sc->sc_brightness;
					mq200_set_brightness(sc, 0);	/* min */
				} else {
					if (sc->sc_brightness_save == 0)
						sc->sc_brightness_save = sc->sc_max_brightness;
					mq200_set_brightness(sc, sc->sc_brightness_save);
				}
				VPRINTF(("mq200_ioctl: SETPARAM:BACKLIGHT: brightness=%d\n", sc->sc_brightness));
			} else { /* off */
				if (dispparam->curval == 0)
					sc->sc_powerstate &= ~PWRSTAT_BACKLIGHT;
				else
					sc->sc_powerstate |= PWRSTAT_BACKLIGHT;
				VPRINTF(("mq200_ioctl: SETPARAM:BACKLIGHT: powerstate %d\n",
						(sc->sc_powerstate & PWRSTAT_BACKLIGHT)?1:0));
				mq200_update_powerstate(sc, PWRSTAT_BACKLIGHT);
				VPRINTF(("mq200_ioctl: SETPARAM:BACKLIGHT:%d\n",
					(sc->sc_powerstate & PWRSTAT_BACKLIGHT)?1:0));
			}
			return 0;
			break;
		case WSDISPLAYIO_PARAM_CONTRAST:
			VPRINTF(("mq200_ioctl: SETPARAM:CONTRAST call\n"));
			if (sc->sc_max_contrast == -1)
				mq200_init_contrast(sc);
			if (dispparam->curval < 0 ||
			    sc->sc_max_contrast < dispparam->curval)
				return (EINVAL);
			if (sc->sc_max_contrast > 0) {
				int org = sc->sc_contrast;
				mq200_set_contrast(sc, dispparam->curval);	
				VPRINTF(("mq200_ioctl: SETPARAM:CONTRAST org=%d, current=%d\n", org, sc->sc_contrast));
				return 0;
			} else {
				VPRINTF(("mq200_ioctl: SETPARAM:CONTRAST ret\n"));
				return (EINVAL);
			}
			break;
		case WSDISPLAYIO_PARAM_BRIGHTNESS:
			VPRINTF(("mq200_ioctl: SETPARAM:BRIGHTNESS call\n"));
			if (sc->sc_max_brightness == -1)
				mq200_init_brightness(sc);
			if (dispparam->curval < 0 ||
			    sc->sc_max_brightness < dispparam->curval)
				return (EINVAL);
			if (sc->sc_max_brightness > 0) {
				int org = sc->sc_brightness;
				mq200_set_brightness(sc, dispparam->curval);	
				VPRINTF(("mq200_ioctl: SETPARAM:BRIGHTNESS org=%d, current=%d\n", org, sc->sc_brightness));
				return 0;
			} else {
				VPRINTF(("mq200_ioctl: SETPARAM:BRIGHTNESS ret\n"));
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
mq200_mmap(ctx, offset, prot)
	void *ctx;
	off_t offset;
	int prot;
{
	struct mq200_softc *sc = (struct mq200_softc *)ctx;

	if (offset < 0 || MQ200_MAPSIZE <= offset)
		return -1;

	return mips_btop(sc->sc_baseaddr + offset);
}


void
mq200_get_backlight(sc)
	struct mq200_softc *sc;
{
	int val = -1;

	if (config_hook_call(CONFIG_HOOK_GET, 
	     CONFIG_HOOK_POWER_LCDLIGHT, &val) != -1) {
		if (val == 0)
			sc->sc_powerstate &= ~PWRSTAT_BACKLIGHT;
		else
			sc->sc_powerstate |= PWRSTAT_BACKLIGHT;
	} else /* assume backlight is on */
		sc->sc_powerstate |= PWRSTAT_BACKLIGHT;
}

void
mq200_init_brightness(sc)
	struct mq200_softc *sc;
{
	int val = -1;

	VPRINTF(("mq200_init_brightness\n"));
	if (config_hook_call(CONFIG_HOOK_GET, 
	     CONFIG_HOOK_BRIGHTNESS, &val) != -1) {
		sc->sc_brightness_save = sc->sc_brightness = val;
	}
	val = -1;
	if (config_hook_call(CONFIG_HOOK_GET, 
	     CONFIG_HOOK_BRIGHTNESS_MAX, &val) != -1) {
		sc->sc_max_brightness = val;
	}
	return;
}


void
mq200_init_contrast(sc)
	struct mq200_softc *sc;
{
	int val = -1;

	VPRINTF(("mq200_init_contrast\n"));
	if (config_hook_call(CONFIG_HOOK_GET, 
	     CONFIG_HOOK_CONTRAST, &val) != -1) {
		sc->sc_contrast = val;
	}
	val = -1;
	if (config_hook_call(CONFIG_HOOK_GET, 
	     CONFIG_HOOK_CONTRAST_MAX, &val) != -1) {
		sc->sc_max_contrast = val;
	}
	return;
}

void
mq200_set_brightness(sc, val)
	struct mq200_softc *sc;
	int val;
{
	sc->sc_brightness = val;

	config_hook_call(CONFIG_HOOK_SET, CONFIG_HOOK_BRIGHTNESS, &val);
	if (config_hook_call(CONFIG_HOOK_GET, 
	     CONFIG_HOOK_BRIGHTNESS, &val) != -1) {
		sc->sc_brightness = val;
	}
}

void
mq200_set_contrast(sc, val)
	struct mq200_softc *sc;
	int val;
{
	sc->sc_contrast = val;

	config_hook_call(CONFIG_HOOK_SET, CONFIG_HOOK_CONTRAST, &val);
	if (config_hook_call(CONFIG_HOOK_GET, 
	     CONFIG_HOOK_CONTRAST, &val) != -1) {
		sc->sc_contrast = val;
	}
}
