/*	$NetBSD: zx.c,v 1.9 2003/08/24 17:32:05 uwe Exp $	*/

/*
 *  Copyright (c) 2002 The NetBSD Foundation, Inc.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to The NetBSD Foundation
 *  by Andrew Doran.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software
 *     must display the following acknowledgement:
 *         This product includes software developed by the NetBSD
 *         Foundation, Inc. and its contributors.
 *  4. Neither the name of The NetBSD Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 * Driver for the Sun ZX display adapter.  This would be called 'leo', but
 * NetBSD/amiga already has a driver by that name.  The XFree86 and Linux
 * drivers were used as "living documentation" when writing this; thanks
 * to the authors.
 *
 * Issues (which can be solved with wscons, happily enough):
 *
 * o There is lots of unnecessary mucking about rasops in here, primarily
 *   to appease the sparc fb code.
 *
 * o RASTERCONSOLE is required.  X needs the board set up correctly, and
 *   that's difficult to reconcile with using the PROM for output.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zx.c,v 1.9 2003/08/24 17:32:05 uwe Exp $");

#include "opt_rcons.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/syslog.h>
#include <sys/buf.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <uvm/uvm_extern.h>

#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>

#include <dev/sbus/zxreg.h>
#include <dev/sbus/zxvar.h>
#include <dev/sbus/sbusvar.h>

#include <dev/wscons/wsconsio.h>

#ifndef RASTERCONSOLE
#error Sorry, this driver needs the RASTERCONSOLE option
#endif

/* Force 32-bit writes. */
#define	SETREG(r, v)	(*((volatile u_int32_t *)&r) = (v))

#define	ZX_STD_ROP	(ZX_ROP_NEW | ZX_ATTR_WE_ENABLE | \
    ZX_ATTR_OE_ENABLE | ZX_ATTR_FORCE_WID)

void	zx_attach(struct device *, struct device *, void *);
int	zx_match(struct device *, struct cfdata *, void *);

void	zx_blank(struct device *);
int	zx_cmap_put(struct zx_softc *);
void	zx_copyrect(struct rasops_info *, int, int, int, int, int, int);
int	zx_cross_loadwid(struct zx_softc *, u_int, u_int, u_int);
int	zx_cross_wait(struct zx_softc *);
void	zx_fillrect(struct rasops_info *, int, int, int, int, long, int);
int	zx_intr(void *);
void	zx_reset(struct zx_softc *);
void	zx_unblank(struct device *);

void	zx_cursor_blank(struct zx_softc *);
void	zx_cursor_color(struct zx_softc *);
void	zx_cursor_move(struct zx_softc *);
void	zx_cursor_set(struct zx_softc *);
void	zx_cursor_unblank(struct zx_softc *);

void	zx_copycols(void *, int, int, int, int);
void	zx_copyrows(void *, int, int, int);
void	zx_cursor(void *, int, int, int);
void	zx_do_cursor(struct rasops_info *);
void	zx_erasecols(void *, int, int, int, long);
void	zx_eraserows(void *, int, int, long);
void	zx_putchar(void *, int, int, u_int, long);

struct zx_mmo {
	off_t	mo_va;
	off_t	mo_pa;
	off_t	mo_size;
} static const zx_mmo[] = {
	{ ZX_FB0_VOFF,		ZX_OFF_SS0,		0x00800000 },
	{ ZX_LC0_VOFF,		ZX_OFF_LC_SS0_USR,	0x00001000 },
	{ ZX_LD0_VOFF,		ZX_OFF_LD_SS0,		0x00001000 },
	{ ZX_LX0_CURSOR_VOFF,	ZX_OFF_LX_CURSOR,	0x00001000 },
	{ ZX_FB1_VOFF,		ZX_OFF_SS1,		0x00800000 },
	{ ZX_LC1_VOFF,		ZX_OFF_LC_SS1_USR,	0x00001000 },
	{ ZX_LD1_VOFF,		ZX_OFF_LD_SS1,		0x00001000 },
	{ ZX_LX_KRN_VOFF,	ZX_OFF_LX_CROSS,	0x00001000 },
	{ ZX_LC0_KRN_VOFF,	ZX_OFF_LC_SS0_KRN,	0x00001000 },
	{ ZX_LC1_KRN_VOFF,	ZX_OFF_LC_SS1_KRN,	0x00001000 },
	{ ZX_LD_GBL_VOFF,	ZX_OFF_LD_GBL,		0x00001000 },
};                                                                                                                

