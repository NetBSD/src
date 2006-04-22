/*	$NetBSD: ipaq_lcd.c,v 1.13.6.1 2006/04/22 11:37:28 simonb Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: ipaq_lcd.c,v 1.13.6.1 2006/04/22 11:37:28 simonb Exp $");

#define IPAQ_LCD_DEBUG

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/wscons/wsconsio.h>

#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <arm/cpufunc.h>
#include <arm/arm32/katelib.h>

#include <arm/sa11x0/sa11x0_reg.h> 
#include <arm/sa11x0/sa11x0_gpioreg.h>

#include <hpcarm/dev/ipaq_gpioreg.h>
#include <hpcarm/dev/ipaq_saipvar.h>
#include <hpcarm/dev/ipaq_lcdreg.h>
#include <hpcarm/dev/ipaq_lcdvar.h>

#ifdef IPAQ_LCD_DEBUG
#define DPRINTFN(n, x)  if (ipaqlcddebug > (n)) printf x
int     ipaqlcddebug = 0xff;
#else
#define DPRINTFN(n, x)
#endif
#define DPRINTF(x) DPRINTFN(0, x)

static int	ipaqlcd_match(struct device *, struct cfdata *, void *);
static void	ipaqlcd_attach(struct device *, struct device *, void *);
static void	ipaqlcd_init(struct ipaqlcd_softc *);
static int	ipaqlcd_fbinit(struct ipaqlcd_softc *);
static int	ipaqlcd_ioctl(void *, u_long, caddr_t, int, struct proc *);
static paddr_t	ipaqlcd_mmap(void *, off_t offset, int);

#if defined __mips__ || defined __sh__ || defined __arm__
#define __BTOP(x)		((paddr_t)(x) >> PGSHIFT)
#define __PTOB(x)		((paddr_t)(x) << PGSHIFT)
#else
#error "define btop, ptob."
#endif

CFATTACH_DECL(ipaqlcd, sizeof(struct ipaqlcd_softc),
    ipaqlcd_match, ipaqlcd_attach, NULL, NULL);

struct hpcfb_accessops ipaqlcd_ha = {
	ipaqlcd_ioctl, ipaqlcd_mmap
};
static int console_flag = 0;

static int
ipaqlcd_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

void
ipaqlcd_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ipaqlcd_softc *sc = (struct ipaqlcd_softc*)self;
	struct hpcfb_attach_args ha;
	struct ipaq_softc *psc = (struct ipaq_softc *)parent;

	sc->sc_iot = psc->sc_iot;
	sc->sc_parent = (struct ipaq_softc *)parent;

	ipaqlcd_init(sc);
	ipaqlcd_fbinit(sc);

	printf("\n");
	printf("%s: iPAQ internal LCD controller\n",  sc->sc_dev.dv_xname);

	DPRINTF(("framebuffer_baseaddr=%lx\n", (u_long)bootinfo->fb_addr));

        ha.ha_console = console_flag;
        ha.ha_accessops = &ipaqlcd_ha;
        ha.ha_accessctx = sc;
        ha.ha_curfbconf = 0;
        ha.ha_nfbconf = 1;
        ha.ha_fbconflist = &sc->sc_fbconf;
        ha.ha_curdspconf = 0;
        ha.ha_ndspconf = 1;
        ha.ha_dspconflist = &sc->sc_dspconf;

        config_found(&sc->sc_dev, &ha, hpcfbprint);
}

void
ipaqlcd_init(sc)
	struct ipaqlcd_softc *sc;
{
	/* Initialization of Extended GPIO */
	sc->sc_parent->ipaq_egpio |= EGPIO_LCD_INIT;
	bus_space_write_2(sc->sc_iot, sc->sc_parent->sc_egpioh,
			  0, sc->sc_parent->ipaq_egpio);

	if (bus_space_map(sc->sc_iot, SALCD_BASE, SALCD_NPORTS,
			  0, &sc->sc_ioh))
                panic("ipaqlcd_init:Cannot map registers");

	(u_long)bootinfo->fb_addr =
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_BA1);

	/*
	 * Initialize LCD Control Register 0 - 3 
	 *  must initialize DMA Channel Base Address Register 
	 *  before enabling LCD(LEN = 1) 
	 */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
					SALCD_CR1, IPAQ_LCCR1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
					SALCD_CR2, IPAQ_LCCR2);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
					SALCD_CR3, IPAQ_LCCR3);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
					SALCD_CR0, IPAQ_LCCR0);

	DPRINTF(("\n"
		 "DMA_BASE= %08x : DMA_CUR = %08x \n"
		 "LCCR0   = %08x : LCCR1   = %08x \n"
		 "LCCR2   = %08x : LCCR3   = %08x",
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_BA1),
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_CA1),
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_CR0),
	        bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_CR1),
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_CR2),
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_CR3)));

}

int
ipaqlcd_fbinit(sc)
	struct ipaqlcd_softc *sc;
{
	struct hpcfb_fbconf *fb;

	fb = &sc->sc_fbconf;

	/* Initialize fb */
	memset(fb, 0, sizeof(*fb));

	fb->hf_conf_index	= 0;    /* configuration index */
	fb->hf_nconfs		= 1;
	strcpy(fb->hf_name, "built-in video");
	strcpy(fb->hf_conf_name, "LCD");
                                        /* configuration name */
	fb->hf_height		= bootinfo->fb_height;
	fb->hf_width		= bootinfo->fb_width;

