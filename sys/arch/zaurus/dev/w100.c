/* $NetBSD: w100.c,v 1.1 2012/01/29 10:12:42 tsutsui Exp $ */
/*
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: w100.c,v 1.1 2012/01/29 10:12:42 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/kernel.h>			/* for cold */

#include <uvm/uvm_extern.h>

#include <dev/cons.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wscons_callbacks.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>

#include <sys/bus.h>
#include <machine/cpu.h>
#include <arm/cpufunc.h>

#include <zaurus/dev/w100reg.h>
#include <zaurus/dev/w100var.h>

#include "wsdisplay.h"

/* Console */
struct {
	int is_console;
	struct w100_wsscreen_descr *descr;
	const struct w100_panel_geometry *geom;
} w100_console;

static void w100_initialize(struct w100_softc *sc,
		 const struct w100_panel_geometry *geom);
static int  w100_new_screen(struct w100_softc *sc, int depth,
		 struct w100_screen **scrpp);
static void w100_lcd_geometry(struct w100_softc *sc,
                 const struct w100_panel_geometry *geom);
#if NWSDISPLAY > 0
static void w100_setup_rasops(struct w100_softc *sc,
		 struct rasops_info *rinfo,
		 struct w100_wsscreen_descr *descr,
		 const struct w100_panel_geometry *geom);
#endif

#define w100_reg_write(sc, offset, value) \
    bus_space_write_4((sc)->iot, (sc)->ioh_reg, offset, value)

static void
w100_lcd_geometry(struct w100_softc *sc,
    const struct w100_panel_geometry *geom)
{

	sc->geometry = geom;
	switch (geom->rotate) {
	case W100_PANEL_ROTATE_CW:
		sc->display_width  = geom->panel_height;
		sc->display_height = geom->panel_width;
		w100_reg_write(sc, W100_REG_PCLK_CTRL, 0x00000061);
		w100_reg_write(sc, W100_REG_GRAPHIC_CTRL, 0x00de1d0e);
		w100_reg_write(sc, W100_REG_GRAPHIC_OFFSET, 0x00895b00);
		w100_reg_write(sc, W100_REG_GRAPHIC_PITCH, 0x00000500);
		break;
	case W100_PANEL_ROTATE_CCW:
		sc->display_width  = geom->panel_height;
		sc->display_height = geom->panel_width;
		w100_reg_write(sc, W100_REG_PCLK_CTRL, 0x00000061);
		w100_reg_write(sc, W100_REG_GRAPHIC_CTRL, 0x00de1d16);
		w100_reg_write(sc, W100_REG_GRAPHIC_OFFSET, 0x008004fc);
		w100_reg_write(sc, W100_REG_GRAPHIC_PITCH, 0x00000500);
		break;
	case W100_PANEL_ROTATE_UD:
		sc->display_width  = geom->panel_width;
		sc->display_height = geom->panel_height;
		w100_reg_write(sc, W100_REG_PCLK_CTRL, 0x00000021);
		w100_reg_write(sc, W100_REG_GRAPHIC_CTRL, 0x00ded7e);
		w100_reg_write(sc, W100_REG_GRAPHIC_OFFSET, 0x00895ffc);
		w100_reg_write(sc, W100_REG_GRAPHIC_PITCH, 0x000003c0);
		break;
	default:
		sc->display_width  = geom->panel_width;
		sc->display_height = geom->panel_height;
		w100_reg_write(sc, W100_REG_PCLK_CTRL, 0x00000021);
		w100_reg_write(sc, W100_REG_GRAPHIC_CTRL, 0x00de1d66);
		w100_reg_write(sc, W100_REG_GRAPHIC_OFFSET, 0x00800000);
		w100_reg_write(sc, W100_REG_GRAPHIC_PITCH, 0x000003c0);
		break;
	}
	w100_reg_write(sc, W100_REG_DISP_DB_BUF_CTRL, 0x0000007b);
}

/*
 * Initialize the LCD controller.
 */
static void
w100_initialize(struct w100_softc *sc, const struct w100_panel_geometry *geom)
{

	w100_lcd_geometry(sc, geom);
}


