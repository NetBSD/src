/* $NetBSD: hd44780_subr.c,v 1.10.4.1 2007/07/11 20:05:47 mjf Exp $ */

/*
 * Copyright (c) 2002 Dennis I. Chernoivanov
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

/*
 * Subroutines for Hitachi HD44870 style displays
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hd44780_subr.c,v 1.10.4.1 2007/07/11 20:05:47 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/types.h>
#include <sys/ioccom.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <uvm/uvm_extern.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_callbacks.h>

#include <dev/ic/hd44780reg.h>
#include <dev/ic/hd44780var.h>

#define COORD_TO_IDX(x, y)	((y) * sc->sc_cols + (x))
#define COORD_TO_DADDR(x, y)	((y) * HD_ROW2_ADDR + (x))
#define IDX_TO_ROW(idx)		((idx) / sc->sc_cols)
#define IDX_TO_COL(idx)		((idx) % sc->sc_cols)
#define IDX_TO_DADDR(idx)	(IDX_TO_ROW((idx)) * HD_ROW2_ADDR + \
				IDX_TO_COL((idx)))
#define DADDR_TO_ROW(daddr)	((daddr) / HD_ROW2_ADDR)
#define DADDR_TO_COL(daddr)	((daddr) % HD_ROW2_ADDR)
#define DADDR_TO_CHIPDADDR(daddr)	((daddr) % (HD_ROW2_ADDR * 2))
#define DADDR_TO_CHIPNO(daddr)	((daddr) / (HD_ROW2_ADDR * 2))

static void	hlcd_cursor(void *, int, int, int);
static int	hlcd_mapchar(void *, int, unsigned int *);
static void	hlcd_putchar(void *, int, int, u_int, long);
static void	hlcd_copycols(void *, int, int, int,int);
static void	hlcd_erasecols(void *, int, int, int, long);
static void	hlcd_copyrows(void *, int, int, int);
static void	hlcd_eraserows(void *, int, int, long);
static int	hlcd_allocattr(void *, int, int, int, long *);
static void	hlcd_updatechar(struct hd44780_chip *, int, int);
static void	hlcd_redraw(void *);

const struct wsdisplay_emulops hlcd_emulops = {
	hlcd_cursor,
	hlcd_mapchar,
	hlcd_putchar,
	hlcd_copycols,
	hlcd_erasecols,
	hlcd_copyrows,
	hlcd_eraserows,
	hlcd_allocattr
};

static int	hlcd_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	hlcd_mmap(void *, void *, off_t, int);
static int	hlcd_alloc_screen(void *, const struct wsscreen_descr *,
		    void **, int *, int *, long *);
static void	hlcd_free_screen(void *, void *);
static int	hlcd_show_screen(void *, void *, int,
		    void (*) (void *, int, int), void *);

const struct wsdisplay_accessops hlcd_accessops = {
	hlcd_ioctl,
	hlcd_mmap,
	hlcd_alloc_screen,
	hlcd_free_screen,
	hlcd_show_screen,
	0 /* load_font */
};

static void
hlcd_cursor(id, on, row, col)
	void *id;
	int on, row, col;
{
	struct hlcd_screen *hdscr = id;

	hdscr->hlcd_curon = on;
	hdscr->hlcd_curx = col;
	hdscr->hlcd_cury = row;
}

static int
hlcd_mapchar(id, uni, index)
	void *id;
	int uni;
	unsigned int *index;
{
	if (uni < 256) {
		*index = uni;
		return (5);
	}
	*index = ' ';
	return (0);
}

static void
hlcd_putchar(id, row, col, c, attr)
	void *id;
	int row, col;
	u_int c;
	long attr;
{
	struct hlcd_screen *hdscr = id;

	c &= 0xff;
	if (row > 0 && (hdscr->hlcd_sc->sc_flags & (HD_MULTILINE|HD_MULTICHIP)))
		hdscr->image[hdscr->hlcd_sc->sc_cols * row + col] = c;
	else
		hdscr->image[col] = c;
}

/*
 * copies columns inside a row.
 */
