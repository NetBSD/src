/*	$NetBSD: ffb.c,v 1.4 2004/03/17 13:58:14 pk Exp $	*/
/*	$OpenBSD: creator.c,v 1.20 2002/07/30 19:48:15 jason Exp $	*/

/*
 * Copyright (c) 2002 Jason L. Wright (jason@thought.net)
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Jason L. Wright
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: ffb.c,v 1.4 2004/03/17 13:58:14 pk Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wscons_raster.h>
#include <dev/rasops/rasops.h>

#include <sparc64/dev/ffbreg.h>
#include <sparc64/dev/ffbvar.h>

struct wsscreen_descr ffb_stdscreen = {
	"std",
	0, 0,	/* will be filled in -- XXX shouldn't, it's global. */
	0,
	0, 0,
	WSSCREEN_REVERSE | WSSCREEN_WSCOLORS
};

const struct wsscreen_descr *ffb_scrlist[] = {
	&ffb_stdscreen,
	/* XXX other formats? */
};

struct wsscreen_list ffb_screenlist = {
	sizeof(ffb_scrlist) / sizeof(struct wsscreen_descr *),
	    ffb_scrlist
};

int	ffb_ioctl(void *, u_long, caddr_t, int, struct proc *);
int	ffb_alloc_screen(void *, const struct wsscreen_descr *, void **,
	    int *, int *, long *);
void	ffb_free_screen(void *, void *);
int	ffb_show_screen(void *, void *, int, void (*cb)(void *, int, int),
	    void *);
paddr_t ffb_mmap(void *, off_t, int);
void	ffb_ras_fifo_wait(struct ffb_softc *, int);
void	ffb_ras_wait(struct ffb_softc *);
void	ffb_ras_init(struct ffb_softc *);
void	ffb_ras_copyrows(void *, int, int, int);
void	ffb_ras_erasecols(void *, int, int, int, long int);
void	ffb_ras_eraserows(void *, int, int, long int);
void	ffb_ras_do_cursor(struct rasops_info *);
void	ffb_ras_fill(struct ffb_softc *);
void	ffb_ras_setfg(struct ffb_softc *, int32_t);

struct wsdisplay_accessops ffb_accessops = {
	ffb_ioctl,
	ffb_mmap,
	ffb_alloc_screen,
	ffb_free_screen,
	ffb_show_screen,
	NULL,	/* load font */
	NULL,	/* scrollback */
	NULL,	/* getchar */
	NULL,	/* burner */
};

