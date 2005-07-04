/*	$NetBSD: ffb.c,v 1.19 2005/07/04 22:18:37 jdc Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: ffb.c,v 1.19 2005/07/04 22:18:37 jdc Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <machine/vmparam.h>

#include <dev/wscons/wsconsio.h>
#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>

#include <sparc64/dev/ffbreg.h>
#include <sparc64/dev/ffbvar.h>

#include <dev/wsfont/wsfont.h>

#ifndef WS_DEFAULT_BG
/* Sun -> background should be white */
#define WS_DEFAULT_BG 0xf
#endif

extern struct cfdriver ffb_cd;

struct wsscreen_descr ffb_stdscreen = {
	"sunffb",
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

static struct ffb_screen ffb_console_screen;

int	ffb_ioctl(void *, u_long, caddr_t, int, struct proc *);
static int ffb_blank(struct ffb_softc *, u_long, u_int *);
paddr_t ffb_mmap(void *, off_t, int);
void	ffb_ras_fifo_wait(struct ffb_softc *, int);
void	ffb_ras_wait(struct ffb_softc *);
void	ffb_ras_init(struct ffb_softc *);
void	ffb_ras_copyrows(void *, int, int, int);
void	ffb_ras_copycols(void *, int, int, int, int);
void	ffb_ras_erasecols(void *, int, int, int, long int);
void	ffb_ras_eraserows(void *, int, int, long int);
void	ffb_ras_do_cursor(struct rasops_info *);
void	ffb_ras_fill(struct ffb_softc *);
void	ffb_ras_setfg(struct ffb_softc *, int32_t);

int	ffb_alloc_screen(void *, const struct wsscreen_descr *, void **, 
	    int *, int *, long *);
void	ffb_free_screen(void *, void *);
int	ffb_show_screen(void *, void *, int, void (*)(void *, int, int),
	    void *);
void	ffb_switch_screen(struct ffb_softc *);
void	ffb_restore_screen(struct ffb_screen *, 
	    const struct wsscreen_descr *, u_int16_t *);
void	ffb_clearscreen(struct ffb_softc *);
int	ffb_load_font(void *, void *, struct wsdisplay_font *);
void	ffb_init_screen(struct ffb_softc *, struct ffb_screen *, int, 
	    long *);
int	ffb_allocattr(void *, int, int, int, long *);
void	ffb_putchar(void *, int, int, u_int, long);
void	ffb_cursor(void *, int, int, int);

/* frame buffer generic driver */   
static void ffbfb_unblank(struct device*);
dev_type_open(ffbfb_open);
dev_type_close(ffbfb_close);
dev_type_ioctl(ffbfb_ioctl);
dev_type_mmap(ffbfb_mmap);

static struct fbdriver ffb_fbdriver = {
        ffbfb_unblank, ffbfb_open, ffbfb_close, ffbfb_ioctl, nopoll,
	ffbfb_mmap, nokqfilter
};

struct wsdisplay_accessops ffb_accessops = {
	ffb_ioctl,
	ffb_mmap,
	ffb_alloc_screen,
	ffb_free_screen,
	ffb_show_screen,
	NULL,	/* load font */
	NULL,	/* pollc */
	NULL,	/* getwschar */
	NULL,	/* putwschar */
	NULL,	/* scroll */
};

void
ffb_attach(struct ffb_softc *sc)
{
	struct wsemuldisplaydev_attach_args waa;
	struct rasops_info *ri;
	long defattr;
	const char *model;
	int btype;
	int maxrow, maxcol;
	u_int blank = WSDISPLAYIO_VIDEO_ON;
	char buf[6+1];

	printf(":");
	
	sc->putchar = NULL;
	sc->copycols = NULL;
	
	if (sc->sc_type == FFB_CREATOR) {
		btype = prom_getpropint(sc->sc_node, "board_type", 0);
		if ((btype & 7) == 3)
			printf(" Creator3D");
		else
			printf(" Creator");
	} else
		printf(" Elite3D");

	model = prom_getpropstring(sc->sc_node, "model");
	if (model == NULL || strlen(model) == 0)
		model = "unknown";

	sc->sc_depth = 24;
	sc->sc_linebytes = 8192;
	sc->sc_height = prom_getpropint(sc->sc_node, "height", 0);
	sc->sc_width = prom_getpropint(sc->sc_node, "width", 0);

	maxcol = (prom_getoption("screen-#columns", buf, sizeof buf) == 0)
		? strtoul(buf, NULL, 10)
		: 80;

	maxrow = (prom_getoption("screen-#rows", buf, sizeof buf) != 0)
		? strtoul(buf, NULL, 10)
		: 34;

	ffb_ras_init(sc);

	/* collect DAC version, as Elite3D cursor enable bit is reversed */
	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_GVERS);
	sc->sc_dacrev = DAC_READ(sc, FFB_DAC_VALUE) >> 28;