CFATTACH_DECL(zx, sizeof(struct zx_softc),
    zx_match, zx_attach, NULL, NULL);

extern struct cfdriver zx_cd;

dev_type_open(zxopen);
dev_type_close(zxclose);
dev_type_ioctl(zxioctl);
dev_type_mmap(zxmmap);

static struct fbdriver zx_fbdriver = {
	zx_unblank, zxopen, zxclose, zxioctl, nopoll, zxmmap
};

int
zx_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sbus_attach_args *sa;
	
	sa = (struct sbus_attach_args *)aux;

	return (strcmp(sa->sa_name, "SUNW,leo") == 0);
}

void
zx_attach(struct device *parent, struct device *self, void *args)
{
	struct zx_softc *sc;
	struct sbus_attach_args *sa;
	bus_space_handle_t bh;
	bus_space_tag_t bt;
	struct fbdevice *fb;
	struct rasops_info *ri;
	volatile struct zx_command *zc;
	int isconsole;

	sc = (struct zx_softc *)self;
	sa = args;
	fb = &sc->sc_fb;
	ri = &fb->fb_rinfo;
	bt = sa->sa_bustag;
	sc->sc_bt = bt;

	sc->sc_paddr = sbus_bus_addr(bt, sa->sa_slot, sa->sa_offset);

	if (sbus_bus_map(bt, sa->sa_slot, sa->sa_offset + ZX_OFF_SS0,
	    0x800000, BUS_SPACE_MAP_LINEAR, &bh) != 0) {
		printf("%s: can't map bits\n", self->dv_xname);
		return;
	}
	fb->fb_pixels = (caddr_t)bus_space_vaddr(bt, bh);
	sc->sc_pixels = (u_int32_t *)fb->fb_pixels;

	if (sbus_bus_map(bt, sa->sa_slot, sa->sa_offset + ZX_OFF_LC_SS0_USR,
	    PAGE_SIZE, BUS_SPACE_MAP_LINEAR, &bh) != 0) {
		printf("%s: can't map zc\n", self->dv_xname);
		return;
	}
	sc->sc_zc = (struct zx_command *)bus_space_vaddr(bt, bh);

	if (sbus_bus_map(bt, sa->sa_slot, sa->sa_offset + ZX_OFF_LD_SS0,
	    PAGE_SIZE, BUS_SPACE_MAP_LINEAR, &bh) != 0) {
		printf("%s: can't map ld/ss0\n", self->dv_xname);
		return;
	}
	sc->sc_zd_ss0 = (struct zx_draw *)bus_space_vaddr(bt, bh);

	if (sbus_bus_map(bt, sa->sa_slot, sa->sa_offset + ZX_OFF_LD_SS1,
	    PAGE_SIZE, BUS_SPACE_MAP_LINEAR, &bh) != 0) {
		printf("%s: can't map ld/ss1\n", self->dv_xname);
		return;
	}
	sc->sc_zd_ss1 =
	    (struct zx_draw_ss1 *)bus_space_vaddr(bt, bh);

	if (sbus_bus_map(bt, sa->sa_slot, sa->sa_offset + ZX_OFF_LX_CROSS,
	    PAGE_SIZE, BUS_SPACE_MAP_LINEAR, &bh) != 0) {
		printf("%s: can't map zx\n", self->dv_xname);
		return;
	}
	sc->sc_zx = (struct zx_cross *)bus_space_vaddr(bt, bh);

	if (sbus_bus_map(bt, sa->sa_slot, sa->sa_offset + ZX_OFF_LX_CURSOR,
	    PAGE_SIZE, BUS_SPACE_MAP_LINEAR, &bh) != 0) {
		printf("%s: can't map zcu\n", self->dv_xname);
		return;
	}
	sc->sc_zcu = (struct zx_cursor *)bus_space_vaddr(bt, bh);

	fb->fb_driver = &zx_fbdriver;
	fb->fb_device = &sc->sc_dv;
	fb->fb_flags = sc->sc_dv.dv_cfdata->cf_flags & FB_USERMASK;
	fb->fb_pfour = NULL;
	fb->fb_linebytes = 8192;

	fb_setsize_obp(fb, 32, 1280, 1024, sa->sa_node);

	fb->fb_type.fb_cmsize = 256;
	fb->fb_type.fb_depth = 32;
	fb->fb_type.fb_size = fb->fb_type.fb_height * fb->fb_linebytes;
	fb->fb_type.fb_type = FBTYPE_SUNLEO;

	printf(": %d x %d", fb->fb_type.fb_width, fb->fb_type.fb_height);
	isconsole = fb_is_console(sa->sa_node);
	if (isconsole)
		printf(" (console)");
	printf("\n");

	sbus_establish(&sc->sc_sd, &sc->sc_dv);
	if (sa->sa_nintr != 0)
		bus_intr_establish(bt, sa->sa_pri, IPL_NONE, zx_intr, sc);

	sc->sc_cmap = malloc(768, M_DEVBUF, M_NOWAIT);
	fb_attach(&sc->sc_fb, isconsole);
	zx_reset(sc);

	/*
	 * Attach to rcons.  XXX At this point, rasops_do_cursor() will be
	 * called before we get our hooks in place.  So, we mask off access
	 * to the framebuffer until it's done.
	 */
	zc = sc->sc_zc;
	SETREG(zc->zc_fontt, 1);
	SETREG(zc->zc_fontmsk, 0);

	fbrcons_init(&sc->sc_fb);

	SETREG(zc->zc_fontt, 0);
	SETREG(zc->zc_fontmsk, 0xffffffff);

	ri->ri_hw = sc;
	ri->ri_do_cursor = zx_do_cursor;
	ri->ri_ops.copycols = zx_copycols;
	ri->ri_ops.copyrows = zx_copyrows;
	ri->ri_ops.erasecols = zx_erasecols;
	ri->ri_ops.eraserows = zx_eraserows;
	ri->ri_ops.putchar = zx_putchar;

	sc->sc_fontw = ri->ri_font->fontwidth;
	sc->sc_fonth = ri->ri_font->fontheight;
}

