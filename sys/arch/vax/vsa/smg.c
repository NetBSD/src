/*	$NetBSD: smg.c,v 1.61.6.4 2024/02/03 12:56:02 martin Exp $ */
/*	$OpenBSD: smg.c,v 1.28 2014/12/23 21:39:12 miod Exp $	*/
/*
 * Copyright (c) 2006, Miodrag Vallat
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
/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 * Copyright (c) 1998 Ludd, University of Lule}, Sweden.
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
 * Copyright (c) 1996 Jason R. Thorpe.  All rights reserved.
 * Copyright (c) 1991 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Mark Davies of the Department of Computer
 * Science, Victoria University of Wellington, New Zealand.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: grf_hy.c 1.2 93/08/13$
 *
 *	@(#)grf_hy.c	8.4 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: smg.c,v 1.61.6.4 2024/02/03 12:56:02 martin Exp $");

#include "dzkbd.h"
#include "wsdisplay.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/conf.h>

#include <machine/vsbus.h>
#include <machine/sid.h>
#include <machine/cpu.h>
#include <machine/ka420.h>
#include <machine/scb.h>

#include <dev/cons.h>

#include <dev/ic/dc503reg.h>

#include <dev/dec/dzreg.h>
#include <dev/dec/dzvar.h>
#include <dev/dec/dzkbdvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

/* Screen hardware defs */
#define SM_XWIDTH	1024
#define SM_YWIDTH	864

#define CUR_XBIAS	216	/* Add to cursor position */
#define CUR_YBIAS	33

static int	smg_match(device_t, cfdata_t, void *);
static void	smg_attach(device_t, device_t, void *);

struct	smg_screen {
	struct rasops_info ss_ri;
	uint8_t		*ss_addr;		/* frame buffer address */
	struct dc503reg	*ss_cursor;		/* cursor registers */
	uint16_t	ss_curcmd;
	struct wsdisplay_curpos ss_curpos, ss_curhot;
	uint16_t	ss_curimg[PCC_CURSOR_SIZE];
	uint16_t	ss_curmask[PCC_CURSOR_SIZE];
};

/* for console */
static struct smg_screen smg_consscr;

struct	smg_softc {
	device_t sc_dev;
	struct smg_screen *sc_scr;
	int	sc_nscreens;
};

CFATTACH_DECL_NEW(smg, sizeof(struct smg_softc),
    smg_match, smg_attach, NULL, NULL);

static struct wsscreen_descr smg_stdscreen = {
	.name = "std",
};

static const struct wsscreen_descr *_smg_scrlist[] = {
	&smg_stdscreen,
};

static const struct wsscreen_list smg_screenlist = {
	.nscreens = sizeof(_smg_scrlist) / sizeof(struct wsscreen_descr *),
	.screens = _smg_scrlist,
};

static int smg_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t smg_mmap(void *, void *, off_t, int);
static int smg_alloc_screen(void *, const struct wsscreen_descr *,
    void **, int *, int *, long *);
static void smg_free_screen(void *, void *);
static int smg_show_screen(void *, void *, int, void (*) (void *, int, int),
    void *);

static const struct wsdisplay_accessops smg_accessops = {
	.ioctl = smg_ioctl,
	.mmap = smg_mmap,
	.alloc_screen = smg_alloc_screen,
	.free_screen = smg_free_screen,
	.show_screen = smg_show_screen,
	.load_font = NULL
};

static void smg_putchar(void *, int, int, u_int, long);
static void smg_cursor(void *, int, int, int);
static void smg_blockmove(struct rasops_info *, u_int, u_int, u_int, u_int,
    u_int, int);
static void smg_copycols(void *, int, int, int, int);
static void smg_erasecols(void *, int, int, int, long);

static int smg_getcursor(struct smg_screen *, struct wsdisplay_cursor *);
static int smg_setup_screen(struct smg_screen *);
static int smg_setcursor(struct smg_screen *, struct wsdisplay_cursor *);
static void smg_updatecursor(struct smg_screen *, u_int);

