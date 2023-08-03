/* $NetBSD: wsemul_vt100_subr.c,v 1.34 2023/08/03 22:11:41 uwe Exp $ */

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wsemul_vt100_subr.c,v 1.34 2023/08/03 22:11:41 uwe Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsemulvar.h>
#include <dev/wscons/vt100_base.h>

#ifdef _KERNEL_OPT
#include "opt_wsemul.h"
#endif

static int vt100_selectattribute(struct vt100base_data *,
				      int, int, int, long *, long *);
static int vt100_ansimode(struct vt100base_data *, int, int);
static int vt100_decmode(struct vt100base_data *, int, int);
#define VTMODE_SET 33
#define VTMODE_RESET 44
#define VTMODE_REPORT 55

/*
 * scroll up within scrolling region
 */
void
wsemul_vt100_scrollup(struct vt100base_data *vd, int n)
{
	int help;

	if (n > vd->scrreg_nrows)
		n = vd->scrreg_nrows;

	help = vd->scrreg_nrows - n;
	if (help > 0) {
		(*vd->emulops->copyrows)(vd->emulcookie,
					  vd->scrreg_startrow + n,
					  vd->scrreg_startrow,
					  help);
		if (vd->dblwid)
			memmove(&vd->dblwid[vd->scrreg_startrow],
				&vd->dblwid[vd->scrreg_startrow + n],
				help);
	}
	(*vd->emulops->eraserows)(vd->emulcookie,
				   vd->scrreg_startrow + help, n,
				   vd->bkgdattr);
	if (vd->dblwid)
		memset(&vd->dblwid[vd->scrreg_startrow + help], 0, n);
	CHECK_DW(vd);
}

/*
 * scroll down within scrolling region
 */
void
wsemul_vt100_scrolldown(struct vt100base_data *vd, int n)
{
	int help;

	if (n > vd->scrreg_nrows)
		n = vd->scrreg_nrows;

	help = vd->scrreg_nrows - n;
	if (help > 0) {
		(*vd->emulops->copyrows)(vd->emulcookie,
					  vd->scrreg_startrow,
					  vd->scrreg_startrow + n,
					  help);
		if (vd->dblwid)
			memmove(&vd->dblwid[vd->scrreg_startrow + n],
				&vd->dblwid[vd->scrreg_startrow],
				help);
	}
	(*vd->emulops->eraserows)(vd->emulcookie,
				   vd->scrreg_startrow, n,
				   vd->bkgdattr);
	if (vd->dblwid)
		memset(&vd->dblwid[vd->scrreg_startrow], 0, n);
	CHECK_DW(vd);
}

/*
 * erase in display
 */
void
wsemul_vt100_ed(struct vt100base_data *vd, int arg)
{
	int n;

	switch (arg) {
	    case 0: /* cursor to end */
		ERASECOLS(vd, vd->ccol, COLS_LEFT(vd) + 1, vd->bkgdattr);
		n = vd->nrows - vd->crow - 1;
		if (n > 0) {
			(*vd->emulops->eraserows)(vd->emulcookie,
						   vd->crow + 1, n,
						   vd->bkgdattr);
			if (vd->dblwid)
				memset(&vd->dblwid[vd->crow + 1], 0, n);
		}
		break;
	    case 1: /* beginning to cursor */
		if (vd->crow > 0) {
			(*vd->emulops->eraserows)(vd->emulcookie,
						   0, vd->crow,
						   vd->bkgdattr);
			if (vd->dblwid)
				memset(&vd->dblwid[0], 0, vd->crow);
		}
		ERASECOLS(vd, 0, vd->ccol + 1, vd->bkgdattr);
		break;
	    case 2: /* complete display */
		(*vd->emulops->eraserows)(vd->emulcookie,
					   0, vd->nrows,
					   vd->bkgdattr);
		if (vd->dblwid)
			memset(&vd->dblwid[0], 0, vd->nrows);
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("ed(%d) unknown\n", arg);
#endif
		break;
	}
	CHECK_DW(vd);
}

/*
 * erase in line
 */
void
wsemul_vt100_el(struct vt100base_data *vd, int arg)
{
	switch (arg) {
	    case 0: /* cursor to end */
		ERASECOLS(vd, vd->ccol, COLS_LEFT(vd) + 1, vd->bkgdattr);
		break;
	    case 1: /* beginning to cursor */
		ERASECOLS(vd, 0, vd->ccol + 1, vd->bkgdattr);
		break;
	    case 2: /* complete line */
		(*vd->emulops->erasecols)(vd->emulcookie, vd->crow,
					   0, vd->ncols,
					   vd->bkgdattr);
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("el(%d) unknown\n", arg);
#endif
		break;
	}
}

