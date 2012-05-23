/* $NetBSD: arcvideo.c,v 1.15.2.2 2012/05/23 10:07:37 yamt Exp $ */
/*-
 * Copyright (c) 1998, 2000 Ben Harris
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
/*
 * arcvideo.c - Archimedes video system driver.
 */

/*
 * The Arc video system is rather closely tied into the heart of the
 * machine, being partly controlled by the MEMC.  Similarly, this
 * driver will probably end up with its tentacles throughout the
 * kernel, though in theory it should be possible to leave it out.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arcvideo.c,v 1.15.2.2 2012/05/23 10:07:37 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/reboot.h>	/* For bootverbose */
#include <sys/systm.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_raster.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <machine/boot.h>
#include <machine/irq.h>
#include <machine/machdep.h>
#include <machine/memcreg.h>

#include <arch/acorn26/iobus/iocreg.h>
#include <arch/acorn26/iobus/iocvar.h>
#include <arch/acorn26/vidc/vidcreg.h>
#include <arch/acorn26/vidc/arcvideovar.h>

static int arcvideo_match(device_t parent, cfdata_t cf, void *aux);
static void arcvideo_attach(device_t parent, device_t self, void *aux);
#if 0
static int arcvideo_setmode(device_t self, struct arcvideo_mode *mode);
static void arcvideo_await_vsync(device_t self);
#endif
static int arcvideo_ioctl(void *cookie, void *vs, u_long cmd, void *data,
			       int flag, struct lwp *l);
static paddr_t arcvideo_mmap(void *cookie, void *vs, off_t off, int prot);
static int arcvideo_alloc_screen(void *cookie, const struct wsscreen_descr *scr,
				      void **scookiep, int *curxp, int *curyp,
				      long *defattrp);
static void arcvideo_free_screen(void *cookie, void *scookie);
static int arcvideo_show_screen(void *cookie, void *scookie, int waitok,
				      void (*cb)(void *, int, int),
				      void *cbarg);
static int arcvideo_load_font(void *cookie, void *scookie,
				   struct wsdisplay_font *);
static void arccons_8bpp_hack(struct rasops_info *ri);

struct arcvideo_softc {
	device_t		sc_dev;
	uint32_t		sc_vidc_ctl;
	int			sc_flags;
#define AV_VIDEO_ON	0x01
};

CFATTACH_DECL_NEW(arcvideo, sizeof(struct arcvideo_softc),
    arcvideo_match, arcvideo_attach, NULL, NULL);

device_t the_arcvideo;

static struct rasops_info arccons_ri;

static struct wsscreen_descr arcscreen;

static struct wsdisplay_accessops arcvideo_accessops = {
	arcvideo_ioctl, arcvideo_mmap, arcvideo_alloc_screen,
	arcvideo_free_screen, arcvideo_show_screen, arcvideo_load_font
};

static int arcvideo_isconsole = 0;

static int
arcvideo_match(device_t parent, cfdata_t cf, void *aux)
{

	/* A system can't sensibly have more than one VIDC. */
	if (the_arcvideo == NULL)
		return 1;
	return 0;
}

static void
arcvideo_attach(device_t parent, device_t self, void *aux)
{
	struct wsemuldisplaydev_attach_args da;
	struct wsscreen_list scrdata;
	const struct wsscreen_descr *screenp;
	struct arcvideo_softc *sc = device_private(self);

	sc->sc_dev = the_arcvideo = self;
	if (arcvideo_isconsole) {
		struct rasops_info *ri = &arccons_ri;
		long defattr;

		if (rasops_init(ri, 1000, 1000) < 0)
			panic("rasops_init failed");

		/* Take rcons stuff and put it in arcscreen */
		/* XXX shouldn't this kind of thing be done by rcons_init? */
		arcscreen.name = "arccons";
		arcscreen.ncols = ri->ri_cols;
		arcscreen.nrows = ri->ri_rows;
		arcscreen.textops = &ri->ri_ops;
		arcscreen.fontwidth = ri->ri_font->fontwidth;
		arcscreen.fontheight = ri->ri_font->fontheight;
		arcscreen.capabilities = ri->ri_caps;

		if ((ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr) != 0)
			panic("allocattr failed");
		wsdisplay_cnattach(&arcscreen, ri, 0, 0, defattr);
	} else {
		aprint_error(": Not console -- I can't cope with this!\n");
		return;
	}

	sc->sc_flags = AV_VIDEO_ON;
	/* Detect monitor type? */
	/* Reset VIDC */

	aprint_verbose(": VSYNC interrupts at IRQ %d", IOC_IRQ_IR);

	aprint_normal("\n");

	scrdata.nscreens = 1;
	scrdata.screens = &screenp;
	screenp = &arcscreen;

	da.console = arcvideo_isconsole;
	da.scrdata = &scrdata;
	da.accessops = &arcvideo_accessops;
	da.accesscookie = sc;

	config_found(self, &da, wsemuldisplaydevprint);
}