static int
smg_match(device_t parent, cfdata_t cf, void *aux)
{
	struct vsbus_attach_args *va = aux;
	volatile short *curcmd;
	volatile short *cfgtst;
	short tmp, tmp2;

	switch (vax_boardtype) {
	default:
		return 0;

	case VAX_BTYP_410:
	case VAX_BTYP_420:
	case VAX_BTYP_43:
		if (va->va_paddr != KA420_CUR_BASE)
			return 0;

		/* not present on microvaxes */
		if ((vax_confdata & KA420_CFG_MULTU) != 0)
			return 0;

		/*
		 * If the color option board is present, do not attach
		 * unless we are explicitely asked to via device flags.
		 */
		if ((vax_confdata & KA420_CFG_VIDOPT) != 0 &&
		    (cf->cf_flags & 1) == 0)
			return 0;
		break;
	}

	/* when already running as console, always fake things */
	if ((vax_confdata & (KA420_CFG_L3CON | KA420_CFG_VIDOPT)) == 0
#if NWSDISPLAY > 0
	    && cn_tab->cn_putc == wsdisplay_cnputc
#endif
	) {
		struct vsbus_softc *sc = device_private(parent);

		sc->sc_mask = 0x08;
		scb_fake(0x44, 0x15);
		return 20;
	} else {
		/*
		 * Try to find the cursor chip by testing the flip-flop.
		 * If nonexistent, no glass tty.
		 */
		curcmd = (short *)va->va_addr;
		cfgtst = (short *)vax_map_physmem(VS_CFGTST, 1);
		curcmd[0] = PCCCMD_HSHI | PCCCMD_FOPB;
		DELAY(300000);
		tmp = cfgtst[0];
		curcmd[0] = PCCCMD_TEST | PCCCMD_HSHI;
		DELAY(300000);
		tmp2 = cfgtst[0];
		vax_unmap_physmem((vaddr_t)cfgtst, 1);

		if (tmp2 != tmp)
			return 20; /* Using periodic interrupt */
		else
			return 0;
	}
}

static void
smg_attach(device_t parent, device_t self, void *aux)
{
	struct smg_softc *sc = device_private(self);
	struct smg_screen *scr;
	struct wsemuldisplaydev_attach_args aa;
	int console;

	console =
#if NWSDISPLAY > 0
	    (vax_confdata & (KA420_CFG_L3CON | KA420_CFG_VIDOPT)) == 0 &&
	    cn_tab->cn_putc == wsdisplay_cnputc;
#else
	    (vax_confdata & (KA420_CFG_L3CON | KA420_CFG_VIDOPT)) == 0;
#endif
	if (console) {
		scr = &smg_consscr;
		sc->sc_nscreens = 1;
	} else {
		scr = kmem_zalloc(sizeof(*scr), KM_SLEEP);

		scr->ss_addr =
		    (void *)vax_map_physmem(SMADDR, SMSIZE / VAX_NBPG);
		if (scr->ss_addr == NULL) {
			aprint_error(": can not map frame buffer\n");
			kmem_free(scr, sizeof(*scr));
			return;
		}

		scr->ss_cursor =
		    (struct dc503reg *)vax_map_physmem(KA420_CUR_BASE, 1);
		if (scr->ss_cursor == NULL) {
			aprint_error(": can not map cursor chip\n");
			vax_unmap_physmem((vaddr_t)scr->ss_addr,
			    SMSIZE / VAX_NBPG);
			kmem_free(scr, sizeof(*scr));
			return;
		}

		if (smg_setup_screen(scr) != 0) {
			aprint_error(": initialization failed\n");
			vax_unmap_physmem((vaddr_t)scr->ss_cursor, 1);
			vax_unmap_physmem((vaddr_t)scr->ss_addr,
			    SMSIZE / VAX_NBPG);
			kmem_free(scr, sizeof(*scr));
			return;
		}
	}
	sc->sc_scr = scr;

	aprint_normal("\n");
	aprint_normal_dev(self, "%dx%d on-board monochrome framebuffer\n",
	    SM_XWIDTH, SM_YWIDTH);

	aa.console = console;
	aa.scrdata = &smg_screenlist;
	aa.accessops = &smg_accessops;
	aa.accesscookie = sc;

	config_found(self, &aa, wsemuldisplaydevprint, CFARGS_NONE);
}