/*
 * handle commands after CSI (ESC[)
 */
void
wsemul_vt100_handle_csi(struct vt100base_data *vd, u_char c)
{
	int n, m, help, flags, fgcol, bgcol;
	long attr, bkgdattr;

#define A3(a, b, c) (((a) << 16) | ((b) << 8) | (c))
	switch (A3(vd->modif1, vd->modif2, c)) {
	    case A3('>', '\0', 'c'): /* DA secondary */
		wsdisplay_emulinput(vd->cbcookie, WSEMUL_VT_ID2,
				    sizeof(WSEMUL_VT_ID2) - 1);
		break;

	    case A3('\0', '\0', 'J'): /* ED selective erase in display */
	    case A3('?', '\0', 'J'): /* DECSED selective erase in display */
		wsemul_vt100_ed(vd, ARG(vd, 0));
		break;
	    case A3('\0', '\0', 'K'): /* EL selective erase in line */
	    case A3('?', '\0', 'K'): /* DECSEL selective erase in line */
		wsemul_vt100_el(vd, ARG(vd, 0));
		break;
	    case A3('\0', '\0', 'h'): /* SM */
		for (n = 0; n < vd->nargs; n++)
			vt100_ansimode(vd, ARG(vd, n), VTMODE_SET);
		break;
	    case A3('?', '\0', 'h'): /* DECSM */
		for (n = 0; n < vd->nargs; n++)
			vt100_decmode(vd, ARG(vd, n), VTMODE_SET);
		break;
	    case A3('\0', '\0', 'l'): /* RM */
		for (n = 0; n < vd->nargs; n++)
			vt100_ansimode(vd, ARG(vd, n), VTMODE_RESET);
		break;
	    case A3('?', '\0', 'l'): /* DECRM */
		for (n = 0; n < vd->nargs; n++)
			vt100_decmode(vd, ARG(vd, n), VTMODE_RESET);
		break;
	    case A3('\0', '$', 'p'): /* DECRQM request mode ANSI */
		vt100_ansimode(vd, ARG(vd, 0), VTMODE_REPORT);
		break;
	    case A3('?', '$', 'p'): /* DECRQM request mode DEC */
		vt100_decmode(vd, ARG(vd, 0), VTMODE_REPORT);
		break;
	    case A3('\0', '\0', 'i'): /* MC printer controller mode */
	    case A3('?', '\0', 'i'): /* MC printer controller mode */
		switch (ARG(vd, 0)) {
		    case 0: /* print screen */
		    case 1: /* print cursor line */
		    case 4: /* off */
		    case 5: /* on */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI%di ignored\n", ARG(vd, 0));
#endif
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%di unknown\n", ARG(vd, 0));
#endif
			break;
		}
		break;

#define A2(a, b) (((a) << 8) | (b))
#if 0 /* XXX: edp not available here */
	    case A2('!', 'p'): /* DECSTR soft reset VT300 only */
		wsemul_vt100_reset(edp);
		break;
#endif

	    case A2('"', 'p'): /* DECSCL */
		switch (ARG(vd, 0)) {
		    case 61: /* VT100 mode (no further arguments!) */
			break;
		    case 62:
		    case 63: /* VT300 mode */
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d\"p unknown\n", ARG(vd, 0));
#endif
			break;
		}
		switch (ARG(vd, 1)) {
		    case 0:
		    case 2: /* 8-bit controls */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI%d;%d\"p ignored\n", ARG(vd, 0), ARG(vd, 1));
#endif
			break;
		    case 1: /* 7-bit controls */
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d;%d\"p unknown\n", ARG(vd, 0), ARG(vd, 1));
#endif
			break;
		}
		break;
	    case A2('"', 'q'): /* DECSCA select character attribute VT300 */
		switch (ARG(vd, 0)) {
		    case 0:
		    case 1: /* erasable */
			break;
		    case 2: /* not erasable */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI2\"q ignored\n");
