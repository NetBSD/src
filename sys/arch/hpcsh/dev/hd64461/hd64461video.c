/*	$NetBSD: hd64461video.c,v 1.50.4.1 2009/05/13 17:17:47 jym Exp $	*/

/*-
 * Copyright (c) 2001, 2002, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hd64461video.c,v 1.50.4.1 2009/05/13 17:17:47 jym Exp $");

#include "opt_hd64461video.h"
// #define HD64461VIDEO_HWACCEL

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <sys/conf.h> /* cdev_decl */
#include <dev/cons.h> /* consdev */

/* ioctl */
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcsh/dev/hd64461/hd64461var.h>
#include <hpcsh/dev/hd64461/hd64461reg.h>
#include <hpcsh/dev/hd64461/hd64461videoreg.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <dev/wscons/wsconsio.h>
#include <dev/hpc/hpcfbvar.h>
#include <dev/hpc/hpcfbio.h>
#include <dev/hpc/video_subr.h>

#include <machine/config_hook.h>
#include <machine/bootinfo.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#ifdef	HD64461VIDEO_DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	hd64461video_debug
#endif
#include <machine/debug.h>

struct hd64461video_chip;
struct hd64461video_font {
	struct wsdisplay_font wsfont;	
	int c, cw, cstep;
	int loaded;
};

struct hd64461video_softc {
	device_t sc_dev;

	enum hd64461_module_id sc_module_id;
	struct hd64461video_chip *sc_vc;

	struct hd64461video_font sc_font;
};

enum hd64461video_display_mode {
	LCD256_C,
	LCD64K_C,
	LCD64_MONO,
	LCD16_MONO,
	LCD4_MONO,
	LCD2_MONO,
	CRT256_C,
	LCDCRT
};

STATIC struct hd64461video_chip {
	struct video_chip vc;
	enum hd64461video_display_mode mode;
	struct hpcfb_dspconf hd;
	struct hpcfb_fbconf hf;
	uint8_t *off_screen_addr;
	size_t off_screen_size;

	struct callout unblank_ch;
	int blanked;

	int console;
} hd64461video_chip;

void	hd64461video_cnprobe(struct consdev *);
void	hd64461video_cninit(struct consdev *);

STATIC int hd64461video_match(device_t, cfdata_t, void *);
STATIC void hd64461video_attach(device_t, device_t, void *);

STATIC void hd64461video_setup_hpcfbif(struct hd64461video_chip *);
STATIC void hd64461video_update_videochip_status(struct hd64461video_chip *);
STATIC size_t hd64461video_frame_buffer_size(struct hd64461video_chip *);
STATIC void hd64461video_hwaccel_init(struct hd64461video_chip *);

STATIC void hd64461video_set_clut(struct hd64461video_chip *, int, int,
    uint8_t *, uint8_t *, uint8_t *);
STATIC void hd64461video_get_clut(struct hd64461video_chip *, int, int,
    uint8_t *, uint8_t *, uint8_t *);
STATIC int hd64461video_power(void *, int, long, void *);
STATIC void hd64461video_off(struct hd64461video_chip *);
STATIC void hd64461video_on(struct hd64461video_chip *);
STATIC void hd64461video_display_onoff(void *, bool);
STATIC void hd64461video_display_on(void *);

#if notyet
STATIC void hd64461video_set_display_mode(struct hd64461video_chip *);
STATIC void hd64461video_set_display_mode_lcdc(struct hd64461video_chip *);
STATIC void hd64461video_set_display_mode_crtc(struct hd64461video_chip *);
#endif

#ifdef HD64461VIDEO_DEBUG
STATIC void hd64461video_info(struct hd64461video_softc *);
STATIC void hd64461video_dump(void) __attribute__((__unused__));
#endif

CFATTACH_DECL_NEW(hd64461video, sizeof(struct hd64461video_softc),
    hd64461video_match, hd64461video_attach, NULL, NULL);

STATIC int hd64461video_ioctl(void *, u_long, void *, int, struct lwp *);
STATIC paddr_t hd64461video_mmap(void *, off_t, int);

#ifdef HD64461VIDEO_HWACCEL
STATIC void hd64461video_cursor(void *, int, int, int, int, int);
STATIC void hd64461video_bitblit(void *, int, int, int, int, int, int);
STATIC void hd64461video_erase(void *, int, int, int, int, int);
STATIC void hd64461video_putchar(void *, int, int, struct wsdisplay_font *,
    int, int, uint, int);
STATIC void hd64461video_setclut(void *, struct rasops_info *);
STATIC void hd64461video_font(void *, struct wsdisplay_font *);
STATIC void hd64461video_iodone(void *);

/* font */
STATIC void hd64461video_font_load_16bpp(uint16_t *, uint8_t *, int, int, int);
STATIC void hd64461video_font_load_8bpp(uint8_t *, uint8_t *, int, int, int);
STATIC void hd64461video_font_set_attr(struct hd64461video_softc *,
    struct wsdisplay_font *);
STATIC void hd64461video_font_load(struct hd64461video_softc *);
STATIC vaddr_t hd64461video_font_start_addr(struct hd64461video_softc *, int);
#endif /* HD64461VIDEO_HWACCEL */

STATIC struct hpcfb_accessops hd64461video_ha = {
	.ioctl	= hd64461video_ioctl,
	.mmap	= hd64461video_mmap,
#ifdef HD64461VIDEO_HWACCEL
	.cursor	= hd64461video_cursor,
	.bitblit= hd64461video_bitblit,
	.erase	= hd64461video_erase,
	.putchar= hd64461video_putchar,
	.setclut= hd64461video_setclut,
	.font	= hd64461video_font,
	.iodone	= hd64461video_iodone
#endif /* HD64461VIDEO_HWACCEL */
};


STATIC int
hd64461video_match(device_t parent, cfdata_t cf, void *aux)
{
	struct hd64461_attach_args *ha = aux;

	return (ha->ha_module_id == HD64461_MODULE_VIDEO);
}