void
ffb_attach(struct ffb_softc *sc)
{
	struct wsemuldisplaydev_attach_args waa;
	char *model;
	int btype;
	int maxrow, maxcol;
	char buf[6+1];

	printf(":");

	if (sc->sc_type == FFB_CREATOR) {
		btype = PROM_getpropint(sc->sc_node, "board_type", 0);
		if ((btype & 7) == 3)
			printf(" Creator3D");
		else
			printf(" Creator");
	} else
		printf(" Elite3D");

	model = PROM_getpropstring(sc->sc_node, "model");
	if (model == NULL || strlen(model) == 0)
		model = "unknown";

	printf(", model %s\n", model);

	sc->sc_depth = 24;
	sc->sc_linebytes = 8192;
	sc->sc_height = PROM_getpropint(sc->sc_node, "height", 0);
	sc->sc_width = PROM_getpropint(sc->sc_node, "width", 0);

	sc->sc_rasops.ri_depth = 32;
	sc->sc_rasops.ri_stride = sc->sc_linebytes;
	sc->sc_rasops.ri_flg = RI_CENTER;
	sc->sc_rasops.ri_bits = (void *)bus_space_vaddr(sc->sc_bt,
	    sc->sc_pixel_h);

	sc->sc_rasops.ri_width = sc->sc_width;
	sc->sc_rasops.ri_height = sc->sc_height;
	sc->sc_rasops.ri_hw = sc;

	maxcol = (prom_getoption("screen-#columns", buf, sizeof buf) == 0)
		? strtoul(buf, NULL, 10)
		: 80;

	maxrow = (prom_getoption("screen-#rows", buf, sizeof buf) != 0)
		? strtoul(buf, NULL, 10)
		: 34;

	rasops_init(&sc->sc_rasops, maxrow, maxcol);

	if ((sc->sc_dv.dv_cfdata->cf_flags & FFB_CFFLAG_NOACCEL) == 0) {
		sc->sc_rasops.ri_hw = sc;
		sc->sc_rasops.ri_ops.eraserows = ffb_ras_eraserows;
		sc->sc_rasops.ri_ops.erasecols = ffb_ras_erasecols;
		sc->sc_rasops.ri_ops.copyrows = ffb_ras_copyrows;
		ffb_ras_init(sc);
	}

	ffb_stdscreen.nrows = sc->sc_rasops.ri_rows;
	ffb_stdscreen.ncols = sc->sc_rasops.ri_cols;
	ffb_stdscreen.textops = &sc->sc_rasops.ri_ops;

	if (sc->sc_console) {
		int *ccolp, *crowp;
		long defattr;

		if (romgetcursoraddr(&crowp, &ccolp))
			ccolp = crowp = NULL;
		if (ccolp != NULL)
			sc->sc_rasops.ri_ccol = *ccolp;
		if (crowp != NULL)
			sc->sc_rasops.ri_crow = *crowp;

		sc->sc_rasops.ri_ops.allocattr(&sc->sc_rasops,
		    0, 0, 0, &defattr);

		wsdisplay_cnattach(&ffb_stdscreen, &sc->sc_rasops,
		    sc->sc_rasops.ri_ccol, sc->sc_rasops.ri_crow, defattr);
	}

	waa.console = sc->sc_console;
	waa.scrdata = &ffb_screenlist;
	waa.accessops = &ffb_accessops;
	waa.accesscookie = sc;
	config_found(&sc->sc_dv, &waa, wsemuldisplaydevprint);
}

int
ffb_ioctl(v, cmd, data, flags, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	struct ffb_softc *sc = v;
	struct wsdisplay_fbinfo *wdf;

#ifdef FFBDEBUG
	printf("ffb_ioctl: %s cmd _IO%s%s('%c', %lu)\n",
	       sc->sc_dv.dv_xname,
	       (cmd & IOC_IN) ? "W" : "", (cmd & IOC_OUT) ? "R" : "",
	       (char)IOCGROUP(cmd), cmd & 0xff);
#endif

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_SUN24;
		break;
	case WSDISPLAYIO_SMODE:
		sc->sc_mode = *(u_int *)data;
		break;
	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = sc->sc_height;
		wdf->width  = sc->sc_width;
		wdf->depth  = 32;
		wdf->cmsize = 256; /* XXX */
		break;
#ifdef WSDISPLAYIO_LINEBYTES
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_linebytes;
		break;
#endif
	case WSDISPLAYIO_GETCMAP:
		break;/* XXX */

	case WSDISPLAYIO_PUTCMAP:
		break;/* XXX */

	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
	default:
		return -1; /* not supported yet */
        }

	return (0);
}

int
ffb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct ffb_softc *sc = v;

	if (sc->sc_nscreens > 0)
		return (ENOMEM);

	*cookiep = &sc->sc_rasops;
	*curyp = 0;
	*curxp = 0;

	sc->sc_rasops.ri_ops.allocattr(&sc->sc_rasops, 0, 0, 0, attrp);

	sc->sc_nscreens++;
	return (0);
}

void
ffb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct ffb_softc *sc = v;

	sc->sc_nscreens--;
}

int
ffb_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb)(void *, int, int);
	void *cbarg;
{
	return (0);
}