	if (sc->sc_type == FFB_AFB)
		sc->sc_dacrev = 10;
	printf(", model %s, dac %u\n", model, sc->sc_dacrev);

	ffb_blank(sc, WSDISPLAYIO_SVIDEO, &blank);

	sc->sc_accel = ((sc->sc_dv.dv_cfdata->cf_flags & FFB_CFFLAG_NOACCEL) == 
	    0);
		
	wsfont_init();

	/* we mess with ffb_console_screen only once */
	if (sc->sc_console) {
		ffb_init_screen(sc, &ffb_console_screen, 1, &defattr);
		ffb_console_screen.active = 1;
		sc->active = &ffb_console_screen;
	}
	ri = &ffb_console_screen.ri;

	ffb_stdscreen.nrows = ri->ri_rows;
	ffb_stdscreen.ncols = ri->ri_cols;
	ffb_stdscreen.textops = &ri->ri_ops;
	ffb_stdscreen.capabilities = ri->ri_caps;
	
	sc->sc_fb.fb_driver = &ffb_fbdriver;
	sc->sc_fb.fb_type.fb_cmsize = 0;
	sc->sc_fb.fb_type.fb_size = maxrow * sc->sc_linebytes;
	sc->sc_fb.fb_type.fb_type = FBTYPE_CREATOR;
	sc->sc_fb.fb_type.fb_width = sc->sc_width;
	sc->sc_fb.fb_type.fb_depth = sc->sc_depth;
	sc->sc_fb.fb_type.fb_height = sc->sc_height;
	sc->sc_fb.fb_device = &sc->sc_dv;
	fb_attach(&sc->sc_fb, sc->sc_console);

	if (sc->sc_console) {
		wsdisplay_cnattach(&ffb_stdscreen, ri, 0, 0, defattr);
	}

	ffb_clearscreen(sc);
	
	waa.console = sc->sc_console;
	waa.scrdata = &ffb_screenlist;
	waa.accessops = &ffb_accessops;
	waa.accesscookie = sc;
	config_found(&sc->sc_dv, &waa, wsemuldisplaydevprint);
}