STATIC void
hd64461video_attach(device_t parent, device_t self, void *aux)
{
	struct hd64461_attach_args *ha = aux;
	struct hd64461video_softc *sc;
	struct hpcfb_attach_args hfa;
	struct video_chip *vc = &hd64461video_chip.vc;
	char pbuf[9];
	size_t fbsize, on_screen_size;

	aprint_naive("\n");
	aprint_normal(": ");

	sc = device_private(self);
	sc->sc_dev = self;

	sc->sc_module_id = ha->ha_module_id;
	sc->sc_vc = &hd64461video_chip;

	/* detect frame buffer size */
	fbsize = hd64461video_frame_buffer_size(&hd64461video_chip);
	format_bytes(pbuf, sizeof(pbuf), fbsize);
	aprint_normal("frame buffer = %s ", pbuf);

	/* update chip status */
	hd64461video_update_videochip_status(&hd64461video_chip);

	hd64461video_display_onoff(&hd64461video_chip, true);
//	hd64461video_set_display_mode(&hd64461video_chip);

	if (hd64461video_chip.console)
		aprint_normal(", console");

	aprint_normal("\n");
#ifdef HD64461VIDEO_DEBUG
	hd64461video_info(sc);
	hd64461video_dump();
#endif
	/* Add a hard power hook to power saving */
	config_hook(CONFIG_HOOK_PMEVENT, CONFIG_HOOK_PMEVENT_HARDPOWER,
	    CONFIG_HOOK_SHARE, hd64461video_power, sc);

	/* setup hpcfb interface */
	hd64461video_setup_hpcfbif(&hd64461video_chip);

	/* setup off-screen buffer */
	on_screen_size = (vc->vc_fbwidth * vc->vc_fbheight * vc->vc_fbdepth) /
	    NBBY;
	hd64461video_chip.off_screen_addr = (uint8_t *)vc->vc_fbvaddr +
	    on_screen_size;
	hd64461video_chip.off_screen_size = fbsize - on_screen_size;
	/* clean up off-screen area */
	{
		uint8_t *p = hd64461video_chip.off_screen_addr;
		uint8_t *end = p + hd64461video_chip.off_screen_size;
		while (p < end)
			*p++ = 0xff;
	}

	/* initialize hardware acceralation */
	hd64461video_hwaccel_init(&hd64461video_chip);

	/* register interface to hpcfb */
	hfa.ha_console	   = hd64461video_chip.console;
	hfa.ha_accessops   = &hd64461video_ha;
	hfa.ha_accessctx   = sc;
	hfa.ha_curfbconf   = 0;
	hfa.ha_nfbconf	   = 1;
	hfa.ha_fbconflist  = &hd64461video_chip.hf;
	hfa.ha_curdspconf  = 0;
	hfa.ha_ndspconf	   = 1;
	hfa.ha_dspconflist = &hd64461video_chip.hd;
	
	config_found(self, &hfa, hpcfbprint);

	/*
	 * XXX: TODO: for now this device manages power using
	 * config_hook(9) registered with hpcapm(4).
	 *
	 * We cannot yet switch it to pmf(9) hooks because only apm(4)
	 * uses them, apmdev(4) doesn't, but hpcapm(4) is the parent
	 * device for both, so its hooks are always run.
	 *
	 * We probably want to register shutdown hook with pmf(9) to
	 * make sure display is powered on before we reboot in case we
	 * end up in ddb early on.
	 */
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "unable to establish power handler\n");
}

/* console support */
void
hd64461video_cninit(struct consdev *cndev)
{
	hd64461video_chip.console = 1;
	hd64461video_chip.vc.vc_reverse = video_reverse_color();

	hd64461video_update_videochip_status(&hd64461video_chip);
	hd64461video_setup_hpcfbif(&hd64461video_chip);	
	hpcfb_cnattach(&hd64461video_chip.hf);

	cn_tab->cn_pri = CN_INTERNAL;
}

void
hd64461video_cnprobe(struct consdev *cndev)
{
#if NWSDISPLAY > 0
	extern const struct cdevsw wsdisplay_cdevsw;
	int maj, unit;
#endif
	cndev->cn_dev = NODEV;
	cndev->cn_pri = CN_NORMAL;

#if NWSDISPLAY > 0
	unit = 0;
	maj = cdevsw_lookup_major(&wsdisplay_cdevsw);

	if (maj != -1) {
		cndev->cn_pri = CN_INTERNAL;
		cndev->cn_dev = makedev(maj, unit);
	}
#endif /* NWSDISPLAY > 0 */
}

/* hpcfb support */
STATIC void
hd64461video_setup_hpcfbif(struct hd64461video_chip *hvc)
{
	struct video_chip *vc = &hvc->vc;
	struct hpcfb_fbconf *fb = &hvc->hf;
	vaddr_t fbvaddr = vc->vc_fbvaddr;
	int height = vc->vc_fbheight;
	int width = vc->vc_fbwidth;
	int depth = vc->vc_fbdepth;
	
	memset(fb, 0, sizeof(struct hpcfb_fbconf));
	
	fb->hf_conf_index	= 0;	/* configuration index		*/
	fb->hf_nconfs		= 1;   	/* how many configurations	*/
	strncpy(fb->hf_name, "HD64461 video module", HPCFB_MAXNAMELEN);

	/* frame buffer name */
	strncpy(fb->hf_conf_name, "LCD", HPCFB_MAXNAMELEN);

	/* configuration name */
	fb->hf_height		= height;
	fb->hf_width		= width;
	fb->hf_baseaddr		= (u_long)fbvaddr;
	fb->hf_offset		= (u_long)fbvaddr - 
	    sh3_ptob(sh3_btop(fbvaddr));

	/* frame buffer start offset */
	fb->hf_bytes_per_line	= (width * depth) / NBBY;
	fb->hf_nplanes		= 1;
	fb->hf_bytes_per_plane	= height * fb->hf_bytes_per_line;

	fb->hf_access_flags |= HPCFB_ACCESS_BYTE;
	fb->hf_access_flags |= HPCFB_ACCESS_WORD;
	fb->hf_access_flags |= HPCFB_ACCESS_DWORD;
	if (vc->vc_reverse)
		fb->hf_access_flags |= HPCFB_ACCESS_REVERSE;

	switch (depth) {
	default:
		panic("%s: not supported color depth", __func__);
		/* NOTREACHED */
	case 16:
		fb->hf_class = HPCFB_CLASS_RGBCOLOR;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
		fb->hf_pack_width = 16;
		fb->hf_pixels_per_pack = 1;
		fb->hf_pixel_width = 16;
		/*
		 * XXX: uwe: if I RTFS correctly, this really means
		 * that uint16_t pixel is fetched as little endian.
		 */
		fb->hf_order_flags = HPCFB_REVORDER_BYTE;

		fb->hf_class_data_length = sizeof(struct hf_rgb_tag);
		/* reserved for future use */
		fb->hf_u.hf_rgb.hf_flags = 0;

		fb->hf_u.hf_rgb.hf_red_width = 5;
		fb->hf_u.hf_rgb.hf_red_shift = 11;
		fb->hf_u.hf_rgb.hf_green_width = 6;
		fb->hf_u.hf_rgb.hf_green_shift = 5;
		fb->hf_u.hf_rgb.hf_blue_width = 5;
		fb->hf_u.hf_rgb.hf_blue_shift = 0;
		fb->hf_u.hf_rgb.hf_alpha_width = 0;
		fb->hf_u.hf_rgb.hf_alpha_shift = 0;
		break;

	case 8:
		fb->hf_class = HPCFB_CLASS_INDEXCOLOR;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
		fb->hf_pack_width = 8;
		fb->hf_pixels_per_pack = 1;
		fb->hf_pixel_width = 8;
		fb->hf_class_data_length = sizeof(struct hf_indexed_tag);
		/* reserved for future use */
		fb->hf_u.hf_indexed.hf_flags = 0;
		break;
	}
}