int
zxopen(dev_t dev, int flags, int mode, struct proc *p)
{

	if (device_lookup(&zx_cd, minor(dev)) == NULL)
		return (ENXIO);
	return (0);
}

int
zxclose(dev_t dev, int flags, int mode, struct proc *p)
{
	struct zx_softc *sc;

	sc = (struct zx_softc *)device_lookup(&zx_cd, minor(dev));

	zx_reset(sc);
	zx_cursor_blank(sc);
	return (0);
}

int
zxioctl(dev_t dev, u_long cmd, caddr_t data, int flags, struct proc *p)
{
	struct zx_softc *sc;
	struct fbcmap *cm;
	struct fbcursor *cu;
	int rv, v, count, i;

	sc = zx_cd.cd_devs[minor(dev)];
	
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
		fba->emu_types[1] = -1;
		fba->emu_types[2] = -1;
#undef fba
		break;

	case FBIOGVIDEO:
		*(int *)data = ((sc->sc_flags & ZX_BLANKED) != 0);
		break;

	case FBIOSVIDEO:
		if (*(int *)data)
			zx_unblank(&sc->sc_dv);
		else
			zx_blank(&sc->sc_dv);
		break;

	case FBIOGETCMAP:
		cm = (struct fbcmap *)data;
		if (cm->index > 256 || cm->count > 256 - cm->index)
			return (EINVAL);
		rv = copyout(sc->sc_cmap + cm->index, cm->red, cm->count);
		if (rv == 0)
			rv = copyout(sc->sc_cmap + 256 + cm->index, cm->green,
			    cm->count);
		if (rv == 0)
			rv = copyout(sc->sc_cmap + 512 + cm->index, cm->blue,
			    cm->count);
		return (rv);

	case FBIOPUTCMAP:
		cm = (struct fbcmap *)data;
		if (cm->index > 256 || cm->count > 256 - cm->index)
			return (EINVAL);
		rv = copyin(cm->red, sc->sc_cmap + cm->index, cm->count);
		if (rv == 0)
			rv = copyin(cm->green, sc->sc_cmap + 256 + cm->index,
			    cm->count);
		if (rv == 0)
			rv = copyin(cm->blue, sc->sc_cmap + 512 + cm->index,
			    cm->count);
		zx_cmap_put(sc);
		return (rv);

	case FBIOGCURPOS:
		*(struct fbcurpos *)data = sc->sc_curpos;
		break;

	case FBIOSCURPOS:
		sc->sc_curpos = *(struct fbcurpos *)data;
		zx_cursor_move(sc);
		break;

	case FBIOGCURMAX:
		((struct fbcurpos *)data)->x = 32;
		((struct fbcurpos *)data)->y = 32;
		break;

	case FBIOSCURSOR:
		cu = (struct fbcursor *)data;
		v = cu->set;

		if ((v & FB_CUR_SETSHAPE) != 0) {
			if ((u_int)cu->size.x > 32 || (u_int)cu->size.y > 32)
				return (EINVAL);
			count = cu->size.y * 4;
			if (!uvm_useracc(cu->image, count, B_READ) ||
			    !uvm_useracc(cu->mask, count, B_READ))
				return (EFAULT);
		}
		if ((v & FB_CUR_SETCUR) != 0) {
			if (cu->enable)
				zx_cursor_unblank(sc);
			else
				zx_cursor_blank(sc);
		}
		if ((v & (FB_CUR_SETPOS | FB_CUR_SETHOT)) != 0) {
			if ((v & FB_CUR_SETPOS) != 0)
				sc->sc_curpos = cu->pos;
			if ((v & FB_CUR_SETHOT) != 0)
				sc->sc_curhot = cu->hot;
			zx_cursor_move(sc);
		}
		if ((v & FB_CUR_SETCMAP) != 0) {
			if (cu->cmap.index > 2 ||
			    cu->cmap.count > 2 - cu->cmap.index)
				return (EINVAL);
			for (i = 0; i < cu->cmap.count; i++) {
				if ((v = fubyte(&cu->cmap.red[i])) < 0)
					return (EFAULT);
				sc->sc_curcmap[i + cu->cmap.index + 0] = v;
				if ((v = fubyte(&cu->cmap.green[i])) < 0)
					return (EFAULT);
				sc->sc_curcmap[i + cu->cmap.index + 2] = v;
				if ((v = fubyte(&cu->cmap.blue[i])) < 0)
					return (EFAULT);
				sc->sc_curcmap[i + cu->cmap.index + 4] = v;
			}
			zx_cursor_color(sc);
		}
		if ((v & FB_CUR_SETSHAPE) != 0) {
			sc->sc_cursize = cu->size;
			count = cu->size.y * 4;
			memset(sc->sc_curbits, 0, sizeof(sc->sc_curbits));
			copyin(cu->mask, (caddr_t)sc->sc_curbits[0], count);
			copyin(cu->image, (caddr_t)sc->sc_curbits[1], count);
			zx_cursor_set(sc);
		}
		break;

	case FBIOGCURSOR:
		cu = (struct fbcursor *)data;

		cu->set = FB_CUR_SETALL;
		cu->enable = ((sc->sc_flags & ZX_CURSOR) != 0);
		cu->pos = sc->sc_curpos;
		cu->hot = sc->sc_curhot;
		cu->size = sc->sc_cursize;

		if (cu->image != NULL) {
			count = sc->sc_cursize.y * 4;
			rv = copyout((caddr_t)sc->sc_curbits[1],
			    (caddr_t)cu->image, count);
			if (rv)
				return (rv);
			rv = copyout((caddr_t)sc->sc_curbits[0],
			    (caddr_t)cu->mask, count);
			if (rv)
				return (rv);
		}
		if (cu->cmap.red != NULL) {
			if (cu->cmap.index > 2 ||
			    cu->cmap.count > 2 - cu->cmap.index)
				return (EINVAL);
			for (i = 0; i < cu->cmap.count; i++) {
				v = sc->sc_curcmap[i + cu->cmap.index + 0];
				if (subyte(&cu->cmap.red[i], v))
					return (EFAULT);
				v = sc->sc_curcmap[i + cu->cmap.index + 2];
				if (subyte(&cu->cmap.green[i], v))
					return (EFAULT);
				v = sc->sc_curcmap[i + cu->cmap.index + 4];
				if (subyte(&cu->cmap.blue[i], v))
					return (EFAULT);
			}
		} else {
			cu->cmap.index = 0;
			cu->cmap.count = 2;
		}
		break;

	default:
#ifdef DEBUG
		log(LOG_NOTICE, "zxioctl(0x%lx) (%s[%d])\n", cmd,
		    p->p_comm, p->p_pid);
#endif
		return (ENOTTY);
	}

	return (0);
}