/*
 * Initialize anything necessary for an emulating wsdisplay to work (i.e.
 * pick a font, initialize a rasops structure, setup the accessops callbacks.)
 */
static int
smg_setup_screen(struct smg_screen *ss)
{
	struct rasops_info *ri = &ss->ss_ri;
	int cookie;

	memset(ri, 0, sizeof(*ri));
	ri->ri_depth = 1;
	ri->ri_width = SM_XWIDTH;
	ri->ri_height = SM_YWIDTH;
	ri->ri_stride = SM_XWIDTH >> 3;
	ri->ri_flg = RI_CLEAR | RI_CENTER;
	ri->ri_bits = (void *)ss->ss_addr;
	ri->ri_hw = ss;
	if (ss == &smg_consscr)
		ri->ri_flg |= RI_NO_AUTO;

	wsfont_init();
	cookie = wsfont_find(NULL, 12, 0, 0, WSDISPLAY_FONTORDER_R2L,
	    WSDISPLAY_FONTORDER_R2L, WSFONT_FIND_BITMAP);
	if (cookie < 0)
		cookie = wsfont_find(NULL, 8, 0, 0, WSDISPLAY_FONTORDER_R2L,
		    WSDISPLAY_FONTORDER_R2L, WSFONT_FIND_BITMAP);
	if (cookie < 0)
		cookie = wsfont_find(NULL, 0, 0, 0, WSDISPLAY_FONTORDER_R2L,
		    WSDISPLAY_FONTORDER_R2L, WSFONT_FIND_BITMAP);
	if (cookie < 0)
		return -1;
	if (wsfont_lock(cookie, &ri->ri_font) != 0)
		return -1;
	ri->ri_wsfcookie = cookie;

	/*
	 * Ask for an unholy big display, rasops will trim this to more
	 * reasonable values.
	 */
	if (rasops_init(ri, 160, 160) != 0)
		return -1;

	ri->ri_ops.cursor = smg_cursor;
	ri->ri_ops.putchar = smg_putchar;
	ri->ri_ops.copycols = smg_copycols;
	ri->ri_ops.erasecols = smg_erasecols;

	smg_stdscreen.ncols = ri->ri_cols;
	smg_stdscreen.nrows = ri->ri_rows;
	smg_stdscreen.textops = &ri->ri_ops;
	smg_stdscreen.fontwidth = ri->ri_font->fontwidth;
	smg_stdscreen.fontheight = ri->ri_font->fontheight;
	smg_stdscreen.capabilities = ri->ri_caps;

	ss->ss_curcmd = PCCCMD_HSHI;
	ss->ss_cursor->cmdr = ss->ss_curcmd;

	return 0;
}

