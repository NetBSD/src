/*	$NetBSD: ite8181.c,v 1.3.2.4 2000/12/08 09:26:31 bouyer Exp $	*/

/*-
 * Copyright (c) 2000 SATO Kazumi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
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
#include <sys/boot_flag.h>
#include <sys/buf.h>

#include <uvm/uvm_extern.h>

#include <dev/wscons/wsconsio.h>

#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcmips/dev/ite8181reg.h>
#include <hpcmips/dev/ite8181var.h>
#include "bivideo.h"
#if NBIVIDEO > 0
#include <hpcmips/dev/bivideovar.h>
#endif
#include <hpcmips/dev/hpccmapvar.h>


#define ITE8181DEBUG
#ifdef ITE8181DEBUG
#ifndef ITE8181DEBUG_CONF
#define ITE8181DEBUG_CONF 0
#endif
int	ite8181_debug = ITE8181DEBUG_CONF;
#define	DPRINTF(arg)     if (ite8181_debug) printf arg
#define	DPRINTFN(n, arg) if (ite8181_debug > (n)) printf arg
#define	VPRINTF(arg)     if (bootverbose || ite8181_debug) printf arg
#define	VPRINTFN(n, arg) if (bootverbose || ite8181_debug > (n)) printf arg
#else
#define	DPRINTF(arg)
#define DPRINTFN(n, arg)
#define	VPRINTF(arg)     if (bootverbose) printf arg
#define	VPRINTFN(n, arg) if (bootverbose) printf arg
#endif

#ifndef ITE8181_LCD_CONTROL_ENABLE
int ite8181_lcd_control_disable = 1;
#else /* ITE8181_LCD_CONTROL_ENABLE */
int ite8181_lcd_control_disable = 0;
#endif /* ITE8181_LCD_CONTROL_ENABLE */

#define ITE8181_WINCE_CMAP

/*
 * XXX:
 * IBM WorkPad z50 power unit has too weak power.
 * So we must wait too many times to access some device
 * after LCD panel and BackLight on.
 * Currently delay is not enough ??? FIXME 
 */
#ifndef ITE8181_LCD_ON_SELF_DELAY
#define ITE8181_LCD_ON_SELF_DELAY 1000
#endif /* ITE8181_LCD_ON__SELF_DELAY */
#ifndef ITE8181_LCD_ON_DELAY
#define ITE8181_LCD_ON_DELAY 2000
#endif /* ITE8181_LCD_ON_DELAY */
int ite8181_lcd_on_self_delay = ITE8181_LCD_ON_SELF_DELAY; /* msec */
int ite8181_lcd_on_delay = ITE8181_LCD_ON_DELAY; /* msec */

#define MSEC	1000
/*
 * function prototypes
 */
static void	ite8181_config_write_4 __P((bus_space_tag_t, bus_space_handle_t, int, int));
static int	ite8181_config_read_4 __P((bus_space_tag_t, bus_space_handle_t, int));

static void	ite8181_gui_write_4 __P((struct ite8181_softc *, int, int));
static int	ite8181_gui_read_4 __P((struct ite8181_softc *, int));

static void	ite8181_gui_write_1 __P((struct ite8181_softc *, int, int));
static int	ite8181_gui_read_1 __P((struct ite8181_softc *, int));

static void	ite8181_graphics_write_1 __P((struct ite8181_softc *, int, int));
static int	ite8181_graphics_read_1 __P((struct ite8181_softc *, int));

static void	ite8181_ema_write_1 __P((struct ite8181_softc *, int, int));
static int	ite8181_ema_read_1 __P((struct ite8181_softc *, int));

static void	ite8181_power __P((int, void *));
static int	ite8181_hardpower __P((void *, int, long, void *));
static int	ite8181_fbinit __P((struct hpcfb_fbconf *));
static int	ite8181_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static paddr_t	ite8181_mmap __P((void *, off_t offset, int));
static void	ite8181_erase_cursor __P((struct ite8181_softc *));
static int	ite8181_lcd_power __P((struct ite8181_softc *, int));
/*
 * static variables
 */
struct hpcfb_accessops ite8181_ha = {
	ite8181_ioctl, ite8181_mmap
};

inline int
ite8181_config_read_4(iot, ioh, byteoffset)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int byteoffset;
{
	return bus_space_read_4(iot, ioh, ITE8181_CONF_OFFSET + byteoffset);
}