int
ffb_ioctl(void *v, u_long cmd, caddr_t data, int flags, struct proc *p)
{
	struct ffb_softc *sc = v;
	struct wsdisplay_fbinfo *wdf;
	struct ffb_screen *ms = sc->active;

#ifdef FFBDEBUG
	printf("ffb_ioctl: %s cmd _IO%s%s('%c', %lu)\n",
	       sc->sc_dv.dv_xname,
	       (cmd & IOC_IN) ? "W" : "", (cmd & IOC_OUT) ? "R" : "",
	       (char)IOCGROUP(cmd), cmd & 0xff);
#endif

	switch (cmd) {
	case FBIOGTYPE:
		*(struct fbtype *)data = sc->sc_fb.fb_type;
		break;
	case FBIOGATTR:
#define fba ((struct fbgattr *)data)
		fba->real_type = sc->sc_fb.fb_type.fb_type;
		fba->owner = 0; 	/* XXX ??? */
		fba->fbtype = sc->sc_fb.fb_type;
		fba->sattr.flags = 0;
		fba->sattr.emu_type = sc->sc_fb.fb_type.fb_type;
		fba->sattr.dev_specific[0] = -1;
		fba->emu_types[0] = sc->sc_fb.fb_type.fb_type;
		fba->emu_types[1] = -1;
#undef fba
		break; 

	case FBIOGETCMAP:
	case FBIOPUTCMAP:
		return EIO;

	case FBIOGVIDEO:
	case FBIOSVIDEO:
		return ffb_blank(sc, cmd == FBIOGVIDEO?
		    WSDISPLAYIO_GVIDEO : WSDISPLAYIO_SVIDEO,
		    (u_int *)data);
		break;
	case FBIOGCURSOR:
	case FBIOSCURSOR:
		/* the console driver is not using the hardware cursor */
		break;
	case FBIOGCURPOS:
		printf("%s: FBIOGCURPOS not implemented\n", sc->sc_dv.dv_xname);
		return EIO;
	case FBIOSCURPOS:
		printf("%s: FBIOSCURPOS not implemented\n", sc->sc_dv.dv_xname);
		return EIO;
	case FBIOGCURMAX:
		printf("%s: FBIOGCURMAX not implemented\n", sc->sc_dv.dv_xname);
		return EIO;

	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_SUNFFB;
		break;
	case WSDISPLAYIO_SMODE:
		{ 
			if (sc->sc_mode != *(u_int *)data) {
				sc->sc_mode = *(u_int *)data;
				if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
					ffb_ras_init(sc);		
					ffb_restore_screen(ms, ms->type, 
					    ms->chars);
					ffb_cursor(ms, ms->cursoron, 
					    ms->cursorrow, 
					    ms->cursorcol);
				}
			}
		}		
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
		return(ffb_blank(sc, cmd, (u_int *)data));
		break;
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
		return EIO; /* not supported yet */
	default:
		return EPASSTHROUGH;
        }

	return (0);
}

/* blank/unblank the screen */
static int
ffb_blank(struct ffb_softc *sc, u_long cmd, u_int *data)
{
	u_int val;

	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_GSBLANK);
	val = DAC_READ(sc, FFB_DAC_VALUE);

	switch (cmd) {
	case WSDISPLAYIO_GVIDEO:
		*data = val & 1;
		return(0);
		break;
	case WSDISPLAYIO_SVIDEO:
		if (*data == WSDISPLAYIO_VIDEO_OFF)
			val &= ~1;
		else if (*data == WSDISPLAYIO_VIDEO_ON)
			val |= 1;
		else
			return(EINVAL);
		break;
	default:
		return(EINVAL);
	}

	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_GSBLANK);
	DAC_WRITE(sc, FFB_DAC_VALUE, val);

	return(0);
}