#endif
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d\"q unknown\n", ARG(vd, 0));
#endif
			break;
		}
		break;

	    case A2('$', 'u'): /* DECRQTSR request terminal status report */
		switch (ARG(vd, 0)) {
		    case 0: /* ignored */
			break;
		    case 1: /* terminal state report */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI1$u ignored\n");
#endif
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d$u unknown\n", ARG(vd, 0));
#endif
			break;
		}
		break;
	    case A2('$', 'w'): /* DECRQPSR request presentation status report
				(VT300 only) */
		switch (ARG(vd, 0)) {
		    case 0: /* error */
			break;
		    case 1: /* cursor information report */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI1$w ignored\n");
#endif
			break;
		    case 2: /* tab stop report */
			{
			int i, j, ps = 0;
			char buf[20];
			KASSERT(vd->tabs != NULL);
			wsdisplay_emulinput(vd->cbcookie, "\033P2$u", 5);
			for (i = 0; i < vd->ncols; i++)
				if (vd->tabs[i]) {
					j = snprintf(buf, sizeof(buf), "%s%d",
					    (ps ? "/" : ""), i + 1);
					wsdisplay_emulinput(vd->cbcookie,
							    buf, j);
					ps = 1;
				}
			}
			wsdisplay_emulinput(vd->cbcookie, "\033\\", 2);
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d$w unknown\n", ARG(vd, 0));
#endif
			break;
		}
		break;
	    case A2('$', '}'): /* DECSASD select active status display */
		switch (ARG(vd, 0)) {
		    case 0: /* main display */
		    case 1: /* status line */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI%d$} ignored\n", ARG(vd, 0));
