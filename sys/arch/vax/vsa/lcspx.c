/*	$NetBSD: lcspx.c,v 1.8 2007/07/09 20:52:34 ad Exp $ */
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed at Ludd, University of 
 *	Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lcspx.c,v 1.8 2007/07/09 20:52:34 ad Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/kernel.h>

#include <machine/vsbus.h>
#include <machine/sid.h>
#include <machine/cpu.h>

#include <dev/cons.h>

#include <dev/dec/dzreg.h>
#include <dev/dec/dzvar.h>
#include <dev/dec/dzkbdvar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_callbacks.h>
#include <dev/wsfont/wsfont.h>

#include "machine/scb.h"

#include "dzkbd.h"
#include "opt_wsfont.h"

/* Screen hardware defs */
#define SPX_COLS	160	/* char width of screen */
#define SPX_ROWS	68	/* rows of char on screen */
#define SPX_CHEIGHT	15	/* lines a char consists of */
#define SPX_CWIDTH	8	/* cols a char consists of */
#define SPX_NEXTROW	(SPX_COLS * SPX_CHEIGHT * SPX_CWIDTH)
#define	SPX_YWIDTH	1024
#define SPX_XWIDTH	1280

#define	SPXADDR		0x38000000	/* Frame buffer */
#define	SPXSIZE		0x00800000	/* 8MB in size */

static	int lcspx_match(struct device *, struct cfdata *, void *);
static	void lcspx_attach(struct device *, struct device *, void *);

struct	lcspx_softc {
	struct	device ss_dev;
};

CFATTACH_DECL(lcspx, sizeof(struct lcspx_softc),
    lcspx_match, lcspx_attach, NULL, NULL);

static void	lcspx_cursor(void *, int, int, int);
static int	lcspx_mapchar(void *, int, unsigned int *);
static void	lcspx_putchar(void *, int, int, u_int, long);
static void	lcspx_copycols(void *, int, int, int,int);
static void	lcspx_erasecols(void *, int, int, int, long);
static void	lcspx_copyrows(void *, int, int, int);
static void	lcspx_eraserows(void *, int, int, long);
static int	lcspx_allocattr(void *, int, int, int, long *);

const struct wsdisplay_emulops lcspx_emulops = {
	lcspx_cursor,
	lcspx_mapchar,
	lcspx_putchar,
	lcspx_copycols,
	lcspx_erasecols,
	lcspx_copyrows,
	lcspx_eraserows,
	lcspx_allocattr
};

const struct wsscreen_descr lcspx_stdscreen = {
	"160x68", SPX_COLS, SPX_ROWS,
	&lcspx_emulops,
	8, SPX_CHEIGHT,
	WSSCREEN_UNDERLINE|WSSCREEN_REVERSE,
};

const struct wsscreen_descr *_lcspx_scrlist[] = {
	&lcspx_stdscreen,
};

const struct wsscreen_list lcspx_screenlist = {
	sizeof(_lcspx_scrlist) / sizeof(struct wsscreen_descr *),
	_lcspx_scrlist,
};

static	char *lcspxaddr;

static  u_char *qf;

#define QCHAR(c) (c < 32 ? 32 : (c > 127 ? c - 66 : c - 32))
#define QFONT(c,line)	qf[QCHAR(c) * 15 + line]
#define	SPX_ADDR(row, col, line, dot) \
	lcspxaddr[(col * SPX_CWIDTH) + (row * SPX_CHEIGHT * SPX_XWIDTH) + \
	    line * SPX_XWIDTH + dot]


static int	lcspx_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	lcspx_mmap(void *, void *, off_t, int);
static int	lcspx_alloc_screen(void *, const struct wsscreen_descr *,
				      void **, int *, int *, long *);
static void	lcspx_free_screen(void *, void *);
static int	lcspx_show_screen(void *, void *, int,
				     void (*) (void *, int, int), void *);
static void	lcspx_crsr_blink(void *);