static int
smg_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct smg_softc *sc = v;
	struct smg_screen *ss = sc->sc_scr;
	struct wsdisplay_fbinfo *wdf;
	struct wsdisplay_curpos *pos;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_VAX_MONO;
		break;

	case WSDISPLAYIO_GINFO:
		wdf = (struct wsdisplay_fbinfo *)data;
		wdf->height = ss->ss_ri.ri_height;
		wdf->width = ss->ss_ri.ri_width;
		wdf->depth = ss->ss_ri.ri_depth;
		wdf->cmsize = 0;
		break;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = ss->ss_ri.ri_stride;
		break;

	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_SVIDEO:
		break;

	case WSDISPLAYIO_GCURPOS:
		pos = (struct wsdisplay_curpos *)data;
		pos->x = ss->ss_curpos.x;
		pos->y = ss->ss_curpos.y;
		break;

	case WSDISPLAYIO_SCURPOS:
		pos = (struct wsdisplay_curpos *)data;
		ss->ss_curpos.x = pos->x;
		ss->ss_curpos.y = pos->y;
		smg_updatecursor(ss, WSDISPLAY_CURSOR_DOPOS);
		break;

	case WSDISPLAYIO_GCURMAX:
		pos = (struct wsdisplay_curpos *)data;
		pos->x = pos->y = PCC_CURSOR_SIZE;

	case WSDISPLAYIO_GCURSOR:
		return smg_getcursor(ss, (struct wsdisplay_cursor *)data);

	case WSDISPLAYIO_SCURSOR:
		return smg_setcursor(ss, (struct wsdisplay_cursor *)data);

	default:
		return EPASSTHROUGH;
	}
	return 0;
}

static paddr_t
smg_mmap(void *v, void *vs, off_t offset, int prot)
{

	if (offset >= SMSIZE || offset < 0)
		return -1;

	return (SMADDR + offset) >> PGSHIFT;
}

static int
smg_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
    int *curxp, int *curyp, long *defattrp)
{
	struct smg_softc *sc = v;
	struct smg_screen *ss = sc->sc_scr;
	struct rasops_info *ri = &ss->ss_ri;

	if (sc->sc_nscreens > 0)
		return ENOMEM;

	*cookiep = ri;
	*curxp = *curyp = 0;
	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, defattrp);
	sc->sc_nscreens++;

	return 0;
}

static void
smg_free_screen(void *v, void *cookie)
{
	struct smg_softc *sc = v;

	sc->sc_nscreens--;
}

static int
smg_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{

	return 0;
}

static int
smg_getcursor(struct smg_screen *ss, struct wsdisplay_cursor *wdc)
{
	int error;

	if ((wdc->which & WSDISPLAY_CURSOR_DOCUR) != 0)
		wdc->enable = ss->ss_curcmd & PCCCMD_ENPA ? 1 : 0;
	if ((wdc->which & WSDISPLAY_CURSOR_DOPOS) != 0) {
		wdc->pos.x = ss->ss_curpos.x;
		wdc->pos.y = ss->ss_curpos.y;
	}
	if ((wdc->which & WSDISPLAY_CURSOR_DOHOT) != 0) {
		wdc->hot.x = ss->ss_curhot.x;
		wdc->hot.y = ss->ss_curhot.y;
	}
	if ((wdc->which & WSDISPLAY_CURSOR_DOCMAP) != 0) {
		wdc->cmap.index = 0;
		wdc->cmap.count = 0;
	}
	if ((wdc->which & WSDISPLAY_CURSOR_DOSHAPE) != 0) {
		wdc->size.x = wdc->size.y = PCC_CURSOR_SIZE;
		error = copyout(ss->ss_curimg, wdc->image,
		    sizeof(ss->ss_curimg));
		if (error != 0)
			return error;
		error = copyout(ss->ss_curmask, wdc->mask,
		    sizeof(ss->ss_curmask));
		if (error != 0)
			return error;
	}

	return 0;
}

