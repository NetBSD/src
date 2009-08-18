/*	$NetBSD: tcx.c,v 1.33 2009/08/18 20:45:42 macallan Exp $ */

/*
 *  Copyright (c) 1996,1998 The NetBSD Foundation, Inc.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to The NetBSD Foundation
 *  by Paul Kranenburg and Michael Lorenz.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * color display (TCX) driver.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * XXX should defer colormap updates to vertical retrace interrupts
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tcx.c,v 1.33 2009/08/18 20:45:42 macallan Exp $");

/*
 * define for cg8 emulation on S24 (24-bit version of tcx) for the SS5;
 * it is bypassed on the 8-bit version (onboard framebuffer for SS4)
 */
#undef TCX_CG8

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#ifdef DEBUG
#include <sys/proc.h>
#include <sys/syslog.h>
#endif

#include <sys/bus.h>
#include <machine/autoconf.h>

#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>
#include <dev/sun/btreg.h>
#include <dev/sun/btvar.h>

#include <dev/sbus/sbusvar.h>
#include <dev/sbus/tcxreg.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>

#include <dev/wscons/wsdisplay_vconsvar.h>

#include "opt_wsemul.h"

/* per-display variables */
struct tcx_softc {
	device_t	sc_dev;		/* base device */
	struct sbusdev	sc_sd;		/* sbus device */
	struct fbdevice	sc_fb;		/* frame buffer device */
	bus_space_tag_t	sc_bustag;
	struct openprom_addr sc_physadr[TCX_NREG];/* phys addr of h/w */

	bus_space_handle_t sc_bt;	/* Brooktree registers */
	bus_space_handle_t sc_thc;	/* THC registers */
	uint8_t *sc_fbaddr;		/* framebuffer */
	uint64_t *sc_rblit;		/* blitspace */
	uint64_t *sc_rstip;		/* stipple space */

	short	sc_8bit;		/* true if 8-bit hardware */
	short	sc_blanked;		/* true if blanked */
	u_char	sc_cmap_red[256];
	u_char	sc_cmap_green[256];
	u_char	sc_cmap_blue[256];
	int 	sc_mode, sc_bg;
	struct vcons_data vd;
};

static struct vcons_screen tcx_console_screen;

extern const u_char rasops_cmap[768];

struct wsscreen_descr tcx_defscreendesc = {
	"default",
	0, 0,
	NULL,
	8, 16,
	WSSCREEN_WSCOLORS,
};

const struct wsscreen_descr *_tcx_scrlist[] = {
	&tcx_defscreendesc,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list tcx_screenlist = {
	sizeof(_tcx_scrlist) / sizeof(struct wsscreen_descr *),
	_tcx_scrlist
};

/*
 * The S24 provides the framebuffer RAM mapped in three ways:
 * 26 bits per pixel, in 32-bit words; the low-order 24 bits are
 * blue, green, and red values, and the other two bits select the
 * display modes, per pixel);
 * 24 bits per pixel, in 32-bit words; the high-order byte reads as
 * zero, and is ignored on writes (so the mode bits cannot be altered);
 * 8 bits per pixel, unpadded; writes to this space do not modify the
 * other 18 bits.
 */
#define TCX_CTL_8_MAPPED	0x00000000	/* 8 bits, uses color map */
#define TCX_CTL_24_MAPPED	0x01000000	/* 24 bits, uses color map */
#define TCX_CTL_24_LEVEL	0x03000000	/* 24 bits, ignores color map */
#define TCX_CTL_PIXELMASK	0x00FFFFFF	/* mask for index/level */

/* autoconfiguration driver */
static void	tcxattach(device_t, device_t, void *);
static int	tcxmatch(device_t, cfdata_t, void *);
static void	tcx_unblank(device_t);

CFATTACH_DECL_NEW(tcx, sizeof(struct tcx_softc),
    tcxmatch, tcxattach, NULL, NULL);

extern struct cfdriver tcx_cd;

dev_type_open(tcxopen);
dev_type_close(tcxclose);
dev_type_ioctl(tcxioctl);
dev_type_mmap(tcxmmap);

const struct cdevsw tcx_cdevsw = {
	tcxopen, tcxclose, noread, nowrite, tcxioctl,
	nostop, notty, nopoll, tcxmmap, nokqfilter,
};

/* frame buffer generic driver */
static struct fbdriver tcx_fbdriver = {
	tcx_unblank, tcxopen, tcxclose, tcxioctl, nopoll, tcxmmap,
	nokqfilter
};

static void tcx_reset(struct tcx_softc *);
static void tcx_loadcmap(struct tcx_softc *, int, int);

static int	tcx_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	tcx_mmap(void *, void *, off_t, int);

static void	tcx_init_screen(void *, struct vcons_screen *, int, long *);
static void	tcx_clearscreen(struct tcx_softc *);
static void	tcx_copyrows(void *, int, int, int);
static void	tcx_eraserows(void *, int, int, long);
static void	tcx_putchar(void *, int, int, u_int, long);

struct wsdisplay_accessops tcx_accessops = {
	tcx_ioctl,
	tcx_mmap,
	NULL,	/* vcons_alloc_screen */
	NULL,	/* vcons_free_screen */
	NULL,	/* vcons_show_screen */
	NULL,	/* load_font */
	NULL,	/* polls */
	NULL,	/* scroll */
};

#define OBPNAME	"SUNW,tcx"

#ifdef TCX_CG8
/*
 * For CG8 emulation, we map the 32-bit-deep framebuffer at an offset of
 * 256K; the cg8 space begins with a mono overlay plane and an overlay
 * enable plane (128K bytes each, 1 bit per pixel), immediately followed
 * by the color planes, 32 bits per pixel.  We also map just the 32-bit
 * framebuffer at 0x04000000 (TCX_USER_RAM_COMPAT), for compatibility
 * with the cg8 driver.
 */
#define	TCX_CG8OVERLAY	(256 * 1024)
#define	TCX_SIZE_DFB32	(1152 * 900 * 4) /* max size of the framebuffer */
#endif

/*
 * Match a tcx.
 */
int
tcxmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(sa->sa_name, OBPNAME) == 0);
}