paddr_t
ffb_mmap(vsc, off, prot)
	void *vsc;
	off_t off;
	int prot;
{
	struct ffb_softc *sc = vsc;
	int i;

	switch (sc->sc_mode) {
	case WSDISPLAYIO_MODE_MAPPED:
		for (i = 0; i < sc->sc_nreg; i++) {
			/* Before this set? */
			if (off < sc->sc_addrs[i])
				continue;
			/* After this set? */
			if (off >= (sc->sc_addrs[i] + sc->sc_sizes[i]))
				continue;

			return (bus_space_mmap(sc->sc_bt, sc->sc_addrs[i],
			    off - sc->sc_addrs[i], prot, BUS_SPACE_MAP_LINEAR));
		}
		break;
#ifdef WSDISPLAYIO_MODE_DUMBFB
	case WSDISPLAYIO_MODE_DUMBFB:
		if (sc->sc_nreg < FFB_REG_DFB24)
			break;
		if (off >= 0 && off < sc->sc_sizes[FFB_REG_DFB24])
			return (bus_space_mmap(sc->sc_bt,
			    sc->sc_addrs[FFB_REG_DFB24], off, prot,
			    BUS_SPACE_MAP_LINEAR));
		break;
#endif
	}

	return (-1);
}

void
ffb_ras_fifo_wait(sc, n)
	struct ffb_softc *sc;
	int n;
{
	int32_t cache = sc->sc_fifo_cache;

	if (cache < n) {
		do {
			cache = FBC_READ(sc, FFB_FBC_UCSR);
			cache = (cache & FBC_UCSR_FIFO_MASK) - 8;
		} while (cache < n);
	}
	sc->sc_fifo_cache = cache - n;
}

void
ffb_ras_wait(sc)
	struct ffb_softc *sc;
{
	u_int32_t ucsr, r;

	while (1) {
		ucsr = FBC_READ(sc, FFB_FBC_UCSR);
		if ((ucsr & (FBC_UCSR_FB_BUSY|FBC_UCSR_RP_BUSY)) == 0)
			break;
		r = ucsr & (FBC_UCSR_READ_ERR | FBC_UCSR_FIFO_OVFL);
		if (r != 0)
			FBC_WRITE(sc, FFB_FBC_UCSR, r);
	}
}

void
ffb_ras_init(sc)
	struct ffb_softc *sc;
{
	ffb_ras_fifo_wait(sc, 7);
	FBC_WRITE(sc, FFB_FBC_PPC,
	    FBC_PPC_VCE_DIS | FBC_PPC_TBE_OPAQUE |
	    FBC_PPC_APE_DIS | FBC_PPC_CS_CONST);
	FBC_WRITE(sc, FFB_FBC_FBC,
	    FFB_FBC_WB_A | FFB_FBC_RB_A | FFB_FBC_SB_BOTH |
	    FFB_FBC_XE_OFF | FFB_FBC_RGBE_MASK);
	FBC_WRITE(sc, FFB_FBC_ROP, FBC_ROP_NEW);
	FBC_WRITE(sc, FFB_FBC_DRAWOP, FBC_DRAWOP_RECTANGLE);
	FBC_WRITE(sc, FFB_FBC_PMASK, 0xffffffff);
	FBC_WRITE(sc, FFB_FBC_FONTINC, 0x10000);
	sc->sc_fg_cache = 0;
	FBC_WRITE(sc, FFB_FBC_FG, sc->sc_fg_cache);
	ffb_ras_wait(sc);
}

void
ffb_ras_eraserows(cookie, row, n, attr)
	void *cookie;
	int row, n;
	long int attr;
{
	struct rasops_info *ri = cookie;
	struct ffb_softc *sc = ri->ri_hw;

	if (row < 0) {
		n += row;
		row = 0;
	}
	if (row + n > ri->ri_rows)
		n = ri->ri_rows - row;
	if (n <= 0)
		return;

	ffb_ras_fill(sc);
	ffb_ras_setfg(sc, ri->ri_devcmap[(attr >> 16) & 0xf]);
	ffb_ras_fifo_wait(sc, 4);
	if ((n == ri->ri_rows) && (ri->ri_flg & RI_FULLCLEAR)) {
		FBC_WRITE(sc, FFB_FBC_BY, 0);
		FBC_WRITE(sc, FFB_FBC_BX, 0);
		FBC_WRITE(sc, FFB_FBC_BH, ri->ri_height);
		FBC_WRITE(sc, FFB_FBC_BW, ri->ri_width);
	} else {
		row *= ri->ri_font->fontheight;
		FBC_WRITE(sc, FFB_FBC_BY, ri->ri_yorigin + row);
		FBC_WRITE(sc, FFB_FBC_BX, ri->ri_xorigin);
		FBC_WRITE(sc, FFB_FBC_BH, n * ri->ri_font->fontheight);
		FBC_WRITE(sc, FFB_FBC_BW, ri->ri_emuwidth);
	}
	ffb_ras_wait(sc);
}