#endif
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d$} unknown\n", ARG(vd, 0));
#endif
			break;
		}
		break;
	    case A2('$', '~'): /* DECSSDD select status line type */
		switch (ARG(vd, 0)) {
		    case 0: /* none */
		    case 1: /* indicator */
		    case 2: /* host-writable */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI%d$~ ignored\n", ARG(vd, 0));
#endif
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d$~ unknown\n", ARG(vd, 0));
#endif
			break;
		}
		break;

	    case A2('&', 'u'): /* DECRQUPSS request user preferred
				  supplemental set */
		wsdisplay_emulinput(vd->cbcookie, "\033P0!u%5\033\\", 9);
		break;

	    case '@': /* ICH insert character VT300 only */
		n = uimin(DEF1_ARG(vd, 0), COLS_LEFT(vd) + 1);
		help = NCOLS(vd) - (vd->ccol + n);
		if (help > 0)
			COPYCOLS(vd, vd->ccol, vd->ccol + n, help);
		ERASECOLS(vd, vd->ccol, n, vd->bkgdattr);
		break;
	    case 'A': /* CUU */
		/* stop at the top scroll margin */
		m = vd->scrreg_startrow;
		if (vd->crow < m)/* but if already above the margin */
			m = 0;	 /* then at the screen top */
		help = vd->crow - m; /* rows above */
		vd->crow -= uimin(DEF1_ARG(vd, 0), help);
		CHECK_DW(vd);
		break;
	    case 'B': /* CUD */
		/* stop at the bottom scroll margin */
		m = vd->scrreg_startrow + vd->scrreg_nrows - 1;
		if (vd->crow > m) /* but if already below the margin */
			m = vd->nrows - 1; /* then at the screen bottom */
		help = m - vd->crow; /* rows below */
		vd->crow += uimin(DEF1_ARG(vd, 0), help);
		CHECK_DW(vd);
		break;
	    case 'C': /* CUF */
		vd->ccol += uimin(DEF1_ARG(vd, 0), COLS_LEFT(vd));
		break;
	    case 'D': /* CUB */
		vd->ccol -= uimin(DEF1_ARG(vd, 0), vd->ccol);
		vd->flags &= ~VTFL_LASTCHAR;
		break;
	    case 'G': /* CHA */
		vd->ccol = uimin(DEF1_ARG(vd, 0) - 1, vd->ncols -1);
		break;
	    case 'H': /* CUP */
	    case 'f': /* HVP */
		if (vd->flags & VTFL_DECOM)
			vd->crow = vd->scrreg_startrow +
			    uimin(DEF1_ARG(vd, 0), vd->scrreg_nrows) - 1;
		else
			vd->crow = uimin(DEF1_ARG(vd, 0), vd->nrows) - 1;
		CHECK_DW(vd);
		vd->ccol = uimin(DEF1_ARG(vd, 1), NCOLS(vd)) - 1;
		vd->flags &= ~VTFL_LASTCHAR;
		break;
	    case 'L': /* IL insert line */
	    case 'M': /* DL delete line */
		/* ignored when the cursor is outside the scrolling region */
		if (vd->crow < vd->scrreg_startrow
		    || vd->scrreg_startrow + vd->scrreg_nrows <= vd->crow)
			break;
		n = uimin(DEF1_ARG(vd, 0), ROWS_BELOW(vd) + 1);
		{
		int savscrstartrow, savscrnrows;
		savscrstartrow = vd->scrreg_startrow;
		savscrnrows = vd->scrreg_nrows;
		vd->scrreg_nrows -= ROWS_ABOVE(vd);
		vd->scrreg_startrow = vd->crow;
		if (c == 'L')
			wsemul_vt100_scrolldown(vd, n);
		else
			wsemul_vt100_scrollup(vd, n);
		vd->scrreg_startrow = savscrstartrow;
		vd->scrreg_nrows = savscrnrows;
		}
		vd->ccol = 0;
		break;
	    case 'P': /* DCH delete character */
		n = uimin(DEF1_ARG(vd, 0), COLS_LEFT(vd) + 1);
		help = NCOLS(vd) - (vd->ccol + n);
		if (help > 0)
			COPYCOLS(vd, vd->ccol + n, vd->ccol, help);
		ERASECOLS(vd, NCOLS(vd) - n, n, vd->bkgdattr);
		break;
	    case 'S': /* SU */
		wsemul_vt100_scrollup(vd, DEF1_ARG(vd, 0));
		break;
	    case 'T': /* SD */
		wsemul_vt100_scrolldown(vd, DEF1_ARG(vd, 0));
		break;
	    case 'X': /* ECH erase character */
		n = uimin(DEF1_ARG(vd, 0), COLS_LEFT(vd) + 1);
		ERASECOLS(vd, vd->ccol, n, vd->bkgdattr);
		break;
	    case 'Z': /* CBT */
		if (vd->ccol == 0)
			break;
		for (m = 0; m < DEF1_ARG(vd, 0); m++) {
			if (vd->tabs) {
				for (n = vd->ccol - 1; n > 0; n--) {
					if (vd->tabs[n])
						break;
				}
			} else
				n = (vd->ccol - 1) & ~7;
			vd->ccol = n;
			if (n == 0)
				break;
		}
		break;
	    case 'c': /* DA primary */
		if (ARG(vd, 0) == 0)
			wsdisplay_emulinput(vd->cbcookie, WSEMUL_VT_ID1,
					    sizeof(WSEMUL_VT_ID1) - 1);
		break;
	    case 'd': /* VPA */
		vd->crow = uimin(DEF1_ARG(vd, 0) - 1, vd->nrows - 1);
 		break;
	    case 'g': /* TBC */
		KASSERT(vd->tabs != NULL);
		switch (ARG(vd, 0)) {
		    case 0:
			vd->tabs[vd->ccol] = 0;
			break;
		    case 3:
			memset(vd->tabs, 0, vd->ncols);
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%dg unknown\n", ARG(vd, 0));
#endif
			break;
		}
		break;
	    case 'm': /* SGR select graphic rendition */
		flags = vd->attrflags;
		fgcol = vd->fgcol;
		bgcol = vd->bgcol;
		for (n = 0; n < vd->nargs; n++) {
			switch (ARG(vd, n)) {
			    case 0: /* reset */
				if (n == vd->nargs - 1) {
					vd->bkgdattr = vd->curattr = vd->defattr;
					vd->attrflags = vd->msgattrs.default_attrs;
					vd->fgcol = vd->msgattrs.default_fg;
					vd->bgcol = vd->msgattrs.default_bg;
					return;
				}
				flags = vd->msgattrs.default_attrs;
				fgcol = vd->msgattrs.default_fg;
				bgcol = vd->msgattrs.default_bg;
				break;
			    case 1: /* bold */
				flags |= WSATTR_HILIT;
				break;
			    case 4: /* underline */
				flags |= WSATTR_UNDERLINE;
				break;
			    case 5: /* blink */
				flags |= WSATTR_BLINK;
				break;
			    case 7: /* reverse */
				flags |= WSATTR_REVERSE;
				break;
			    case 22: /* ~bold VT300 only */
				flags &= ~WSATTR_HILIT;
				break;
			    case 24: /* ~underline VT300 only */
				flags &= ~WSATTR_UNDERLINE;
				break;
			    case 25: /* ~blink VT300 only */
				flags &= ~WSATTR_BLINK;
				break;
			    case 27: /* ~reverse VT300 only */
				flags &= ~WSATTR_REVERSE;
				break;
			    case 30: case 31: case 32: case 33:
			    case 34: case 35: case 36: case 37:
				/* fg color */
				flags |= WSATTR_WSCOLORS;
				fgcol = ARG(vd, n) - 30;
				break;
			    case 39:
				fgcol = vd->msgattrs.default_fg;
				break;
			    case 40: case 41: case 42: case 43:
			    case 44: case 45: case 46: case 47:
				/* bg color */
				flags |= WSATTR_WSCOLORS;
				bgcol = ARG(vd, n) - 40;
				break;
			    case 49:
				bgcol = vd->msgattrs.default_bg;
				break;
			    default:
#ifdef VT100_PRINTUNKNOWN
				printf("CSI%dm unknown\n", ARG(vd, n));
#endif
				break;
			}
		}
		if (vt100_selectattribute(vd, flags, fgcol, bgcol, &attr,
		    &bkgdattr)) {
#ifdef VT100_DEBUG
			printf("error allocating attr %d/%d/%x\n",
			       fgcol, bgcol, flags);
#endif
		} else {
			vd->curattr = attr;
			vd->bkgdattr = bkgdattr;
			vd->attrflags = flags;
			vd->fgcol = fgcol;
			vd->bgcol = bgcol;
		}
		break;
	    case 't': /* terminal size and such */
		switch (ARG(vd, 0)) {
		    case 18: {	/* xterm size */
			char buf[20];

			n = snprintf(buf, sizeof(buf), "\033[8;%d;%dt",
			    vd->nrows, vd->ncols);
			wsdisplay_emulinput(vd->cbcookie, buf, n);
			}
			break;
		}
		break;	
	    case 'n': /* reports */
		switch (ARG(vd, 0)) {
		    case 5: /* DSR operating status */
			/* 0 = OK, 3 = malfunction */
			wsdisplay_emulinput(vd->cbcookie, "\033[0n", 4);
			break;
		    case 6: { /* CPR cursor position report */
			char buf[20];
			int row;
			if (vd->flags & VTFL_DECOM)
				row = ROWS_ABOVE(vd);
			else
				row = vd->crow;
			n = snprintf(buf, sizeof(buf), "\033[%d;%dR",
			    row + 1, vd->ccol + 1);
			wsdisplay_emulinput(vd->cbcookie, buf, n);
			}
			break;
		    case 15: /* DSR printer status */
			/* 13 = no printer, 10 = ready, 11 = not ready */
			wsdisplay_emulinput(vd->cbcookie, "\033[?13n", 6);
			break;
		    case 25: /* UDK status - VT300 only */
			/* 20 = locked, 21 = unlocked */
			wsdisplay_emulinput(vd->cbcookie, "\033[?21n", 6);
			break;
		    case 26: /* keyboard dialect */
			/* 1 = north american , 7 = german */
			wsdisplay_emulinput(vd->cbcookie, "\033[?27;1n", 8);
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%dn unknown\n", ARG(vd, 0));
#endif
			break;
		}
		break;
	    case 'r': /* DECSTBM set top/bottom margins */
		help = uimin(DEF1_ARG(vd, 0), vd->nrows) - 1;
		n = uimin(DEFx_ARG(vd, 1, vd->nrows), vd->nrows) - help;
		if (n < 2) {
			/* minimal scrolling region has 2 lines */
			return;
		} else {
			vd->scrreg_startrow = help;
			vd->scrreg_nrows = n;
		}
		vd->crow = ((vd->flags & VTFL_DECOM) ?
			     vd->scrreg_startrow : 0);
		vd->ccol = 0;
		break;
	    case 'y':
		switch (ARG(vd, 0)) {
		    case 4: /* DECTST invoke confidence test */
			/* ignore */
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%dy unknown\n", ARG(vd, 0));
#endif
			break;
		}
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("CSI%c (%d, %d) unknown\n", c, ARG(vd, 0), ARG(vd, 1));
#endif
		break;
	}
}