#if 0
static int
arcvideo_setmode(device_t self, struct arcvideo_mode *mode)
{
	struct arcvideo_softc *sc = device_private(self);
	uint32_t newctl, ctlmask;
	uint32_t newhswr, newhbsr, newhdsr, newhder, newhber, newhcr, newhir;
	uint32_t newvswr, newvbsr, newvdsr, newvder, newvber, newvcr;

	newctl = 0;
	/* Dot clock */
	/* XXX: should this be abstracted a little to allow for
           variable VIDCLKs? */
	switch (mode->timings.pixelrate) {
	case 8000000:
		newctl |= VIDC_CTL_DOTCLOCK_8MHZ;
		break;
	case 12000000:
		newctl |= VIDC_CTL_DOTCLOCK_12MHZ;
		break;
	case 16000000:
		newctl |= VIDC_CTL_DOTCLOCK_16MHZ;
		break;
	case 24000000:
		newctl |= VIDC_CTL_DOTCLOCK_24MHZ;
		break;
	default:
		return ENXIO;
	};
	/* Bits per pixel */
	switch (mode->bpp) {
	case 1:
		newctl |= VIDC_CTL_BPP_ONE;
		break;
	case 2:
		newctl |= VIDC_CTL_BPP_TWO;
		break;
	case 4:
		newctl |= VIDC_CTL_BPP_FOUR;
		break;
	case 8:
		newctl |= VIDC_CTL_BPP_EIGHT;
		break;
	default:
		return EINVAL;
	}
	/* DMA timings */
	/* XXX: should work this out from pixelrate, bpp and MCLK rate. */
	newctl |= VIDC_CTL_DMARQ_37;

	ctlmask = ~(VIDC_CTL_DOTCLOCK_MASK
		    | VIDC_CTL_BPP_MASK
		    | VIDC_CTL_DMARQ_MASK);
	
	newhswr = (mode->timings.hsw - 2) / 2 << 14;
	newhbsr = (mode->timings.hbs - 1) / 2 << 14;
	switch (mode->bpp) {
	case 8:
		newhdsr = (mode->timings.hds - 5) / 2 << 14;
		newhder = (mode->timings.hde - 5) / 2 << 14;
		break;
	case 4:
		newhdsr = (mode->timings.hds - 7) / 2 << 14;
		newhder = (mode->timings.hde - 7) / 2 << 14;
		break;
	case 2:
		newhdsr = (mode->timings.hds - 11) / 2 << 14;
		newhder = (mode->timings.hde - 11) / 2 << 14;
		break;
	case 1:
		newhdsr = (mode->timings.hds - 19) / 2 << 14;
		newhder = (mode->timings.hde - 19) / 2 << 14;
		break;
	}
	newhber = (mode->timings.hbe - 1) / 2 << 14;
	newhcr  = (mode->timings.hc  - 2) / 2 << 14;
	newhir  = mode->timings.hc / 4 << 14;
	newvswr = (mode->timings.vsw - 1) << 14;
	newvbsr = (mode->timings.vbs - 1) << 14;
	newvdsr = (mode->timings.vds - 1) << 14;
	newvder = (mode->timings.vde - 1) << 14;
	newvber = (mode->timings.vbe - 1) << 14;
	newvcr  = (mode->timings.vc  - 1) << 14;
	arcvideo_await_vsync(self);
	spltty(); /* XXX audio? */
	newctl |= sc->sc_vidc_ctl & ctlmask;
	VIDC_WRITE(VIDC_CONTROL | newctl);
	sc->sc_vidc_ctl = newctl;
	VIDC_WRITE(VIDC_VCR  | newvcr);
	VIDC_WRITE(VIDC_VSWR | newvswr);
	VIDC_WRITE(VIDC_VBSR | newvbsr);
	VIDC_WRITE(VIDC_VDSR | newvdsr);
	VIDC_WRITE(VIDC_VDER | newvder);
	VIDC_WRITE(VIDC_VBER | newvber);
	VIDC_WRITE(VIDC_HCR  | newhcr);
	VIDC_WRITE(VIDC_HIR  | newhir);
	VIDC_WRITE(VIDC_HSWR | newhswr);
	VIDC_WRITE(VIDC_HBSR | newhbsr);
	VIDC_WRITE(VIDC_HDSR | newhdsr);
	VIDC_WRITE(VIDC_HDER | newhder);
	VIDC_WRITE(VIDC_HBER | newhber);
	return 0;
}

static void
arcvideo_await_vsync(device_t self)
{

	panic("arcvideo_await_vsync not implemented");
}
#endif

/*
 * In the standard RISC OS 8-bit palette (which we use), the bits go
 * BGgRbrTt, feeding RrTt, GgTt and BbTt to the DACs.  The top four of
 * these bits are fixed in hardware.
 *
 * The following table is the closest match I can get to the colours
 * at the top of rasops.c.
 */