int
zx_intr(void *cookie)
{

	return (1);
}

void
zx_reset(struct zx_softc *sc)
{
	volatile struct zx_draw *zd;
	volatile struct zx_command *zc;
	struct fbtype *fbt;
	u_int i;

	zd = sc->sc_zd_ss0;
	zc = sc->sc_zc;
	fbt = &sc->sc_fb.fb_type;

	zx_cross_loadwid(sc, ZX_WID_DBL_8, 0, 0x2c0);
	zx_cross_loadwid(sc, ZX_WID_DBL_8, 1, 0x30);
	zx_cross_loadwid(sc, ZX_WID_DBL_8, 2, 0x20);
	zx_cross_loadwid(sc, ZX_WID_DBL_24, 1, 0x30);

	i = sc->sc_zd_ss1->zd_misc;
	i |= ZX_SS1_MISC_ENABLE;
	SETREG(sc->sc_zd_ss1->zd_misc, i);

	SETREG(zd->zd_wid, 0xffffffff);
	SETREG(zd->zd_widclip, 0);
	SETREG(zd->zd_wmask, 0xffff);
	SETREG(zd->zd_vclipmin, 0);
	SETREG(zd->zd_vclipmax,
	    (fbt->fb_width - 1) | ((fbt->fb_height - 1) << 16));
	SETREG(zd->zd_fg, 0);
	SETREG(zd->zd_planemask, 0xff000000);
	SETREG(zd->zd_rop, ZX_STD_ROP);

	SETREG(zc->zc_extent,
	    (fbt->fb_width - 1) | ((fbt->fb_height - 1) << 11));
	SETREG(zc->zc_addrspace, ZX_ADDRSPC_FONT_OBGR);
	SETREG(zc->zc_fontt, 0);

	for (i = 0; i < 256; i++) {
		sc->sc_cmap[i] = rasops_cmap[i * 3];
		sc->sc_cmap[i + 256] = rasops_cmap[i * 3 + 1];
		sc->sc_cmap[i + 512] = rasops_cmap[i * 3 + 2];
	}

	zx_cmap_put(sc);
}