STATIC void
hd64461video_hwaccel_init(struct hd64461video_chip *hvc)
{
	uint16_t r;

	r = HD64461_LCDGRCFGR_ACCRESET;
	switch (hvc->vc.vc_fbdepth) {
	default:
		panic("no bitblit acceralation.");
	case 16:
		break;
	case 8:
		r |= HD64461_LCDGRCFGR_COLORDEPTH_8BPP;
		break;
	}
	hd64461_reg_write_2(HD64461_LCDGRCFGR_REG16, r);

	while ((hd64461_reg_read_2(HD64461_LCDGRCFGR_REG16) &
	    HD64461_LCDGRCFGR_ACCSTATUS) != 0)
		continue;

	r &= ~HD64461_LCDGRCFGR_ACCRESET;
	hd64461_reg_write_2(HD64461_LCDGRCFGR_REG16, r);

	while ((hd64461_reg_read_2(HD64461_LCDGRCFGR_REG16) &
	    HD64461_LCDGRCFGR_ACCSTATUS) != 0)
		continue;

	hd64461_reg_write_2(HD64461_LCDGRDOR_REG16,
	    (hvc->vc.vc_fbwidth - 1) & HD64461_LCDGRDOR_MASK);
}

/* hpcfb ops */
STATIC int
hd64461video_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct hd64461video_softc *sc = (struct hd64461video_softc *)v;
	struct hpcfb_fbconf *hf = &sc->sc_vc->hf;
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_dspconf *dspconf;
	struct wsdisplay_cmap *cmap;
	struct wsdisplay_param *dispparam;
	long id, idmax;
	int turnoff;
	uint8_t *r, *g, *b;
	int error;
	size_t idx, cnt;

	switch (cmd) {
	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = sc->sc_vc->blanked ?
		    WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON;
		return (0);

	case WSDISPLAYIO_SVIDEO:
		turnoff = (*(u_int *)data == WSDISPLAYIO_VIDEO_OFF);
		if (sc->sc_vc->blanked != turnoff) {
			sc->sc_vc->blanked = turnoff;
			if (turnoff)
				hd64461video_off(sc->sc_vc);
			else
				hd64461video_on(sc->sc_vc);
		}

		return (0);

	case WSDISPLAYIO_GETPARAM:
		dispparam = (struct wsdisplay_param *)data;
		dispparam->min = 0;
		switch (dispparam->param) {
		case WSDISPLAYIO_PARAM_BACKLIGHT:
			id = CONFIG_HOOK_POWER_LCDLIGHT;
			idmax = -1;
			dispparam->max = ~0;
			break;
		case WSDISPLAYIO_PARAM_BRIGHTNESS:
			id = CONFIG_HOOK_BRIGHTNESS;
			idmax = CONFIG_HOOK_BRIGHTNESS_MAX;
			break;
		case WSDISPLAYIO_PARAM_CONTRAST:
			id = CONFIG_HOOK_CONTRAST;
			idmax = CONFIG_HOOK_CONTRAST_MAX;
			break;
		default:
			return (EINVAL);
		}

		if (idmax >= 0) {
			error = config_hook_call(CONFIG_HOOK_GET, idmax,
						 &dispparam->max);
			if (error)
				return (error);
		}
		return config_hook_call(CONFIG_HOOK_GET, id,
					&dispparam->curval);

	case WSDISPLAYIO_SETPARAM:
		dispparam = (struct wsdisplay_param *)data;
		switch (dispparam->param) {
		case WSDISPLAYIO_PARAM_BACKLIGHT:
			id = CONFIG_HOOK_POWER_LCDLIGHT;
			break;
		case WSDISPLAYIO_PARAM_BRIGHTNESS:
			id = CONFIG_HOOK_BRIGHTNESS;
			break;
		case WSDISPLAYIO_PARAM_CONTRAST:
			id = CONFIG_HOOK_CONTRAST;
			break;
		default:
			return (EINVAL);
		}
		return config_hook_call(CONFIG_HOOK_SET, id,
					&dispparam->curval);

	case WSDISPLAYIO_GETCMAP:
		cmap = (struct wsdisplay_cmap *)data;
		cnt = cmap->count;
		idx = cmap->index;

		if (hf->hf_class != HPCFB_CLASS_INDEXCOLOR ||
		    hf->hf_pack_width != 8 ||
		    !LEGAL_CLUT_INDEX(idx) ||
		    !LEGAL_CLUT_INDEX(idx + cnt -1)) {
			return (EINVAL);
		}

		error = cmap_work_alloc(&r, &g, &b, 0, cnt);
		if (error)
			goto out;
		hd64461video_get_clut(sc->sc_vc, idx, cnt, r, g, b);
		error = copyout(r, cmap->red, cnt);
		if (error)
			goto out;
		error = copyout(g, cmap->green,cnt);
		if (error)
			goto out;
		error = copyout(b, cmap->blue, cnt);

out:
		cmap_work_free(r, g, b, 0);
		return error;
		
	case WSDISPLAYIO_PUTCMAP:
		cmap = (struct wsdisplay_cmap *)data;
		cnt = cmap->count;
		idx = cmap->index;

		if (hf->hf_class != HPCFB_CLASS_INDEXCOLOR ||
		    hf->hf_pack_width != 8 ||
		    !LEGAL_CLUT_INDEX(idx) ||
		    !LEGAL_CLUT_INDEX(idx + cnt -1)) {
			return (EINVAL);
		}

		error = cmap_work_alloc(&r, &g, &b, 0, cnt);
		if (error)
			goto out;

		error = copyin(cmap->red, r, cnt);
		if (error)
			goto out;
		error = copyin(cmap->green,g, cnt);
		if (error)
			goto out;
		error = copyin(cmap->blue, b, cnt);
		if (error)
			goto out;
		hd64461video_set_clut(sc->sc_vc, idx, cnt, r, g, b);
		goto out;

	case HPCFBIO_GCONF:
		fbconf = (struct hpcfb_fbconf *)data;
		if (fbconf->hf_conf_index != 0 &&
		    fbconf->hf_conf_index != HPCFB_CURRENT_CONFIG) {
			return (EINVAL);
		}
		*fbconf = *hf;	/* structure assignment */
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
		*dspconf = sc->sc_vc->hd;	/* structure assignment */
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
		/* XXX not implemented yet */
		return (EINVAL);
	}

	return (EPASSTHROUGH);
}

