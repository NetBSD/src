/*	$NetBSD: p9100.c,v 1.1.2.3 2001/01/18 09:23:34 bouyer Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*
 * color display (p9100) driver.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * XXX should defer colormap updates to vertical retrace interrupts
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>
#include <dev/sun/btreg.h>
#include <dev/sun/btvar.h>
#if 0
#include <dev/sbus/p9100reg.h>
#endif

#include <dev/sbus/sbusvar.h>

#include "tctrl.h"
#if NTCTRL > 0
#include <machine/tctrl.h>
#include <sparc/dev/tctrlvar.h>/*XXX*/
#endif

#include <machine/conf.h>

/* per-display variables */
struct p9100_softc {
	struct device	sc_dev;		/* base device */
	struct sbusdev	sc_sd;		/* sbus device */
	struct fbdevice	sc_fb;		/* frame buffer device */
	bus_space_tag_t	sc_bustag;
	bus_type_t	sc_ctl_btype;	/* phys address description */
	bus_addr_t	sc_ctl_paddr;	/*   for device mmap() */
	bus_size_t	sc_ctl_psize;	/*   for device mmap() */
	bus_space_handle_t sc_ctl_memh;	/*   bus space handle */
	bus_type_t	sc_cmd_btype;	/* phys address description */
	bus_addr_t	sc_cmd_paddr;	/*   for device mmap() */
	bus_size_t	sc_cmd_psize;	/*   for device mmap() */
	bus_space_handle_t sc_cmd_memh;	/*   bus space handle */
	bus_type_t	sc_fb_btype;	/* phys address description */
	bus_addr_t	sc_fb_paddr;	/*   for device mmap() */
	bus_size_t	sc_fb_psize;	/*   for device mmap() */
	bus_space_handle_t sc_fb_memh;	/*   bus space handle */
	uint32_t sc_junk;

	union	bt_cmap sc_cmap;	/* Brooktree color map */
};

/* The Tadpole 3GX Technical Reference Manual lies.  The ramdac registers
 * are map in 4 byte increments, not 8.
 */
#define	SCRN_RPNT_CTL_1	0x0138	/* Screen Respaint Timing Control 1 */
#define	VIDEO_ENABLED	0x00000020
#define	PWRUP_CNFG	0x0194	/* Power Up Configuration */
#define	DAC_CMAP_WRIDX	0x0200	/* IBM RGB528 Palette Address (Write) */
#define	DAC_CMAP_DATA	0x0204	/* IBM RGB528 Palette Data */
#define	DAC_PXL_MASK	0x0208	/* IBM RGB528 Pixel Mask */
#define	DAC_CMAP_RDIDX	0x020c	/* IBM RGB528 Palette Address (Read) */
#define	DAC_INDX_LO	0x0210	/* IBM RGB528 Index Low */
#define	DAC_INDX_HI	0x0214	/* IBM RGB528 Index High */
#define	DAC_INDX_DATA	0x0218	/* IBM RGB528 Index Data (Indexed Registers) */
#define	DAC_INDX_CTL	0x021c	/* IBM RGB528 Index Control */

/* autoconfiguration driver */
static int	p9100_sbus_match(struct device *, struct cfdata *, void *);
static void	p9100_sbus_attach(struct device *, struct device *, void *);

static void	p9100unblank(struct device *);
static void	p9100_shutdown(void *);

/* cdevsw prototypes */
cdev_decl(p9100);

struct cfattach pnozz_ca = {
	sizeof(struct p9100_softc), p9100_sbus_match, p9100_sbus_attach
};

extern struct cfdriver pnozz_cd;

/* frame buffer generic driver */
static struct fbdriver p9100fbdriver = {
	p9100unblank, p9100open, p9100close, p9100ioctl, p9100poll,
	p9100mmap
};

static void p9100loadcmap(struct p9100_softc *, int, int);
static void p9100_set_video(struct p9100_softc *, int);
static int p9100_get_video(struct p9100_softc *);
static uint32_t p9100_ctl_read_4(struct p9100_softc *, bus_size_t);
static void p9100_ctl_write_4(struct p9100_softc *, bus_size_t, uint32_t);
#if 0
static uint8_t p9100_ramdac_read(struct p9100_softc *, bus_size_t);
#endif
static void p9100_ramdac_write(struct p9100_softc *, bus_size_t, uint8_t);

/*
 * Match a p9100.
 */
static int
p9100_sbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sbus_attach_args *sa = aux;

	return (strcmp("p9100", sa->sa_name) == 0);
}