inline void
ite8181_config_write_4(iot, ioh, byteoffset, data)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int byteoffset;
	int data;
{
	bus_space_write_4(iot, ioh, ITE8181_CONF_OFFSET + byteoffset, data);
}

inline int
ite8181_gui_read_4(sc, byteoffset)
	struct ite8181_softc *sc;
	int byteoffset;
{
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, 
			sc->sc_gba + byteoffset);
}

inline void
ite8181_gui_write_4(sc, byteoffset, data)
	struct ite8181_softc *sc;
	int byteoffset;
	int data;
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, sc->sc_gba + byteoffset, data);
}

inline int
ite8181_gui_read_1(sc, byteoffset)
	struct ite8181_softc *sc;
	int byteoffset;
{
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, 
			sc->sc_gba + byteoffset);
}

inline void
ite8181_gui_write_1(sc, byteoffset, data)
	struct ite8181_softc *sc;
	int byteoffset;
	int data;
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, sc->sc_gba + byteoffset, data);
}

inline int
ite8181_graphics_read_1(sc, byteoffset)
	struct ite8181_softc *sc;
	int byteoffset;
{
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, 
			sc->sc_sba + byteoffset);
}

inline void
ite8181_graphics_write_1(sc, byteoffset, data)
	struct ite8181_softc *sc;
	int byteoffset;
	int data;
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, sc->sc_sba + byteoffset, data);
}

inline int
ite8181_ema_read_1(sc, byteoffset)
	struct ite8181_softc *sc;
	int byteoffset;
{
	ite8181_graphics_write_1(sc, ITE8181_EMA_EXAX, byteoffset);
	return ite8181_graphics_read_1(sc, ITE8181_EMA_EXADATA);
}

inline void
ite8181_ema_write_1(sc, byteoffset, data)
	struct ite8181_softc *sc;
	int byteoffset;
	int data;
{
	ite8181_graphics_write_1(sc, ITE8181_EMA_EXAX, byteoffset);
	ite8181_graphics_write_1(sc, ITE8181_EMA_EXADATA, data);
}
	
int
ite8181_probe(iot, ioh)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{
	unsigned long regval;

#if NBIVIDEO > 0
	if (bivideo_dont_attach)	/* some video driver already attached */
		return (0);
#endif /* NBIVIDEO > 0 */

	regval = ite8181_config_read_4(iot, ioh, ITE8181_ID);
	VPRINTF(("ite8181_probe: vendor id=%04lx product id=%04lx\n",
		 regval & 0xffff, (regval >> 16) & 0xffff));
	if (regval != ((ITE8181_PRODUCT_ID << 16) | ITE8181_VENDER_ID))
		return (0);

	return (1);
}

void
ite8181_attach(sc)
	struct ite8181_softc *sc;
{
	unsigned long regval;
	struct hpcfb_attach_args ha;
	int console = (bootinfo->bi_cnuse & BI_CNUSE_SERIAL) ? 0 : 1;

	regval = ite8181_config_read_4(sc->sc_iot, sc->sc_ioh, ITE8181_CLASS);
	printf(": ITE8181 Rev.%02lx\n", regval & ITE8181_REV_MASK);

	/* set base offsets */
	sc->sc_mba = ite8181_config_read_4(sc->sc_iot, sc->sc_ioh, ITE8181_MBA);
	DPRINTFN(1, ("ite8181: Memory base offset %08x\n", sc->sc_mba));
	sc->sc_gba = ite8181_config_read_4(sc->sc_iot, sc->sc_ioh, ITE8181_GBA);
	DPRINTFN(1, ("ite8181: GUI base offset %08x\n", sc->sc_gba));
	sc->sc_sba = ite8181_config_read_4(sc->sc_iot, sc->sc_ioh, ITE8181_SBA);
	DPRINTFN(1, ("ite8181: Graphics base offset %08x\n", sc->sc_sba));

	/* assume lcd is on */
	sc->sc_lcd = 1;

	/* Add a power hook to power saving */
	sc->sc_powerstate = ITE8181_POWERSTATE_D0;
	sc->sc_powerhook = powerhook_establish(ite8181_power, sc);
	if (sc->sc_powerhook == NULL)
		printf("%s: WARNING: unable to establish power hook\n",
			sc->sc_dev.dv_xname);