/*
 * Common driver attachment code.
 */
void
w100_attach_subr(struct w100_softc *sc, bus_space_tag_t iot,
    const struct w100_panel_geometry *geom)
{
	bus_space_handle_t ioh_cfg, ioh_reg, ioh_vram;
	int error;

	aprint_normal(": ATI Imageon100 LCD controller\n");
	aprint_naive("\n");

	sc->n_screens = 0;
	LIST_INIT(&sc->screens);

	/* config handler */
	error = bus_space_map(iot, W100_CFG_ADDRESS, W100_CFG_SIZE, 0,
	    &ioh_cfg);
	if (error) {
		aprint_error_dev(sc->dev,
		    "failed to map config (errorno=%d)\n", error);
		return;
	}

	/* register handler */
	error = bus_space_map(iot, W100_REG_ADDRESS, W100_REG_SIZE, 0,
	    &ioh_reg);
	if (error) {
		aprint_error_dev(sc->dev,
		    "failed to map register (errorno=%d)\n", error);
		return;
	}

	/* videomem handler */
	error = bus_space_map(iot, W100_EXTMEM_ADDRESS, W100_EXTMEM_SIZE, 0,
	    &ioh_vram);
	if (error) {
		aprint_error_dev(sc->dev,
		    "failed to vram register (errorno=%d)\n", error);
		return;
	}

	sc->iot = iot;
	sc->ioh_cfg = ioh_cfg;
	sc->ioh_reg = ioh_reg;
	sc->ioh_vram = ioh_vram;

	w100_initialize(sc, geom);

#if NWSDISPLAY > 0
	if (w100_console.is_console) {
		struct w100_wsscreen_descr *descr = w100_console.descr;
		struct w100_screen *scr;
		struct rasops_info *ri;
		long defattr;

		error = w100_new_screen(sc, descr->depth, &scr);
		if (error) {
			aprint_error_dev(sc->dev,
			    "unable to create new screen (errno=%d)", error);
			return;
		}

		ri = &scr->rinfo;
		ri->ri_hw = (void *)scr;
		ri->ri_bits = scr->buf_va;
		w100_setup_rasops(sc, ri, descr, geom);

		/* assumes 16 bpp */
		(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);

		sc->active = scr;

		wsdisplay_cnattach(&descr->c, ri, ri->ri_ccol, ri->ri_crow,
		    defattr);

		aprint_normal_dev(sc->dev, "console\n");
	}
#endif
}

int
w100_cnattach(struct w100_wsscreen_descr *descr,
    const struct w100_panel_geometry *geom)
{

	w100_console.descr = descr;
	w100_console.geom = geom;
	w100_console.is_console = 1;

	return 0;
}

/*
 * Create and initialize a new screen buffer.
 */
static int
w100_new_screen(struct w100_softc *sc, int depth, struct w100_screen **scrpp)
{
	struct w100_screen *scr = NULL;
	int error = 0;

	scr = malloc(sizeof(*scr), M_DEVBUF, M_NOWAIT);
	if (scr == NULL)
		return ENOMEM;

	memset(scr, 0, sizeof(*scr));

	scr->buf_va = (u_char *)bus_space_vaddr(sc->iot, sc->ioh_vram);
	scr->depth = depth;

	LIST_INSERT_HEAD(&sc->screens, scr, link);
	sc->n_screens++;

	*scrpp = scr;

	return error;
}

#if NWSDISPLAY > 0
/*
 * Initialize rasops for a screen, as well as struct wsscreen_descr if this
 * is the first screen creation.
 */
static void
w100_setup_rasops(struct w100_softc *sc, struct rasops_info *rinfo,
    struct w100_wsscreen_descr *descr, const struct w100_panel_geometry *geom)
{

	rinfo->ri_flg = descr->flags;
	rinfo->ri_depth = descr->depth;
	rinfo->ri_width = sc->display_width;
	rinfo->ri_height = sc->display_height;
	rinfo->ri_stride = rinfo->ri_width * rinfo->ri_depth / 8;
#ifdef notyet
	rinfo->ri_wsfcookie = -1;	/* XXX */
#endif