/*
 * get an attribute from the graphics driver,
 * try to find replacements if the desired appearance
 * is not supported
 */
static int
vt100_selectattribute(struct vt100base_data *vd,
	int flags, int fgcol, int bgcol, long *attr, long *bkgdattr)
{
	int error;

	if (!(vd->scrcapabilities & WSSCREEN_WSCOLORS)) {
		flags &= ~WSATTR_WSCOLORS;
#ifdef VT100_DEBUG
		printf("colors ignored (impossible)\n");
#endif
	} else
		flags |= WSATTR_WSCOLORS;
	error = (*vd->emulops->allocattr)(vd->emulcookie, fgcol, bgcol,
					   flags & WSATTR_WSCOLORS, bkgdattr);
	if (error)
		return (error);

	if ((flags & WSATTR_HILIT) &&
	    !(vd->scrcapabilities & WSSCREEN_HILIT)) {
		flags &= ~WSATTR_HILIT;
		if (vd->scrcapabilities & WSSCREEN_WSCOLORS) {
#if defined(WSEMUL_VT100_HILIT_FG) && WSEMUL_VT100_HILIT_FG != -1
			fgcol = WSEMUL_VT100_HILIT_FG;
#elif !defined(WSEMUL_VT100_HILIT_FG)
			fgcol = WSCOL_RED;
#endif
#if defined(WSEMUL_VT100_HILIT_BG) && WSEMUL_VT100_HILIT_BG != -1
			bgcol = WSEMUL_VT100_HILIT_BG;
#endif
			flags |= WSATTR_WSCOLORS;
		} else {
#ifdef VT100_DEBUG
			printf("bold ignored (impossible)\n");
#endif
		}
	}
	if ((flags & WSATTR_UNDERLINE) &&
	    !(vd->scrcapabilities & WSSCREEN_UNDERLINE)) {
		flags &= ~WSATTR_UNDERLINE;
		if (vd->scrcapabilities & WSSCREEN_WSCOLORS) {
#if defined(WSEMUL_VT100_UNDERLINE_FG) && WSEMUL_VT100_UNDERLINE_FG != -1
			fgcol = WSEMUL_VT100_UNDERLINE_FG;
#endif
#if defined(WSEMUL_VT100_UNDERLINE_BG) && WSEMUL_VT100_UNDERLINE_BG != -1
			bgcol = WSEMUL_VT100_UNDERLINE_BG;
#elif !defined(WSEMUL_VT100_UNDERLINE_BG)
			bgcol = WSCOL_BROWN;
#endif
			flags |= WSATTR_WSCOLORS;
		} else {
#ifdef VT100_DEBUG
			printf("underline ignored (impossible)\n");
#endif
		}
	}
	if ((flags & WSATTR_BLINK) &&
	    !(vd->scrcapabilities & WSSCREEN_BLINK)) {
		flags &= ~WSATTR_BLINK;
#ifdef VT100_DEBUG
		printf("blink ignored (impossible)\n");
#endif
	}
	if ((flags & WSATTR_REVERSE) &&
	    !(vd->scrcapabilities & WSSCREEN_REVERSE)) {
		flags &= ~WSATTR_REVERSE;
		if (vd->scrcapabilities & WSSCREEN_WSCOLORS) {
			int help;
			help = bgcol;
			bgcol = fgcol;
			fgcol = help;
			flags |= WSATTR_WSCOLORS;
		} else {
#ifdef VT100_DEBUG
			printf("reverse ignored (impossible)\n");
#endif
		}
	}
	error = (*vd->emulops->allocattr)(vd->emulcookie, fgcol, bgcol,
					   flags, attr);
	if (error)
		return (error);

	return (0);
}