	/* Add a hard power hook to power saving */
	sc->sc_hardpowerhook = config_hook(CONFIG_HOOK_PMEVENT,
					   CONFIG_HOOK_PMEVENT_HARDPOWER,
					   CONFIG_HOOK_SHARE,
					   ite8181_hardpower, sc);
	if (sc->sc_hardpowerhook == NULL)
		printf("%s: WARNING: unable to establish hard power hook\n",
			sc->sc_dev.dv_xname);

	ite8181_fbinit(&sc->sc_fbconf);

	ite8181_erase_cursor(sc);

	if (console && hpcfb_cnattach(&sc->sc_fbconf) != 0) {
		panic("ite8181_attach: can't init fb console");
	}

	ha.ha_console = console;
	ha.ha_accessops = &ite8181_ha;
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

int ite8181_lcd_power(sc, on)
	struct ite8181_softc *sc;
	int on;
{
	int lcd_p;
	int lcd_s;
	int lcd_seq;
	int loop = 10;

	if (ite8181_lcd_control_disable) {
		VPRINTF(("ite8171_lcd_control_disable!: %s\n", on?"on":"off"));
		return 0;
	}

	if (sc->sc_lcd != on) {
		ite8181_ema_write_1(sc, ITE8181_EMA_ENABLEEMA, 
			ITE8181_EMA_ENABLEPASS);
		lcd_p = ite8181_ema_read_1(sc, ITE8181_EMA_LCDPOWER);
		lcd_s = ite8181_ema_read_1(sc, ITE8181_EMA_LCDPOWERSTAT);
		lcd_seq = ite8181_ema_read_1(sc, ITE8181_EMA_LCDPOWERSEQ);
		DPRINTFN(1,("ite8181_lcd_power(%d)< p=%x, s=%x, seq=%x\n",
			on,
			lcd_p, lcd_s, lcd_seq));
		if (on) {
			sc->sc_lcd = 1;
			lcd_seq |= (ITE8181_PUP0|ITE8181_PUP1|ITE8181_PUP2);
			ite8181_ema_write_1(sc, ITE8181_EMA_LCDPOWERSEQ, lcd_seq);
			lcd_p &= ~ITE8181_LCDSTANDBY;
			ite8181_ema_write_1(sc, ITE8181_EMA_LCDPOWER, lcd_p);
			/*
			 * XXX:
			 * IBM WorkPad z50 power unit has too weak power.
			 * So we must wait too many times to access self device
			 * after LCD panel and BackLight on.
			 * Currently delay is not enough ??? FIXME 
			 */
			delay(ite8181_lcd_on_self_delay*MSEC);
			while (loop--) {
				lcd_p = ite8181_ema_read_1(sc, ITE8181_EMA_LCDPOWER);
				lcd_s = ite8181_ema_read_1(sc, ITE8181_EMA_LCDPOWERSTAT);
				lcd_seq = ite8181_ema_read_1(sc, ITE8181_EMA_LCDPOWERSEQ);
				DPRINTFN(1,("ite8181_lcd_power(%d)%d| p=%x, s=%x, seq=%x\n",
					on, loop,
					lcd_p, lcd_s, lcd_seq));
				/* XXX the states which are not described in manual.*/
				if (!(lcd_s&ITE8181_LCDPSTANDBY) &&
					!(lcd_s&ITE8181_LCDPUP) &&
					(lcd_s&ITE8181_LCDPON))
					break;
				delay(100);
			}
			lcd_s |= ITE8181_PPTOBEON;
			ite8181_ema_write_1(sc, ITE8181_EMA_LCDPOWERSTAT, lcd_s);
		} else {
			sc->sc_lcd = 0;
			lcd_p |= ITE8181_LCDSTANDBY;
			ite8181_ema_write_1(sc, ITE8181_EMA_LCDPOWER, lcd_p);
			while (loop--) {
				lcd_p = ite8181_ema_read_1(sc, ITE8181_EMA_LCDPOWER);
				lcd_s = ite8181_ema_read_1(sc, ITE8181_EMA_LCDPOWERSTAT);
				lcd_seq = ite8181_ema_read_1(sc, ITE8181_EMA_LCDPOWERSEQ);
				DPRINTFN(1,("ite8181_lcd_power(%d)%d| p=%x, s=%x, seq=%x\n",
					on, loop,
					lcd_p, lcd_s, lcd_seq));
				/* XXX the states which are not described in manual.*/
				if ((lcd_s&ITE8181_LCDPSTANDBY) &&
					!(lcd_s&ITE8181_LCDPDOWN) &&
					!(lcd_s&ITE8181_LCDPON))
					break;
				delay(100);
			}
			lcd_s &= ~ITE8181_PPTOBEON;
			ite8181_ema_write_1(sc, ITE8181_EMA_LCDPOWERSTAT, lcd_s);
		}
		DPRINTFN(1,("ite8181_lcd_power(%d)> p=%x, s=%x, seq=%x\n",
			on,
			ite8181_ema_read_1(sc, ITE8181_EMA_LCDPOWER),
			ite8181_ema_read_1(sc, ITE8181_EMA_LCDPOWERSTAT),
			ite8181_ema_read_1(sc, ITE8181_EMA_LCDPOWERSEQ)));
		ite8181_ema_write_1(sc, ITE8181_EMA_ENABLEEMA,
					ITE8181_EMA_DISABLEPASS);
	}	
	return 0;
}

static void
ite8181_erase_cursor(sc)
	struct ite8181_softc *sc;
{
	ite8181_gui_write_1(sc, ITE8181_GUI_C1C, 0); /* Cursor 1 Control Reg. */
	/* other ? */
}

static void 
ite8181_power(why, arg)
	int why;
	void *arg;
{
}

static int
ite8181_hardpower(ctx, type, id, msg)
	void *ctx;
	int type;
	long id;
	void *msg;
{
	struct ite8181_softc *sc = ctx;
	int why = (int)msg;

	switch (why) {
	case PWR_STANDBY:
		sc->sc_powerstate = ITE8181_POWERSTATE_D3;
		/* ite8181_lcd_power(sc, 0); */
		delay(MSEC);
		break;
	case PWR_SUSPEND:
		sc->sc_powerstate = ITE8181_POWERSTATE_D2;
		ite8181_lcd_power(sc, 0);	
		delay(MSEC);
		break;
	case PWR_RESUME:
		delay(MSEC);
		sc->sc_powerstate = ITE8181_POWERSTATE_D0;
		ite8181_lcd_power(sc, 1);	
		/*
		 * XXX:
		 * IBM WorkPad z50 power unit has too weak power.
		 * So we must wait too many times to access other devices
		 * after LCD panel and BackLight on.
		 */
		delay(ite8181_lcd_on_delay*MSEC);	
		break;
	}

	/*
	 * you should wait until the
	 * power state transit sequence will end.
	 */

	return (0);
}


static int
ite8181_fbinit(fb)
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
	fb->hf_baseaddr		= (u_long)bootinfo->fb_addr;
	fb->hf_offset		= (u_long)bootinfo->fb_addr -
				     mips_ptob(mips_btop(bootinfo->fb_addr));
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
ite8181_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct ite8181_softc *sc = (struct ite8181_softc *)v;
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_dspconf *dspconf;
	struct wsdisplay_cmap *cmap;