int
zx_cross_wait(struct zx_softc *sc)
{
	volatile struct zx_cross *zx;
	int i;

	zx = sc->sc_zx;

	for (i = 300000; i != 0; i--) {
		if ((zx->zx_csr & ZX_CROSS_CSR_PROGRESS) == 0)
			break;
		DELAY(1);
	}

	if (i == 0)
		printf("zx_cross_wait: timed out\n");

	return (i);
}

int
zx_cross_loadwid(struct zx_softc *sc, u_int type, u_int index, u_int value)
{
	volatile struct zx_cross *zx;
	u_int tmp;

	zx = sc->sc_zx;
	SETREG(zx->zx_type, ZX_CROSS_TYPE_WID);

	if (zx_cross_wait(sc))
		return (1);

	if (type == ZX_WID_DBL_8)
		tmp = (index & 0x0f) + 0x40;
	else if (type == ZX_WID_DBL_24)
		tmp = index & 0x3f;

	SETREG(zx->zx_type, 0x5800 + tmp);
	SETREG(zx->zx_value, value);
	SETREG(zx->zx_type, ZX_CROSS_TYPE_WID);
	SETREG(zx->zx_csr, ZX_CROSS_CSR_UNK | ZX_CROSS_CSR_UNK2);

	return (0);
}