static int
smg_setcursor(struct smg_screen *ss, struct wsdisplay_cursor *wdc)
{
	uint16_t curfg[PCC_CURSOR_SIZE], curmask[PCC_CURSOR_SIZE];
	int error;

	if ((wdc->which & WSDISPLAY_CURSOR_DOCMAP) != 0) {
		/* No cursor colormap since we are a B&W device. */
		if (wdc->cmap.count != 0)
			return EINVAL;
	}

	/*
	 * First, do the userland-kernel data transfers, so that we can fail
	 * if necessary before altering anything.
	 */
	if ((wdc->which & WSDISPLAY_CURSOR_DOSHAPE) != 0) {
		if (wdc->size.x != PCC_CURSOR_SIZE ||
		    wdc->size.y != PCC_CURSOR_SIZE)
			return EINVAL;
		error = copyin(wdc->image, curfg, sizeof(curfg));
		if (error != 0)
			return error;
		error = copyin(wdc->mask, curmask, sizeof(curmask));
		if (error != 0)
			return error;
	}

	/*
	 * Now update our variables...
	 */
	if ((wdc->which & WSDISPLAY_CURSOR_DOCUR) != 0) {
		if (wdc->enable)
			ss->ss_curcmd |= PCCCMD_ENPB | PCCCMD_ENPA;
		else
			ss->ss_curcmd &= ~(PCCCMD_ENPB | PCCCMD_ENPA);
	}
	if ((wdc->which & WSDISPLAY_CURSOR_DOPOS) != 0) {
		ss->ss_curpos.x = wdc->pos.x;
		ss->ss_curpos.y = wdc->pos.y;
	}
	if ((wdc->which & WSDISPLAY_CURSOR_DOHOT) != 0) {
		ss->ss_curhot.x = wdc->hot.x;
		ss->ss_curhot.y = wdc->hot.y;
	}
	if ((wdc->which & WSDISPLAY_CURSOR_DOSHAPE) != 0) {
		memcpy(ss->ss_curimg, curfg, sizeof(ss->ss_curimg));
		memcpy(ss->ss_curmask, curmask, sizeof(ss->ss_curmask));
	}

	/*
	 * ...and update the cursor
	 */
	smg_updatecursor(ss, wdc->which);

	return 0;
}

static void
smg_updatecursor(struct smg_screen *ss, u_int which)
{
	u_int i;

	if ((which & (WSDISPLAY_CURSOR_DOPOS | WSDISPLAY_CURSOR_DOHOT)) != 0) {
		ss->ss_cursor->xpos =
		    ss->ss_curpos.x - ss->ss_curhot.x + CUR_XBIAS;
		ss->ss_cursor->ypos =
		    ss->ss_curpos.y - ss->ss_curhot.y + CUR_YBIAS;
	}
	if ((which & WSDISPLAY_CURSOR_DOSHAPE) != 0) {
		ss->ss_cursor->cmdr = ss->ss_curcmd | PCCCMD_LODSA;
		for (i = 0; i < PCC_CURSOR_SIZE; i++)
			ss->ss_cursor->load = ss->ss_curimg[i];
		for (i = 0; i < PCC_CURSOR_SIZE; i++)
			ss->ss_cursor->load = ss->ss_curmask[i];
		ss->ss_cursor->cmdr = ss->ss_curcmd;
	} else
	if ((which & WSDISPLAY_CURSOR_DOCUR) != 0)
		ss->ss_cursor->cmdr = ss->ss_curcmd;
}

/*
 * Faster console operations
 */

#include <vax/vsa/maskbits.h>

/* putchar() and cursor() ops are taken from luna68k omrasops.c */

#define	ALL1BITS	(~0U)
#define	ALL0BITS	(0U)
#define	BLITWIDTH	(32)
#define	ALIGNMASK	(0x1f)
#define	BYTESDONE	(4)