	switch (cmd) {
	case WSDISPLAYIO_GETCMAP:
		cmap = (struct wsdisplay_cmap*)data;

		if (sc->sc_fbconf.hf_class != HPCFB_CLASS_INDEXCOLOR ||
		    sc->sc_fbconf.hf_pack_width != 8 ||
		    256 <= cmap->index ||
		    256 < (cmap->index + cmap->count))
			return (EINVAL);

		if (!uvm_useracc(cmap->red, cmap->count, B_WRITE) ||
		    !uvm_useracc(cmap->green, cmap->count, B_WRITE) ||
		    !uvm_useracc(cmap->blue, cmap->count, B_WRITE))
			return (EFAULT);

#ifdef ITE8181_WINCE_CMAP
		copyout(&bivideo_cmap_r[cmap->index], cmap->red, cmap->count);
		copyout(&bivideo_cmap_g[cmap->index], cmap->green,cmap->count);
		copyout(&bivideo_cmap_b[cmap->index], cmap->blue, cmap->count);
		return (0);
#else /* ITE8181_WINCE_CMAP */
		return EINVAL;
#endif /* ITE8181_WINCE_CMAP */

	case WSDISPLAYIO_PUTCMAP:
		/*
		 * This driver can't set color map.
		 */
		return (EINVAL);

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
ite8181_mmap(ctx, offset, prot)
	void *ctx;
	off_t offset;
	int prot;
{
	struct ite8181_softc *sc = (struct ite8181_softc *)ctx;

	if (offset < 0 ||
	    (sc->sc_fbconf.hf_bytes_per_plane +
		sc->sc_fbconf.hf_offset) <  offset)
		return -1;

	return mips_btop((u_long)bootinfo->fb_addr + offset);
}