static void
hlcd_copycols(id, row, srccol, dstcol, ncols)
	void *id;
	int row, srccol, dstcol, ncols;
{
	struct hlcd_screen *hdscr = id;

	if ((dstcol + ncols - 1) > hdscr->hlcd_sc->sc_cols)
		ncols = hdscr->hlcd_sc->sc_cols - srccol;

	if (row > 0 && (hdscr->hlcd_sc->sc_flags & (HD_MULTILINE|HD_MULTICHIP)))
		bcopy(&hdscr->image[hdscr->hlcd_sc->sc_cols * row + srccol],
		    &hdscr->image[hdscr->hlcd_sc->sc_cols * row + dstcol], ncols);
	else
		bcopy(&hdscr->image[srccol], &hdscr->image[dstcol], ncols);
}


/*
 * Erases a bunch of chars inside one row.
 */
static void
hlcd_erasecols(id, row, startcol, ncols, fillattr)
	void *id;
	int row, startcol, ncols;
	long fillattr;
{
	struct hlcd_screen *hdscr = id;

	if ((startcol + ncols) > hdscr->hlcd_sc->sc_cols)
		ncols = hdscr->hlcd_sc->sc_cols - startcol;

	if (row > 0 && (hdscr->hlcd_sc->sc_flags & (HD_MULTILINE|HD_MULTICHIP)))
		memset(&hdscr->image[hdscr->hlcd_sc->sc_cols * row + startcol],
		    ' ', ncols);
	else
		memset(&hdscr->image[startcol], ' ', ncols);
}


static void
hlcd_copyrows(id, srcrow, dstrow, nrows)
	void *id;
	int srcrow, dstrow, nrows;
{
	struct hlcd_screen *hdscr = id;
	int ncols = hdscr->hlcd_sc->sc_cols;

	if (!(hdscr->hlcd_sc->sc_flags & (HD_MULTILINE|HD_MULTICHIP)))
		return;
	bcopy(&hdscr->image[srcrow * ncols], &hdscr->image[dstrow * ncols],
	    nrows * ncols);
}

static void
hlcd_eraserows(id, startrow, nrows, fillattr)
	void *id;
	int startrow, nrows;
	long fillattr;
{
	struct hlcd_screen *hdscr = id;
	int ncols = hdscr->hlcd_sc->sc_cols;

	memset(&hdscr->image[startrow * ncols], ' ', ncols * nrows);
}


static int
hlcd_allocattr(id, fg, bg, flags, attrp)
	void *id;
	int fg, bg, flags;
	long *attrp;
{
        *attrp = flags;
        return 0;
}

static int
hlcd_ioctl(v, vs, cmd, data, flag, l)
	void *v;
	void *vs;
	u_long cmd;
	void *data;
	int flag;
	struct lwp *l;
{

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_HDLCD;
		break;

	case WSDISPLAYIO_SVIDEO:
		break;

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = WSDISPLAYIO_VIDEO_ON;
		break;

	default:
		return EPASSTHROUGH;
	}
	return 0;
}

static paddr_t
hlcd_mmap(v, vs, offset, prot)
	void *v;
	void *vs;
	off_t offset;
	int prot;
{
	return -1;
}

