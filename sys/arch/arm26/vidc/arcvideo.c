/* $NetBSD: arcvideo.c,v 1.6 2000/12/27 22:13:42 bjh21 Exp $ */
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * arcvideo.c - Archimedes video system driver.
 */

/*
 * The Arc video system is rather closely tied into the heart of the
 * machine, being partly controlled by the MEMC.  Similarly, this
 * driver will probably end up with its tentacles throughout the
 * kernel, though in theory it should be posible to leave it out.
 */

#include <sys/param.h>

__RCSID("$NetBSD: arcvideo.c,v 1.6 2000/12/27 22:13:42 bjh21 Exp $");

#include <sys/device.h>
#include <sys/errno.h>
#include <sys/reboot.h>	/* For bootverbose */
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/wscons/wsconsio.h>
#define WSDISPLAY_TYPE_ARCHIMEDES 42 /* XXX Should be in wsconsio.h */
#include <dev/wscons/wscons_raster.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <machine/boot.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/irq.h>
#include <machine/memcreg.h>

#include <arch/arm26/iobus/iocreg.h>
#include <arch/arm26/iobus/iocvar.h>
#include <arch/arm26/vidc/vidcreg.h>
#include <arch/arm26/vidc/arcvideovar.h>

#include "ioc.h"

static int arcvideo_match __P((struct device *parent, struct cfdata *cf,
			       void *aux));
static void arcvideo_attach __P((struct device *parent, struct device *self,
				 void *aux));
#if 0
static int arcvideo_setmode __P((struct device *self,
				 struct arcvideo_mode *mode));
static void arcvideo_await_vsync __P((struct device *self));
#endif
static int arcvideo_intr __P((void *cookie));
static int arcvideo_ioctl __P((void *cookie, u_long cmd, caddr_t data,
			       int flag, struct proc *p));
static paddr_t arcvideo_mmap __P((void *cookie, off_t off, int prot));
static int arcvideo_alloc_screen __P((void *cookie,
				      const struct wsscreen_descr *scr,
				      void **scookiep, int *curxp, int *curyp,
				      long *defattrp));
static void arcvideo_free_screen __P((void *cookie, void *scookie));
static void arcvideo_show_screen __P((void *cookie, void *scookie, int waitok,
				      void (*cb)(void *, int, int),
				      void *cbarg));
static int arcvideo_load_font __P((void *cookie, void *scookie,
				   struct wsdisplay_font *));
static void arccons_8bpp_hack __P((struct rasops_info *ri));

struct arcvideo_softc {
	struct device		sc_dev;
	paddr_t			sc_screenmem_base;
	struct arcvideo_mode	sc_current_mode;
	u_int32_t		sc_vidc_ctl;
	struct device		*sc_ioc;
	struct irq_handler	*sc_irq;
	int			sc_flags;
#define AV_VIDEO_ON	0x01
};

struct cfattach arcvideo_ca = {
	sizeof(struct arcvideo_softc), arcvideo_match, arcvideo_attach
};

extern struct cfdriver arcvideo_cd;
#if NIOC > 0
extern struct cfdriver ioc_cd;
#endif

static struct rasops_info arccons_ri;

static struct wsscreen_descr arcscreen;

static struct wsdisplay_accessops arcvideo_accessops = {
	arcvideo_ioctl, arcvideo_mmap, arcvideo_alloc_screen,
	arcvideo_free_screen, arcvideo_show_screen, arcvideo_load_font
};

static int arcvideo_isconsole = 0;

static int
arcvideo_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	/* A system can't sensibly have more than one VIDC. */
	if (cf->cf_unit == 0)
		return 1;
	else
		return 0;
}

static void
arcvideo_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct wsemuldisplaydev_attach_args da;
	struct wsscreen_list scrdata;
	const struct wsscreen_descr *screenp;
	struct arcvideo_softc *sc = (void *)self;

	if (!arcvideo_isconsole) {
		printf(": Not console -- I can't cope with this!\n");
		return;
	}

	sc->sc_flags = AV_VIDEO_ON;
	/* Detect monitor type? */
	/* Reset VIDC */

	/* Find IRQ */
#if NIOC > 0
	/*
	 * We could probe, but until someone comes up with a machine
	 * that's different from all the others, why bother?
	 */
	if (ioc_cd.cd_ndevs > 0 && ioc_cd.cd_devs[0] != NULL) {
		/* ioc0 exists */
		sc->sc_ioc = ioc_cd.cd_devs[0];
		sc->sc_irq = ioc_irq_establish(sc->sc_ioc, IOC_IRQ_IR, IPL_TTY,
					       arcvideo_intr, self);
		if (bootverbose)
			printf(": VSYNC interrupts at %s",
			    irq_string(sc->sc_irq));
	} else
#endif /* NIOC > 0 */
		if (bootverbose)
			printf(": no VSYNC sensing");

	printf("\n");

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
arcvideo_setmode(self, mode)
	struct device *self;
	struct arcvideo_mode *mode;
{
	struct arcvideo_softc *sc = (void *)self;
	u_int32_t newctl, ctlmask;
	u_int32_t newhswr, newhbsr, newhdsr, newhder, newhber, newhcr, newhir;
	u_int32_t newvswr, newvbsr, newvdsr, newvder, newvber, newvcr;

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
arcvideo_await_vsync(self)
	struct device *self;
{