/*
 * handle device control sequences if the main state machine
 * told so by setting vd->dcstype to a nonzero value
 */
void
wsemul_vt100_handle_dcs(struct vt100base_data *vd)
{
	int i, pos;

	switch (vd->dcstype) {
	    case 0: /* not handled */
		return;
	    case DCSTYPE_TABRESTORE:
		KASSERT(vd->tabs != NULL);
		KASSERT(vd->ncols <= 1024);
		memset(vd->tabs, 0, vd->ncols);
		pos = 0;
		for (i = 0; i < vd->dcspos; i++) {
			char c = vd->dcsarg[i];
			switch (c) {
			    case '0': case '1': case '2': case '3': case '4':
			    case '5': case '6': case '7': case '8': case '9': {
				const int c0 = c - '0';
				if (pos < 0 ||
				    pos > INT_MAX/10 ||
				    pos * 10 > vd->ncols - c0) {
					pos = -1;
					break;
				}
				pos = pos * 10 + c0;
				break;
			    }
			    case '/':
				if (pos > 0) {
					KASSERT(pos <= vd->ncols);
					vd->tabs[pos - 1] = 1;
				}
				pos = 0;
				break;
			    default:
#ifdef VT100_PRINTUNKNOWN
				printf("unknown char %c in DCS\n", c);
#endif
				break;
			}
		}
		if (pos > 0) {
			KASSERT(pos <= vd->ncols);
			vd->tabs[pos - 1] = 1;
		}
		break;
	    default:
		panic("wsemul_vt100_handle_dcs: bad type %d", vd->dcstype);
	}
	vd->dcstype = 0;
}