const struct wsdisplay_accessops lcspx_accessops = {
	lcspx_ioctl,
	lcspx_mmap,
	lcspx_alloc_screen,
	lcspx_free_screen,
	lcspx_show_screen,
	0 /* load_font */
};

struct	lcspx_screen {
	int	ss_curx;
	int	ss_cury;
	u_char	ss_image[SPX_ROWS][SPX_COLS];	/* Image of current screen */
	u_char	ss_attr[SPX_ROWS][SPX_COLS];	/* Reversed etc... */
};

static	struct lcspx_screen lcspx_conscreen;
static	struct lcspx_screen *curscr;

static	callout_t lcspx_cursor_ch;

int
lcspx_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct vsbus_softc *sc = (void *)parent;
	struct vsbus_attach_args *va = aux;
	char *ch = (char *)va->va_addr;

	if (vax_boardtype != VAX_BTYP_49)
		return 0;

	*ch = 1;
	if ((*ch & 1) == 0)
		return 0;
	*ch = 0;
	if ((*ch & 1) != 0)
		return 0;

	sc->sc_mask = 0x04; /* XXX - should be generated */
	scb_fake(0x120, 0x15);
	return 20;
}

void
lcspx_attach(struct device *parent, struct device *self, void *aux)
{
	struct vsbus_attach_args *va = aux;
	struct wsemuldisplaydev_attach_args aa;
	int fcookie;
	struct wsdisplay_font *console_font;

	printf("\n");
	aa.console = lcspxaddr != NULL;
	if (lcspxaddr == 0) {
		callout_init(&lcspx_cursor_ch, 0);
		lcspxaddr = (void *)vax_map_physmem(va->va_paddr, (SPXSIZE/VAX_NBPG));
	}
	if (lcspxaddr == 0) {
		printf("%s: Couldn't alloc graphics memory.\n", self->dv_xname);
		return;
	}
	curscr = &lcspx_conscreen;

	aa.scrdata = &lcspx_screenlist;
	aa.accessops = &lcspx_accessops;
	if ((fcookie = wsfont_find(NULL, 8, 15, 0,
		WSDISPLAY_FONTORDER_R2L, WSDISPLAY_FONTORDER_L2R)) < 0)
	{
		printf("%s: could not find 8x15 font\n", self->dv_xname);
		return;
	}
	if (wsfont_lock(fcookie, &console_font) != 0) {
		printf("%s: could not lock 8x15 font\n", self->dv_xname);
		return;
	}
	qf = console_font->data;

	/* enable software cursor */
	callout_reset(&lcspx_cursor_ch, hz / 2, lcspx_crsr_blink, NULL);

	config_found(self, &aa, wsemuldisplaydevprint);
}

static	char *cursor;
static	int cur_on;

static void
lcspx_crsr_blink(void *arg)
{
	int i;

	if (cur_on)
		for (i = 0; i < 8; ++i)
			cursor[i] ^= 255;
	callout_reset(&lcspx_cursor_ch, hz / 2, lcspx_crsr_blink, NULL);
}

void
lcspx_cursor(void *id, int on, int row, int col)
{
	struct lcspx_screen *ss = id;
	int i;

	if (ss == curscr) {
		char ch = QFONT(ss->ss_image[ss->ss_cury][ss->ss_curx], 14);

		if (cursor != NULL)
			for (i = 0; i < 8; i++)
				cursor[i] = (ch >> i) & 1;
		cursor = &SPX_ADDR(row, col, 14, 0);
		if ((cur_on = on))
			for (i = 0; i < 8; i++)
				cursor[i] ^= cursor[i];
	}
	ss->ss_curx = col;
	ss->ss_cury = row;
}

int
lcspx_mapchar(void *id, int uni, unsigned int *index)
{
	if (uni < 256) {
		*index = uni;
		return (5);
	}
	*index = ' ';
	return (0);
}