STATIC paddr_t
hd64461video_mmap(void *ctx, off_t offset, int prot)
{
	struct hd64461video_softc *sc = (struct hd64461video_softc *)ctx;
	struct hpcfb_fbconf *hf = &sc->sc_vc->hf;

	if (offset < 0 || (hf->hf_bytes_per_plane + hf->hf_offset) < offset)
		return (-1);
	
	return (sh3_btop(HD64461_FBBASE + offset));
}


#ifdef HD64461VIDEO_HWACCEL

STATIC void
hd64461video_cursor(void *ctx, int on, int xd, int yd, int w, int h)
{
	struct hd64461video_softc *sc = (struct hd64461video_softc *)ctx;
	int xw, yh, width, bpp, adr;
	uint16_t r;

	width = sc->sc_vc->vc.vc_fbwidth;
	bpp = sc->sc_vc->vc.vc_fbdepth;
	xw = w - 1;
	yh = h - 1;

	/* Wait until previous command done. */
	hd64461video_iodone(ctx);

	/* Destination addr */
	adr = width * yd + xd;
	if (bpp == 16)
		adr *= 2;
	hd64461_reg_write_2(HD64461_LCDBBTDSARH_REG16,
	    HD64461_LCDBBTDSARH(adr));
	hd64461_reg_write_2(HD64461_LCDBBTDSARL_REG16,
	    HD64461_LCDBBTDSARL(adr));
	
	// Width
	hd64461_reg_write_2(HD64461_LCDBBTDWR_REG16,
	    xw & HD64461_LCDBBTDWR_MASK);

	// Height
	hd64461_reg_write_2(HD64461_LCDBBTDHR_REG16,
	    yh & HD64461_LCDBBTDHR_MASK);
	
	// Operation (Destination Invert)
	hd64461_reg_write_2(HD64461_LCDBBTROPR_REG16,
	    HD64461_LCDC_BITBLT_DSTINVERT);

	// BitBLT mode (Destination Invert)
	hd64461_reg_write_2(HD64461_LCDBBTMDR_REG16, 0);

	// Kick.
	r = hd64461_reg_read_2(HD64461_LCDGRCFGR_REG16);
	r &= ~HD64461_LCDGRCFGR_ACCSTART_MASK;
	r |= HD64461_LCDGRCFGR_ACCSTART_BITBLT;
	hd64461_reg_write_2(HD64461_LCDGRCFGR_REG16, r);
}

STATIC void
hd64461video_bitblit(void *ctx, int xs, int ys, int xd, int yd, int h, int w)
{
	struct hd64461video_softc *sc = (struct hd64461video_softc *)ctx;
	int xw, yh, width, bpp, condition_a, adr;
	uint16_t r;

	xw = w - 1;
	yh = h - 1;
	width = sc->sc_vc->vc.vc_fbwidth;
	bpp = sc->sc_vc->vc.vc_fbdepth;
	condition_a = ((ys == yd) && (xs <= xd)) || (ys < yd);

	hd64461video_iodone(ctx);

	// Source addr
	if (condition_a)
		adr = (width * (ys + yh)) + (xs + xw);
	else
		adr = width * ys + xs;
	if (bpp == 16)
		adr *= 2;

	hd64461_reg_write_2(HD64461_LCDBBTSSARH_REG16,
	    HD64461_LCDBBTSSARH(adr));
	hd64461_reg_write_2(HD64461_LCDBBTSSARL_REG16,
	    HD64461_LCDBBTSSARL(adr));
	
	// Destination addr
	if (condition_a)
		adr = (width * (yd + yh)) + (xd + xw);
	else 
		adr = width * yd + xd;
	if (bpp == 16)
		adr *= 2;

	hd64461_reg_write_2(HD64461_LCDBBTDSARH_REG16,
	    HD64461_LCDBBTDSARH(adr));
	hd64461_reg_write_2(HD64461_LCDBBTDSARL_REG16,
	    HD64461_LCDBBTDSARL(adr));
	
	// Width
	hd64461_reg_write_2(HD64461_LCDBBTDWR_REG16,
	    xw & HD64461_LCDBBTDWR_MASK);

	// Height
	hd64461_reg_write_2(HD64461_LCDBBTDHR_REG16,
	    yh & HD64461_LCDBBTDHR_MASK);

	// Operation (source copy)
	hd64461_reg_write_2(HD64461_LCDBBTROPR_REG16,
	    HD64461_LCDC_BITBLT_SRCCOPY);
	
	// BitBLT mode (on screen to on screen)
	r = HD64461_LCDBBTMDR_SET(0,
	    HD64461_LCDBBTMDR_ON_SCREEN_TO_ON_SCREEN);
	if (condition_a)	/* reverse direction */
	    r |= HD64461_LCDBBTMDR_SCANDRCT_RL_BT;
	hd64461_reg_write_2(HD64461_LCDBBTMDR_REG16, r);

	// Kick.
	r = hd64461_reg_read_2(HD64461_LCDGRCFGR_REG16);
	r &= ~HD64461_LCDGRCFGR_ACCSTART_MASK;
	r |= HD64461_LCDGRCFGR_ACCSTART_BITBLT;
	hd64461_reg_write_2(HD64461_LCDGRCFGR_REG16, r);
}