static int
vt100_ansimode(struct vt100base_data *vd, int nr, int op)
{
	int res = 0; /* default: unknown */

	switch (nr) {
	    case 2: /* KAM keyboard locked/unlocked */
		break;
	    case 3: /* CRM control representation */
		break;
	    case 4: /* IRM insert/replace characters */
		if (op == VTMODE_SET)
			vd->flags |= VTFL_INSERTMODE;
		else if (op == VTMODE_RESET)
			vd->flags &= ~VTFL_INSERTMODE;
		res = ((vd->flags & VTFL_INSERTMODE) ? 1 : 2);
		break;
	    case 10: /* HEM horizontal editing (permanently reset) */
		res = 4;
		break;
	    case 12: /* SRM local echo off/on */
		res = 4; /* permanently reset ??? */
		break;
	    case 20: /* LNM newline = newline/linefeed */
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("ANSI mode %d unknown\n", nr);
#endif
		break;
	}
	return (res);
}

static int
vt100_decmode(struct vt100base_data *vd, int nr, int op)
{
	int res = 0; /* default: unknown */
	int flags;

	flags = vd->flags;
	switch (nr) {
	    case 1: /* DECCKM application/nomal cursor keys */
		if (op == VTMODE_SET)
			flags |= VTFL_APPLCURSOR;
		else if (op == VTMODE_RESET)
			flags &= ~VTFL_APPLCURSOR;
		res = ((flags & VTFL_APPLCURSOR) ? 1 : 2);
		break;
	    case 2: /* DECANM ANSI vt100/vt52 */
		res = 3; /* permanently set ??? */
		break;
	    case 3: /* DECCOLM 132/80 cols */
	    case 4: /* DECSCLM smooth/jump scroll */
	    case 5: /* DECSCNM light/dark background */
		res = 4; /* all permanently reset ??? */
		break;
	    case 6: /* DECOM move within/outside margins */
		if (op == VTMODE_SET)
			flags |= VTFL_DECOM;
		else if (op == VTMODE_RESET)
			flags &= ~VTFL_DECOM;
		res = ((flags & VTFL_DECOM) ? 1 : 2);
		break;
	    case 7: /* DECAWM autowrap */
		if (op == VTMODE_SET)
			flags |= VTFL_DECAWM;
		else if (op == VTMODE_RESET)
			flags &= ~VTFL_DECAWM;
		res = ((flags & VTFL_DECAWM) ? 1 : 2);
		break;
	    case 8: /* DECARM keyboard autorepeat */
		break;
	    case 18: /* DECPFF print form feed */
		break;
	    case 19: /* DECPEX printer extent: screen/scrolling region */
		break;
	    case 25: /* DECTCEM text cursor on/off */
		if (op == VTMODE_SET)
			flags |= VTFL_CURSORON;
		else if (op == VTMODE_RESET)
			flags &= ~VTFL_CURSORON;
		if (flags != vd->flags)
			(*vd->emulops->cursor)(vd->emulcookie,
						flags & VTFL_CURSORON,
						vd->crow, vd->ccol);
		res = ((flags & VTFL_CURSORON) ? 1 : 2);
		break;
	    case 42: /* DECNRCM use 7-bit NRC /
		      7/8 bit from DEC multilingual or ISO-latin-1*/
		if (op == VTMODE_SET)
			flags |= VTFL_NATCHARSET;
		else if (op == VTMODE_RESET)
			flags &= ~VTFL_NATCHARSET;
		res = ((flags & VTFL_NATCHARSET) ? 1 : 2);
		break;
	    case 66: /* DECNKM numeric keypad */
		break;
	    case 68: /* DECKBUM keyboard usage data processing/typewriter */
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("DEC mode %d unknown\n", nr);
#endif
		break;
	}
	vd->flags = flags;

	return (res);
}
