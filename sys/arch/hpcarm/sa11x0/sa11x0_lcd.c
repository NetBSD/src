/*	$NetBSD: sa11x0_lcd.c,v 1.5 2001/07/02 13:52:30 ichiro Exp $	*/
#define SALCD_DEBUG

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
#include <machine/irqhandler.h>
#include <machine/cpufunc.h>
#include <machine/katelib.h>

#include <hpcarm/sa11x0/sa11x0_reg.h> 
#include <hpcarm/sa11x0/sa11x0_var.h>
#include <hpcarm/sa11x0/sa11x0_lcdreg.h>
#include <hpcarm/sa11x0/sa11x0_lcdvar.h>

#ifdef SALCD_DEBUG
#define DPRINTFN(n, x)  if (salcddebug > (n)) printf x
int     salcddebug = 0xff;
#else
#define DPRINTFN(n, x)
#endif
#define DPRINTF(x) DPRINTFN(0, x)

static int	salcd_match(struct device *, struct cfdata *, void *);
static void	salcd_attach(struct device *, struct device *, void *);
static void	salcd_init(struct salcd_softc *);
static int	salcd_fbinit(struct hpcfb_fbconf *);
static int	salcd_ioctl(void *, u_long, caddr_t, int, struct proc *);
static paddr_t	salcd_mmap(void *, off_t offset, int);

extern  struct bus_space sa11x0_bs_tag;

#if defined __mips__ || defined __sh__ || defined __arm__
#define __BTOP(x)		((paddr_t)(x) >> PGSHIFT)
#define __PTOB(x)		((paddr_t)(x) << PGSHIFT)
#else
#error "define btop, ptob."
#endif

struct cfattach salcd_ca = {
	sizeof(struct salcd_softc), salcd_match, salcd_attach
};

struct hpcfb_accessops salcd_ha = {
	salcd_ioctl, salcd_mmap
};
static int console_flag = 0;

static int
salcd_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

void
salcd_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct salcd_softc *sc = (struct salcd_softc*)self;
	struct hpcfb_attach_args ha;

	sc->sc_iot = &sa11x0_bs_tag;
	sc->sc_parent = (struct sa11x0_softc *)parent;
#ifdef XXX
	(u_long)bootinfo->fb_addr = 0x48200000; /* XXX For iPAQ*/
#endif
	salcd_init(sc);
	salcd_fbinit(&sc->sc_fbconf);

	printf("\n");
	printf("%s: SA-11x0 internal LCD controller\n",  sc->sc_dev.dv_xname);

	DPRINTF(("framebuffer_baseaddr=%lx\n", (u_long)bootinfo->fb_addr));

        ha.ha_console = console_flag;
        ha.ha_accessops = &salcd_ha;
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
salcd_init(sc)
	struct salcd_softc *sc;
{
	int buf_r0, buf_r1, buf_r2, buf_r3;

	if (bus_space_map(sc->sc_iot, SALCD_BASE, SALCD_NPORTS,
			  0, &sc->sc_ioh))
                panic("salcd_init:Cannot map registers\n");
	/*
	 * must initialize DMA Channel Base Address Register 
	 * before enabling LCD(LEN = 1) 
	 */

	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			SALCD_BA1, (u_long)bootinfo->fb_addr);

#if 1 /* XXX for iPAQ */
	buf_r0 = buf_r1 = buf_r2 = buf_r3 = 0x0;
	/*
	 * LCD Control Register 0 
	 * LEN = 1, PAS = 1
	 */
	buf_r0 |= CR0_LEN | CR0_PAS;

	/*
	 * LCD Control Register 1
	 * PPL = 304(0x0130),VSW = (1 << 11)
	 * EFW = (1 << 16),BFW = (0x0a << 24)
	 */
	buf_r1 |= (bootinfo->fb_width - 16) | (1 << 11) | (1 << 20) | (0x0b << 24);

	/*
	 * LCD Control Register 2
	 * LPP = 240(0xf0), VSW = (1 << 11)
	 * EFW = (1 << 16), (0x0a << 24)
	 */
	buf_r2 |= (bootinfo->fb_height) | (1 << 11) | (1 << 16) |(0x0a << 24);

	/*
	 * LCD Control Register 3
	 * PCD = 0x10,
	 * VSP = (1 << 20) , HSP = (1 << 21)
	 */
	buf_r3 |= 0x10 | (1 << 20) | (1 << 21);

#endif
	/*
	 * initialize LCD Control Register 0 - 3 
	 */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			SALCD_CR0, buf_r0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			SALCD_CR1, buf_r1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			SALCD_CR2, buf_r2);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			SALCD_CR3, buf_r3);

	(u_long)bootinfo->fb_addr = 
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_CA1);

	DPRINTF(("DMA_BASE= %08x : DMA_CUR = %08x \n"
		 "LCCR0   = %08x : LCCR1   = %08x \n"
		 "LCCR2   = %08x : LCCR3   = %08x \n",
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_BA1),
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_CA1),
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_CR0),
	        bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_CR1),
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_CR2),
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, SALCD_CR3)));

}

int
salcd_fbinit(fb)
	struct hpcfb_fbconf *fb;
{
	/* Initialize fb */
	bzero(fb, sizeof(*fb));

	fb->hf_conf_index	= 0;    /* configuration index */
	fb->hf_nconfs		= 1;
	strcpy(fb->hf_name, "built-in video");
	strcpy(fb->hf_conf_name, "LCD");
                                        /* configuration name */
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
salcd_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct salcd_softc *sc = (struct salcd_softc *)v;
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
                        DPRINTF(("salcd_ioctl: GETPARAM:BACKLIGHT\n"));
                        return (EINVAL);
                case WSDISPLAYIO_PARAM_CONTRAST:
                        DPRINTF(("salcd_ioctl: GETPARAM:CONTRAST\n"));
                        return (EINVAL);
                case WSDISPLAYIO_PARAM_BRIGHTNESS:
                        DPRINTF(("salcd_ioctl: GETPARAM:BRIGHTNESS\n"));
                        return (EINVAL);
                default:
                        return (EINVAL);
                }
		return (0);
	case WSDISPLAYIO_SETPARAM:
                dispparam = (struct wsdisplay_param*)data;
                switch (dispparam->param) {
                case WSDISPLAYIO_PARAM_BACKLIGHT:
                        DPRINTF(("salcd_ioctl: GETPARAM:BACKLIGHT\n"));
                        return (EINVAL);
                case WSDISPLAYIO_PARAM_CONTRAST:
                        DPRINTF(("salcd_ioctl: GETPARAM:CONTRAST\n"));
                        return (EINVAL);
                case WSDISPLAYIO_PARAM_BRIGHTNESS:
                        DPRINTF(("salcd_ioctl: GETPARAM:BRIGHTNESS\n"));
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
 		 * because we have only one unit and one configration
		 */
		return (0);

	case HPCFBIO_GOP:
        case HPCFBIO_SOP:
		return (EINVAL);
        }
	return (ENOTTY);
}

paddr_t
salcd_mmap(ctx, offset, prot)
	void *ctx;
	off_t offset;
	int prot;
{
	struct salcd_softc *sc = (struct salcd_softc *)ctx;

	if (offset < 0 ||
	   (sc->sc_fbconf.hf_bytes_per_plane +
	    sc->sc_fbconf.hf_offset) <  offset)
		return -1;

	return __BTOP((u_long)bootinfo->fb_addr + offset);
}