STATIC void
hd64461video_erase(void *ctx, int xd, int yd, int h, int w, int attr)
{
	struct hd64461video_softc *sc = (struct hd64461video_softc *)ctx;
	int xw, yh, width, bpp, adr;
	uint16_t r;

	width = sc->sc_vc->vc.vc_fbwidth;
	bpp = sc->sc_vc->vc.vc_fbdepth;
	xw = w - 1;
	yh = h - 1;

	/* Wait until previous command done. */
	hd64461video_iodone(ctx);

	/* Destination addr */
	adr = width * yd + xd;
	if (bpp == 16)
		adr *= 2;
	hd64461_reg_write_2(HD64461_LCDBBTDSARH_REG16,
	    HD64461_LCDBBTDSARH(adr));
	hd64461_reg_write_2(HD64461_LCDBBTDSARL_REG16,
	    HD64461_LCDBBTDSARL(adr));
	
	// Width
	hd64461_reg_write_2(HD64461_LCDBBTDWR_REG16,
	    xw & HD64461_LCDBBTDWR_MASK);

	// Height
	hd64461_reg_write_2(HD64461_LCDBBTDHR_REG16,
	    yh & HD64461_LCDBBTDHR_MASK);
	
	// Color
	hd64461_reg_write_2(HD64461_LCDGRSCR_REG16, 0); //XXX black only

	// Operation (Solid Color Fill)
	hd64461_reg_write_2(HD64461_LCDBBTROPR_REG16,
	    HD64461_LCDC_BITBLT_PATCOPY);

	// BitBLT mode (Solid Color)
	hd64461_reg_write_2(HD64461_LCDBBTMDR_REG16,
	    HD64461_LCDBBTMDR_PATSELECT_SOLIDCOLOR);

	// Kick.
	r = hd64461_reg_read_2(HD64461_LCDGRCFGR_REG16);
	r &= ~HD64461_LCDGRCFGR_ACCSTART_MASK;
	r |= HD64461_LCDGRCFGR_ACCSTART_BITBLT;
	hd64461_reg_write_2(HD64461_LCDGRCFGR_REG16, r);
}

STATIC void
hd64461video_putchar(void *ctx, int row, int col, struct wsdisplay_font *font,
    int fclr, int uclr, u_int uc, int attr)
{
	struct hd64461video_softc *sc = (struct hd64461video_softc *)ctx;
	int w, h, cw;

	w = font->fontwidth;
	h = font->fontheight;
	cw = sc->sc_font.cw;
	hd64461video_bitblit(ctx, (uc % cw) * w,
	    sc->sc_vc->vc.vc_fbheight + (uc / cw) * h, row, col, h, w);
}

STATIC void
hd64461video_setclut(void *ctx, struct rasops_info *info)
{
	struct hd64461video_softc *sc = (struct hd64461video_softc *)ctx;

	if (sc->sc_vc->vc.vc_fbdepth != 8)
		return;
}

STATIC void
hd64461video_font(void *ctx, struct wsdisplay_font *font)
{
	struct hd64461video_softc *sc = (struct hd64461video_softc *)ctx;

	hd64461video_font_set_attr(sc, font);
	hd64461video_font_load(sc);
}

STATIC void
hd64461video_iodone(void *ctx)
{

	while ((hd64461_reg_read_2(HD64461_LCDGRCFGR_REG16) &
	    HD64461_LCDGRCFGR_ACCSTATUS) != 0)
		continue;
}

/* internal */
STATIC void
hd64461video_font_load_16bpp(uint16_t *d, uint8_t *s, int w, int h, int step)
{
	int i, j, n;
	n = step / sizeof(uint16_t);
	
	for (i = 0; i < h; i++, d += n) {
		for (j = 0; j < w; j++) {
			d[j] = *s & (1 << (w - j - 1)) ? 0xffff : 0x0000;
		}
		s++;
	}
}

STATIC void
hd64461video_font_load_8bpp(uint8_t *d, uint8_t *s, int w, int h, int step)
{
	int i, j, n;
	n = step / sizeof(uint8_t);
	
	for (i = 0; i < h; i++, d += n) {
		for (j = 0; j < w; j++) {
			d[j] = *s & (1 << (w - j - 1)) ? 0xff : 0x00;
		}
		s++;
	}
}

STATIC void
hd64461video_font_set_attr(struct hd64461video_softc *sc,
    struct wsdisplay_font *f)
{
	struct hd64461video_chip *hvc = sc->sc_vc;
	struct wsdisplay_font *font = &sc->sc_font.wsfont;
	int w, h, bpp;

	w	= f->fontwidth;
	h	= f->fontheight;
	bpp	= hvc->vc.vc_fbdepth;

	*font = *f;
	sc->sc_font.c = (w * bpp) / NBBY;
	sc->sc_font.cw = hvc->hf.hf_width / w;
	sc->sc_font.cstep = ((w * h * bpp) / NBBY) * sc->sc_font.cw;

	DPRINTF("c = %d cw = %d cstep = %d\n", sc->sc_font.c,
	    sc->sc_font.cw, sc->sc_font.cstep);

}

/* return frame buffer virtual address of charcter #n */
STATIC vaddr_t
hd64461video_font_start_addr(struct hd64461video_softc *sc, int n)
{
	struct hd64461video_chip *hvc = sc->sc_vc;
	struct hd64461video_font *font = &sc->sc_font;
	vaddr_t base;

	base = (vaddr_t)hvc->off_screen_addr;
	base += (n / font->cw) * font->cstep + font->c * (n % font->cw);

	return base;
}