int
zx_cmap_put(struct zx_softc *sc)
{
	volatile struct zx_cross *zx;
	const u_char *b;
	u_int i, t;

	zx = sc->sc_zx;

	SETREG(zx->zx_type, ZX_CROSS_TYPE_CLUT0);
	if (zx_cross_wait(sc))
		return (1);

	SETREG(zx->zx_type, ZX_CROSS_TYPE_CLUTDATA);

	for (i = 0, b = sc->sc_cmap; i < 256; i++) {
		t = b[i];
		t |= b[i + 256] << 8;
		t |= b[i + 512] << 16;
		SETREG(zx->zx_value, t);
	}

	SETREG(zx->zx_type, ZX_CROSS_TYPE_CLUT0);
	i = zx->zx_csr;
	i = i | ZX_CROSS_CSR_UNK | ZX_CROSS_CSR_UNK2;
	SETREG(zx->zx_csr, i);
	return (0);
}

void
zx_cursor_move(struct zx_softc *sc)
{
	volatile struct zx_cursor *zcu;
	int sx, sy, x, y;

	x = sc->sc_curpos.x - sc->sc_curhot.x;
	y = sc->sc_curpos.y - sc->sc_curhot.y;
	zcu = sc->sc_zcu;

	if (x < 0) {
		sx = min(-x, 32);
		x = 0;
	} else
		sx = 0;

	if (y < 0) {
		sy = min(-y, 32);
		y = 0;
	} else
		sy = 0;

	if (sx != sc->sc_shiftx || sy != sc->sc_shifty) {
		sc->sc_shiftx = sx;
		sc->sc_shifty = sy;
		zx_cursor_set(sc);
	}

	SETREG(zcu->zcu_sxy, ((y & 0x7ff) << 11) | (x & 0x7ff));
	SETREG(zcu->zcu_misc, zcu->zcu_misc | 0x30);

	/* XXX Necessary? */
	SETREG(zcu->zcu_misc, zcu->zcu_misc | 0x80);
}

void
zx_cursor_set(struct zx_softc *sc)
{
	volatile struct zx_cursor *zcu;
	int i, j, data;

	zcu = sc->sc_zcu;

	if ((sc->sc_flags & ZX_CURSOR) != 0)
		SETREG(zcu->zcu_misc, zcu->zcu_misc & ~0x80);

	for (j = 0; j < 2; j++) {
		SETREG(zcu->zcu_type, 0x20 << i);

		for (i = sc->sc_shifty; i < 32; i++) {
			data = sc->sc_curbits[j][i];
			SETREG(zcu->zcu_data, data >> sc->sc_shiftx);
		}
		for (i = sc->sc_shifty; i != 0; i--)
			SETREG(zcu->zcu_data, 0);
	}

	if ((sc->sc_flags & ZX_CURSOR) != 0)
		SETREG(zcu->zcu_misc, zcu->zcu_misc | 0x80);
}

void
zx_cursor_blank(struct zx_softc *sc)
{
	volatile struct zx_cursor *zcu;

	sc->sc_flags &= ~ZX_CURSOR;
	zcu = sc->sc_zcu;
	SETREG(zcu->zcu_misc, zcu->zcu_misc & ~0x80);
}

void
zx_cursor_unblank(struct zx_softc *sc)
{
	volatile struct zx_cursor *zcu;

	sc->sc_flags |= ZX_CURSOR;
	zcu = sc->sc_zcu;
	SETREG(zcu->zcu_misc, zcu->zcu_misc | 0x80);
}