/*
 * Attach a display.
 */
void
tcxattach(device_t parent, device_t self, void *args)
{
	struct tcx_softc *sc = device_private(self);
	struct sbus_attach_args *sa = args;
	struct wsemuldisplaydev_attach_args aa;
	struct rasops_info *ri;
	unsigned long defattr;
	int node, ramsize;
	struct fbdevice *fb = &sc->sc_fb;
	bus_space_handle_t bh;
	int isconsole, i, j;
	uint32_t confreg;

	sc->sc_dev = self;
	sc->sc_bustag = sa->sa_bustag;
	node = sa->sa_node;

	fb->fb_driver = &tcx_fbdriver;
	fb->fb_device = sc->sc_dev;
	/* Mask out invalid flags from the user. */
	fb->fb_flags = device_cfdata(sc->sc_dev)->cf_flags & FB_USERMASK;
	/*
	 * The onboard framebuffer on the SS4 supports only 8-bit mode;
	 * it can be distinguished from the S24 card for the SS5 by the
	 * presence of the "tcx-8-bit" attribute on the SS4 version.
	 */
	sc->sc_8bit = node_has_property(node, "tcx-8-bit");
	fb->fb_type.fb_depth = 8;
	fb_setsize_obp(fb, fb->fb_type.fb_depth, 1152, 900, node);

	if (sc->sc_8bit) {
		printf(" {8bit only TCX)");
		ramsize = 1024 * 1024;
		/* XXX - fix THC and TEC offsets */
		sc->sc_physadr[TCX_REG_TEC].oa_base += 0x1000;
		sc->sc_physadr[TCX_REG_THC].oa_base += 0x1000;
	} else {
		printf(" (S24)\n");
		ramsize = 4 * 1024 * 1024;
	}

	fb->fb_type.fb_cmsize = 256;
	fb->fb_type.fb_size = ramsize;
	printf(": %s, %d x %d", OBPNAME,
		fb->fb_type.fb_width,
		fb->fb_type.fb_height);

	fb->fb_type.fb_type = FBTYPE_SUNTCX;


	if (sa->sa_nreg != TCX_NREG) {
		printf("%s: only %d register sets\n",
			device_xname(self), sa->sa_nreg);
		return;
	}
	memcpy(sc->sc_physadr, sa->sa_reg,
	      sa->sa_nreg * sizeof(struct openprom_addr));

	/* Map the register banks we care about */
	if (sbus_bus_map(sa->sa_bustag,
			 sc->sc_physadr[TCX_REG_THC].oa_space,
			 sc->sc_physadr[TCX_REG_THC].oa_base,
			 0x1000,
			 BUS_SPACE_MAP_LINEAR, &sc->sc_thc) != 0) {
		printf("tcxattach: cannot map thc registers\n");
		return;
	}

	if (sbus_bus_map(sa->sa_bustag,
			 sc->sc_physadr[TCX_REG_CMAP].oa_space,
			 sc->sc_physadr[TCX_REG_CMAP].oa_base,
			 0x1000,
			 BUS_SPACE_MAP_LINEAR, &sc->sc_bt) != 0) {
		printf("tcxattach: cannot map bt registers\n");
		return;
	}

	/* map the 8bit dumb FB for the console */
	if (sbus_bus_map(sa->sa_bustag,
		 sc->sc_physadr[TCX_REG_DFB8].oa_space,
		 sc->sc_physadr[TCX_REG_DFB8].oa_base,
			 1024 * 1024,
			 BUS_SPACE_MAP_LINEAR,
			 &bh) != 0) {
		printf("tcxattach: cannot map framebuffer\n");
		return;
	}
	sc->sc_fbaddr = bus_space_vaddr(sa->sa_bustag, bh);

	/* RBLIT space */
	if (sbus_bus_map(sa->sa_bustag,
		 sc->sc_physadr[TCX_REG_RBLIT].oa_space,
		 sc->sc_physadr[TCX_REG_RBLIT].oa_base,
			 8 * 1024 * 1024,
			 BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_LARGE,
			 &bh) != 0) {
		printf("tcxattach: cannot map RBLIT space\n");
		return;
	}
	sc->sc_rblit = bus_space_vaddr(sa->sa_bustag, bh);

	/* RSTIP space */
	if (sbus_bus_map(sa->sa_bustag,
		 sc->sc_physadr[TCX_REG_RSTIP].oa_space,
		 sc->sc_physadr[TCX_REG_RSTIP].oa_base,
			 8 * 1024 * 1024,
			 BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_LARGE,
			 &bh) != 0) {
		printf("tcxattach: cannot map RSTIP space\n");
		return;
	}
	sc->sc_rstip = bus_space_vaddr(sa->sa_bustag, bh);

	isconsole = fb_is_console(node);

	confreg = bus_space_read_4(sa->sa_bustag, sc->sc_thc, THC_CONFIG);
	printf(", id %d, rev %d, sense %d",
		(confreg & THC_CFG_FBID) >> THC_CFG_FBID_SHIFT,
		(confreg & THC_CFG_REV) >> THC_CFG_REV_SHIFT,
		(confreg & THC_CFG_SENSE) >> THC_CFG_SENSE_SHIFT
	);

	/* reset cursor & frame buffer controls */
	tcx_reset(sc);

	/* Initialize the default color map. */
	j = 0;
	for (i = 0; i < 256; i++) {

		sc->sc_cmap_red[i] = rasops_cmap[j];
		sc->sc_cmap_green[i] = rasops_cmap[j + 1];
		sc->sc_cmap_blue[i] = rasops_cmap[j + 2];
		j += 3;
	}
	tcx_loadcmap(sc, 0, 256);

	/* enable video */
	confreg = bus_space_read_4(sa->sa_bustag, sc->sc_thc, THC_MISC);
	confreg |= THC_MISC_VIDEN;
	bus_space_write_4(sa->sa_bustag, sc->sc_thc, THC_MISC, confreg);

	if (isconsole) {
		printf(" (console)\n");
	} else
		printf("\n");

	sbus_establish(&sc->sc_sd, sc->sc_dev);
	fb_attach(&sc->sc_fb, isconsole);

	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	wsfont_init();

	vcons_init(&sc->vd, sc, &tcx_defscreendesc, &tcx_accessops);
	sc->vd.init_screen = tcx_init_screen;

	vcons_init_screen(&sc->vd, &tcx_console_screen, 1, &defattr);
	tcx_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

	sc->sc_bg = (defattr >> 16) & 0xff;
	tcx_clearscreen(sc);

	ri = &tcx_console_screen.scr_ri;

	tcx_defscreendesc.nrows = ri->ri_rows;
	tcx_defscreendesc.ncols = ri->ri_cols;
	tcx_defscreendesc.textops = &ri->ri_ops;
	tcx_defscreendesc.capabilities = ri->ri_caps;

	if(isconsole) {
		wsdisplay_cnattach(&tcx_defscreendesc, ri, 0, 0, defattr);
	}

	aa.console = isconsole;
	aa.scrdata = &tcx_screenlist;
	aa.accessops = &tcx_accessops;
	aa.accesscookie = &sc->vd;

	config_found(self, &aa, wsemuldisplaydevprint);
	/*
	 * we need to do this again - something overwrites a handful
	 * palette registers and we end up with white in reg. 0
	 */
	tcx_loadcmap(sc, 0, 256);
}