	panic("arcvideo_await_vsync not implemented");
}
#endif

static int
arcvideo_intr(cookie)
	void *cookie;
{
/*	struct arcvideo_softc *sc = cookie; */

	return IRQ_HANDLED;
}

/*
 * In the standard RISC OS 8-bit palette (which we use), the bits go
 * BGgRbrTt, feeding RrTt, GgTt and BbTt to the DACs.  The top four of
 * these bits are fixed in hardware.
 *
 * The following table is the closest match I can get to the colours
 * at the top of rasops.c.
 */

static u_int8_t rasops_cmap_8bpp[] = {
	0x00, 0x10, 0x40, 0x50, 0x80, 0x90, 0xc0, 0xfc,
	0xd0, 0x17, 0x63, 0x77, 0x8b, 0x9f, 0xeb, 0xff,
};

void
arccons_init()
{
	long defattr;
	int clear = 0;
	int crow;
	struct rasops_info *ri = &arccons_ri;

	/* Force the screen to be at a known location */
	if (bootconfig.screenbase != 0)
		clear = 1;
	MEMC_WRITE(MEMC_SET_PTR(MEMC_VSTART, 0));
	MEMC_WRITE(MEMC_SET_PTR(MEMC_VINIT, 0));
	MEMC_WRITE(MEMC_SET_PTR(MEMC_VEND, 0x00080000));

	/* TODO: We should really set up the VIDC ourselves here. */

	/* Set up arccons_ri */
	memset(ri, 0, sizeof(*ri));
	ri->ri_depth = bootconfig.bpp;
	ri->ri_bits = (u_char *)(MEMC_PHYS_BASE + 0);
	ri->ri_width = bootconfig.xpixels;
	ri->ri_height = bootconfig.ypixels;
	ri->ri_stride = ((bootconfig.xpixels * bootconfig.bpp + 31) >> 5) << 2;
	ri->ri_flg = RI_CENTER | (clear ? RI_CLEAR : 0);

	if (rasops_init(ri, 1000, 1000) < 0)
		panic("rasops_init failed");

	/* Register video memory with UVM now we know how much we're using. */
	uvm_page_physload(0, atop(MEMC_DMA_MAX),
			  atop(round_page(ri->ri_height * ri->ri_stride)),
			  atop(MEMC_DMA_MAX), VM_FREELIST_LOW);

	if (ri->ri_depth == 8)
		arccons_8bpp_hack(&arccons_ri);

	/* Take rcons stuff and put it in arcscreen */
	/* XXX shouldn't this kind of thing be done by rcons_init? */
	arcscreen.name = "arccons";
	arcscreen.ncols = ri->ri_cols;
	arcscreen.nrows = ri->ri_rows;
	arcscreen.textops = &ri->ri_ops;
	arcscreen.fontwidth = ri->ri_font->fontwidth;
	arcscreen.fontheight = ri->ri_font->fontheight;
	arcscreen.capabilities = ri->ri_caps;

	/* work out cursor row */
	if (clear)
		crow = 0;
	else
		/* +/-1 is to round up */
		crow = (bootconfig.cpixelrow - ri->ri_yorigin - 1) /
			ri->ri_font->fontheight + 1;
	if (crow < 0) crow = 0;
	if (crow > ri->ri_rows) crow = ri->ri_rows;

	if ((arccons_ri.ri_ops.alloc_attr)(&arccons_ri, 0, 0, 0, &defattr) !=
	    0)
		panic("alloc_attr failed");
	wsdisplay_cnattach(&arcscreen, &arccons_ri, 0, crow, defattr);

	/* That should be all */
	arcvideo_isconsole = 1;
}

/*
 * The following is a gross hack because the rasops code has no way
 * for us to specify the devcmap if we don't want the default.  I think
 * it assumes that all 8-bit displays are PseudoColor.
 */

static void
arccons_8bpp_hack(ri)
	struct rasops_info *ri;
{
	int i, c;

	for (i = 0; i < 16; i++) {
		c = rasops_cmap_8bpp[i];
		ri->ri_devcmap[i] = c | (c<<8) | (c<<16) | (c<<24);
	}
}


/* wsdisplay access functions */

static int
arcvideo_ioctl(cookie, cmd, data, flag, p)
	void *cookie;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct arcvideo_softc *sc = cookie;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_ARCHIMEDES;
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
	return -1;
}

static paddr_t
arcvideo_mmap(cookie, off, prot)
	void *cookie;
	off_t off;
	int prot;
{

	return ENODEV;
}

static int
arcvideo_alloc_screen(cookie, scr, scookiep, curxp, curyp, defattrp)
	void *cookie, **scookiep;
	const struct wsscreen_descr *scr;
	int *curxp, *curyp;
	long *defattrp;
{

	return ENODEV;
}

static void
arcvideo_free_screen(cookie, scookie)
	void *cookie, *scookie;
{

	panic("arcvideo_free_screen not implemented");
}

static void
arcvideo_show_screen(cookie, scookie, waitok, cb, cbarg)
	void *cookie, *scookie;
	int waitok;
	void (*cb) __P((void *cbarg, int error, int waitok));
	void *cbarg;
{

	panic("arcvideo_show_screen not implemented");
}

static int
arcvideo_load_font(cookie, emulcookie, font)
	void *cookie, *emulcookie;
	struct wsdisplay_font *font;
{

	return ENODEV;
}