static uint8_t rasops_cmap_8bpp[] = {
	0x00, 0x10, 0x40, 0x50, 0x80, 0x90, 0xc0, 0xfc,
	0xd0, 0x17, 0x63, 0x77, 0x8b, 0x9f, 0xeb, 0xff,
};

void
arccons_init(void)
{
	struct rasops_info *ri = &arccons_ri;
	int i;

	MEMC_WRITE(MEMC_SET_PTR(MEMC_VSTART, 0));
	MEMC_WRITE(MEMC_SET_PTR(MEMC_VINIT, 0));
	MEMC_WRITE(MEMC_SET_PTR(MEMC_VEND, 0x00080000));

	/* TODO: We should really set up the VIDC ourselves here. */

	/* Set up arccons_ri */
	memset(ri, 0, sizeof(*ri));
	ri->ri_depth = bootconfig.bpp;
	ri->ri_bits = (u_char *)(MEMC_PHYS_BASE);
	ri->ri_width = bootconfig.xpixels;
	ri->ri_height = bootconfig.ypixels;
	ri->ri_stride = ((bootconfig.xpixels * bootconfig.bpp + 31) >> 5) << 2;
	ri->ri_flg = RI_CENTER | ((bootconfig.screenbase != 0) ? RI_CLEAR : 0);

	/* Register video memory with UVM now we know how much we're using. */
	uvm_page_physload(0, atop(MEMC_DMA_MAX),
			  atop(round_page(ri->ri_height * ri->ri_stride)),
			  atop(MEMC_DMA_MAX), VM_FREELIST_LOW);

	if (ri->ri_depth == 8)
		arccons_8bpp_hack(&arccons_ri);
	else if (ri->ri_depth == 4)
		for (i = 0; i < 1 << ri->ri_depth; i++)
			VIDC_WRITE(VIDC_PALETTE_LCOL(i) | 
			    VIDC_PALETTE_ENTRY(rasops_cmap[3*i + 0] >> 4,
					       rasops_cmap[3*i + 1] >> 4,
					       rasops_cmap[3*i + 2] >> 4, 0));

	/* That should be all */
	arcvideo_isconsole = 1;
}

/*
 * The following is a gross hack because the rasops code has no way
 * for us to specify the devcmap if we don't want the default.  I think
 * it assumes that all 8-bit displays are PseudoColor.
 */

static void
arccons_8bpp_hack(struct rasops_info *ri)
{
	int i, c;

	for (i = 0; i < 16; i++) {
		c = rasops_cmap_8bpp[i];
		ri->ri_devcmap[i] = c | (c<<8) | (c<<16) | (c<<24);
	}
}


/* wsdisplay access functions */

static int
arcvideo_ioctl(void *cookie, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct arcvideo_softc *sc = cookie;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_VIDC;
		return 0;
	case WSDISPLAYIO_GVIDEO:
		if (sc->sc_flags & AV_VIDEO_ON)
			*(u_int *)data = WSDISPLAYIO_VIDEO_ON;
		else
			*(u_int *)data = WSDISPLAYIO_VIDEO_OFF;
		return 0;
	case WSDISPLAYIO_SVIDEO:
		switch (*(u_int *)data) {
		case WSDISPLAYIO_VIDEO_OFF:
			sc->sc_flags &= ~AV_VIDEO_ON;
			update_memc(MEMC_CTL_VIDEODMA | MEMC_CTL_RFRSH_MASK,
				    MEMC_CTL_RFRSH_CONTIN);
			return 0;
		case WSDISPLAYIO_VIDEO_ON:
			sc->sc_flags |= AV_VIDEO_ON;
			update_memc(MEMC_CTL_VIDEODMA | MEMC_CTL_RFRSH_MASK,
				    MEMC_CTL_VIDEODMA |
				    MEMC_CTL_RFRSH_FLYBACK);
			return 0;
		}
	}
	return EPASSTHROUGH;
}

static paddr_t
arcvideo_mmap(void *cookie, void *vs, off_t off, int prot)
{

	return ENODEV;
}

static int
arcvideo_alloc_screen(void *cookie, const struct wsscreen_descr *scr,
    void **scookiep, int *curxp, int *curyp, long *defattrp)
{

	return ENODEV;
}

static void
arcvideo_free_screen(void *cookie, void *scookie)
{

	panic("arcvideo_free_screen not implemented");
}

static int
arcvideo_show_screen(void *cookie, void *scookie, int waitok,
    void (*cb)(void *cbarg, int error, int waitok), void *cbarg)
{

	/* Do nothing, since there can only be one screen. */
	return 0;
}

static int
arcvideo_load_font(void *cookie, void *emulcookie, struct wsdisplay_font *font)
{

	return EPASSTHROUGH;
}