#ifdef TCX_CG8
/*
 * keep track of the number of opens, so we can switch to 24-bit mode
 * when the device is first opened, and return to 8-bit mode on the
 * last close.  (stolen from cgfourteen driver...)  There can only be
 * one TCX per system, so we only need one flag.
 */
static int tcx_opens = 0;
#endif

int
tcxopen(dev_t dev, int flags, int mode, struct lwp *l)
{
#ifdef TCX_CG8
	int unit = minor(dev);
	struct tcx_softc *sc;
	int i, s, oldopens;
	volatile ulong *cptr;
	struct fbdevice *fb;

	sc = device_lookup_private(&tcx_cd, unit);
	if (!sc)
		return (ENXIO);
	if (!sc->sc_8bit) {
		s = splhigh();
		oldopens = tcx_opens++;
		splx(s);
		if (oldopens == 0) {
			/*
			 * rewrite the control planes to select 24-bit mode
			 * and clear the screen
			 */
			fb = &sc->sc_fb;
			i = fb->fb_type.fb_height * fb->fb_type.fb_width;
			cptr = sc->sc_cplane;
			while (--i >= 0)
				*cptr++ = TCX_CTL_24_LEVEL;
		}
	}
#endif
	return (0);
}

int
tcxclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct tcx_softc *sc = device_lookup_private(&tcx_cd, minor(dev));
#ifdef TCX_CG8
	int i, s, opens;
	volatile ulong *cptr;
	struct fbdevice *fb;