static void
lcspx_putchar(void *id, int row, int col, u_int c, long attr)
{
	struct lcspx_screen *ss = id;
	int i, j;

	c &= 0xff;

	ss->ss_image[row][col] = c;
	ss->ss_attr[row][col] = attr;
	if (ss != curscr)
		return;
	for (i = 0; i < 15; i++) {
		unsigned char ch = QFONT(c, i);
		char dot;

		for (j = 0; j < 8; j++) {
			dot = (ch >> j) & 1;
			if (attr & WSATTR_REVERSE)
				dot = (~dot) & 1;
			SPX_ADDR(row, col, i, j) = dot;
		}
	}
	if (attr & WSATTR_UNDERLINE) {
		char *p = &SPX_ADDR(row, col, i, 0);
		for (i = 0; i < 8; i++)
			p[i] = ~p[i];
	}
}

/*
 * copies columns inside a row.
 */
static void
lcspx_copycols(void *id, int row, int srccol, int dstcol, int ncols)
{
	struct lcspx_screen *ss = id;
	int i;

	bcopy(&ss->ss_image[row][srccol], &ss->ss_image[row][dstcol], ncols);
	bcopy(&ss->ss_attr[row][srccol], &ss->ss_attr[row][dstcol], ncols);
	if (ss != curscr)
		return;
	for (i = 0; i < SPX_CHEIGHT; i++)
		memcpy(&SPX_ADDR(row, dstcol, i, 0),
		    &SPX_ADDR(row,srccol, i, 0), ncols * SPX_CWIDTH);
}

/*
 * Erases a bunch of chars inside one row.
 */
static void
lcspx_erasecols(void *id, int row, int startcol, int ncols, long fillattr)
{
	struct lcspx_screen *ss = id;
	int i;

	bzero(&ss->ss_image[row][startcol], ncols);
	bzero(&ss->ss_attr[row][startcol], ncols);
	if (ss != curscr)
		return;
	for (i = 0; i < SPX_CHEIGHT; i++)
		memset(&SPX_ADDR(row, startcol, i, 0), 0, ncols * SPX_CWIDTH);
}

static void
lcspx_copyrows(void *id, int srcrow, int dstrow, int nrows)
{
	struct lcspx_screen *ss = id;

	bcopy(&ss->ss_image[srcrow][0], &ss->ss_image[dstrow][0],
	    nrows * SPX_COLS);
	bcopy(&ss->ss_attr[srcrow][0], &ss->ss_attr[dstrow][0],
	    nrows * SPX_COLS);
	if (ss != curscr)
		return;
	memcpy(&lcspxaddr[dstrow * SPX_NEXTROW],
	    &lcspxaddr[srcrow * SPX_NEXTROW], nrows * SPX_NEXTROW);
}

static void
lcspx_eraserows(void *id, int startrow, int nrows, long fillattr)
{
	struct lcspx_screen *ss = id;

	bzero(&ss->ss_image[startrow][0], nrows * SPX_COLS);
	bzero(&ss->ss_attr[startrow][0], nrows * SPX_COLS);
	if (ss != curscr)
		return;
	memset(&lcspxaddr[startrow * SPX_NEXTROW], 0, nrows * SPX_NEXTROW);
}

static int
lcspx_allocattr(void *id, int fg, int bg, int flags, long *attrp)
{
	*attrp = flags;
	return 0;
}

int
lcspx_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct wsdisplay_fbinfo *fb = (void *)data;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_SPX;
		break;

	case WSDISPLAYIO_GINFO:
		fb->height = SPX_YWIDTH;
		fb->width = SPX_XWIDTH;
		fb->depth = 1;
		fb->cmsize = 2;
		break;

#if 0
	case WSDISPLAYIO_SVIDEO:
		if (*(u_int *)data == WSDISPLAYIO_VIDEO_ON) {
			curcmd = curc;
		} else {
			curc = curcmd;
			curcmd &= ~(CUR_CMD_FOPA|CUR_CMD_ENPA);
			curcmd |= CUR_CMD_FOPB;
		}
		WRITECUR(CUR_CMD, curcmd);
		break;

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = (curcmd & CUR_CMD_FOPB ?
		    WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON);
		break;