static int
hlcd_alloc_screen(v, type, cookiep, curxp, curyp, defattrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *defattrp;
{
	struct hlcd_screen *hdscr = v, *new;

	new = *cookiep = malloc(sizeof(struct hlcd_screen), M_DEVBUF, M_WAITOK);
	bzero(*cookiep, sizeof(struct hlcd_screen));
	new->hlcd_sc = hdscr->hlcd_sc;
	new->image = malloc(PAGE_SIZE, M_DEVBUF, M_WAITOK);
	memset(new->image, ' ', PAGE_SIZE);
	*curxp = *curyp = *defattrp = 0;
	return 0;
}

static void
hlcd_free_screen(v, cookie)
	void *v, *cookie;
{
}

static int
hlcd_show_screen(v, cookie, waitok, cb, cbarg)
	void *v, *cookie, *cbarg;
	int waitok;
	void (*cb)(void *, int, int);
{
	struct hlcd_screen *hdscr = v;

	hdscr->hlcd_sc->sc_curscr = cookie;
	callout_schedule(&hdscr->hlcd_sc->redraw, 1);
	return (0);
}

static void
hlcd_updatechar(sc, daddr, c)
	struct hd44780_chip *sc;
	int daddr, c;
{
	int curdaddr, en, chipdaddr;

	curdaddr = COORD_TO_DADDR(sc->sc_screen.hlcd_curx,
	    sc->sc_screen.hlcd_cury);
	en = DADDR_TO_CHIPNO(daddr);
	chipdaddr = DADDR_TO_CHIPDADDR(daddr);
	if (daddr != curdaddr)
		hd44780_ir_write(sc, en, cmd_ddramset(chipdaddr));

	hd44780_dr_write(sc, en, c);

	daddr++;
	sc->sc_screen.hlcd_curx = DADDR_TO_COL(daddr);
	sc->sc_screen.hlcd_cury = DADDR_TO_ROW(daddr);
}

static void
hlcd_redraw(arg)
	void *arg;
{
	struct hd44780_chip *sc = arg;
	int len, crsridx, startidx, x, y;
	int old_en, new_en;
	u_char *img, *curimg;

	if (sc->sc_curscr == NULL)
		return;

	if (sc->sc_flags & HD_MULTILINE)
		len = 2 * sc->sc_cols;
	else
		len = sc->sc_cols;

	if (sc->sc_flags & HD_MULTICHIP)
		len = len * 2;

	x = sc->sc_screen.hlcd_curx;
	y = sc->sc_screen.hlcd_cury;
	old_en = DADDR_TO_CHIPNO(COORD_TO_DADDR(x, y));

	img = sc->sc_screen.image;
	curimg = sc->sc_curscr->image;
	startidx = crsridx =
	    COORD_TO_IDX(sc->sc_screen.hlcd_curx, sc->sc_screen.hlcd_cury);
	do {
		if (img[crsridx] != curimg[crsridx]) {
			hlcd_updatechar(sc, IDX_TO_DADDR(crsridx),
			    curimg[crsridx]);
			img[crsridx] = curimg[crsridx];
		}
		crsridx++;
		if (crsridx == len)
			crsridx = 0;
	} while (crsridx != startidx);

	x = sc->sc_curscr->hlcd_curx;
	y = sc->sc_curscr->hlcd_cury;
	new_en = DADDR_TO_CHIPNO(COORD_TO_DADDR(x, y));

	if (sc->sc_screen.hlcd_curx != sc->sc_curscr->hlcd_curx ||
	    sc->sc_screen.hlcd_cury != sc->sc_curscr->hlcd_cury) {

		x = sc->sc_screen.hlcd_curx = sc->sc_curscr->hlcd_curx;
		y = sc->sc_screen.hlcd_cury = sc->sc_curscr->hlcd_cury;

		hd44780_ir_write(sc, new_en, cmd_ddramset(
		    DADDR_TO_CHIPDADDR(COORD_TO_DADDR(x, y))));

	}

	/* visible cursor switched to other chip */
	if (old_en != new_en && sc->sc_screen.hlcd_curon) {
		hd44780_ir_write(sc, old_en, cmd_dispctl(1, 0, 0));
		hd44780_ir_write(sc, new_en, cmd_dispctl(1, 1, 1));
	}

	if (sc->sc_screen.hlcd_curon != sc->sc_curscr->hlcd_curon) {
		sc->sc_screen.hlcd_curon = sc->sc_curscr->hlcd_curon;
		if (sc->sc_screen.hlcd_curon)
			hd44780_ir_write(sc, new_en, cmd_dispctl(1, 1, 1));
		else
			hd44780_ir_write(sc, new_en, cmd_dispctl(1, 0, 0));
	}

	callout_schedule(&sc->redraw, 1);
}


/*
 * Finish device attach. sc_writereg, sc_readreg and sc_flags must be properly
 * initialized prior to this call.
 */
void
hd44780_attach_subr(sc)
	struct hd44780_chip *sc;
{
	int err = 0;
	/* Putc/getc are supposed to be set by platform-dependent code. */
	if ((sc->sc_writereg == NULL) || (sc->sc_readreg == NULL))
		sc->sc_dev_ok = 0;

	/* Make sure that HD_MAX_CHARS is enough. */
	if ((sc->sc_flags & HD_MULTILINE) && (2 * sc->sc_cols > HD_MAX_CHARS))
		sc->sc_dev_ok = 0;
	else if (sc->sc_cols > HD_MAX_CHARS)
		sc->sc_dev_ok = 0;

	if (sc->sc_dev_ok) {
		if ((sc->sc_flags & HD_UP) == 0)
			err = hd44780_init(sc);
		if (err != 0)
			printf("%s: LCD not responding or unconnected\n", sc->sc_dev->dv_xname);

	}

	sc->sc_screen.hlcd_sc = sc;

	sc->sc_screen.image = malloc(PAGE_SIZE, M_DEVBUF, M_WAITOK);
	memset(sc->sc_screen.image, ' ', PAGE_SIZE);
	sc->sc_curscr = NULL;
	sc->sc_curchip = 0;
	callout_init(&sc->redraw, 0);
	callout_setfunc(&sc->redraw, hlcd_redraw, sc);
}

int hd44780_init(sc)
	struct hd44780_chip *sc;
{
	int ret;

	ret = hd44780_chipinit(sc, 0);
	if (ret != 0 || !(sc->sc_flags & HD_MULTICHIP)) return ret;
	else return hd44780_chipinit(sc, 1);
}

/*
 * Initialize 4-bit or 8-bit connected device.
 */
int
hd44780_chipinit(sc, en)
	struct hd44780_chip *sc;
	u_int32_t en;
{
	u_int8_t cmd, dat;

	sc->sc_flags &= ~(HD_TIMEDOUT|HD_UP);
	sc->sc_dev_ok = 1;

	cmd = cmd_init(sc->sc_flags & HD_8BIT);
	hd44780_ir_write(sc, en, cmd);
	delay(HD_TIMEOUT_LONG);
	hd44780_ir_write(sc, en, cmd);
	hd44780_ir_write(sc, en, cmd);

	cmd = cmd_funcset(
			sc->sc_flags & HD_8BIT,
			sc->sc_flags & HD_MULTILINE,
			sc->sc_flags & HD_BIGFONT);

	if ((sc->sc_flags & HD_8BIT) == 0)
		hd44780_ir_write(sc, en, cmd);

	sc->sc_flags |= HD_UP;

	hd44780_ir_write(sc, en, cmd);
	hd44780_ir_write(sc, en, cmd_dispctl(0, 0, 0));
	hd44780_ir_write(sc, en, cmd_clear());
	hd44780_ir_write(sc, en, cmd_modset(1, 0));

	if (sc->sc_flags & HD_TIMEDOUT) {
		sc->sc_flags &= ~HD_UP;
		return EIO;
	}

	/* Turn display on and clear it. */
	hd44780_ir_write(sc, en, cmd_clear());
	hd44780_ir_write(sc, en, cmd_dispctl(1, 0, 0));

	/* Attempt a simple probe for presence */
	hd44780_ir_write(sc, en, cmd_ddramset(0x5));
	hd44780_ir_write(sc, en, cmd_shift(0, 1));
	hd44780_busy_wait(sc, en);
	if ((dat = hd44780_ir_read(sc, en) & 0x7f) != 0x6) {
		sc->sc_dev_ok = 0;
		sc->sc_flags &= ~HD_UP;
		return EIO;
	}
	hd44780_ir_write(sc, en, cmd_ddramset(0));

	return 0;
}

/*
 * Standard hd44780 ioctl() functions.
 */
int
hd44780_ioctl_subr(sc, cmd, data)
	struct hd44780_chip *sc;
	u_long cmd;
	void *data;
{
	u_int8_t tmp;
	int error = 0;
	u_int32_t en = sc->sc_curchip;

#define hd44780_io()	((struct hd44780_io *)data)
#define hd44780_info()	((struct hd44780_info*)data)
#define hd44780_ctrl()	((struct hd44780_dispctl*)data)

	switch (cmd) {
		/* Clear the LCD. */
		case HLCD_CLEAR:
			hd44780_ir_write(sc, en, cmd_clear());
			break;

		/* Move the cursor one position to the left. */
		case HLCD_CURSOR_LEFT:
			hd44780_ir_write(sc, en, cmd_shift(0, 0));
			break;

		/* Move the cursor one position to the right. */
		case HLCD_CURSOR_RIGHT:
			hd44780_ir_write(sc, en, cmd_shift(0, 1));
			break;

		/* Control the LCD. */
		case HLCD_DISPCTL:
			hd44780_ir_write(sc, en, cmd_dispctl(
						hd44780_ctrl()->display_on,
						hd44780_ctrl()->cursor_on,
						hd44780_ctrl()->blink_on));
			break;

		/* Get LCD configuration. */
		case HLCD_GET_INFO:
			hd44780_info()->lines
				= (sc->sc_flags & HD_MULTILINE) ? 2 : 1;
			if (sc->sc_flags & HD_MULTICHIP)
				hd44780_info()->lines *= 2;
			hd44780_info()->phys_rows = sc->sc_cols;
			hd44780_info()->virt_rows = sc->sc_vcols;
			hd44780_info()->is_wide = sc->sc_flags & HD_8BIT;
			hd44780_info()->is_bigfont = sc->sc_flags & HD_BIGFONT;
			hd44780_info()->kp_present = sc->sc_flags & HD_KEYPAD;
			break;


		/* Reset the LCD. */
		case HLCD_RESET:
			error = hd44780_init(sc);
			break;

		/* Get the current cursor position. */
		case HLCD_GET_CURSOR_POS:
			hd44780_io()->dat = (hd44780_ir_read(sc, en) & 0x7f);
			break;

		/* Set the cursor position. */
		case HLCD_SET_CURSOR_POS:
			hd44780_ir_write(sc, en, cmd_ddramset(hd44780_io()->dat));
			break;

		/* Get the value at the current cursor position. */
		case HLCD_GETC:
			tmp = (hd44780_ir_read(sc, en) & 0x7f);
			hd44780_ir_write(sc, en, cmd_ddramset(tmp));
			hd44780_io()->dat = hd44780_dr_read(sc, en);
			break;

		/* Set the character at the cursor position + advance cursor. */
		case HLCD_PUTC:
			hd44780_dr_write(sc, en, hd44780_io()->dat);
			break;

		/* Shift display left. */
		case HLCD_SHIFT_LEFT:
			hd44780_ir_write(sc, en, cmd_shift(1, 0));
			break;

		/* Shift display right. */
		case HLCD_SHIFT_RIGHT:
			hd44780_ir_write(sc, en, cmd_shift(1, 1));
			break;

		/* Return home. */
		case HLCD_HOME:
			hd44780_ir_write(sc, en, cmd_rethome());
			break;

		/* Write a string to the LCD virtual area. */
		case HLCD_WRITE:
			error = hd44780_ddram_io(sc, en, hd44780_io(), HD_DDRAM_WRITE);
			break;

		/* Read LCD virtual area. */
		case HLCD_READ:
			error = hd44780_ddram_io(sc, en, hd44780_io(), HD_DDRAM_READ);
			break;

		/* Write to the LCD visible area. */
		case HLCD_REDRAW:
			hd44780_ddram_redraw(sc, en, hd44780_io());
			break;

		/* Write raw instruction. */
		case HLCD_WRITE_INST:
			hd44780_ir_write(sc, en, hd44780_io()->dat);
			break;

		/* Write raw data. */
		case HLCD_WRITE_DATA:
			hd44780_dr_write(sc, en, hd44780_io()->dat);
			break;

		/* Get current chip 0 or 1 (top or bottom) */
		case HLCD_GET_CHIPNO:
			*(u_int8_t *)data = sc->sc_curchip;
			break;

		/* Set current chip 0 or 1 (top or bottom) */
		case HLCD_SET_CHIPNO:
			sc->sc_curchip = *(u_int8_t *)data;
			break;

		default:
			error = EINVAL;
	}

	if (sc->sc_flags & HD_TIMEDOUT)
		error = EIO;

	return error;
}

/*
 * Read/write particular area of the LCD screen.
 */
int
hd44780_ddram_io(sc, en, io, dir)
	struct hd44780_chip *sc;
	u_int32_t en;
	struct hd44780_io *io;
	u_char dir;
{
	u_int8_t hi;
	u_int8_t addr;

	int error = 0;
	u_int8_t i = 0;

	if (io->dat < sc->sc_vcols) {
		hi = HD_ROW1_ADDR + sc->sc_vcols;
		addr = HD_ROW1_ADDR + io->dat;
		for (; (addr < hi) && (i < io->len); addr++, i++) {
			hd44780_ir_write(sc, en, cmd_ddramset(addr));
			if (dir == HD_DDRAM_READ)
				io->buf[i] = hd44780_dr_read(sc, en);
			else
				hd44780_dr_write(sc, en, io->buf[i]);
		}
	}
	if (io->dat < 2 * sc->sc_vcols) {
		hi = HD_ROW2_ADDR + sc->sc_vcols;
		if (io->dat >= sc->sc_vcols)
			addr = HD_ROW2_ADDR + io->dat - sc->sc_vcols;
		else
			addr = HD_ROW2_ADDR;
		for (; (addr < hi) && (i < io->len); addr++, i++) {
			hd44780_ir_write(sc, en, cmd_ddramset(addr));
			if (dir == HD_DDRAM_READ)
				io->buf[i] = hd44780_dr_read(sc, en);
			else
				hd44780_dr_write(sc, en, io->buf[i]);
		}
		if (i < io->len)
			io->len = i;
	} else {
		error = EINVAL;
	}
	return error;
}

/*
 * Write to the visible area of the display.
 */
void
hd44780_ddram_redraw(sc, en, io)
	struct hd44780_chip *sc;
	u_int32_t en;
	struct hd44780_io *io;
{
	u_int8_t i;

	hd44780_ir_write(sc, en, cmd_clear());
	hd44780_ir_write(sc, en, cmd_rethome());
	for (i = 0; (i < io->len) && (i < sc->sc_cols); i++) {
		hd44780_dr_write(sc, en, io->buf[i]);
	}
	hd44780_ir_write(sc, en, cmd_ddramset(HD_ROW2_ADDR));
	for (; (i < io->len); i++)
		hd44780_dr_write(sc, en, io->buf[i]);
}

void
hd44780_busy_wait(sc, en)
	struct hd44780_chip *sc;
	u_int32_t en;
{
	int nloops = 100;

	if (sc->sc_flags & HD_TIMEDOUT)
		return;

	while(nloops-- && (hd44780_ir_read(sc, en) & BUSY_FLAG) == BUSY_FLAG);

	if (nloops == 0) {
		sc->sc_flags |= HD_TIMEDOUT;
		sc->sc_dev_ok = 0;
	}
}

#if defined(HD44780_STD_WIDE)
/*
 * Standard 8-bit version of 'sc_writereg' (8-bit port, 8-bit access)
 */
void
hd44780_writereg(sc, en, reg, cmd)
	struct hd44780_chip *sc;
	u_int32_t en, reg;
	u_int8_t cmd;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh;

	if (sc->sc_dev_ok == 0)
		return;

	if (reg == 0)
		ioh = sc->sc_ioir;
	else
		ioh = sc->sc_iodr;

	bus_space_write_1(iot, ioh, 0x00, cmd);
	delay(HD_TIMEOUT_NORMAL);
}