	/* swap B and R */
	if (descr->depth == 16) {
		rinfo->ri_rnum = 5;
		rinfo->ri_rpos = 11;
		rinfo->ri_gnum = 6;
		rinfo->ri_gpos = 5;
		rinfo->ri_bnum = 5;
		rinfo->ri_bpos = 0;
	}

	if (descr->c.nrows == 0) {
		/* get rasops to compute screen size the first time */
		rasops_init(rinfo, 100, 100);
	} else {
		rasops_init(rinfo, descr->c.nrows, descr->c.ncols);
	}

	descr->c.nrows = rinfo->ri_rows;
	descr->c.ncols = rinfo->ri_cols;
	descr->c.capabilities = rinfo->ri_caps;
	descr->c.textops = &rinfo->ri_ops;
}
#endif

/*
 * Power management
 */
void
w100_suspend(struct w100_softc *sc)
{

	if (sc->active) {
		/* XXX not yet */
	}
}

void
w100_resume(struct w100_softc *sc)
{

	if (sc->active) {
		w100_initialize(sc, sc->geometry);
		/* XXX: and more */
	}
}

void
w100_power(int why, void *v)
{
	struct w100_softc *sc = v;

	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		w100_suspend(sc);
		break;

	case PWR_RESUME:
		w100_resume(sc);
		break;
	}
}

/*
 * Initialize struct wsscreen_descr based on values calculated by
 * raster operation subsystem.
 */
#if 0
int
w100_setup_wsscreen(struct w100_wsscreen_descr *descr,
    const struct w100_panel_geometry *geom,
    const char *fontname)
{
	int width = geom->panel_width;
	int height = geom->panel_height;
	int cookie = -1;
	struct rasops_info rinfo;

	memset(&rinfo, 0, sizeof rinfo);

	if (fontname) {
		wsfont_init();
		cookie = wsfont_find(fontname, 0, 0, 0,
		    WSDISPLAY_FONTORDER_L2R, WSDISPLAY_FONTORDER_L2R);
		if (cookie < 0 ||
		    wsfont_lock(cookie, &rinfo.ri_font))
			return -1;
	}
	else {
		/* let rasops_init() choose any font */
	}

	/* let rasops_init calculate # of cols and rows in character */
	rinfo.ri_flg = 0;
	rinfo.ri_depth = descr->depth;
	rinfo.ri_bits = NULL;
	rinfo.ri_width = width;
	rinfo.ri_height = height;
	rinfo.ri_stride = width * rinfo.ri_depth / 8;
	rinfo.ri_wsfcookie = cookie;

	rasops_init(&rinfo, 100, 100);

	descr->c.nrows = rinfo.ri_rows;
	descr->c.ncols = rinfo.ri_cols;
	descr->c.capabilities = rinfo.ri_caps;

	return cookie;
}
#endif

int
w100_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{

	return 0;
}

int
w100_alloc_screen(void *v, const struct wsscreen_descr *_type,
    void **cookiep, int *curxp, int *curyp, long *attrp)
{
	struct w100_softc *sc = v;
	struct w100_screen *scr;
	const struct w100_wsscreen_descr *type =
		(const struct w100_wsscreen_descr *)_type;
	int error;

	if (sc->n_screens > 0)
		return ENOMEM;

	error = w100_new_screen(sc, type->depth, &scr);
	if (error)
		return error;

	/*
	 * initialize raster operation for this screen.
	 */
	scr->rinfo.ri_flg = 0;
	scr->rinfo.ri_depth = type->depth;
	scr->rinfo.ri_bits = scr->buf_va;
	scr->rinfo.ri_width = sc->display_width;
	scr->rinfo.ri_height = sc->display_height;
	scr->rinfo.ri_stride = scr->rinfo.ri_width * scr->rinfo.ri_depth / 8;
	scr->rinfo.ri_wsfcookie = -1;	/* XXX */

	rasops_init(&scr->rinfo, type->c.nrows, type->c.ncols);

	(*scr->rinfo.ri_ops.allocattr)(&scr->rinfo, 0, 0, 0, attrp);

	*cookiep = scr;
	*curxp = 0;
	*curyp = 0;

	return 0;
}