#endif

	default:
		return EPASSTHROUGH;
	}
	return 0;
}

static paddr_t
lcspx_mmap(void *v, void *vs, off_t offset, int prot)
{
	if (offset >= SPXSIZE || offset < 0)
		return -1;
	return (SPXADDR + offset) >> PGSHIFT;
}

int
lcspx_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
    int *curxp, int *curyp, long *defattrp)
{
	*cookiep = malloc(sizeof(struct lcspx_screen), M_DEVBUF, M_WAITOK);
	bzero(*cookiep, sizeof(struct lcspx_screen));
	*curxp = *curyp = *defattrp = 0;
	return 0;
}

void
lcspx_free_screen(void *v, void *cookie)
{
}

int
lcspx_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{
	struct lcspx_screen *ss = cookie;
	int row, col, line;

	if (ss == curscr)
		return (0);

	for (row = 0; row < SPX_ROWS; row++)
		for (line = 0; line < SPX_CHEIGHT; line++) {
			for (col = 0; col < SPX_COLS; col++) {
				u_char s, c = ss->ss_image[row][col];

				if (c < 32)
					c = 32;
				s = QFONT(c, line);
				if (ss->ss_attr[row][col] & WSATTR_REVERSE)
					s ^= 255;
				SPX_ADDR(row, col, line, 0) = s;
				if (ss->ss_attr[row][col] & WSATTR_UNDERLINE)
					SPX_ADDR(row, col, line, 0) = 255;
			}
		}
	cursor = &lcspxaddr[(ss->ss_cury * SPX_CHEIGHT * SPX_COLS) + ss->ss_curx +
	    ((SPX_CHEIGHT - 1) * SPX_COLS)];
	curscr = ss;
	return (0);
}

cons_decl(lcspx);

void
lcspxcninit(struct consdev *cndev)
{
	int fcookie;
	struct wsdisplay_font *console_font;

	callout_init(&lcspx_cursor_ch, 0);

	/* Clear screen */
	memset(lcspxaddr, 0, SPX_XWIDTH * SPX_YWIDTH);

	curscr = &lcspx_conscreen;
	wsdisplay_cnattach(&lcspx_stdscreen, &lcspx_conscreen, 0, 0, 0);
	cn_tab->cn_pri = CN_INTERNAL;
	if ((fcookie = wsfont_find(NULL, 8, 15, 0,
		WSDISPLAY_FONTORDER_R2L, WSDISPLAY_FONTORDER_L2R)) < 0)
	{
		printf("lcspx: could not find 8x15 font\n");
		return;
	}
	if (wsfont_lock(fcookie, &console_font) != 0) {
		printf("lcspx: could not lock 8x15 font\n");
		return;
	}
	qf = console_font->data;

#if NDZKBD > 0
	dzkbd_cnattach(0); /* Connect keyboard and screen together */
#endif
}

/*
 * Called very early to setup the glass tty as console.
 * Because it's called before the VM system is inited, virtual memory
 * for the framebuffer can be stolen directly without disturbing anything.
 */
void
lcspxcnprobe(struct consdev *cndev)
{
	extern vaddr_t virtual_avail;
	extern const struct cdevsw wsdisplay_cdevsw;

	if (vax_boardtype != VAX_BTYP_49)
		return; /* Only for 4000/90 */

	if (vax_confdata & 8)
		return; /* Diagnostic console */
	lcspxaddr = (void *)virtual_avail;
	virtual_avail += SPXSIZE;
	ioaccess((vaddr_t)lcspxaddr, SPXADDR, (SPXSIZE/VAX_NBPG));
	cndev->cn_pri = CN_INTERNAL;
	cndev->cn_dev = makedev(cdevsw_lookup_major(&wsdisplay_cdevsw), 0);
}