void
zx_cursor_color(struct zx_softc *sc)
{
	volatile struct zx_cursor *zcu;
	u_int8_t tmp;

	zcu = sc->sc_zcu;

	SETREG(zcu->zcu_type, 0x50);

	tmp = sc->sc_curcmap[0] | (sc->sc_curcmap[2] << 8) |
	    (sc->sc_curcmap[4] << 16);
	SETREG(zcu->zcu_data, tmp);

	tmp = sc->sc_curcmap[1] | (sc->sc_curcmap[3] << 8) |
	    (sc->sc_curcmap[5] << 16);
	SETREG(zcu->zcu_data, sc->sc_curcmap[1]);

	SETREG(zcu->zcu_misc, zcu->zcu_misc | 0x03);
}

void
zx_blank(struct device *dv)
{
	struct zx_softc *sc;
	volatile struct zx_cross *zx;

	sc = (struct zx_softc *)dv;

	if ((sc->sc_flags & ZX_BLANKED) != 0)
		return;
	sc->sc_flags |= ZX_BLANKED;

	zx = sc->sc_zx;
	SETREG(zx->zx_type, ZX_CROSS_TYPE_VIDEO);
	SETREG(zx->zx_csr, zx->zx_csr & ~ZX_CROSS_CSR_ENABLE);
}

void
zx_unblank(struct device *dv)
{
	struct zx_softc *sc;
	volatile struct zx_cross *zx;

	sc = (struct zx_softc *)dv;

	if ((sc->sc_flags & ZX_BLANKED) == 0)
		return;
	sc->sc_flags &= ~ZX_BLANKED;

	zx = sc->sc_zx;
	SETREG(zx->zx_type, ZX_CROSS_TYPE_VIDEO);
	SETREG(zx->zx_csr, zx->zx_csr | ZX_CROSS_CSR_ENABLE);
}

paddr_t
zxmmap(dev_t dev, off_t off, int prot)
{
	struct zx_softc *sc;
	const struct zx_mmo *mm, *max;

	sc = device_lookup(&zx_cd, minor(dev));
	off = trunc_page(off);
	mm = zx_mmo;
	max = mm + sizeof(zx_mmo) / sizeof(zx_mmo[0]);

	for (; mm < max; mm++)
		if (off >= mm->mo_va && off < mm->mo_va + mm->mo_size) {
			off = off - mm->mo_va + mm->mo_pa;
			return (bus_space_mmap(sc->sc_bt, sc->sc_paddr,
			    off, prot, BUS_SPACE_MAP_LINEAR));
		}

	return (-1);
}

void
zx_fillrect(struct rasops_info *ri, int x, int y, int w, int h, long attr,
	    int rop)
{
	struct zx_softc *sc;
	volatile struct zx_command *zc;
	volatile struct zx_draw *zd;
	int fg, bg;

	sc = ri->ri_hw;
	zc = sc->sc_zc;
	zd = sc->sc_zd_ss0;

	rasops_unpack_attr(attr, &fg, &bg, NULL);
	x = x * sc->sc_fontw + ri->ri_xorigin;
	y = y * sc->sc_fonth + ri->ri_yorigin;
	w = sc->sc_fontw * w - 1;
	h = sc->sc_fonth * h - 1;

	while ((zc->zc_csr & ZX_CSR_BLT_BUSY) != 0)
		;

	SETREG(zd->zd_rop, rop);
	SETREG(zd->zd_fg, (bg & 7) ? 0x00000000 : 0xff000000);
	SETREG(zc->zc_extent, w | (h << 11));
	SETREG(zc->zc_fill, x | (y << 11) | 0x80000000);
}