paddr_t
ffb_mmap(void *vsc, off_t off, int prot)
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
ffb_ras_fifo_wait(struct ffb_softc *sc, int n)
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
ffb_ras_wait(struct ffb_softc *sc)
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
ffb_ras_init(struct ffb_softc *sc)
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
ffb_ras_eraserows(void *cookie, int row, int n, long attr)
{
	struct rasops_info *ri = cookie;
	struct ffb_screen *scr = ri->ri_hw;
	struct ffb_softc *sc = scr->sc;;

	int start, end, i;
	
	start = ri->ri_cols * row;
	end = ri->ri_cols * (row + n);
	
	for (i = start; i < end; i++) {
		scr->attrs[i] = attr;
		scr->chars[i] = 0x20;
	}

	if ((scr->active) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
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
}

void
ffb_ras_erasecols(void *cookie, int row, int col, int n, long attr)
{
	struct rasops_info *ri = cookie;
	struct ffb_screen *scr = ri->ri_hw;
	struct ffb_softc *sc = scr->sc;;

	int start = col + row * ri->ri_cols;
	int end = start + n, i;
	
	for (i = start; i < end; i++) {
		scr->attrs[i] = attr;
		scr->chars[i] = 0x20;
	}
	if ((scr->active) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {

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
}

void
ffb_ras_fill(struct ffb_softc *sc)
{
	ffb_ras_fifo_wait(sc, 2);
	FBC_WRITE(sc, FFB_FBC_ROP, FBC_ROP_NEW);
	FBC_WRITE(sc, FFB_FBC_DRAWOP, FBC_DRAWOP_RECTANGLE);
	ffb_ras_wait(sc);
}

void
ffb_ras_copyrows(void *cookie, int src, int dst, int n)
{
	struct rasops_info *ri = cookie;
	struct ffb_screen *scr = ri->ri_hw;
	struct ffb_softc *sc = scr->sc;

	int from, to, len;
	
	from = ri->ri_cols * src;
	to = ri->ri_cols * dst;
	len = ri->ri_cols * n;
	
	memmove(&scr->attrs[to], &scr->attrs[from], len * sizeof(long));
	memmove(&scr->chars[to], &scr->chars[from], len * sizeof(uint16_t));
	
	if ((scr->active) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {

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
}

void
ffb_ras_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct ffb_screen *scr = ri->ri_hw;
	struct ffb_softc *sc = scr->sc;
	int from = srccol + row * ri->ri_cols;
	int to = dstcol + row * ri->ri_cols;
	
	memmove(&scr->attrs[to], &scr->attrs[from], ncols * sizeof(long));
	memmove(&scr->chars[to], &scr->chars[from], ncols * sizeof(uint16_t));

	if ((scr->active) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		ffb_ras_wait(sc);
		sc->copycols(cookie, row, srccol, dstcol, ncols);
	}
}

void
ffb_ras_setfg(struct ffb_softc *sc, int32_t fg)
{
	ffb_ras_fifo_wait(sc, 1);
	if (fg == sc->sc_fg_cache)
		return;
	sc->sc_fg_cache = fg;
	FBC_WRITE(sc, FFB_FBC_FG, fg);
	ffb_ras_wait(sc);
}

/* frame buffer generic driver support functions */   
static void
ffbfb_unblank(struct device *dev)
{
	/* u_int on = 1; */

	if (dev && dev->dv_xname)
		printf("%s: ffbfb_unblank\n", dev->dv_xname);
	else
		printf("ffbfb_unblank(%p)n", dev);
	/* ffb_blank((struct ffb_softc*)dev, WSDISPLAYIO_SVIDEO, &on); */
}

int
ffbfb_open(dev_t dev, int flags, int mode, struct proc *p)
{
	int unit = minor(dev);

	if (unit >= ffb_cd.cd_ndevs || ffb_cd.cd_devs[unit] == NULL)
		return ENXIO;

	return 0;
}

int
ffbfb_close(dev_t dev, int flags, int mode, struct proc *p)
{
	return 0;
}

int
ffbfb_ioctl(dev_t dev, u_long cmd, caddr_t data, int flags, struct proc *p)
{
	struct ffb_softc *sc = ffb_cd.cd_devs[minor(dev)];

	return ffb_ioctl(sc, cmd, data, flags, p);
}

paddr_t
ffbfb_mmap(dev_t dev, off_t off, int prot)
{
	struct ffb_softc *sc = ffb_cd.cd_devs[minor(dev)];
	uint64_t size;
	int i, reg;
	off_t o;

	/*
	 * off is a magic cookie (see xfree86/drivers/sunffb/ffb.h),
	 * which we map to an index into the "reg" property, and use
	 * our copy of the firmware data as arguments for the real
	 * mapping.
	 */
	static struct { unsigned long voff; int reg; } map[] = {
		{ 0x00000000, FFB_REG_SFB8R },
		{ 0x00400000, FFB_REG_SFB8G },
		{ 0x00800000, FFB_REG_SFB8B },
		{ 0x00c00000, FFB_REG_SFB8X },
		{ 0x01000000, FFB_REG_SFB32 },
		{ 0x02000000, FFB_REG_SFB64  },
		{ 0x04000000, FFB_REG_FBC },
		{ 0x04004000, FFB_REG_DFB8R },
		{ 0x04404000, FFB_REG_DFB8G },
		{ 0x04804000, FFB_REG_DFB8B },
		{ 0x04c04000, FFB_REG_DFB8X },
		{ 0x05004000, FFB_REG_DFB24 },
		{ 0x06004000, FFB_REG_DFB32 },
		{ 0x07004000, FFB_REG_DFB422A },
		{ 0x0bc06000, FFB_REG_DAC },
		{ 0x0bc08000, FFB_REG_PROM },
		{ 0x0bc18000, 0 }
	};

	/* special value "FFB_EXP_VOFF" - not backed by any "reg" entry */
	if (off == 0x0bc18000)
		return bus_space_mmap(sc->sc_bt, sc->sc_addrs[FFB_REG_PROM],
		    0x00200000, prot, BUS_SPACE_MAP_LINEAR);
		    
	/* 
	 * FFB_VOFF_FBC_KREGS - used by afbinit to upload firmware. We should 
	 * probably mmap them only on afb boards 
	 */
	if ((off >= 0x0bc04000) && (off < 0x0bc06000))
		return bus_space_mmap(sc->sc_bt, sc->sc_addrs[FFB_REG_PROM], 
		    0x00610000 + (off - 0x0bc04000), prot, 
		    BUS_SPACE_MAP_LINEAR);
		    
#define NELEMS(arr) (sizeof(arr)/sizeof((arr)[0]))

	/* the map is ordered by voff */
	for (i = 0; i < NELEMS(map)-1; i++) {
		reg = map[i].reg;
		/* the number of entries in reg seems to vary */
		if (reg < sc->sc_nreg) {
			size = min((map[i + 1].voff - map[i].voff), 
			    sc->sc_sizes[reg]);
			if ((off >= map[i].voff) && 
			    (off < (map[i].voff + size))) {
				o = off - map[i].voff;
				return bus_space_mmap(sc->sc_bt, 
				    sc->sc_addrs[reg], o, prot, 
				    BUS_SPACE_MAP_LINEAR);
			}
		}
	}

	return -1;
}

void
ffb_clearscreen(struct ffb_softc *sc)
{
	struct rasops_info *ri = &ffb_console_screen.ri;
	ffb_ras_fill(sc);
	ffb_ras_setfg(sc, ri->ri_devcmap[WS_DEFAULT_BG]);
	ffb_ras_fifo_wait(sc, 4);
	FBC_WRITE(sc, FFB_FBC_BY, 0);
	FBC_WRITE(sc, FFB_FBC_BX, 0);
	FBC_WRITE(sc, FFB_FBC_BH, ri->ri_height);
	FBC_WRITE(sc, FFB_FBC_BW, ri->ri_width);
}

void
ffb_switch_screen(struct ffb_softc *sc)
{
	struct ffb_screen *scr, *oldscr;

	scr = sc->wanted;
	if (!scr) {
		printf("ffb_switch_screen: disappeared\n");
		(*sc->switchcb)(sc->switchcbarg, EIO, 0);
		return;
	}
	oldscr = sc->active; /* can be NULL! */
#ifdef DIAGNOSTIC
	if (oldscr) {
		if (!oldscr->active)
			panic("ffb_switch_screen: not active");
	}
#endif
	if (scr == oldscr)
		return;

#ifdef DIAGNOSTIC
	if (scr->active)
		panic("ffb_switch_screen: active");
#endif

	if (oldscr)
		oldscr->active = 0;
#ifdef notyet
	if (sc->currenttype != type) {
		ffb_set_screentype(sc, type);
		sc->currenttype = type;
	}
#endif

	/* Clear the entire screen. */		

	scr->active = 1;
	ffb_restore_screen(scr, &ffb_stdscreen, scr->chars);

	sc->active = scr;
	
	scr->ri.ri_ops.cursor(scr, scr->cursoron, scr->cursorrow, 
	    scr->cursorcol);

	sc->wanted = 0;
	if (sc->switchcb)
		(*sc->switchcb)(sc->switchcbarg, 0, 0);
}

void
ffb_restore_screen(struct ffb_screen *scr,
    const struct wsscreen_descr *type, u_int16_t *mem)
{
	int i, j, offset = 0;
	uint16_t *charptr = scr->chars;
	long *attrptr = scr->attrs;
	
	ffb_clearscreen(scr->sc);
	ffb_ras_wait(scr->sc);
	for (i = 0; i < scr->ri.ri_rows; i++) {
		for (j = 0; j < scr->ri.ri_cols; j++) {
			scr->sc->putchar(scr, i, j, charptr[offset], 
			    attrptr[offset]);
			offset++;
		}
	}
	scr->cursordrawn = 0;
}

void
ffb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct ffb_screen *scr = ri->ri_hw;
	struct ffb_softc *sc = scr->sc;
	int x, y, wi, he, coffset;
	
	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;
	
	if ((scr->active) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = scr->cursorcol * wi + ri->ri_xorigin;
		y = scr->cursorrow * he + ri->ri_yorigin;
		
		if (scr->cursordrawn) {
			/* remove cursor */
			coffset = scr->cursorcol + 
			    (scr->cursorrow * ri->ri_cols);
			ffb_ras_wait(sc);
			sc->putchar(cookie, scr->cursorrow, scr->cursorcol, 
			    scr->chars[coffset], scr->attrs[coffset]);
			scr->cursordrawn = 0;
		}
		scr->cursorrow = row;
		scr->cursorcol = col;
		if ((scr->cursoron = on) != 0)
		{
			long attr, revattr;
			x = scr->cursorcol * wi + ri->ri_xorigin;
			y = scr->cursorrow * he + ri->ri_yorigin;
			coffset = col + (row * ri->ri_cols);
			attr = scr->attrs[coffset];
#ifdef FFB_CURSOR_SWAP_COLOURS
			revattr=((attr >> 8 ) & 0x000f0000) | ((attr & 
			    0x000f0000)<<8) | (attr & 0x0000ffff);
#else
			revattr = attr ^ 0xffff0000;
#endif
			ffb_ras_wait(sc);
			sc->putchar(cookie, scr->cursorrow, scr->cursorcol, 
			    scr->chars[coffset], revattr);
			scr->cursordrawn = 1;
		}
	} else {
		scr->cursoron = on;
		scr->cursorrow = row;
		scr->cursorcol = col;
		scr->cursordrawn = 0;
	}
}

void
ffb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct ffb_screen *scr = ri->ri_hw;
	struct ffb_softc *sc = scr->sc;
	int pos;
	
	if ((row >= 0) && (row < ri->ri_rows) && (col >= 0) && 
	     (col < ri->ri_cols)) {
		pos = col + row * ri->ri_cols;
		scr->attrs[pos] = attr;
		scr->chars[pos] = c;

#if 1
		if ((sc->putchar != NULL) && (	scr->active) && 
		    (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
			ffb_ras_wait(sc);
			sc->putchar(cookie, row, col, c, attr);
		}
#else
		if ((scr->active) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
			int fg, bg, uc, i;
			uint8_t *data;
			int x, y, wi,he;
			
			wi = ri->ri_font->fontwidth;
			he = ri->ri_font->fontheight;
		
			if (!CHAR_IN_FONT(c, ri->ri_font))
				return;
			bg = (u_char)ri->ri_devcmap[(attr >> 16) & 0xff];
			fg = (u_char)ri->ri_devcmap[(attr >> 24) & 0xff];
			x = ri->ri_xorigin + col * wi;
			y = ri->ri_yorigin + row * he;
			if (c == 0x20) {
				ffb_rectfill(sc, x, y, wi, he, bg);
			} else {
				uc = c-ri->ri_font->firstchar;
				data = (uint8_t *)ri->ri_font->data + uc * 
				    ri->ri_fontscale;

				ffb_setup_mono(sc, x, y, wi, 1, fg, bg);		
				for (i = 0; i < he; i++) {
					ffb_feed_line(sc, ri->ri_font->stride,
					    data);
					data += ri->ri_font->stride;
				}
				/*ffb_ras_wait(sc);*/
			}
		}
#endif
	}
}

int
ffb_allocattr(void *cookie, int fg, int bg, int flags, long *attrp)
{
	if ((fg == 0) && (bg == 0))
	{
		fg = WS_DEFAULT_FG;
		bg = WS_DEFAULT_BG;
	}
	if (flags & WSATTR_REVERSE) {
		*attrp = (bg & 0xff) << 24 | (fg & 0xff) << 16 | 
		    (flags & 0xff);
	} else
		*attrp = (fg & 0xff) << 24 | (bg & 0xff) << 16 | 
		    (flags & 0xff);
	return 0;
}

void
ffb_init_screen(struct ffb_softc *sc, struct ffb_screen *scr,
    int existing, long *defattr)
{
	struct rasops_info *ri = &scr->ri;
	int cnt;

	scr->sc = sc;
	scr->cursorcol = 0;
	scr->cursorrow = 0;
	scr->cursordrawn=0;
	   
	ri->ri_depth = 32;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_linebytes;
	ri->ri_flg = RI_CENTER;

	ri->ri_bits = bus_space_vaddr(sc->sc_bt, sc->sc_pixel_h);
	
#ifdef DEBUG_FFB
	printf("addr: %08lx\n",(ulong)ri->ri_bits);
#endif
	rasops_init(ri, sc->sc_height/8, sc->sc_width/8);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ffb_allocattr(ri, WS_DEFAULT_FG, WS_DEFAULT_BG, 0, defattr);

	/* 
	 * we allocate both chars and attributes in one chunk, attributes first 
	 * because they have the (potentially) bigger alignment 
	 */
	cnt=ri->ri_rows * ri->ri_cols;
	scr->attrs = (long *)malloc(cnt * (sizeof(long) + sizeof(uint16_t)),
	    M_DEVBUF, M_WAITOK);
	scr->chars = (uint16_t *)&scr->attrs[cnt];

	/* enable acceleration */
	ri->ri_hw = scr;
	ri->ri_ops.copyrows = ffb_ras_copyrows;
	ri->ri_ops.eraserows = ffb_ras_eraserows;
	ri->ri_ops.erasecols = ffb_ras_erasecols;
	ri->ri_ops.cursor = ffb_cursor;
	ri->ri_ops.allocattr = ffb_allocattr;
	if (sc->putchar == NULL)
		sc->putchar = ri->ri_ops.putchar;
		sc->copycols = ri->ri_ops.copycols;
	ri->ri_ops.putchar = ffb_putchar;
	ri->ri_ops.copycols = ffb_ras_copycols;
	
	
	if (existing) {
		scr->active = 1;
	} else {
		scr->active = 0;
	}

	ffb_ras_eraserows(&scr->ri, 0, ri->ri_rows, *defattr);

	LIST_INSERT_HEAD(&sc->screens, scr, next);
}

int
ffb_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
    int *curxp, int *curyp, long *defattrp)
{
	struct ffb_softc *sc = v;
	struct ffb_screen *scr;

	scr = malloc(sizeof(struct ffb_screen), M_DEVBUF, M_WAITOK | M_ZERO);
	ffb_init_screen(sc, scr, 0, defattrp);

	if (sc->active == NULL) {
		scr->active = 1;
		sc->active = scr;
		sc->currenttype = type;
	}

	*cookiep = scr;
	*curxp = scr->cursorcol;
	*curyp = scr->cursorrow;
	return 0;
}

void
ffb_free_screen(void *v, void *cookie)
{
	struct ffb_softc *sc = v;
	struct ffb_screen *scr = cookie;

	LIST_REMOVE(scr, next);
	if (scr != &ffb_console_screen) {
		free(scr->attrs, M_DEVBUF);
		free(scr, M_DEVBUF);
	} else
		panic("ffb_free_screen: console");

	if (sc->active == scr)
		sc->active = 0;
}

int
ffb_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{
	struct ffb_softc *sc = v;
	struct ffb_screen *scr, *oldscr;

	scr = cookie;
	oldscr = sc->active;
	if (scr == oldscr)
		return 0;

	sc->wanted = scr;
	sc->switchcb = cb;
	sc->switchcbarg = cbarg;
	if (cb) {
		callout_reset(&sc->switch_callout, 0,
		    (void(*)(void *))ffb_switch_screen, sc);
		return EAGAIN;
	}

	ffb_switch_screen(sc);
	return 0;
}