void
w100_free_screen(void *v, void *cookie)
{
	struct w100_softc *sc = v;
	struct w100_screen *scr = cookie;

	LIST_REMOVE(scr, link);
	sc->n_screens--;
}

int
w100_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct w100_softc *sc = v;
	struct w100_screen *scr = sc->active;  /* ??? */
	struct wsdisplay_fbinfo *wsdisp_info;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_UNKNOWN;
		return 0;

	case WSDISPLAYIO_GINFO:
		wsdisp_info = (struct wsdisplay_fbinfo *)data;
		wsdisp_info->height = sc->display_height;
		wsdisp_info->width = sc->display_width;
		wsdisp_info->depth = scr->depth;
		wsdisp_info->cmsize = 0;
		return 0;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = scr->rinfo.ri_stride;
		return 0;

	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		return EPASSTHROUGH;	/* XXX Colormap */

	case WSDISPLAYIO_SVIDEO:
		if (*(int *)data == WSDISPLAYIO_VIDEO_ON) {
			/* XXX: turn it on */
		} else {
			/* XXX: start LCD shutdown */
			/* XXX: sleep until interrupt */
		}
		return 0;

	case WSDISPLAYIO_GVIDEO:
		/* XXX: not yet */
		*(u_int *)data = WSDISPLAYIO_VIDEO_ON;
		return 0;

	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
		return EPASSTHROUGH;	/* XXX */
	}

	return EPASSTHROUGH;
}

paddr_t
w100_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct w100_softc *sc = v;
	struct w100_screen *scr = sc->active;  /* ??? */

	if (scr == NULL)
		return -1;

	if (offset < 0 ||
	    offset >= scr->rinfo.ri_stride * scr->rinfo.ri_height)
		return -1;

	return arm_btop(W100_EXTMEM_ADDRESS + offset);
}


static void
w100_cursor(void *cookie, int on, int row, int col)
{
	struct w100_screen *scr = cookie;

	(*scr->rinfo.ri_ops.cursor)(&scr->rinfo, on, row, col);
}

static int
w100_mapchar(void *cookie, int c, unsigned int *cp)
{
	struct w100_screen *scr = cookie;

	return (*scr->rinfo.ri_ops.mapchar)(&scr->rinfo, c, cp);
}

static void
w100_putchar(void *cookie, int row, int col, u_int uc, long attr)
{
	struct w100_screen *scr = cookie;

	(*scr->rinfo.ri_ops.putchar)(&scr->rinfo, row, col, uc, attr);
}

static void
w100_copycols(void *cookie, int row, int src, int dst, int num)
{
	struct w100_screen *scr = cookie;

	(*scr->rinfo.ri_ops.copycols)(&scr->rinfo, row, src, dst, num);
}

static void
w100_erasecols(void *cookie, int row, int col, int num, long attr)
{
	struct w100_screen *scr = cookie;

	(*scr->rinfo.ri_ops.erasecols)(&scr->rinfo, row, col, num, attr);
}

static void
w100_copyrows(void *cookie, int src, int dst, int num)
{
	struct w100_screen *scr = cookie;

	(*scr->rinfo.ri_ops.copyrows)(&scr->rinfo, src, dst, num);
}

static void
w100_eraserows(void *cookie, int row, int num, long attr)
{
	struct w100_screen *scr = cookie;

	(*scr->rinfo.ri_ops.eraserows)(&scr->rinfo, row, num, attr);
}

static int
w100_alloc_attr(void *cookie, int fg, int bg, int flg, long *attr)
{
	struct w100_screen *scr = cookie;

	return (*scr->rinfo.ri_ops.allocattr)(&scr->rinfo, fg, bg, flg, attr);
}

const struct wsdisplay_emulops w100_emulops = {
	w100_cursor,
	w100_mapchar,
	w100_putchar,
	w100_copycols,
	w100_erasecols,
	w100_copyrows,
	w100_eraserows,
	w100_alloc_attr
};