void
zx_copyrect(struct rasops_info *ri, int sx, int sy, int dx, int dy, int w,
	    int h)
{
	struct zx_softc *sc;
	volatile struct zx_command *zc;
	volatile struct zx_draw *zd;
	int dir;

	sc = ri->ri_hw;
	zc = sc->sc_zc;
	zd = sc->sc_zd_ss0;

	sx = sx * sc->sc_fontw + ri->ri_xorigin;
	sy = sy * sc->sc_fonth + ri->ri_yorigin;
	dx = dx * sc->sc_fontw + ri->ri_xorigin;
	dy = dy * sc->sc_fonth + ri->ri_yorigin;
	w = w * sc->sc_fontw - 1;
	h = h * sc->sc_fonth - 1;

	if (sy < dy || sx < dx) {
		dir = 0x80000000;
		sx += w;
		sy += h;
		dx += w;
		dy += h;
	} else
		dir = 0;

	while ((zc->zc_csr & ZX_CSR_BLT_BUSY) != 0)
		;

	SETREG(zd->zd_rop, ZX_STD_ROP);
	SETREG(zc->zc_extent, w | (h << 11) | dir);
	SETREG(zc->zc_src, sx | (sy << 11));
	SETREG(zc->zc_copy, dx | (dy << 11));
}

void
zx_do_cursor(struct rasops_info *ri)
{

	zx_fillrect(ri, ri->ri_ccol, ri->ri_crow, 1, 1, 0,
	    ZX_ROP_NEW_XOR_OLD | ZX_ATTR_WE_ENABLE | ZX_ATTR_OE_ENABLE |
	    ZX_ATTR_FORCE_WID);
}

void
zx_erasecols(void *cookie, int row, int col, int num, long attr)
{
	struct rasops_info *ri;

	ri = (struct rasops_info *)cookie;

	zx_fillrect(ri, col, row, num, 1, attr, ZX_STD_ROP);
}

void
zx_eraserows(void *cookie, int row, int num, long attr)
{
	struct rasops_info *ri;

	ri = (struct rasops_info *)cookie;

	zx_fillrect(ri, 0, row, ri->ri_cols, num, attr, ZX_STD_ROP);
}

void
zx_copyrows(void *cookie, int src, int dst, int num)
{
	struct rasops_info *ri;

	ri = (struct rasops_info *)cookie;

	zx_copyrect(ri, 0, src, 0, dst, ri->ri_cols, num);
}

void
zx_copycols(void *cookie, int row, int src, int dst, int num)
{
	struct rasops_info *ri;

	ri = (struct rasops_info *)cookie;

	zx_copyrect(ri, src, row, dst, row, num, 1);
}

void
zx_putchar(void *cookie, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri;
	struct zx_softc *sc;
	struct wsdisplay_font *font;
	volatile struct zx_command *zc;
	volatile struct zx_draw *zd;
	volatile u_int32_t *dp;
	u_int8_t *fb;
	int fs, i, fg, bg, ul;

	ri = (struct rasops_info *)cookie;

	if (uc == ' ') {
		zx_fillrect(ri, col, row, 1, 1, attr, ZX_STD_ROP);
		return;
	}

	sc = (struct zx_softc *)ri->ri_hw;
	font = ri->ri_font;
	zc = sc->sc_zc;
	zd = sc->sc_zd_ss0;

	dp = (volatile u_int32_t *)sc->sc_pixels +
	    ((row * sc->sc_fonth + ri->ri_yorigin) << 11) +
	    (col * sc->sc_fontw + ri->ri_xorigin);
	fb = (u_int8_t *)font->data + (uc - font->firstchar) *
	    ri->ri_fontscale;
	fs = font->stride;
	rasops_unpack_attr(attr, &fg, &bg, &ul);

	while ((zc->zc_csr & ZX_CSR_BLT_BUSY) != 0)
		;

	SETREG(zd->zd_rop, ZX_STD_ROP);
	SETREG(zd->zd_fg, (fg & 7) ? 0x00000000 : 0xff000000);
	SETREG(zd->zd_bg, (bg & 7) ? 0x00000000 : 0xff000000);
	SETREG(zc->zc_fontmsk, 0xffffffff << (32 - sc->sc_fontw));

	if (sc->sc_fontw <= 8) {
		for (i = sc->sc_fonth; i != 0; i--, dp += 2048) {
			*dp = *fb << 24;
			fb += fs;
		}
	} else {
		for (i = sc->sc_fonth; i != 0; i--, dp += 2048) {
			*dp = *((u_int16_t *)fb) << 16;
			fb += fs;
		}
	}

	if (ul) {
		dp -= 4096;
		*dp = 0xffffffff;
	}
}
