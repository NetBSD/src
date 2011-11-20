/*	$NetBSD: epsonlcd.c,v 1.2 2011/11/20 11:20:32 kiyohara Exp $ */
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: epsonlcd.c,v 1.2 2011/11/20 11:20:32 kiyohara Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/bootinfo.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/hpc/hpcfbio.h>
#include <dev/hpc/hpcfbvar.h>
#include <dev/hpc/video_subr.h>

#include <arm/xscale/pxa2x0var.h>

#include <hpcarm/dev/s1d138xxreg.h>


struct epsonlcd_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	bus_addr_t sc_fbaddr;
	bus_space_tag_t sc_fbt;
	bus_space_handle_t sc_fbh;

	struct video_chip sc_vc;
	struct hpcfb_fbconf sc_fb;
	struct hpcfb_dspconf sc_dsp;
};

static int epsonlcd_match(device_t, cfdata_t, void *);
static void epsonlcd_attach(device_t, device_t, void *);

static int epsonlcd_ioctl(void *, u_long, void *, int, struct lwp *);
static paddr_t epsonlcd_mmap(void *, off_t, int);

static void epsonlcd_setup_hpcfbif(struct epsonlcd_softc *,
				   struct hpcfb_fbconf *);
static void epsonlcd_get_video_chip(struct epsonlcd_softc *,
				    struct video_chip *);

CFATTACH_DECL_NEW(epsonlcd, sizeof(struct epsonlcd_softc),
    epsonlcd_match, epsonlcd_attach, NULL, NULL);


static struct hpcfb_accessops epsonlcd_ha = {
	.ioctl = epsonlcd_ioctl,
	.mmap = epsonlcd_mmap,
};

/* ARGSUSED */
static int
epsonlcd_match(device_t parent, cfdata_t match, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (strcmp(pxa->pxa_name, match->cf_name) != 0 ||
	    !platid_match(&platid, &platid_mask_MACH_PSIONTEKLOGIX_NETBOOK_PRO))
		return 0;

	return 1;
}

/* ARGSUSED */
static void
epsonlcd_attach(device_t parent, device_t self, void *aux)
{
	struct epsonlcd_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;
	struct hpcfb_attach_args haa;
	int rv;

	aprint_naive("\n");
	aprint_normal(": Epson S1D138xx LCD Controller\n");

	sc->sc_dev = self;
	sc->sc_iot = pxa->pxa_iot;
	sc->sc_fbt = pxa->pxa_iot;

	/* NETBOOK PRO map Framebuffer here. */
	sc->sc_fbaddr = pxa->pxa_addr + 0x200000;

	if (bus_space_map(sc->sc_iot, pxa->pxa_addr, 0x2000, 0, &sc->sc_fbh) != 0) {
		aprint_error_dev(self, "can't map I/O space\n");
		return;
	}
	rv = bus_space_map(sc->sc_fbt, sc->sc_fbaddr, S1D138XX_FRAMEBUFFER_SIZE,
	    0, &sc->sc_fbh);
	if (rv != 0) {
		aprint_error_dev(self, "can't map Framebuffer\n");
		return;
	}

	epsonlcd_get_video_chip(sc, &sc->sc_vc);
	epsonlcd_setup_hpcfbif(sc, &sc->sc_fb);
	if (hpcfb_cnattach(&sc->sc_fb) != 0)
		panic("%s: hpcfb_cnattach failed\n", __func__);
	/* always console */
	aprint_normal_dev(self, "console\n");

	/* register interface to hpcfb */
	haa.ha_console = 1;	/* Always console */
	haa.ha_accessops = &epsonlcd_ha;
	haa.ha_accessctx = sc;
	haa.ha_curfbconf = 0;
	haa.ha_nfbconf = 1;
	haa.ha_fbconflist = &sc->sc_fb;
	haa.ha_curdspconf = 0;
	haa.ha_ndspconf = 1;
	haa.ha_dspconflist = &sc->sc_dsp;

	config_found(self, &haa, hpcfbprint);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "unable to establish power handler\n");
}


static int
epsonlcd_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
printf("%s: To Be Done...: cmd=0x%lx\n", __func__, cmd);
	switch (cmd) {
	}
	return EPASSTHROUGH;
}

static paddr_t
epsonlcd_mmap(void *ctx, off_t offset, int prot)
{
	struct epsonlcd_softc *sc = ctx;
	struct hpcfb_fbconf *fb = &sc->sc_fb;

	if (offset < 0 || (fb->hf_bytes_per_plane + fb->hf_offset) < offset)
		return -1;

	return arm_btop(sc->sc_fbaddr + offset);
}

static void
epsonlcd_setup_hpcfbif(struct epsonlcd_softc *sc, struct hpcfb_fbconf *fb)
{
	struct video_chip *vc = &sc->sc_vc;

	memset(fb, 0, sizeof(struct hpcfb_fbconf));
	fb->hf_conf_index = 0;		/* configuration index */
	fb->hf_nconfs = 1;		/* how many configurations */
	/* frame buffer name */
	strncpy(fb->hf_name, "Epson S1D138xx LCD Controller", HPCFB_MAXNAMELEN);

	/* configuration name */
	strncpy(fb->hf_conf_name, "LCD", HPCFB_MAXNAMELEN);

	fb->hf_height = vc->vc_fbheight;
	fb->hf_width = vc->vc_fbwidth;
	fb->hf_baseaddr = vc->vc_fbvaddr;
	/* frame buffer start offset */
	fb->hf_offset = vc->vc_fbvaddr - arm_ptob(arm_btop(vc->vc_fbvaddr));

	fb->hf_bytes_per_line = (vc->vc_fbwidth * vc->vc_fbdepth) / NBBY;
	fb->hf_nplanes = 1;
	fb->hf_bytes_per_plane = vc->vc_fbheight * fb->hf_bytes_per_line;

	fb->hf_access_flags |= HPCFB_ACCESS_BYTE;
	fb->hf_access_flags |= HPCFB_ACCESS_WORD;
	fb->hf_access_flags |= HPCFB_ACCESS_DWORD;
	if (vc->vc_reverse)
		fb->hf_access_flags |= HPCFB_ACCESS_REVERSE;

	switch (vc->vc_fbdepth) {
	default:
		panic("%s: not supported color depth %d\n",
		    __func__, vc->vc_fbdepth);

		/* NOTREACHED */
	case 16:
		fb->hf_class = HPCFB_CLASS_RGBCOLOR;
		fb->hf_access_flags |= HPCFB_ACCESS_STATIC;
		fb->hf_order_flags = HPCFB_REVORDER_BYTE;
		fb->hf_pack_width = 16;
		fb->hf_pixels_per_pack = 1;
		fb->hf_pixel_width = 16;

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

static void
epsonlcd_get_video_chip(struct epsonlcd_softc *sc, struct video_chip *vc)
{

#define BSH2VADDR(ioh)	(ioh)

	vc->vc_fbvaddr = BSH2VADDR(sc->sc_fbh);
	vc->vc_fbpaddr = sc->sc_fbaddr;
	vc->vc_fbsize = S1D138XX_FRAMEBUFFER_SIZE;
	vc->vc_fbdepth = (bootinfo->fb_line_bytes / bootinfo->fb_width) * NBBY;
	vc->vc_fbwidth = bootinfo->fb_width;
	vc->vc_fbheight = bootinfo->fb_height;
	vc->vc_reverse = video_reverse_color();
}