#endif

	tcx_reset(sc);
#ifdef TCX_CG8
	if (!sc->sc_8bit) {
		s = splhigh();
		opens = --tcx_opens;
		if (tcx_opens <= 0)
			opens = tcx_opens = 0;
		splx(s);
		if (opens == 0) {
			/*
			 * rewrite the control planes to select 8-bit mode,
			 * preserving the contents of the screen.
			 * (or we could just bzero the whole thing...)
			 */
			fb = &sc->sc_fb;
			i = fb->fb_type.fb_height * fb->fb_type.fb_width;
			cptr = sc->sc_cplane;
			while (--i >= 0)
				*cptr++ &= TCX_CTL_PIXELMASK;
		}
	}
#endif
	return (0);
}

int
tcxioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct tcx_softc *sc = device_lookup_private(&tcx_cd, minor(dev));
	//int error;

	switch (cmd) {

	case FBIOGTYPE:
		*(struct fbtype *)data = sc->sc_fb.fb_type;
		break;

	case FBIOGATTR:
#define fba ((struct fbgattr *)data)
		fba->real_type = sc->sc_fb.fb_type.fb_type;
		fba->owner = 0;		/* XXX ??? */
		fba->fbtype = sc->sc_fb.fb_type;
		fba->sattr.flags = 0;
		fba->sattr.emu_type = sc->sc_fb.fb_type.fb_type;
		fba->sattr.dev_specific[0] = -1;
		fba->emu_types[0] = sc->sc_fb.fb_type.fb_type;
		fba->emu_types[1] = FBTYPE_SUN3COLOR;
		fba->emu_types[2] = -1;
#undef fba
		break;
#if 0
	case FBIOGETCMAP:
#define	p ((struct fbcmap *)data)
		return (bt_getcmap(p, &sc->sc_cmap, 256, 1));

	case FBIOPUTCMAP:
		/* copy to software map */
#ifdef TCX_CG8
		if (!sc->sc_8bit) {
			/*
			 * cg8 has extra bits in high-order byte of the index
			 * that bt_putcmap doesn't recognize
			 */
			p->index &= 0xffffff;
		}
#endif
		error = bt_putcmap(p, &sc->sc_cmap, 256, 1);
		if (error)
			return (error);
		/* now blast them into the chip */
		/* XXX should use retrace interrupt */
		tcx_loadcmap(sc, p->index, p->count);
#undef p
		break;
#endif
	case FBIOGVIDEO:
		*(int *)data = sc->sc_blanked;
		break;

	case FBIOSVIDEO:
		if (*(int *)data)
			tcx_unblank(sc->sc_dev);
		else if (!sc->sc_blanked) {
			sc->sc_blanked = 1;
			//sc->sc_thc->thc_hcmisc &= ~THC_MISC_VIDEN;
			/* Put monitor in `power-saving mode' */
			//sc->sc_thc->thc_hcmisc |= THC_MISC_VSYNC_DISABLE;
			//sc->sc_thc->thc_hcmisc |= THC_MISC_HSYNC_DISABLE;
		}
		break;

	default:
#ifdef DEBUG
		log(LOG_NOTICE, "tcxioctl(0x%lx) (%s[%d])\n", cmd,
		    l->l_proc->p_comm, l->l_proc->p_pid);
#endif
		return (ENOTTY);
	}
	return (0);
}

