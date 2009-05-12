/*	$NetBSD: tcx.c,v 1.31 2009/05/12 14:43:59 cegger Exp $ */

/*
 *  Copyright (c) 1996,1998 The NetBSD Foundation, Inc.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to The NetBSD Foundation
 *  by Paul Kranenburg.
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
__KERNEL_RCSID(0, "$NetBSD: tcx.c,v 1.31 2009/05/12 14:43:59 cegger Exp $");

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

/* per-display variables */
struct tcx_softc {
	struct device	sc_dev;		/* base device */
	struct sbusdev	sc_sd;		/* sbus device */
	struct fbdevice	sc_fb;		/* frame buffer device */
	bus_space_tag_t	sc_bustag;
	struct openprom_addr sc_physadr[TCX_NREG];/* phys addr of h/w */

	volatile struct bt_regs *sc_bt;	/* Brooktree registers */
	volatile struct tcx_thc *sc_thc;/* THC registers */
#ifdef TCX_CG8
	volatile ulong *sc_cplane;	/* framebuffer with control planes */
#endif
	short	sc_8bit;		/* true if 8-bit hardware */
	short	sc_blanked;		/* true if blanked */
	union	bt_cmap sc_cmap;	/* Brooktree color map */
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

CFATTACH_DECL(tcx, sizeof(struct tcx_softc),
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
	int node, ramsize;
	volatile struct bt_regs *bt;
	struct fbdevice *fb = &sc->sc_fb;
	bus_space_handle_t bh;
	int isconsole;

	sc->sc_bustag = sa->sa_bustag;
	node = sa->sa_node;

	fb->fb_driver = &tcx_fbdriver;
	fb->fb_device = &sc->sc_dev;
	/* Mask out invalid flags from the user. */
	fb->fb_flags = device_cfdata(&sc->sc_dev)->cf_flags & FB_USERMASK;
	/*
	 * The onboard framebuffer on the SS4 supports only 8-bit mode;
	 * it can be distinguished from the S24 card for the SS5 by the
	 * presence of the "tcx-8-bit" attribute on the SS4 version.
	 */
	sc->sc_8bit = node_has_property(node, "tcx-8-bit");
#ifdef TCX_CG8
	if (sc->sc_8bit) {
#endif
		/*
		 * cg8 emulation is either not compiled in or not supported
		 * on this hardware.  Report values for the 8-bit framebuffer
		 * so cg3 emulation works.  (If this hardware supports
		 * 24-bit mode, the 24-bit framebuffer will also be available)
		 */
		fb->fb_type.fb_depth = 8;
		fb_setsize_obp(fb, fb->fb_type.fb_depth, 1152, 900, node);

		ramsize = fb->fb_type.fb_height * fb->fb_linebytes;
#ifdef TCX_CG8
	} else {
		/*
		 * for cg8 emulation, unconditionally report the depth as
		 * 32 bits, but use the height and width reported by the
		 * boot prom.  cg8 users want to see the full size of
		 * overlay planes plus color planes included in the
		 * reported framebuffer size.
		 */
		fb->fb_type.fb_depth = 32;
		fb_setsize_obp(fb, fb->fb_type.fb_depth, 1152, 900, node);
		fb->fb_linebytes =
			(fb->fb_type.fb_width * fb->fb_type.fb_depth) / 8;
		ramsize = TCX_CG8OVERLAY +
			(fb->fb_type.fb_height * fb->fb_linebytes);
	}
#endif
	fb->fb_type.fb_cmsize = 256;
	fb->fb_type.fb_size = ramsize;
	printf(": %s, %d x %d", OBPNAME,
		fb->fb_type.fb_width,
		fb->fb_type.fb_height);
#ifdef TCX_CG8
	/*
	 * if cg8 emulation is enabled, say so; but if hardware can't
	 * emulate cg8, explain that instead
	 */
	printf( (sc->sc_8bit)?
		" (8-bit only)" :
		" (emulating cg8)");
#endif

	/*
	 * XXX - should be set to FBTYPE_TCX.
	 * XXX For CG3 emulation to work in current (96/6) X11 servers,
	 * XXX `fbtype' must point to an "unregocnised" entry.
	 */
#ifdef TCX_CG8
	if (sc->sc_8bit) {
		fb->fb_type.fb_type = FBTYPE_RESERVED3;
	} else {
		fb->fb_type.fb_type = FBTYPE_MEMCOLOR;
	}
#else
	fb->fb_type.fb_type = FBTYPE_RESERVED3;
#endif


	if (sa->sa_nreg != TCX_NREG) {
		printf("%s: only %d register sets\n",
			device_xname(self), sa->sa_nreg);
		return;
	}
	memcpy(sc->sc_physadr, sa->sa_reg,
	      sa->sa_nreg * sizeof(struct openprom_addr));

	/* XXX - fix THC and TEC offsets */
	sc->sc_physadr[TCX_REG_TEC].oa_base += 0x1000;
	sc->sc_physadr[TCX_REG_THC].oa_base += 0x1000;

	/* Map the register banks we care about */
	if (sbus_bus_map(sa->sa_bustag,
			 sc->sc_physadr[TCX_REG_THC].oa_space,
			 sc->sc_physadr[TCX_REG_THC].oa_base,
			 sizeof (struct tcx_thc),
			 BUS_SPACE_MAP_LINEAR, &bh) != 0) {
		printf("tcxattach: cannot map thc registers\n");
		return;
	}
	sc->sc_thc = (volatile struct tcx_thc *)
		bus_space_vaddr(sa->sa_bustag, bh);

	if (sbus_bus_map(sa->sa_bustag,
			 sc->sc_physadr[TCX_REG_CMAP].oa_space,
			 sc->sc_physadr[TCX_REG_CMAP].oa_base,
			 sizeof (struct bt_regs),
			 BUS_SPACE_MAP_LINEAR, &bh) != 0) {
		printf("tcxattach: cannot map bt registers\n");
		return;
	}
	sc->sc_bt = bt = (volatile struct bt_regs *)
		bus_space_vaddr(sa->sa_bustag, bh);

#ifdef TCX_CG8
	if (!sc->sc_8bit) {
		if (sbus_bus_map(sa->sa_bustag,
			 sc->sc_physadr[TCX_REG_RDFB32].oa_space,
			 sc->sc_physadr[TCX_REG_RDFB32].oa_base,
			 TCX_SIZE_DFB32,
			 BUS_SPACE_MAP_LINEAR,
			 &bh) != 0) {
			printf("tcxattach: cannot map control planes\n");
			return;
		}
		sc->sc_cplane = (volatile ulong *)bh;
	}
#endif

	isconsole = fb_is_console(node);

	printf(", id %d, rev %d, sense %d",
		(sc->sc_thc->thc_config & THC_CFG_FBID) >> THC_CFG_FBID_SHIFT,
		(sc->sc_thc->thc_config & THC_CFG_REV) >> THC_CFG_REV_SHIFT,
		(sc->sc_thc->thc_config & THC_CFG_SENSE) >> THC_CFG_SENSE_SHIFT
	);

	/* reset cursor & frame buffer controls */
	tcx_reset(sc);

	/* Initialize the default color map. */
	bt_initcmap(&sc->sc_cmap, 256);
	tcx_loadcmap(sc, 0, 256);

	/* enable video */
	sc->sc_thc->thc_hcmisc |= THC_MISC_VIDEN;

	if (isconsole) {
		printf(" (console)\n");
	} else
		printf("\n");

	sbus_establish(&sc->sc_sd, &sc->sc_dev);
	fb_attach(&sc->sc_fb, isconsole);
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
	int error;

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

	case FBIOGVIDEO:
		*(int *)data = sc->sc_blanked;
		break;

	case FBIOSVIDEO:
		if (*(int *)data)
			tcx_unblank(&sc->sc_dev);
		else if (!sc->sc_blanked) {
			sc->sc_blanked = 1;
			sc->sc_thc->thc_hcmisc &= ~THC_MISC_VIDEN;
			/* Put monitor in `power-saving mode' */
			sc->sc_thc->thc_hcmisc |= THC_MISC_VSYNC_DISABLE;
			sc->sc_thc->thc_hcmisc |= THC_MISC_HSYNC_DISABLE;
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
	volatile struct bt_regs *bt;

	/* Enable cursor in Brooktree DAC. */
	bt = sc->sc_bt;
	bt->bt_addr = 0x06 << 24;
	bt->bt_ctrl |= 0x03 << 24;
}

/*
 * Load a subset of the current (new) colormap into the color DAC.
 */
static void
tcx_loadcmap(struct tcx_softc *sc, int start, int ncolors)
{
	volatile struct bt_regs *bt;
	u_int *ip, i;
	int count;

	ip = &sc->sc_cmap.cm_chip[BT_D4M3(start)];	/* start/4 * 3 */
	count = BT_D4M3(start + ncolors - 1) - BT_D4M3(start) + 3;
	bt = sc->sc_bt;
	bt->bt_addr = BT_D4M4(start) << 24;
	while (--count >= 0) {
		i = *ip++;
		/* hardware that makes one want to pound boards with hammers */
		bt->bt_cmap = i;
		bt->bt_cmap = i << 8;
		bt->bt_cmap = i << 16;
		bt->bt_cmap = i << 24;
	}
}

static void
tcx_unblank(device_t dev)
{
	struct tcx_softc *sc = device_private(dev);

	if (sc->sc_blanked) {
		sc->sc_blanked = 0;
		sc->sc_thc->thc_hcmisc &= ~THC_MISC_VSYNC_DISABLE;
		sc->sc_thc->thc_hcmisc &= ~THC_MISC_HSYNC_DISABLE;
		sc->sc_thc->thc_hcmisc |= THC_MISC_VIDEN;
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
		{ TCX_USER_THC, sizeof(struct tcx_thc), TCX_REG_THC },
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