void
ffb_ras_erasecols(cookie, row, col, n, attr)
	void *cookie;
	int row, col, n;
	long int attr;
{
	struct rasops_info *ri = cookie;
	struct ffb_softc *sc = ri->ri_hw;

	if ((row < 0) || (row >= ri->ri_rows))
		return;
	if (col < 0) {
		n += col;
		col = 0;
	}
	if (col + n > ri->ri_cols)
		n = ri->ri_cols - col;
	if (n <= 0)
		return;
	n *= ri->ri_font->fontwidth;
	col *= ri->ri_font->fontwidth;
	row *= ri->ri_font->fontheight;

	ffb_ras_fill(sc);
	ffb_ras_setfg(sc, ri->ri_devcmap[(attr >> 16) & 0xf]);
	ffb_ras_fifo_wait(sc, 4);
	FBC_WRITE(sc, FFB_FBC_BY, ri->ri_yorigin + row);
	FBC_WRITE(sc, FFB_FBC_BX, ri->ri_xorigin + col);
	FBC_WRITE(sc, FFB_FBC_BH, ri->ri_font->fontheight);
	FBC_WRITE(sc, FFB_FBC_BW, n - 1);
	ffb_ras_wait(sc);
}

void
ffb_ras_fill(sc)
	struct ffb_softc *sc;
{
	ffb_ras_fifo_wait(sc, 2);
	FBC_WRITE(sc, FFB_FBC_ROP, FBC_ROP_NEW);
	FBC_WRITE(sc, FFB_FBC_DRAWOP, FBC_DRAWOP_RECTANGLE);
	ffb_ras_wait(sc);
}

void
ffb_ras_copyrows(cookie, src, dst, n)
	void *cookie;
	int src, dst, n;
{
	struct rasops_info *ri = cookie;
	struct ffb_softc *sc = ri->ri_hw;

	if (dst == src)
		return;
	if (src < 0) {
		n += src;
		src = 0;
	}
	if ((src + n) > ri->ri_rows)
		n = ri->ri_rows - src;
	if (dst < 0) {
		n += dst;
		dst = 0;
	}
	if ((dst + n) > ri->ri_rows)
		n = ri->ri_rows - dst;
	if (n <= 0)
		return;
	n *= ri->ri_font->fontheight;
	src *= ri->ri_font->fontheight;
	dst *= ri->ri_font->fontheight;

	ffb_ras_fifo_wait(sc, 8);
	FBC_WRITE(sc, FFB_FBC_ROP, FBC_ROP_OLD | (FBC_ROP_OLD << 8));
	FBC_WRITE(sc, FFB_FBC_DRAWOP, FBC_DRAWOP_VSCROLL);
	FBC_WRITE(sc, FFB_FBC_BY, ri->ri_yorigin + src);
	FBC_WRITE(sc, FFB_FBC_BX, ri->ri_xorigin);
	FBC_WRITE(sc, FFB_FBC_DY, ri->ri_yorigin + dst);
	FBC_WRITE(sc, FFB_FBC_DX, ri->ri_xorigin);
	FBC_WRITE(sc, FFB_FBC_BH, n);
	FBC_WRITE(sc, FFB_FBC_BW, ri->ri_emuwidth);
	ffb_ras_wait(sc);
}

void
ffb_ras_setfg(sc, fg)
	struct ffb_softc *sc;
	int32_t fg;
{
	ffb_ras_fifo_wait(sc, 1);
	if (fg == sc->sc_fg_cache)
		return;
	sc->sc_fg_cache = fg;
	FBC_WRITE(sc, FFB_FBC_FG, fg);
	ffb_ras_wait(sc);
}