STATIC void
hd64461video_font_load(struct hd64461video_softc *sc)
{
	struct hd64461video_chip *hvc = sc->sc_vc;
	struct wsdisplay_font *font = &sc->sc_font.wsfont;
	uint8_t *q;
	int w, h, step, i, n;

	if (sc->sc_font.loaded) {
		printf("reload font\n");
	}

	w	= font->fontwidth;
	h	= font->fontheight;
	step	= sc->sc_font.cw * sc->sc_font.c;
	n	= (w * h) / NBBY;
	q	= font->data;

	DPRINTF("%s (%dx%d) %d+%d\n", font->name, w, h, font->firstchar,
	    font->numchars);
	DPRINTF("bitorder %d byteorder %d stride %d\n", font->bitorder,
	    font->byteorder, font->stride);
	
	switch (hvc->vc.vc_fbdepth) {
	case 8:
		for (i = font->firstchar; i < font->numchars; i++) {
			hd64461video_font_load_8bpp
			    ((uint8_t *)hd64461video_font_start_addr(sc, i),
				q, w, h, step);
			q += n;
		}
		break;
	case 16:
		for (i = font->firstchar; i < font->numchars; i++) {
			hd64461video_font_load_16bpp
			    ((uint16_t *)hd64461video_font_start_addr(sc, i),
				q, w, h, step);
			q += n;
		}
		break;
	}

	sc->sc_font.loaded = true;
}
#endif /* HD64461VIDEO_HWACCEL */

STATIC void
hd64461video_update_videochip_status(struct hd64461video_chip *hvc)
{
	struct video_chip *vc = &hvc->vc;
	uint16_t r;
	int i;
	int depth, width, height;

	depth = 0;		/* XXX: -Wuninitialized */

	/* display mode */
	r = hd64461_reg_read_2(HD64461_LCDLDR3_REG16);	
	i = HD64461_LCDLDR3_CG(r);
	switch (i) {
	case HD64461_LCDLDR3_CG_COLOR16:
		depth = 16;
		hvc->mode = LCD64K_C;
		break;
	case HD64461_LCDLDR3_CG_COLOR8:
		depth = 8;
		hvc->mode = LCD256_C;
		break;
	case HD64461_LCDLDR3_CG_GRAY6:
		depth = 6;
		hvc->mode = LCD64_MONO;
		break;
	case HD64461_LCDLDR3_CG_GRAY4:
		depth = 4;
		hvc->mode = LCD16_MONO;
		break;
	case HD64461_LCDLDR3_CG_GRAY2:
		depth = 2;
		hvc->mode = LCD4_MONO;
		break;
	case HD64461_LCDLDR3_CG_GRAY1:
		depth = 1;
		hvc->mode = LCD2_MONO;
		break;
	}

	r = hd64461_reg_read_2(HD64461_LCDCCR_REG16);
	i = HD64461_LCDCCR_DSPSEL(i);
	switch (i) {
	case HD64461_LCDCCR_DSPSEL_LCD_CRT:
		depth = 8;
		hvc->mode = LCDCRT;
		break;
	case HD64461_LCDCCR_DSPSEL_CRT:
		depth = 8;
		hvc->mode = CRT256_C;
		break;
	case HD64461_LCDCCR_DSPSEL_LCD:
		/* nothing to do */
		break;
	}

	callout_init(&hvc->unblank_ch, 0);
	hvc->blanked = 0;

	width = bootinfo->fb_width;
	height = bootinfo->fb_height;

	vc->vc_fbvaddr	= HD64461_FBBASE;
	vc->vc_fbpaddr	= HD64461_FBBASE;
	vc->vc_fbdepth	= depth;
	vc->vc_fbsize	= (width * height * depth) / NBBY;
	vc->vc_fbwidth	= width;
	vc->vc_fbheight	= height;
}

#if notyet
STATIC void
hd64461video_set_display_mode(struct hd64461video_chip *hvc)
{

	if (hvc->mode == LCDCRT || hvc->mode == CRT256_C)
		hd64461video_set_display_mode_crtc(hvc);

	hd64461video_set_display_mode_lcdc(hvc);
}

STATIC void
hd64461video_set_display_mode_lcdc(struct hd64461video_chip *hvc)
{
	struct {
		uint16_t clor;	/* display size 640 x 240 */
		uint16_t ldr3;
		const char *name;
	} *conf, disp_conf[] = {
		[LCD256_C]	= { 640, HD64461_LCDLDR3_CG_COLOR8,
				    "8bit color" },
		[LCD64K_C]	= { 640 * 2, HD64461_LCDLDR3_CG_COLOR16,
				    "16bit color" },
		[LCD64_MONO]	= { 640, HD64461_LCDLDR3_CG_GRAY6 ,
				    "6bit grayscale" },
		[LCD16_MONO]	= { 640 / 2, HD64461_LCDLDR3_CG_GRAY4,
				    "4bit grayscale" },
		[LCD4_MONO]	= { 640 / 4, HD64461_LCDLDR3_CG_GRAY2,
				    "2bit grayscale" },
		[LCD2_MONO]	= { 640 / 8, HD64461_LCDLDR3_CG_GRAY1,
				    "monochrome" },
	};
	uint16_t r;
	int omode;
	
	conf = &disp_conf[hvc->mode];
	
	hd64461_reg_write_2(HD64461_LCDCLOR_REG16, conf->clor);
	r = hd64461_reg_read_2(HD64461_LCDLDR3_REG16);
	omode = HD64461_LCDLDR3_CG(r);
	r = HD64461_LCDLDR3_CG_CLR(r);
	r = HD64461_LCDLDR3_CG_SET(r, conf->ldr3);
	hd64461_reg_write_2(HD64461_LCDLDR3_REG16, r);

	printf("%s ", conf->name);
}

STATIC void
hd64461video_set_display_mode_crtc(struct hd64461video_chip *hvc)
{
	/* not yet */
}

#endif /* notyet */

STATIC size_t
hd64461video_frame_buffer_size(struct hd64461video_chip *hvc)
{
	vaddr_t page, startaddr, endaddr;
	int x;
	
	startaddr = HD64461_FBBASE;
	endaddr = startaddr + HD64461_FBSIZE - 1;

	page = startaddr;

	x = random();
	*(volatile int *)(page + 0) = x;
	*(volatile int *)(page + 4) = ~x;

	if (*(volatile int *)(page + 0) != x ||
	    *(volatile int *)(page + 4) != ~x)
		return (0);
	
	for (page += HD64461_FBPAGESIZE; page < endaddr;
	    page += HD64461_FBPAGESIZE) {
		if (*(volatile int *)(page + 0) == x &&
		    *(volatile int *)(page + 4) == ~x)
			goto fbend_found;
	}

	page -= HD64461_FBPAGESIZE;
	*(volatile int *)(page + 0) = x;
	*(volatile int *)(page + 4) = ~x;

	if (*(volatile int *)(page + 0) != x ||
	    *(volatile int *)(page + 4) != ~x)
		return (0);

 fbend_found:
	return (page - startaddr);
}