/*
 * Standard 8-bit version of 'sc_readreg' (8-bit port, 8-bit access)
 */
u_int8_t
hd44780_readreg(sc, en, reg)
	struct hd44780_chip *sc;
	u_int32_t en, reg;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh;

	if (sc->sc_dev_ok == 0)
		return;

	if (reg == 0)
		ioh = sc->sc_ioir;
	else
		ioh = sc->sc_iodr;

	delay(HD_TIMEOUT_NORMAL);
	return bus_space_read_1(iot, ioh, 0x00);
}
#elif defined(HD44780_STD_SHORT)
/*
 * Standard 4-bit version of 'sc_writereg' (4-bit port, 8-bit access)
 */
void
hd44780_writereg(sc, en, reg, cmd)
	struct hd44780_chip *sc;
	u_int32_t en, reg;
	u_int8_t cmd;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh;

	if (sc->sc_dev_ok == 0)
		return;

	if (reg == 0)
		ioh = sc->sc_ioir;
	else
		ioh = sc->sc_iodr;

	bus_space_write_1(iot, ioh, 0x00, hi_bits(cmd));
	if (sc->sc_flags & HD_UP)
		bus_space_write_1(iot, ioh, 0x00, lo_bits(cmd));
	delay(HD_TIMEOUT_NORMAL);
}

/*
 * Standard 4-bit version of 'sc_readreg' (4-bit port, 8-bit access)
 */
u_int8_t
hd44780_readreg(sc, en, reg)
	struct hd44780_chip *sc;
	u_int32_t en, reg;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh;
	u_int8_t rd, dat;

	if (sc->sc_dev_ok == 0)
		return;

	if (reg == 0)
		ioh = sc->sc_ioir;
	else
		ioh = sc->sc_iodr;

	rd = bus_space_read_1(iot, ioh, 0x00);
	dat = (rd & 0x0f) << 4;
	rd = bus_space_read_1(iot, ioh, 0x00);
	return (dat | (rd & 0x0f));
}
#endif
