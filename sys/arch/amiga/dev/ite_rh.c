/*
 *	$Id: ite_rh.c,v 1.2 1994/06/15 19:06:18 chopps Exp $
 */

#include "grfrh.h"
#if NGRFRH > 0

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <dev/cons.h>
#include <machine/cpu.h>
#include <amiga/amiga/device.h>
#include <amiga/dev/grfioctl.h>
#include <amiga/dev/grfvar.h>
#include <amiga/dev/grf_rhreg.h>
#include <amiga/dev/itevar.h>


/*
 * grfrh_cnprobe is called when the console is being initialized
 * i.e. very early.  grfconfig() has been called, so this implies
 * that rt_init() was called.  If we are functioning rh_inited
 * will be true.
 */
int
grfrh_cnprobe()
{
	static int done;
	int rv;

	if (done == 0)
		rv = CN_INTERNAL;
	else
		rv = CN_NORMAL;
	done = 1;
	return(rv);
}

void
grfrh_iteinit(gp)
	struct grf_softc *gp;
{
	gp->g_iteinit = rh_init;
	gp->g_itedeinit = rh_deinit;
	gp->g_iteclear = rh_clear;
	gp->g_iteputc = rh_putc;
	gp->g_itescroll = rh_scroll;
	gp->g_itecursor = rh_cursor;
}

void
rh_init(ip)
	struct ite_softc *ip;
{
	struct MonDef *md;
	extern unsigned char RZ3StdPalette[];

#if 0 /* Not in ite_rt.c - DC */
	if (ip->grf == 0)
		ip->grf = &grf_softc[ip - ite_softc];
#endif

	ip->priv = ip->grf->g_data;
	md = (struct MonDef *) ip->priv;

	ip->cols = md->TX;
	ip->rows = md->TY;
}


void
rh_cursor(ip, flag)
	struct ite_softc *ip;
	int flag;
{
	volatile u_char *ba = ip->grf->g_regkva;

	if (flag == START_CURSOROPT || flag == END_CURSOROPT)
		return;

	if (flag == ERASE_CURSOR) {
#if 0
		/* disable cursor */
		WCrt (ba, CRT_ID_CURSOR_START,
		    RCrt (ba, CRT_ID_CURSOR_START) | 0x20);
#endif
	} else {
		int pos = ip->curx + ip->cury * ip->cols;
#if 0
		/* make sure to enable cursor */
		WCrt (ba, CRT_ID_CURSOR_START,
		    RCrt (ba, CRT_ID_CURSOR_START) & ~0x20);
#endif

		/* and position it */
		RZ3SetCursorPos (ip->grf, pos);

		ip->cursorx = ip->curx;
		ip->cursory = ip->cury;
	}
}



static void
screen_up(ip, top, bottom, lines)
	struct ite_softc *ip;
	int top;
	int bottom;
	int lines;
{
	volatile u_char * ba = ip->grf->g_regkva;
	volatile u_char * fb = ip->grf->g_fbkva;

	/* do some bounds-checking here.. */
	if (top >= bottom)
		return;

	if (top + lines >= bottom) {
		RZ3AlphaErase(ip->grf, 0, top, bottom - top, ip->cols);
		return;
	}

	RZ3AlphaCopy(ip->grf, 0, top+lines, 0, top, ip->cols, bottom-top-lines+1);
	RZ3AlphaErase(ip->grf, 0, bottom - lines + 1, ip->cols, lines);
}

static void
screen_down (ip, top, bottom, lines)
	struct ite_softc *ip;
	int top;
	int bottom;
	int lines;
{
	volatile u_char * ba = ip->grf->g_regkva;
	volatile u_char * fb = ip->grf->g_fbkva;

	/* do some bounds-checking here.. */
	if (top >= bottom)
		return;

	if (top + lines >= bottom) {
		RZ3AlphaErase(ip->grf, 0, top, bottom - top, ip->cols);
		return;
	}

	RZ3AlphaCopy(ip->grf, 0, top, 0, top+lines, ip->cols, bottom-top-lines+1);
	RZ3AlphaErase(ip->grf, 0, top, ip->cols, lines);
}

void
rh_deinit(ip)
	struct ite_softc *ip;
{
	ip->flags &= ~ITE_INITED;
}


void
rh_putc(ip, c, dy, dx, mode)
	struct ite_softc *ip;
	int c;
	int dy;
	int dx;
	int mode;
{
	volatile u_char * ba = ip->grf->g_regkva;
	volatile u_char * fb = ip->grf->g_fbkva;
	register u_char attr;

	attr = (mode & ATTR_INV) ? 0x21 : 0x10;
	if (mode & ATTR_UL)     attr  = 0x01;	/* ???????? */
	if (mode & ATTR_BOLD)   attr |= 0x08;
	if (mode & ATTR_BLINK)  attr |= 0x80;

	fb += 4 * (dy * ip->cols + dx);
	*fb++ = c; *fb = attr;
}

void
rh_clear(ip, sy, sx, h, w)
	struct ite_softc *ip;
	int sy;
	int sx;
	int h;
	int w;
{
	RZ3AlphaErase (ip->grf, sx, sy, w, h);
}

void
rh_scroll(ip, sy, sx, count, dir)
	struct ite_softc *ip;
	int sy;
	int sx;
	int count;
	int dir;
{
	volatile u_char * ba = ip->grf->g_regkva;
	u_long * fb = (u_long *) ip->grf->g_fbkva;
	register int height, dy, i;

	rh_cursor(ip, ERASE_CURSOR);

	if (dir == SCROLL_UP) {
		screen_up(ip, sy - count, ip->bottom_margin, count);
		/* bcopy(fb + sy * ip->cols, fb + (sy - count) * ip->cols,
		    4 * (ip->bottom_margin - sy + 1) * ip->cols); */
		/* rh_clear(ip, ip->bottom_margin + 1 - count, 0,
		    count, ip->cols); */
	} else if (dir == SCROLL_DOWN) {
		screen_down(ip, sy, ip->bottom_margin, count);
		/* bcopy(fb + sy * ip->cols, fb + (sy + count) * ip->cols,
		    4 * (ip->bottom_margin - sy - count + 1) * ip->cols); */
		/* rh_clear(ip, sy, 0, count, ip->cols); */
	} else if (dir == SCROLL_RIGHT) {
		RZ3AlphaCopy(ip->grf, sx, sy, sx + count, sy,
		    ip->cols - (sx + count), 1);
		RZ3AlphaErase(ip->grf, sx, sy, count, 1);
	} else {
		RZ3AlphaCopy(ip->grf, sx + count, sy, sx, sy,
		    ip->cols - (sx + count), 1);
		RZ3AlphaErase(ip->grf, ip->cols - count, sy, count, 1);
	}
}
#endif /* NGRFRH */