STATIC void
hd64461video_set_clut(struct hd64461video_chip *vc, int idx, int cnt,
    uint8_t *r, uint8_t *g, uint8_t *b)
{
	KASSERT(r && g && b);

	/* index pallete */
	hd64461_reg_write_2(HD64461_LCDCPTWAR_REG16,
	    HD64461_LCDCPTWAR_SET(0, idx));
	/* set data */
	while (cnt && LEGAL_CLUT_INDEX(idx)) {
		uint16_t v;
#define	HD64461VIDEO_SET_CLUT(x)					\
		v = (x >> 2) & 0x3f;					\
		hd64461_reg_write_2(HD64461_LCDCPTWDR_REG16, v)
		HD64461VIDEO_SET_CLUT(*r);
		HD64461VIDEO_SET_CLUT(*g);
		HD64461VIDEO_SET_CLUT(*b);
#undef	HD64461VIDEO_SET_CLUT
		r++, g++, b++;
		idx++, cnt--;
	}
}

STATIC void
hd64461video_get_clut(struct hd64461video_chip *vc, int idx, int cnt,
    uint8_t *r, uint8_t *g, uint8_t *b)
{
	KASSERT(r && g && b);

	/* index pallete */
	hd64461_reg_write_2(HD64461_LCDCPTRAR_REG16,
	    HD64461_LCDCPTRAR_SET(0, idx));
	
	/* get data */
	while (cnt && LEGAL_CLUT_INDEX(idx)) {
		uint16_t v;
#define	HD64461VIDEO_GET_CLUT(x)					\
	v = hd64461_reg_read_2(HD64461_LCDCPTRDR_REG16);		\
	x = HD64461_LCDCPTRDR(v);					\
	x <<= 2
		HD64461VIDEO_GET_CLUT(*r);
		HD64461VIDEO_GET_CLUT(*g);
		HD64461VIDEO_GET_CLUT(*b);
#undef	HD64461VIDEO_GET_CLUT
		r++, g++, b++;
		idx++, cnt--;
	} 
}

STATIC int
hd64461video_power(void *ctx, int type, long id, void *msg)
{
	struct hd64461video_softc *sc = ctx;
	struct hd64461video_chip *hvc = sc->sc_vc;

	switch ((int)msg) {
	case PWR_RESUME:
		DPRINTF("%s: ON%s\n", device_xname(sc->sc_dev),
			sc->sc_vc->blanked ? " (blanked)" : "");
		if (!sc->sc_vc->blanked)
			hd64461video_on(hvc);
		break;
	case PWR_SUSPEND:
		/* FALLTHROUGH */
	case PWR_STANDBY:
		DPRINTF("%s: OFF\n", device_xname(sc->sc_dev));
		hd64461video_off(hvc);
		break;
	}

	return 0;
}

STATIC void
hd64461video_off(struct hd64461video_chip *vc)
{

	callout_stop(&vc->unblank_ch);

	/* turn off display in LCDC */
	hd64461video_display_onoff(vc, false);

	/* turn off the LCD */
	config_hook_call(CONFIG_HOOK_POWERCONTROL,
			 CONFIG_HOOK_POWERCONTROL_LCD,
			 (void *)0);
}

STATIC void
hd64461video_on(struct hd64461video_chip *vc)
{
	int err;

	/* turn on the LCD */
	err = config_hook_call(CONFIG_HOOK_POWERCONTROL,
			       CONFIG_HOOK_POWERCONTROL_LCD,
			       (void *)1);

	if (err == 0)
		/* let the LCD warm up before turning on the display */
		callout_reset(&vc->unblank_ch, hz/2,
		    hd64461video_display_on, vc);
	else
		hd64461video_display_onoff(vc, true);
}

STATIC void
hd64461video_display_on(void *arg)
{

	hd64461video_display_onoff(arg, true);
}

STATIC void
hd64461video_display_onoff(void *arg, bool on)
{
	/* struct hd64461video_chip *vc = arg; */
	uint16_t r;

	if (platid_match(&platid, &platid_mask_MACH_HITACHI_PERSONA))
		return;

	/* turn on/off display in LCDC */
	r = hd64461_reg_read_2(HD64461_LCDLDR1_REG16);
	if (on)
		r |= HD64461_LCDLDR1_DON;
	else
		r &= ~HD64461_LCDLDR1_DON;
	hd64461_reg_write_2(HD64461_LCDLDR1_REG16, r);
}