static void
smg_putchar(void *cookie, int row, int startcol, u_int uc, long attr)
{
	struct rasops_info *ri = cookie;
	uint8_t *p;
	int scanspan, startx, height, width, align, y;
	uint32_t lmask, rmask, glyph, inverse;
	int i;
	uint8_t *fb;

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * row;
	startx = ri->ri_font->fontwidth * startcol;
	height = ri->ri_font->fontheight;
	fb = (uint8_t *)ri->ri_font->data +
	    (uc - ri->ri_font->firstchar) * ri->ri_fontscale;
	inverse = ((attr & WSATTR_REVERSE) != 0) ? ALL1BITS : ALL0BITS;

	p = (uint8_t *)ri->ri_bits + y * scanspan + ((startx / 32) * 4);
	align = startx & ALIGNMASK;
	width = ri->ri_font->fontwidth + align;
	/* lmask and rmask are in WSDISPLAY_FONTORDER_R2L bitorder */
	lmask = ALL1BITS << align;
	rmask = ALL1BITS >> (-width & ALIGNMASK);
	if (width <= BLITWIDTH) {
		uint32_t mask = lmask & rmask;
		while (height > 0) {
			uint32_t image;
			/*
			 * The font glyph is stored in byteorder and bitorder
			 * WSDISPLAY_FONTORDER_R2L to use proper shift ops.
			 * On the other hand, VRAM data is stored in
			 * WSDISPLAY_FONTORDER_R2L bitorder and
			 * WSDIPPLAY_FONTORDER_L2R byteorder.
			 */
			glyph = 0;
			for (i = ri->ri_font->stride; i != 0; i--)
				glyph = (glyph << 8) | *fb++;
			glyph = (glyph << align) ^ inverse;
			image = *(uint32_t *)p;
			*(uint32_t *)p = (image & ~mask) | (glyph & mask);
			p += scanspan;
			height--;
		}
	} else {
		uint8_t *q = p;
		uint32_t lhalf, rhalf;

		while (height > 0) {
			uint32_t image;
			glyph = 0;
			for (i = ri->ri_font->stride; i != 0; i--)
				glyph = (glyph << 8) | *fb++;
			lhalf = (glyph << align) ^ inverse;
			image = *(uint32_t *)p;
			*(uint32_t *)p = (image & ~lmask) | (lhalf & lmask);
			p += BYTESDONE;
			rhalf = (glyph >> (BLITWIDTH - align)) ^ inverse;
			image = *(uint32_t *)p;
			*(uint32_t *)p = (rhalf & rmask) | (image & ~rmask);

			p = (q += scanspan);
			height--;
		}
	}
}

static void
smg_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	uint8_t *p;
	int scanspan, startx, height, width, align, y;
	uint32_t lmask, rmask, image;

	if (!on) {
		/* make sure it's on */
		if ((ri->ri_flg & RI_CURSOR) == 0)
			return;

		row = ri->ri_crow;
		col = ri->ri_ccol;
	} else {
		/* unpaint the old copy. */
		ri->ri_crow = row;
		ri->ri_ccol = col;
	}

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * row;
	startx = ri->ri_font->fontwidth * col;
	height = ri->ri_font->fontheight;

	p = (uint8_t *)ri->ri_bits + y * scanspan + ((startx / 32) * 4);
	align = startx & ALIGNMASK;
	width = ri->ri_font->fontwidth + align;
	/* lmask and rmask are in WSDISPLAY_FONTORDER_R2L bitorder */
	lmask = ALL1BITS << align;
	rmask = ALL1BITS >> (-width & ALIGNMASK);
	if (width <= BLITWIDTH) {
		uint32_t mask = lmask & rmask;
		while (height > 0) {
			image = *(uint32_t *)p;
			*(uint32_t *)p =
			    (image & ~mask) | ((image ^ ALL1BITS) & mask);
			p += scanspan;
			height--;
		}
	} else {
		uint8_t *q = p;

		while (height > 0) {
			image = *(uint32_t *)p;
			*(uint32_t *)p =
			    (image & ~lmask) | ((image ^ ALL1BITS) & lmask);
			p += BYTESDONE;
			image = *(uint32_t *)p;
			*(uint32_t *)p =
			    ((image ^ ALL1BITS) & rmask) | (image & ~rmask);

			p = (q += scanspan);
			height--;
		}
	}
	ri->ri_flg ^= RI_CURSOR;
}