/*
 * Attach a display.  We need to notice if it is the console, too.
 */
static void
p9100_sbus_attach(struct device *parent, struct device *self, void *args)
{
	struct p9100_softc *sc = (struct p9100_softc *)self;
	struct sbus_attach_args *sa = args;
	struct fbdevice *fb = &sc->sc_fb;
	int isconsole;
	int node;
	int i;

	/* Remember cookies for p9100_mmap() */
	sc->sc_bustag = sa->sa_bustag;
	sc->sc_ctl_btype = (bus_type_t)sa->sa_reg[0].sbr_slot;
	sc->sc_ctl_paddr = (bus_addr_t)sa->sa_reg[0].sbr_offset;
	sc->sc_ctl_psize = (bus_size_t)sa->sa_reg[0].sbr_size;
	sc->sc_cmd_btype = (bus_type_t)sa->sa_reg[1].sbr_slot;
	sc->sc_cmd_paddr = (bus_addr_t)sa->sa_reg[1].sbr_offset;
	sc->sc_cmd_psize = (bus_size_t)sa->sa_reg[1].sbr_size;
	sc->sc_fb_btype = (bus_type_t)sa->sa_reg[2].sbr_slot;
	sc->sc_fb_paddr = (bus_addr_t)sa->sa_reg[2].sbr_offset;
	sc->sc_fb_psize = (bus_size_t)sa->sa_reg[2].sbr_size;

	fb->fb_driver = &p9100fbdriver;
	fb->fb_device = &sc->sc_dev;
	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags & FB_USERMASK;
	fb->fb_type.fb_type = FBTYPE_SUN3COLOR;

	node = sa->sa_node;

	/*
	 * When the ROM has mapped in a p9100 display, the address
	 * maps only the video RAM, so in any case we have to map the
	 * registers ourselves.  We only need the video RAM if we are
	 * going to print characters via rconsole.
	 */
	if (sbus_bus_map(sc->sc_bustag, sc->sc_ctl_btype,
			 sc->sc_ctl_paddr, sc->sc_ctl_psize,
			 BUS_SPACE_MAP_LINEAR, 0,
			 &sc->sc_ctl_memh) != 0) {
		printf("%s: cannot map control registers\n", self->dv_xname);
		return;
	}

	if (sbus_bus_map(sc->sc_bustag, sc->sc_cmd_btype,
			 sc->sc_cmd_paddr, sc->sc_cmd_psize,
			 BUS_SPACE_MAP_LINEAR, 0,
			 &sc->sc_cmd_memh) != 0) {
		printf("%s: cannot map command registers\n", self->dv_xname);
		return;
	}

	isconsole = fb_is_console(node);

	if (sa->sa_npromvaddrs != 0)
		fb->fb_pixels = (caddr_t)sa->sa_promvaddrs[0];
	if (isconsole && fb->fb_pixels == NULL) {
		if (sbus_bus_map(sc->sc_bustag, sc->sc_fb_btype,
				 sc->sc_fb_paddr, sc->sc_fb_psize,
				 BUS_SPACE_MAP_LINEAR, 0,
				 &sc->sc_fb_memh) != 0) {
			printf("%s: cannot map framebuffer\n", self->dv_xname);
			return;
		}
		fb->fb_pixels = (char *)sc->sc_fb_memh;
	} else {
		sc->sc_fb_memh = (bus_space_handle_t) fb->fb_pixels;
	}

	i = p9100_ctl_read_4(sc, 0x0004);
	switch ((i >> 26) & 7) {
	    case 5: fb->fb_type.fb_depth = 32; break;
	    case 7: fb->fb_type.fb_depth = 24; break;
	    case 3: fb->fb_type.fb_depth = 16; break;
	    case 2: fb->fb_type.fb_depth = 8; break;
	    default: {
		panic("pnozz: can't determine screen depth (0x%02x)", i);
	    }
	}
	fb_setsize_obp(fb, fb->fb_type.fb_depth, 800, 600, node);

	sbus_establish(&sc->sc_sd, &sc->sc_dev);

	fb->fb_type.fb_size = fb->fb_type.fb_height * fb->fb_linebytes;
	printf(": rev %d, %dx%d, depth %d",
	       (i & 7), fb->fb_type.fb_width, fb->fb_type.fb_height,
	       fb->fb_type.fb_depth);

	fb->fb_type.fb_cmsize = getpropint(node, "cmsize", 256);
	if ((1 << fb->fb_type.fb_depth) != fb->fb_type.fb_cmsize)
		printf(", %d entry colormap", fb->fb_type.fb_cmsize);

	/* Initialize the default color map. */
	bt_initcmap(&sc->sc_cmap, 256);
	p9100loadcmap(sc, 0, 256);

	/* make sure we are not blanked */
	p9100_set_video(sc, 1);

	if (shutdownhook_establish(p9100_shutdown, sc) == NULL) {
		panic("%s: could not establish shutdown hook",
		      sc->sc_dev.dv_xname);
	}

	if (isconsole) {
		printf(" (console)\n");
#ifdef RASTERCONSOLE
		for (i = 0; i < fb->fb_type.fb_size; i++) {
		     if (fb->fb_pixels[i] == 0) {
			 fb->fb_pixels[i] = 1;
		     } else if (fb->fb_pixels[i] == (char) 255) {
			 fb->fb_pixels[i] = 0;
		     }
		}
		p9100loadcmap(sc, 255, 1);
		fbrcons_init(fb);
#endif
	} else
		printf("\n");

	fb_attach(fb, isconsole);
}