/*
 * Clean up hardware state (e.g., after bootup or after X crashes).
 */
static void
tcx_reset(struct tcx_softc *sc)
{

	/* Enable cursor in Brooktree DAC. */
	/* TODO: bus_spacify */
//	bt->bt_addr = 0x06 << 24;
//	bt->bt_ctrl |= 0x03 << 24;
}

/*
 * Load a subset of the current (new) colormap into the color DAC.
 */
static void
tcx_loadcmap(struct tcx_softc *sc, int start, int ncolors)
{
	int i;

	for (i = 0; i < ncolors; i++) {
		bus_space_write_4(sc->sc_bustag, sc->sc_bt, DAC_ADDRESS,
		    (start + i) << 24);
		bus_space_write_4(sc->sc_bustag, sc->sc_bt, DAC_FB_LUT,
		    sc->sc_cmap_red[i + start] << 24);
		bus_space_write_4(sc->sc_bustag, sc->sc_bt, DAC_FB_LUT,
		    sc->sc_cmap_green[i + start] << 24);
		bus_space_write_4(sc->sc_bustag, sc->sc_bt, DAC_FB_LUT,
		    sc->sc_cmap_blue[i + start] << 24);
	}
	bus_space_write_4(sc->sc_bustag, sc->sc_bt, DAC_ADDRESS, 0);
}

static void
tcx_unblank(device_t dev)
{
	struct tcx_softc *sc = device_private(dev);

	if (sc->sc_blanked) {
	
		sc->sc_blanked = 0;
		//sc->sc_thc->thc_hcmisc &= ~THC_MISC_VSYNC_DISABLE;
		//sc->sc_thc->thc_hcmisc &= ~THC_MISC_HSYNC_DISABLE;
		//sc->sc_thc->thc_hcmisc |= THC_MISC_VIDEN;
	}
}

/*
 * Base addresses at which users can mmap() the various pieces of a tcx.
 */
#define	TCX_USER_RAM	0x00000000
#define	TCX_USER_RAM24	0x01000000
#define	TCX_USER_RAM_COMPAT	0x04000000	/* cg3 emulation */
#define	TCX_USER_STIP	0x10000000
#define	TCX_USER_BLIT	0x20000000
#define	TCX_USER_RDFB32	0x28000000
#define	TCX_USER_RSTIP	0x30000000
#define	TCX_USER_RBLIT	0x38000000
#define	TCX_USER_TEC	0x70001000
#define	TCX_USER_BTREGS	0x70002000
#define	TCX_USER_THC	0x70004000
#define	TCX_USER_DHC	0x70008000
#define	TCX_USER_ALT	0x7000a000
#define	TCX_USER_UART	0x7000c000
#define	TCX_USER_VRT	0x7000e000
#define	TCX_USER_ROM	0x70010000