static void
smg_blockmove(struct rasops_info *ri, u_int sx, u_int y, u_int dx, u_int cx,
    u_int cy, int rop)
{
	int width;		/* add to get to same position in next line */

	unsigned int *psrcLine, *pdstLine;
				/* pointers to line with current src and dst */
	unsigned int *psrc;	/* pointer to current src longword */
	unsigned int *pdst;	/* pointer to current dst longword */

				/* following used for looping through a line */
	unsigned int startmask, endmask;  /* masks for writing ends of dst */
	int nlMiddle;		/* whole longwords in dst */
	int nl;			/* temp copy of nlMiddle */
	int xoffSrc;		/* offset (>= 0, < 32) from which to
				   fetch whole longwords fetched in src */
	int nstart;		/* number of ragged bits at start of dst */
	int nend;		/* number of ragged bits at end of dst */
	int srcStartOver;	/* pulling nstart bits from src
				   overflows into the next word? */

	width = SM_XWIDTH >> 5;

	/* start at first scanline */
	psrcLine = pdstLine = ((u_int *)ri->ri_bits) + (y * width);

	/* x direction doesn't matter for < 1 longword */
	if (cx <= 32) {
		int srcBit, dstBit;	/* bit offset of src and dst */

		pdstLine += (dx >> 5);
		psrcLine += (sx >> 5);
		psrc = psrcLine;
		pdst = pdstLine;

		srcBit = sx & ALIGNMASK;
		dstBit = dx & ALIGNMASK;

		while (cy--) {
			getandputrop(psrc, srcBit, dstBit, cx, pdst, rop);
			pdst += width;
			psrc += width;
		}
	} else {
		startmask = ALL1BITS << (dx & ALIGNMASK);
		endmask   = ALL1BITS >> (~cx & ALIGNMASK);
		if (startmask)
			nlMiddle = (cx - (32 - (dx & ALIGNMASK))) >> 5;
		else
			nlMiddle = cx >> 5;

		if (startmask)
			nstart = 32 - (dx & ALIGNMASK);
		else
			nstart = 0;
		if (endmask)
			nend = (dx + cx) & ALIGNMASK;
		else
			nend = 0;

		xoffSrc = ((sx & ALIGNMASK) + nstart) & ALIGNMASK;
		srcStartOver = ((sx & ALIGNMASK) + nstart) > 31;

		if (sx >= dx) {	/* move left to right */
			pdstLine += (dx >> 5);
			psrcLine += (sx >> 5);

			while (cy--) {
				psrc = psrcLine;
				pdst = pdstLine;

				if (startmask) {
					getandputrop(psrc, (sx & ALIGNMASK),
					    (dx & ALIGNMASK), nstart, pdst, rop);
					pdst++;
					if (srcStartOver)
						psrc++;
				}

				/* special case for aligned operations */
				if (xoffSrc == 0) {
					nl = nlMiddle;
					while (nl--) {
						switch (rop) {
						case RR_CLEAR:
							*pdst = 0;
							break;
						case RR_SET:
							*pdst = ~0;
							break;
						default:
							*pdst = *psrc;
							break;
						}
						psrc++;
						pdst++;
					}
				} else {
					nl = nlMiddle + 1;
					while (--nl) {
						switch (rop) {
						case RR_CLEAR:
							*pdst = 0;
							break;
						case RR_SET:
							*pdst = ~0;
							break;
						default:
							getunalignedword(psrc,
							    xoffSrc, *pdst);
							break;
						}
						pdst++;
						psrc++;
					}
				}

				if (endmask) {
					getandputrop(psrc, xoffSrc, 0, nend,
					    pdst, rop);
				}

				pdstLine += width;
				psrcLine += width;
			}
		} else {	/* move right to left */
			pdstLine += ((dx + cx) >> 5);
			psrcLine += ((sx + cx) >> 5);
			/*
			 * If fetch of last partial bits from source crosses
			 * a longword boundary, start at the previous longword
			 */
			if (xoffSrc + nend >= 32)
				--psrcLine;

			while (cy--) {
				psrc = psrcLine;
				pdst = pdstLine;

				if (endmask) {
					getandputrop(psrc, xoffSrc, 0, nend,
					    pdst, rop);
				}

				nl = nlMiddle + 1;
				while (--nl) {
					--psrc;
					--pdst;
					switch (rop) {
					case RR_CLEAR:
						*pdst = 0;
						break;
					case RR_SET:
						*pdst = ~0;
						break;
					default:
						getunalignedword(psrc, xoffSrc,
						    *pdst);
						break;
					}
				}

				if (startmask) {
					if (srcStartOver)
						--psrc;
					--pdst;
					getandputrop(psrc, (sx & ALIGNMASK),
					    (dx & ALIGNMASK), nstart, pdst,
					    rop);
				}

				pdstLine += width;
				psrcLine += width;
			}
		}
	}
}