	if (bus_space_map(sc->sc_iot, (bus_addr_t)bootinfo->fb_addr,
			   bootinfo->fb_height * bootinfo->fb_line_bytes,
			   0, &fb->hf_baseaddr)) {
		printf("unable to map framebuffer\n");
		return (-1);
	}

	fb->hf_offset		= (u_long)bootinfo->fb_addr -
					__PTOB(__BTOP(bootinfo->fb_addr));
					/* frame buffer start offset    */
	fb->hf_bytes_per_line   = bootinfo->fb_line_bytes;
	fb->hf_nplanes          = 1;
	fb->hf_bytes_per_plane  = bootinfo->fb_height *
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
		case BIFB_D2_M2L_0:
		case BIFB_D2_M2L_0x2:
			fb->hf_class = HPCFB_CLASS_GRAYSCALE;
			break;
		case BIFB_D4_M2L_F:
		case BIFB_D4_M2L_Fx2:
			fb->hf_access_flags |= HPCFB_ACCESS_REVERSE;
		case BIFB_D4_M2L_0:
		case BIFB_D4_M2L_0x2:
			fb->hf_class = HPCFB_CLASS_GRAYSCALE;
			break;
		/*
		 * indexed color
		 */
		case BIFB_D8_FF:
		case BIFB_D8_00:
			fb->hf_offset	= 0x200;
			break;
		/*
		 * RGB color
		 */
		case BIFB_D16_FFFF:
		case BIFB_D16_0000:
			fb->hf_class = HPCFB_CLASS_RGBCOLOR;
			fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
			fb->hf_order_flags = HPCFB_REVORDER_BYTE;
			fb->hf_pack_width = 16;
			fb->hf_pixels_per_pack = 1;
			fb->hf_pixel_width = 16;

			fb->hf_class_data_length = sizeof(struct hf_rgb_tag);
			fb->hf_u.hf_rgb.hf_flags = 0;
				/* reserved for future use */
			fb->hf_u.hf_rgb.hf_red_width = 5;
			fb->hf_u.hf_rgb.hf_red_shift = 11;
			fb->hf_u.hf_rgb.hf_green_width = 6;
			fb->hf_u.hf_rgb.hf_green_shift = 5;
			fb->hf_u.hf_rgb.hf_blue_width = 5;
			fb->hf_u.hf_rgb.hf_blue_shift = 0;
			fb->hf_u.hf_rgb.hf_alpha_width = 0;
			fb->hf_u.hf_rgb.hf_alpha_shift = 0;
			break;
		default :
			printf("unknown type (=%d).\n", bootinfo->fb_type);
			return (-1);
			break;
	}

	return(0);
}

int
ipaqlcd_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct ipaqlcd_softc *sc = (struct ipaqlcd_softc *)v;
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
		return (0);
	case WSDISPLAYIO_PUTCMAP:
		return (EINVAL);
	case WSDISPLAYIO_GETPARAM:
                dispparam = (struct wsdisplay_param*)data;
                switch (dispparam->param) {
                case WSDISPLAYIO_PARAM_BACKLIGHT:
                        DPRINTF(("ipaqlcd_ioctl: GETPARAM:BACKLIGHT\n"));
                        return (EINVAL);
                case WSDISPLAYIO_PARAM_CONTRAST:
                        DPRINTF(("ipaqlcd_ioctl: GETPARAM:CONTRAST\n"));
                        return (EINVAL);
                case WSDISPLAYIO_PARAM_BRIGHTNESS:
                        DPRINTF(("ipaqlcd_ioctl: GETPARAM:BRIGHTNESS\n"));
                        return (EINVAL);
                default:
                        return (EINVAL);
                }
		return (0);
	case WSDISPLAYIO_SETPARAM:
                dispparam = (struct wsdisplay_param*)data;
                switch (dispparam->param) {
                case WSDISPLAYIO_PARAM_BACKLIGHT:
                        DPRINTF(("ipaqlcd_ioctl: GETPARAM:BACKLIGHT\n"));
                        return (EINVAL);
                case WSDISPLAYIO_PARAM_CONTRAST:
                        DPRINTF(("ipaqlcd_ioctl: GETPARAM:CONTRAST\n"));
                        return (EINVAL);
                case WSDISPLAYIO_PARAM_BRIGHTNESS:
                        DPRINTF(("ipaqlcd_ioctl: GETPARAM:BRIGHTNESS\n"));
                        return (EINVAL);
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
		*fbconf = sc->sc_fbconf;        /* structure assignment */
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
		*dspconf = sc->sc_dspconf;      /* structure assignment */
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
		return (EINVAL);
        }
	return (EPASSTHROUGH);
}

paddr_t
ipaqlcd_mmap(ctx, offset, prot)
	void *ctx;
	off_t offset;
	int prot;
{
	struct ipaqlcd_softc *sc = (struct ipaqlcd_softc *)ctx;

	if (offset < 0 ||
	   (sc->sc_fbconf.hf_bytes_per_plane +
	    sc->sc_fbconf.hf_offset) <  offset)
		return -1;

	return __BTOP((u_long)bootinfo->fb_addr + offset);
}