struct mmo {
	u_int	mo_uaddr;	/* user (virtual) address */
	u_int	mo_size;	/* size, or 0 for video ram size */
	u_int	mo_bank;	/* register bank number */
};

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 *
 * XXX	needs testing against `demanding' applications (e.g., aviator)
 */
paddr_t
tcxmmap(dev_t dev, off_t off, int prot)
{
	struct tcx_softc *sc = device_lookup_private(&tcx_cd, minor(dev));
	struct openprom_addr *rr = sc->sc_physadr;
	struct mmo *mo, *mo_end;
	u_int u, sz;
	static struct mmo mmo[] = {
		{ TCX_USER_RAM, 0, TCX_REG_DFB8 },
		{ TCX_USER_RAM24, 0, TCX_REG_DFB24 },
		{ TCX_USER_RAM_COMPAT, 0, TCX_REG_DFB8 },

		{ TCX_USER_STIP, 1, TCX_REG_STIP },
		{ TCX_USER_BLIT, 1, TCX_REG_BLIT },
		{ TCX_USER_RDFB32, 0, TCX_REG_RDFB32 },
		{ TCX_USER_RSTIP, 1, TCX_REG_RSTIP },
		{ TCX_USER_RBLIT, 1, TCX_REG_RBLIT },
		{ TCX_USER_TEC, 1, TCX_REG_TEC },
		{ TCX_USER_BTREGS, 8192 /* XXX */, TCX_REG_CMAP },
		{ TCX_USER_THC, 0x1000, TCX_REG_THC },
		{ TCX_USER_DHC, 1, TCX_REG_DHC },
		{ TCX_USER_ALT, 1, TCX_REG_ALT },
		{ TCX_USER_ROM, 65536, TCX_REG_ROM },
	};
#define NMMO (sizeof mmo / sizeof *mmo)
#ifdef TCX_CG8
	/*
	 * alternate mapping for CG8 emulation:
	 * map part of the 8-bit-deep framebuffer into the cg8 overlay
	 * space, just so there's something there, and map the 32-bit-deep
	 * framebuffer where cg8 users expect to find it.
	 */
	static struct mmo mmo_cg8[] = {
		{ TCX_USER_RAM, TCX_CG8OVERLAY, TCX_REG_DFB8 },
		{ TCX_CG8OVERLAY, TCX_SIZE_DFB32, TCX_REG_DFB24 },
		{ TCX_USER_RAM_COMPAT, TCX_SIZE_DFB32, TCX_REG_DFB24 }
	};
#define NMMO_CG8 (sizeof mmo_cg8 / sizeof *mmo_cg8)
#endif

	if (off & PGOFSET)
		panic("tcxmmap");

	/*
	 * Entries with size 0 map video RAM (i.e., the size in fb data).
	 * Entries that map 32-bit deep regions are adjusted for their
	 * depth (fb_size gives the size of the 8-bit-deep region).
	 *
	 * Since we work in pages, the fact that the map offset table's
	 * sizes are sometimes bizarre (e.g., 1) is effectively ignored:
	 * one byte is as good as one page.
	 */
#ifdef TCX_CG8
	if (sc->sc_8bit) {
		mo = mmo;
		mo_end = &mmo[NMMO];
	} else {
		mo = mmo_cg8;
		mo_end = &mmo_cg8[NMMO_CG8];
	}
#else
	mo = mmo;
	mo_end = &mmo[NMMO];
#endif
	for (; mo < mo_end; mo++) {
		if ((u_int)off < mo->mo_uaddr)
			continue;
		u = off - mo->mo_uaddr;
		sz = mo->mo_size;
		if (sz == 0) {
			sz = sc->sc_fb.fb_type.fb_size;
			/*
			 * check for the 32-bit-deep regions and adjust
			 * accordingly
			 */
			if (mo->mo_uaddr == TCX_USER_RAM24 ||
			    mo->mo_uaddr == TCX_USER_RDFB32) {
				if (sc->sc_8bit) {
					/*
					 * not present on 8-bit hardware
					 */
					continue;
				}
				sz *= 4;
			}
		}
		if (u < sz) {
			return (bus_space_mmap(sc->sc_bustag,
				BUS_ADDR(rr[mo->mo_bank].oa_space,
					 rr[mo->mo_bank].oa_base),
				u,
				prot,
				BUS_SPACE_MAP_LINEAR));
		}
	}
	return (-1);
}

int
tcx_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct tcx_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_SUNTCX;
			return 0;

#if 0
		case FBIOGVIDEO:
		case WSDISPLAYIO_GVIDEO:
			*(int *)data = tcx_get_video(sc);
			return 0;

		case WSDISPLAYIO_SVIDEO:
		case FBIOSVIDEO:
			tcx_set_video(sc, *(int *)data);
			return 0;
#endif
		case WSDISPLAYIO_GINFO:
			wdf = (void *)data;
			wdf->height = ms->scr_ri.ri_height;
			wdf->width = ms->scr_ri.ri_width;
			wdf->depth = ms->scr_ri.ri_depth;
			wdf->cmsize = 256;
			return 0;
#if 0
		case WSDISPLAYIO_GETCMAP:
			return tcx_getcmap(sc, (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_PUTCMAP:
			return tcx_putcmap(sc, (struct wsdisplay_cmap *)data);
#endif
		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int*)data;
				if (new_mode != sc->sc_mode)
				{
					sc->sc_mode = new_mode;
					if (new_mode == WSDISPLAYIO_MODE_EMUL)
					{
#if 0
						tcxloadcmap(sc, 0, 256);
						tcx_clearscreen(sc);
#endif
						vcons_redraw_screen(ms);
					}
				}
			}
	}
	return EPASSTHROUGH;
}

static paddr_t
tcx_mmap(void *v, void *vs, off_t offset, int prot)
{
#if 0
	struct vcons_data *vd = v;
	struct tcx_softc *sc = vd->cookie;
	paddr_t pa;

	/* 'regular' framebuffer mmap()ing */
	if (offset < sc->sc_fb_psize) {
		pa = bus_space_mmap(sc->sc_bustag, sc->sc_fb_paddr + offset, 0,
		    prot, BUS_SPACE_MAP_LINEAR);
		return pa;
	}

	if ((offset >= sc->sc_fb_paddr) && (offset < (sc->sc_fb_paddr +
	    sc->sc_fb_psize))) {
		pa = bus_space_mmap(sc->sc_bustag, offset, 0, prot,
		    BUS_SPACE_MAP_LINEAR);
		return pa;
	}

	if ((offset >= sc->sc_ctl_paddr) && (offset < (sc->sc_ctl_paddr +
	    sc->sc_ctl_psize))) {
		pa = bus_space_mmap(sc->sc_bustag, offset, 0, prot,
		    BUS_SPACE_MAP_LINEAR);
		return pa;
	}
#endif
	return -1;
}

static void
tcx_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct tcx_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = 8;
	ri->ri_width = sc->sc_fb.fb_type.fb_width;
	ri->ri_height = sc->sc_fb.fb_type.fb_height;
	ri->ri_stride = sc->sc_fb.fb_linebytes;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;

	ri->ri_bits = sc->sc_fbaddr;

	rasops_init(ri, ri->ri_height/8, ri->ri_width/8);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
		    ri->ri_width / ri->ri_font->fontwidth);

	/* enable acceleration */
	ri->ri_ops.copyrows  = tcx_copyrows;
	ri->ri_ops.eraserows = tcx_eraserows;
	ri->ri_ops.putchar   = tcx_putchar;
#if 0
	ri->ri_ops.cursor    = tcx_cursor;
	ri->ri_ops.copycols  = tcx_copycols;
	ri->ri_ops.erasecols = tcx_erasecols;
#endif
}

static void
tcx_clearscreen(struct tcx_softc *sc)
{
	uint64_t bg = ((uint64_t)sc->sc_bg << 32) | 0xffffffffLL;
	int i;

	for (i = 0; i < 1024 * 1024; i += 32)
		sc->sc_rstip[i] = bg;
}

static void
tcx_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct tcx_softc *sc = scr->scr_cookie;
	int i, last, first, len, dest, leftover;

	i = ri->ri_width * ri->ri_font->fontheight * nrows;
	len = i & 0xffffe0;
	leftover = i & 0x1f;
	if (srcrow < dstrow) {
		/* we must go bottom to top */
		first = ri->ri_width * 
		    (ri->ri_font->fontheight * srcrow + ri->ri_yorigin);
		last = first + len;
		dest = ri->ri_width *
		    (ri->ri_font->fontheight * dstrow + ri->ri_yorigin) + len;
		if (leftover > 0) {
			sc->sc_rblit[dest + 32] = 
			    (uint64_t)((leftover - 1) << 24) | 
			    (uint64_t)(i + 32);
		}
		for (i = last; i >= first; i -= 32) {
			sc->sc_rblit[dest] = 0x300000001f000000LL | (uint64_t)i;
			dest -= 32;
		}
	} else {
		/* top to bottom */
		first = ri->ri_width * 
		    (ri->ri_font->fontheight * srcrow + ri->ri_yorigin);
		dest = ri->ri_width * 
		    (ri->ri_font->fontheight * dstrow + ri->ri_yorigin);
		last = first + len;
		for (i = first; i <= last; i+= 32) {
			sc->sc_rblit[dest] = 0x300000001f000000LL | (uint64_t)i;
			dest += 32;
		}
		if (leftover > 0) {
			sc->sc_rblit[dest] = 
			    (uint64_t)((leftover - 1) << 24) | (uint64_t)i;
		}
	}
}

static void
tcx_eraserows(void *cookie, int start, int nrows, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct tcx_softc *sc = scr->scr_cookie;
	uint64_t temp;
	int i, last, first, len, leftover;

	i = ri->ri_width * ri->ri_font->fontheight * nrows;
	len = i & 0xffffe0;
	leftover = i & 0x1f;
	first = ri->ri_width * 
	    (ri->ri_font->fontheight * start + ri->ri_yorigin);
	last = first + len;
	temp = 0x30000000ffffffffLL | 
	    ((uint64_t)ri->ri_devcmap[(attr >> 16) & 0xff] << 32);

	for (i = first; i <= last; i+= 32)
		sc->sc_rblit[i] = temp;

	if (leftover > 0) {
		temp &= 0xffffffffffffffffLL << (32 - leftover);
		sc->sc_rblit[i] = temp;
	}
}
/*
 * The stipple engine is 100% retarded. All drawing operations have to start 
 * at 32 pixel boundaries so we'll have to deal with characters being split.
 */

static void
tcx_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct tcx_softc *sc = scr->scr_cookie;
	uint64_t bg, fg, temp, mask;
	int addr, i, uc, shift;
	uint32_t fmask;
	uint8_t *cdata;
	uint16_t *wdata;

	addr = ri->ri_xorigin +
	    col * ri->ri_font->fontwidth +
	    (ri->ri_yorigin + row * ri->ri_font->fontheight) * ri->ri_width;

	/* check if the character is crossing a 32 pixel boundary */
	if ((addr & 0xffffe0) ==
	    ((addr + ri->ri_font->fontwidth - 1) & 0xffffe0)) {
		/* phew, not split */
		shift = addr & 0x1f;
		addr &= 0xffffe0;
		fmask = 0xffffffff >> (32 - ri->ri_font->fontwidth);
		fmask = fmask << (32 - ri->ri_font->fontwidth - shift);
		mask = fmask;
		bg = 0x3000000000000000LL |
		    ((uint64_t)ri->ri_devcmap[(attr >> 16) & 0xff] &
		      0xff) << 32;
		bg |= mask;
		temp = 0x3000000000000000LL |
		    ((uint64_t)ri->ri_devcmap[(attr >> 24) & 0xff] & 0xff) << 
		    	32;
		uc = c - ri->ri_font->firstchar;
		cdata = (uint8_t *)ri->ri_font->data + uc * ri->ri_fontscale;

		if (ri->ri_font->fontwidth < 9) {
			/* byte by byte */
			for (i = 0; i < ri->ri_font->fontheight; i++) {
				sc->sc_rstip[addr] = bg;
				if (*cdata != 0) {
					if (shift > 24) {
						fg = (uint64_t)*cdata >>
					  	  (shift - 24);
					} else {
						fg = (uint64_t)*cdata <<
					  	  (24 - shift);
					}
					sc->sc_rstip[addr] = fg | temp;
				}
				cdata++;
				addr += ri->ri_width;
			}
		} else if (ri->ri_font->fontwidth < 17) {
			/* short by short */
			wdata = (uint16_t *)cdata;
			for (i = 0; i < ri->ri_font->fontheight; i++) {
				sc->sc_rstip[addr] = bg;
				if (*wdata != 0) {
					if (shift > 16) {
						fg = temp | (uint64_t)*wdata >> 
					  	  (shift - 16);
					} else {
						fg = temp | (uint64_t)*wdata << 
					  	  (16 - shift);
					}
					sc->sc_rstip[addr] = fg;
				}
				wdata++;
				addr += ri->ri_width;
			}
		}
	} else {
		/* and now the split case ( man this hardware is dumb ) */
		uint64_t bgr, maskr, fgr;
		uint32_t bork;

		shift = addr & 0x1f;
		addr &= 0xffffe0;
		mask = 0xffffffff >> shift;
		maskr = (uint64_t)(0xffffffffUL << 
		    (32 - (ri->ri_font->fontwidth + shift - 32)));
		bg = 0x3000000000000000LL |
		    ((uint64_t)ri->ri_devcmap[(attr >> 16) & 0xff] &
		      0xff) << 32;
		bgr = bg | maskr;
		bg |= mask;
		temp = 0x3000000000000000LL |
		    ((uint64_t)ri->ri_devcmap[(attr >> 24) & 0xff] & 0xff) << 
		      32;

		uc = c - ri->ri_font->firstchar;
		cdata = (uint8_t *)ri->ri_font->data + uc * ri->ri_fontscale;

		if (ri->ri_font->fontwidth < 9) {
			/* byte by byte */
			for (i = 0; i < ri->ri_font->fontheight; i++) {
				sc->sc_rstip[addr] = bg;
				sc->sc_rstip[addr + 32] = bgr;
				bork = *cdata;
				if (bork != 0) {
					fg = (uint64_t)bork >> (shift - 24);
					sc->sc_rstip[addr] = fg | temp;
					fgr = (uint64_t)(bork << (52 - shift));
					sc->sc_rstip[addr] = fgr | temp;
				}
				cdata++;
				addr += ri->ri_width;
			}
		} else if (ri->ri_font->fontwidth < 17) {
			/* short by short */
			wdata = (uint16_t *)cdata;
			for (i = 0; i < ri->ri_font->fontheight; i++) {
				sc->sc_rstip[addr] = bg;
				sc->sc_rstip[addr + 32] = bgr;
				bork = *wdata;
				if (bork != 0) {
					fg = (uint64_t)bork >> (shift - 16);
					sc->sc_rstip[addr] = fg | temp;
					fgr = (uint64_t)(bork << (48 - shift));
					sc->sc_rstip[addr + 32] = fgr | temp;
				}
				wdata++;
				addr += ri->ri_width;
			}
		}
		
	}
}