static void
smg_copycols(void *cookie, int row, int src, int dst, int n)
{
	struct rasops_info *ri = cookie;

	n *= ri->ri_font->fontwidth;
	src *= ri->ri_font->fontwidth;
	dst *= ri->ri_font->fontwidth;
	row *= ri->ri_font->fontheight;

	smg_blockmove(ri, src, row, dst, n, ri->ri_font->fontheight,
	    RR_COPY);
}

static void
smg_erasecols(void *cookie, int row, int col, int num, long attr)
{
	struct rasops_info *ri = cookie;
	int fg, bg;

	rasops_unpack_attr(attr, &fg, &bg, NULL);

	num *= ri->ri_font->fontwidth;
	col *= ri->ri_font->fontwidth;
	row *= ri->ri_font->fontheight;

	smg_blockmove(ri, col, row, col, num, ri->ri_font->fontheight,
	    bg == 0 ? RR_CLEAR : RR_SET);
}

/*
 * Console support code
 */

cons_decl(smg);

void
smgcnprobe(struct consdev *cndev)
{
	extern const struct cdevsw wsdisplay_cdevsw;

	switch (vax_boardtype) {
	case VAX_BTYP_410:
	case VAX_BTYP_420:
	case VAX_BTYP_43:
		if ((vax_confdata & (KA420_CFG_L3CON | KA420_CFG_MULTU)) != 0)
			break;	/* doesn't use graphics console */

		if ((vax_confdata & KA420_CFG_VIDOPT) != 0)
			break;	/* there is a color option */

		cndev->cn_pri = CN_INTERNAL;
		cndev->cn_dev =
		    makedev(cdevsw_lookup_major(&wsdisplay_cdevsw), 0);
		break;

	default:
		break;
	}
}

/*
 * Called very early to setup the glass tty as console.
 * Because it's called before the VM system is initialized, virtual memory
 * for the framebuffer can be stolen directly without disturbing anything.
 */
void
smgcninit(struct consdev *cndev)
{
	struct smg_screen *ss = &smg_consscr;
	vaddr_t ova;
	long defattr;
	struct rasops_info *ri;
	extern vaddr_t virtual_avail;

	ova = virtual_avail;

	ss->ss_addr = (uint8_t *)virtual_avail;
	ioaccess(virtual_avail, SMADDR, SMSIZE / VAX_NBPG);
	virtual_avail += SMSIZE;

	ss->ss_cursor = (struct dc503reg *)virtual_avail;
	ioaccess(virtual_avail, KA420_CUR_BASE, 1);
	virtual_avail += VAX_NBPG;

	virtual_avail = round_page(virtual_avail);

	/* this had better not fail */
	if (smg_setup_screen(ss) != 0) {
		iounaccess((vaddr_t)ss->ss_addr, SMSIZE / VAX_NBPG);
		iounaccess((vaddr_t)ss->ss_cursor, 1);
		virtual_avail = ova;
		return;
	}

	ri = &ss->ss_ri;
	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&smg_stdscreen, ri, 0, 0, defattr);
	cn_tab->cn_pri = CN_INTERNAL;

#if NDZKBD > 0
	dzkbd_cnattach(0); /* Connect keyboard and screen together */
#endif
}