#ifdef HD64461VIDEO_DEBUG
STATIC void
hd64461video_info(struct hd64461video_softc *sc)
{
	uint16_t r;
	int color;
	int i;

	dbg_banner_function();
	printf("---[LCD]---\n");
	/* Base Address Register */
	r = hd64461_reg_read_2(HD64461_LCDCBAR_REG16);
	printf("LCDCBAR Frame buffer base address (4KB align): 0x%08x\n",
	    HD64461_LCDCBAR_BASEADDR(r));

	/* Line Address Offset Register */
	r = hd64461_reg_read_2(HD64461_LCDCLOR_REG16);
	printf("LCDCLOR Line address offset: %d\n", HD64461_LCDCLOR(r));

	/* LCDC Control Register */
	r = hd64461_reg_read_2(HD64461_LCDCCR_REG16);
	i = HD64461_LCDCCR_DSPSEL(r);
#define	DBG_BITMASK_PRINT(r, m)	dbg_bitmask_print(r, HD64461_LCDCCR_##m, #m)
	printf("LCDCCR (LCD Control Register)\n");
	DBG_BITMASK_PRINT(r, STBAK);
	DBG_BITMASK_PRINT(r, STREQ);
	DBG_BITMASK_PRINT(r, MOFF);
	DBG_BITMASK_PRINT(r, REFSEL);
	DBG_BITMASK_PRINT(r, EPON);
	DBG_BITMASK_PRINT(r, SPON);
	printf("\n");
#undef	DBG_BITMASK_PRINT
	printf("LCDCCR Display select LCD[%c] CRT[%c]\n",
	    i == HD64461_LCDCCR_DSPSEL_LCD_CRT ||
	    i == HD64461_LCDCCR_DSPSEL_LCD ? 'x' : '_',
	    i == HD64461_LCDCCR_DSPSEL_LCD_CRT ||
	    i == HD64461_LCDCCR_DSPSEL_CRT ? 'x' : '_');

	/* LCD Display Register */
	/* 1 */
	r = hd64461_reg_read_2(HD64461_LCDLDR1_REG16);	
	printf("(LCD Display Register)\n");
#define	DBG_BITMASK_PRINT(r, m)	dbg_bitmask_print(r, HD64461_LCDLDR1_##m, #m)
	printf("LCDLDR1: ");
	DBG_BITMASK_PRINT(r, DINV);	
	DBG_BITMASK_PRINT(r, DON);
	printf("\n");
#undef	DBG_BITMASK_PRINT
	/* 2 */
	r = hd64461_reg_read_2(HD64461_LCDLDR2_REG16);	
	i = HD64461_LCDLDR2_LM(r);
#define	DBG_BITMASK_PRINT(r, m)	dbg_bitmask_print(r, HD64461_LCDLDR2_##m, #m)
	printf("LCDLDR2: ");
	DBG_BITMASK_PRINT(r, CC1);
	DBG_BITMASK_PRINT(r, CC2);
#undef	DBG_BITMASK_PRINT
	color = 0;
	switch (i) {
	default:
		panic("unknown unknown LCD interface.");
		break;
	case HD64461_LCDLDR2_LM_COLOR:
		color = 1;
		printf("Color");
		break;
	case HD64461_LCDLDR2_LM_GRAY8:
		printf("8-bit grayscale");
		break;
	case HD64461_LCDLDR2_LM_GRAY4:
		printf("4-bit grayscale");
		break;
	}
	printf(" LCD interface\n");
	/* 3 */
	printf("LCDLDR3: ");
	r = hd64461_reg_read_2(HD64461_LCDLDR3_REG16);	
	i = HD64461_LCDLDR3_CS(r);
	printf("CS ");
	switch (i) {
	case 0:
		printf("15");
		break;
	case 1:
		printf("2.5");
		break;
	case 2:
		printf("3.75");
		break;
	case 4:
		printf("5");
		break;
	case 8:
		printf("7.5");
		break;
	case 16:
		printf("10");
		break;
	}
	printf("%s MHz ", color ? "" : "/2");
	i = HD64461_LCDLDR3_CG(r);
	switch (i) {
	case HD64461_LCDLDR3_CG_COLOR16:
		printf("Color 64K colors\n");
		break;
	case HD64461_LCDLDR3_CG_COLOR8:
		printf("Color 256 colors\n");
		break;
	case HD64461_LCDLDR3_CG_GRAY6:
		printf("6-bit Grayscale\n");
		break;
	case HD64461_LCDLDR3_CG_GRAY4:
		printf("4-bit Grayscale\n");
		break;
	case HD64461_LCDLDR3_CG_GRAY2:
		printf("2-bit Grayscale\n");
		break;
	case HD64461_LCDLDR3_CG_GRAY1:
		printf("1-bit Grayscale\n");
		break;
	}

	/* LCD Number of Characters in Horizontal Register */
	r = hd64461_reg_read_2(HD64461_LCDLDHNCR_REG16);
	printf("LDHNCR: NHD %d NHT %d (# of horizontal characters)\n",
	    HD64461_LCDLDHNCR_NHD(r), HD64461_LCDLDHNCR_NHT(r));
	
	/* Start Position of Horizontal Register */	
	r = hd64461_reg_read_2(HD64461_LCDLDHNSR_REG16);
	printf("LDHNSR: HSW %d HSP %d (start position of horizontal)\n",
	    HD64461_LCDLDHNSR_HSW(r), HD64461_LCDLDHNSR_HSP(r));

	/* Total Vertical Lines Register */
	r = hd64461_reg_read_2(HD64461_LCDLDVNTR_REG16);
	printf("LDVNTR: %d (total vertical lines)\n",
	    HD64461_LCDLDVNTR_VTL(r));

	/* Display Vertical Lines Register */
	r = hd64461_reg_read_2(HD64461_LCDLDVNDR_REG16);
	printf("LDVNDR: %d (display vertical lines)\n",
	    HD64461_LCDLDVSPR_VSP(r));

	/* Vertical Synchronization Position Register */
	r = hd64461_reg_read_2(HD64461_LCDLDVSPR_REG16);
	printf("LDVSPR: %d (vertical synchronization position)\n",
	    HD64461_LCDLDVSPR_VSP(r));

	/*
	 * CRT Control Register
	 */
	printf("---[CRT]---\n");	
	r = hd64461_reg_read_2(HD64461_LCDCRTVTR_REG16);
	printf("CRTVTR: %d (CRTC total vertical lines)\n",
	    HD64461_LCDCRTVTR(r));
	r = hd64461_reg_read_2(HD64461_LCDCRTVRSR_REG16);
	printf("CRTVRSR: %d (CRTC vertical retrace start line)\n",
	    HD64461_LCDCRTVRSR(r));
	r = hd64461_reg_read_2(HD64461_LCDCRTVRER_REG16);	
	printf("CRTVRER: %d (CRTC vertical retrace end line)\n",
	    HD64461_LCDCRTVRER(r));

}

STATIC void
hd64461video_dump(void)
{
	uint16_t r;
	printf("---[Display Mode Setting]---\n");
#define	DUMPREG(x)							\
	r = hd64461_reg_read_2(HD64461_LCD ## x ## _REG16);		\
	__dbg_bit_print(r, sizeof(uint16_t), 0, 0, #x, DBG_BIT_PRINT_COUNT)
	DUMPREG(CBAR);
	DUMPREG(CLOR);
	DUMPREG(CCR);
	DUMPREG(LDR1);
	DUMPREG(LDR2);
	DUMPREG(LDHNCR);
	DUMPREG(LDHNSR);
	DUMPREG(LDVNTR);
	DUMPREG(LDVNDR);
	DUMPREG(LDVSPR);
	DUMPREG(LDR3);
	DUMPREG(CRTVTR);
	DUMPREG(CRTVRSR);
	DUMPREG(CRTVRER);
#undef	DUMPREG
	dbg_banner_line();
}

#endif /* HD64461VIDEO_DEBUG */