static void
p9100_shutdown(arg)
	void *arg;
{
	struct p9100_softc *sc = arg;
#ifdef RASTERCONSOLE
	struct fbdevice *fb = &sc->sc_fb;
	int i;

	for (i = 0; i < fb->fb_type.fb_size; i++) {
	     if (fb->fb_pixels[i] == 1) {
		 fb->fb_pixels[i] = 0;
	     } else if (fb->fb_pixels[i] == 0) {
		 fb->fb_pixels[i] = 255;
	     }
	}
	sc->sc_cmap.cm_map[0][0] = 0xff;
	sc->sc_cmap.cm_map[0][1] = 0xff;
	sc->sc_cmap.cm_map[0][2] = 0xff;
	sc->sc_cmap.cm_map[1][0] = 0;
	sc->sc_cmap.cm_map[1][1] = 0;
	sc->sc_cmap.cm_map[1][2] = 0x80;
	p9100loadcmap(sc, 0, 2);
	sc->sc_cmap.cm_map[255][0] = 0;
	sc->sc_cmap.cm_map[255][1] = 0;
	sc->sc_cmap.cm_map[255][2] = 0;
	p9100loadcmap(sc, 255, 1);
#endif
	p9100_set_video(sc, 1);
}

int
p9100open(dev_t dev, int flags, int mode, struct proc *p)
{
	int unit = minor(dev);

	if (unit >= pnozz_cd.cd_ndevs || pnozz_cd.cd_devs[unit] == NULL)
		return (ENXIO);
	return (0);
}

int
p9100close(dev_t dev, int flags, int mode, struct proc *p)
{
	return (0);
}

int
p9100ioctl(dev_t dev, u_long cmd, caddr_t data, int flags, struct proc *p)
{
	struct p9100_softc *sc = pnozz_cd.cd_devs[minor(dev)];
	struct fbgattr *fba;
	int error;

	switch (cmd) {

	case FBIOGTYPE:
		*(struct fbtype *)data = sc->sc_fb.fb_type;
		break;

	case FBIOGATTR:
		fba = (struct fbgattr *)data;
		fba->real_type = sc->sc_fb.fb_type.fb_type;
		fba->owner = 0;		/* XXX ??? */
		fba->fbtype = sc->sc_fb.fb_type;
		fba->sattr.flags = 0;
		fba->sattr.emu_type = sc->sc_fb.fb_type.fb_type;
		fba->sattr.dev_specific[0] = -1;
		fba->emu_types[0] = sc->sc_fb.fb_type.fb_type;
		fba->emu_types[1] = -1;
		break;

	case FBIOGETCMAP:
#define p ((struct fbcmap *)data)
		return (bt_getcmap(p, &sc->sc_cmap, 256, 1));

	case FBIOPUTCMAP:
		/* copy to software map */
		error = bt_putcmap(p, &sc->sc_cmap, 256, 1);
		if (error)
			return (error);
		/* now blast them into the chip */
		/* XXX should use retrace interrupt */
		p9100loadcmap(sc, p->index, p->count);
#undef p
		break;

	case FBIOGVIDEO:
		*(int *)data = p9100_get_video(sc);
		break;

	case FBIOSVIDEO:
		p9100_set_video(sc, *(int *)data);
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

int
p9100poll(dev_t dev, int events, struct proc *p)
{
	return seltrue(dev, events, p);
}

static uint32_t
p9100_ctl_read_4(struct p9100_softc *sc, bus_size_t off)
{
	sc->sc_junk = bus_space_read_4(sc->sc_bustag, sc->sc_fb_memh, off);
	return bus_space_read_4(sc->sc_bustag, sc->sc_ctl_memh, off);
}

static void
p9100_ctl_write_4(struct p9100_softc *sc, bus_size_t off, uint32_t v)
{
	sc->sc_junk = bus_space_read_4(sc->sc_bustag, sc->sc_fb_memh, off);
	bus_space_write_4(sc->sc_bustag, sc->sc_ctl_memh, off, v);
}

#if 0
static uint8_t
p9100_ramdac_read(struct p9100_softc *sc, bus_size_t off)
{
	sc->sc_junk = p9100_ctl_read_4(sc, PWRUP_CNFG);
	return p9100_ctl_read_4(sc, off) >> 16;
}
#endif

static void
p9100_ramdac_write(struct p9100_softc *sc, bus_size_t off, uint8_t v)
{
	sc->sc_junk = p9100_ctl_read_4(sc, PWRUP_CNFG);
	p9100_ctl_write_4(sc, off, v << 16);
}

/*
 * Undo the effect of an FBIOSVIDEO that turns the video off.
 */
static void
p9100unblank(struct device *dev)
{

	p9100_set_video((struct p9100_softc *)dev, 1);
}

static void
p9100_set_video(struct p9100_softc *sc, int enable)
{
	u_int32_t v = p9100_ctl_read_4(sc, SCRN_RPNT_CTL_1);
	if (enable)
		v |= VIDEO_ENABLED;
	else
		v &= ~VIDEO_ENABLED;
	p9100_ctl_write_4(sc, SCRN_RPNT_CTL_1, v);
#if NTCTRL > 0
	/* Turn On/Off the TFT if we know how.
	 */
	tadpole_set_video(enable);
#endif
}

static int
p9100_get_video(struct p9100_softc *sc)
{
	return (p9100_ctl_read_4(sc, SCRN_RPNT_CTL_1) & VIDEO_ENABLED) != 0;
}

/*
 * Load a subset of the current (new) colormap into the IBM RAMDAC.
 */
static void
p9100loadcmap(struct p9100_softc *sc, int start, int ncolors)
{
	u_char *p;

	p9100_ramdac_write(sc, DAC_CMAP_WRIDX, start);

	for (p = sc->sc_cmap.cm_map[start], ncolors *= 3; ncolors-- > 0; p++) {
		p9100_ramdac_write(sc, DAC_CMAP_DATA, *p);
	}
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 */
paddr_t
p9100mmap(dev_t dev, off_t off, int prot)
{
	struct p9100_softc *sc = pnozz_cd.cd_devs[minor(dev)];
	bus_space_handle_t bh;

	if (off & PGOFSET)
		panic("p9100mmap");
	if (off < 0)
		return (-1);

#define CG3_MMAP_OFFSET	0x04000000
	/* Make Xsun think we are a CG3 (SUN3COLOR)
	 */
	if (off >= CG3_MMAP_OFFSET && off < CG3_MMAP_OFFSET + sc->sc_fb_psize) {
		off -= CG3_MMAP_OFFSET;
		if (bus_space_mmap(sc->sc_bustag,
				   sc->sc_fb_btype,
				   sc->sc_fb_paddr + off,
				   BUS_SPACE_MAP_LINEAR, &bh))
			return (-1);
		return ((paddr_t)bh);
	}

	if (off >= sc->sc_fb_psize + sc->sc_ctl_psize + sc->sc_cmd_psize)
		return (-1);

	if (off < sc->sc_fb_psize) {
		if (bus_space_mmap(sc->sc_bustag,
				   sc->sc_fb_btype,
				   sc->sc_fb_paddr + off,
				   BUS_SPACE_MAP_LINEAR, &bh))
			return (-1);
		return ((paddr_t)bh);
	}
	off -= sc->sc_fb_psize;
	if (off < sc->sc_ctl_psize) {
		if (bus_space_mmap(sc->sc_bustag,
				   sc->sc_ctl_btype,
				   sc->sc_ctl_paddr + off,
				   BUS_SPACE_MAP_LINEAR, &bh))
			return (-1);
		return ((paddr_t)bh);
	}
	off -= sc->sc_ctl_psize;

	if (bus_space_mmap(sc->sc_bustag,
			   sc->sc_cmd_btype,
			   sc->sc_cmd_paddr + off,
			   BUS_SPACE_MAP_LINEAR, &bh))
		return (-1);
	return ((paddr_t)bh);
}
