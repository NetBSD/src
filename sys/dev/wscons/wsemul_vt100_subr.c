/* $NetBSD: wsemul_vt100_subr.c,v 1.1 1998/06/20 19:17:47 drochner Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsemulvar.h>
#include <dev/wscons/wsemul_vt100var.h>

static int vt100_selectattribute __P((struct wsemul_vt100_emuldata *,
				      int, int, int, long *));
static int vt100_ansimode __P((struct wsemul_vt100_emuldata *, int, int));
static int vt100_decmode __P((struct wsemul_vt100_emuldata *, int, int));
#define MODE_SET 33
#define MODE_RESET 44
#define MODE_REPORT 55

/*
 * scroll up within scrolling region
 */
void
wsemul_vt100_scrollup(edp, n)
	struct wsemul_vt100_emuldata *edp;
	int n;
{
	int help;

	if (n > edp->scrreg_nrows)
		n = edp->scrreg_nrows;

	help = edp->scrreg_nrows - n;
	if (help > 0)
		(*edp->emulops->copyrows)(edp->emulcookie,
					  edp->scrreg_startrow + n,
					  edp->scrreg_startrow,
					  help);
	(*edp->emulops->eraserows)(edp->emulcookie,
				   edp->scrreg_startrow + help, n,
				   edp->curattr);
}

/*
 * scroll down within scrolling region
 */
void
wsemul_vt100_scrolldown(edp, n)
	struct wsemul_vt100_emuldata *edp;
	int n;
{
	int help;

	if (n > edp->scrreg_nrows)
		n = edp->scrreg_nrows;

	help = edp->scrreg_nrows - n;
	if (help > 0)
		(*edp->emulops->copyrows)(edp->emulcookie,
					  edp->scrreg_startrow,
					  edp->scrreg_startrow + n,
					  help);
	(*edp->emulops->eraserows)(edp->emulcookie,
				   edp->scrreg_startrow, n,
				   edp->curattr);
}

/*
 * erase in display
 */
void
wsemul_vt100_ed(edp, arg)
	struct wsemul_vt100_emuldata *edp;
	int arg;
{
	int n;

	switch (arg) {
	    case 0: /* cursor to end */
		(*edp->emulops->erasecols)(edp->emulcookie, edp->crow,
					   edp->ccol, COLS_LEFT + 1,
					   edp->curattr);
		n = edp->nrows - edp->crow - 1;
		if (n > 0) {
			(*edp->emulops->eraserows)(edp->emulcookie,
						   edp->crow + 1, n,
						   edp->curattr);
		}
		break;
	    case 1: /* beginning to cursor */
		if (edp->crow > 0) {
			(*edp->emulops->eraserows)(edp->emulcookie,
						   0, edp->crow,
						   edp->curattr);
		}
		(*edp->emulops->erasecols)(edp->emulcookie, edp->crow,
					   0, edp->ccol + 1,
					   edp->curattr);
		break;
	    case 2: /* complete display */
		(*edp->emulops->eraserows)(edp->emulcookie,
					   0, edp->nrows,
					   edp->curattr);
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("ed(%d) unknown\n", arg);
#endif
		break;
	}
}

/*
 * erase in line
 */
void
wsemul_vt100_el(edp, arg)
	struct wsemul_vt100_emuldata *edp;
	int arg;
{
	switch (arg) {
	    case 0: /* cursor to end */
		(*edp->emulops->erasecols)(edp->emulcookie, edp->crow,
					   edp->ccol, COLS_LEFT + 1,
					   edp->curattr);
		break;
	    case 1: /* beginning to cursor */
		(*edp->emulops->erasecols)(edp->emulcookie, edp->crow,
					   0, edp->ccol + 1,
					   edp->curattr);
		break;
	    case 2: /* complete line */
		(*edp->emulops->erasecols)(edp->emulcookie, edp->crow,
					   0, edp->ncols,
					   edp->curattr);
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
wsemul_vt100_handle_csi(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	int n, help, flags, fgcol, bgcol;
	long attr;

	switch (c) {
	    case '@': /* ICH insert character VT300 only */
		n = min(DEF1_ARG(0), COLS_LEFT + 1);
		help = edp->ncols - (edp->ccol + n);
		if (help > 0)
			(*edp->emulops->copycols)(edp->emulcookie, edp->crow,
						  edp->ccol, edp->ccol + n,
						  help);
		(*edp->emulops->erasecols)(edp->emulcookie, edp->crow,
					   edp->ccol, n, edp->curattr);
		break;
	    case 'A': /* CUU */
		edp->crow -= min(DEF1_ARG(0), ROWS_ABOVE);
		break;
	    case 'B': /* CUD */
		edp->crow += min(DEF1_ARG(0), ROWS_BELOW);
		break;
	    case 'C': /* CUF */
		edp->ccol += min(DEF1_ARG(0), COLS_LEFT);
		break;
	    case 'D': /* CUB */
		edp->ccol -= min(DEF1_ARG(0), edp->ccol);
		edp->flags &= ~VTFL_LASTCHAR;
		break;
	    case 'H': /* CUP */
	    case 'f': /* HVP */
		if (edp->flags & VTFL_DECOM)
			edp->crow = edp->scrreg_startrow +
			    min(DEF1_ARG(0), edp->scrreg_nrows) - 1;
		else
			edp->crow = min(DEF1_ARG(0), edp->nrows) - 1;
		edp->ccol = min(DEF1_ARG(1), edp->ncols) - 1;
		edp->flags &= ~VTFL_LASTCHAR;
		break;
	    case 'J': /* ED erase in display */
		wsemul_vt100_ed(edp, ARG(0));
		break;
	    case 'K': /* EL erase in line */
		wsemul_vt100_el(edp, ARG(0));
		break;
	    case 'L': /* IL insert line */
	    case 'M': /* DL delete line */
		n = min(DEF1_ARG(0), ROWS_BELOW + 1);
		{
			int savscrstartrow, savscrnrows;
			savscrstartrow = edp->scrreg_startrow;
			savscrnrows = edp->scrreg_nrows;
			edp->scrreg_nrows -= ROWS_ABOVE;
			edp->scrreg_startrow = edp->crow;
			if (c == 'L')
				wsemul_vt100_scrolldown(edp, n);
			else
				wsemul_vt100_scrollup(edp, n);
			edp->scrreg_startrow = savscrstartrow;
			edp->scrreg_nrows = savscrnrows;
		}
		break;
	    case 'P': /* DCH delete character */
		n = min(DEF1_ARG(0), COLS_LEFT + 1);
		help = edp->ncols - (edp->ccol + n);
		if (help > 0)
			(*edp->emulops->copycols)(edp->emulcookie, edp->crow,
						  edp->ccol + n, edp->ccol,
						  help);
		(*edp->emulops->erasecols)(edp->emulcookie, edp->crow,
					   edp->ncols - n, n, edp->curattr);
		break;
	    case 'X': /* ECH erase character */
		n = min(DEF1_ARG(0), COLS_LEFT + 1);
		(*edp->emulops->erasecols)(edp->emulcookie, edp->crow,
					   edp->ccol, n,
					   edp->curattr);
		break;
	    case 'c': /* DA primary */
		if (ARG(0) == 0)
			wsdisplay_emulinput(edp->cbcookie, WSEMUL_VT_ID1,
					    sizeof(WSEMUL_VT_ID1));
		break;
	    case 'g': /* TBC */
		KASSERT(edp->tabs != 0);
		switch (ARG(0)) {
		    case 0:
			edp->tabs[edp->ccol] = 0;
			break;
		    case 3:
			bzero(edp->tabs, edp->ncols);
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%dg unknown\n", ARG(0));
#endif
			break;
		}
		break;
	    case 'h': /* SM */
		vt100_setmode(edp, ARG(0), 0);
		break;
	    case 'i': /* MC printer controller mode */
		switch (ARG(0)) {
		    case 0: /* print screen */
			break;
		    case 4: /* off */
		    case 5: /* on */
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%di unknown\n", ARG(0));
#endif
			break;
		}
		break;
	    case 'l': /* RM */
		vt100_resetmode(edp, ARG(0), 0);
		break;
	    case 'm': /* SGR select graphic rendition */
		flags = edp->attrflags;
		fgcol = edp->fgcol;
		bgcol = edp->bgcol;
		for (n = 0; n < edp->nargs; n++) {
			switch (ARG(n)) {
			    case 0: /* reset */
				edp->attrflags = 0;
				edp->fgcol = WSCOL_WHITE;
				edp->bgcol = WSCOL_BLACK;
				if (n == edp->nargs - 1) {
					edp->curattr = edp->defattr;
					return;
				}
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
				fgcol = ARG(n) - 30;
				break;
			    case 40: case 41: case 42: case 43:
			    case 44: case 45: case 46: case 47:
				/* bg color */
				flags |= WSATTR_WSCOLORS;
				bgcol = ARG(n) - 40;
				break;
			    default:
#ifdef VT100_PRINTUNKNOWN
				printf("CSI%dm unknown\n", ARG(n));
#endif
			}
		}
		if (vt100_selectattribute(edp, flags, fgcol, bgcol, &attr)) {
#ifdef VT100_DEBUG
			printf("error allocating attr %d/%d/%x\n",
			       fgcol, bgcol, flags);
#endif
		} else {
			edp->curattr = attr;
			edp->attrflags = flags;
			edp->fgcol = fgcol;
			edp->bgcol = bgcol;
		}
		break;
	    case 'n': /* reports */
		switch (ARG(0)) {
		    case 5: /* DSR operating status */
			/* 0 = OK, 3 = malfunction */
			wsdisplay_emulinput(edp->cbcookie, "\033[0n", 4);
			break;
		    case 6: { /* DSR cursor position report */
			char buf[20];
			int row;
			if (edp->flags & VTFL_DECOM)
				row = ROWS_ABOVE;
			else
				row = edp->crow;
			n = sprintf(buf, "\033[%d;%dR",
				    row + 1, edp->ccol + 1);
			wsdisplay_emulinput(edp->cbcookie, buf, n);
			}
			break;
		    case 15: /* DSR printer status */
			/* 13 = no printer, 10 = ready, 11 = not ready */
			wsdisplay_emulinput(edp->cbcookie, "\033[?13n", 6);
			break;
		    case 25: /* UDK status - VT300 only */
			/* 20 = locked, 21 = unlocked */
			wsdisplay_emulinput(edp->cbcookie, "\033[?21n", 6);
			break;
		    case 26: /* keyboard dialect */
			/* 1 = north american , 7 = german */
			wsdisplay_emulinput(edp->cbcookie, "\033[?27;1n", 8);
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%dn unknown\n", ARG(0));
#endif
			break;
		}
		break;
	    case 'r': /* DECSTBM set top/bottom margins */
		if (ARG(0) == 0 && ARG(1) == 0) {
			edp->scrreg_startrow = 0;
			edp->scrreg_nrows = edp->nrows;
		} else {
			edp->scrreg_startrow = min(DEF1_ARG(0),
						   edp->nrows) - 1;
			edp->scrreg_nrows = min(DEF1_ARG(1), edp->nrows) -
			    edp->scrreg_startrow;
		}
		edp->crow = ((edp->flags & VTFL_DECOM) ?
			     edp->scrreg_startrow : 0);
		edp->ccol = 0;
		break;
	    case 'y':
		switch (ARG(0)) {
		    case 4: /* DECTST invoke confidence test */
			/* ignore */
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%dy unknown\n", ARG(0));
#endif
			break;
		}
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("CSI%c (%d, %d) unknown\n", c, ARG(0), ARG(1));
#endif
	}
}

/*
 * handle commands after CSI?
 */
void
wsemul_vt100_handle_csi_qm(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	switch (c) {
	    case 'J': /* DECSED selective erase in display */
		wsemul_vt100_ed(edp, ARG(0));
		break;
	    case 'K': /* DECSEL selective erase in line */
		wsemul_vt100_el(edp, ARG(0));
		break;
	    case 'h': /* SM, DEC private */
		vt100_setmode(edp, ARG(0), 1);
		break;
	    case 'i': /* MC printer controller mode */
		switch (ARG(0)) {
		    case 1: /* print cursor line */
		    case 4: /* off */
		    case 5: /* on */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI?%di ignored\n", ARG(0));
#endif
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI?%di unknown\n", ARG(0));
#endif
			break;
		}
		break;
	    case 'l': /* RM, DEC private */
		vt100_resetmode(edp, ARG(0), 1);
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("CSI?%c (%d, %d) unknown\n", c, ARG(0), ARG(1));
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
vt100_selectattribute(edp, flags, fgcol, bgcol, attr)
	struct wsemul_vt100_emuldata *edp;
	int flags, fgcol, bgcol;
	long *attr;
{
	if ((flags & WSATTR_HILIT) &&
	    !(edp->scrcapabilities & WSSCREEN_HILIT)) {
		flags &= ~WSATTR_HILIT;
		if (edp->scrcapabilities & WSSCREEN_WSCOLORS) {
			fgcol = WSCOL_RED;
			flags |= WSATTR_WSCOLORS;
		} else {
#ifdef VT100_DEBUG
			printf("bold ignored (impossible)\n");
#endif
		}
	}
	if ((flags & WSATTR_UNDERLINE) &&
	    !(edp->scrcapabilities & WSSCREEN_UNDERLINE)) {
		flags &= ~WSATTR_UNDERLINE;
		if (edp->scrcapabilities & WSSCREEN_WSCOLORS) {
			bgcol = WSCOL_BROWN;
			flags &= ~WSATTR_UNDERLINE;
			flags |= WSATTR_WSCOLORS;
		} else {
#ifdef VT100_DEBUG
			printf("underline ignored (impossible)\n");
#endif
		}
	}
	if ((flags & WSATTR_BLINK) &&
	    !(edp->scrcapabilities & WSSCREEN_BLINK)) {
		flags &= ~WSATTR_BLINK;
#ifdef VT100_DEBUG
		printf("blink ignored (impossible)\n");
#endif
	}
	if ((flags & WSATTR_REVERSE) &&
	    !(edp->scrcapabilities & WSSCREEN_REVERSE)) {
		flags &= ~WSATTR_REVERSE;
		if (edp->scrcapabilities & WSSCREEN_WSCOLORS) {
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
	if ((flags & WSATTR_WSCOLORS) &&
	    !(edp->scrcapabilities & WSSCREEN_WSCOLORS)) {
		flags &= ~WSATTR_WSCOLORS;
#ifdef VT100_DEBUG
		printf("colors ignored (impossible)\n");
#endif
	}
	return ((*edp->emulops->alloc_attr)(edp->emulcookie,
					    fgcol, bgcol, flags,
					    attr));
}

/*
 * handle device control sequences if the main state machine
 * told so by setting edp->dcstype to a nonzero value
 */
void
wsemul_vt100_handle_dcs(edp)
	struct wsemul_vt100_emuldata *edp;
{
	int i, pos;

	switch (edp->dcstype) {
	    case 0: /* not handled */
		return;
	    case DCSTYPE_TABRESTORE:
		KASSERT(edp->tabs != 0);
		bzero(edp->tabs, edp->ncols);
		pos = 0;
		for (i = 0; i < edp->dcspos; i++) {
			char c = edp->dcsarg[i];
			switch (c) {
			    case '0': case '1': case '2': case '3': case '4':
			    case '5': case '6': case '7': case '8': case '9':
				pos = pos * 10 + (edp->dcsarg[i] - '0');
				break;
			    case '/':
				if (pos > 0)
					edp->tabs[pos - 1] = 1;
				pos = 0;
				break;
			    default:
#ifdef VT100_PRINTUNKNOWN
				printf("unknown char %c in DCS\n", c);
#endif
			}
		}
		if (pos > 0)
			edp->tabs[pos - 1] = 1;
		break;
	    default:
		panic("wsemul_vt100_handle_dcs: bad type %d", edp->dcstype);
	}
	edp->dcstype = 0;
}

static int
vt100_ansimode(edp, nr, op)
	struct wsemul_vt100_emuldata *edp;
	int nr, op;
{
	int res = 0; /* default: unknown */

	switch (nr) {
	    case 2: /* KAM keyboard locked/unlocked */
		break;
	    case 3: /* CRM control representation */
		break;
	    case 4: /* IRM insert/replace characters */
		if (op == MODE_SET)
			edp->flags |= VTFL_INSERTMODE;
		else if (op == MODE_RESET)
			edp->flags &= ~VTFL_INSERTMODE;
		res = ((edp->flags & VTFL_INSERTMODE) ? 1 : 2);
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
	}
	return (res);
}

static int
vt100_decmode(edp, nr, op)
	struct wsemul_vt100_emuldata *edp;
	int nr, op;
{
	int res = 0; /* default: unknown */

	switch (nr) {
	    case 1: /* DECCKM application/nomal cursor keys */
		if (op == MODE_SET)
			edp->flags |= VTFL_APPLCURSOR;
		else if (op == MODE_RESET)
			edp->flags &= ~VTFL_APPLCURSOR;
		res = ((edp->flags & VTFL_APPLCURSOR) ? 1 : 2);
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
		if (op == MODE_SET)
			edp->flags |= VTFL_DECOM;
		else if (op == MODE_RESET)
			edp->flags &= ~VTFL_DECOM;
		res = ((edp->flags & VTFL_DECOM) ? 1 : 2);
		break;
	    case 7: /* DECAWM autowrap */
		if (op == MODE_SET)
			edp->flags |= VTFL_DECAWM;
		else if (op == MODE_RESET)
			edp->flags &= ~VTFL_DECAWM;
		res = ((edp->flags & VTFL_DECAWM) ? 1 : 2);
		break;
	    case 8: /* DECARM keyboard autorepeat */
		break;
	    case 18: /* DECPFF print form feed */
		break;
	    case 19: /* DECPEX printer extent: screen/scrolling region */
		break;
	    case 25: /* DECTCEM text cursor on/off */
		if (op == MODE_SET || op == MODE_RESET) {
			edp->flags = (edp->flags & ~VTFL_CURSORON) |
			    ((op == MODE_SET) ? VTFL_CURSORON : 0);
			(*edp->emulops->cursor)(edp->emulcookie,
						(op == MODE_SET),
						edp->crow, edp->ccol);
		}
		res = ((edp->flags & VTFL_CURSORON) ? 1 : 2);
		break;
	    case 42: /* DECNRCM use 7-bit NRC /
		      7/8 bit from DEC multilingual or ISO-latin-1*/
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
	return (res);
}

void
vt100_setmode(edp, nr, decmode)
	struct wsemul_vt100_emuldata *edp;
	int nr, decmode;
{
	if (decmode)
		vt100_decmode(edp, nr, MODE_SET);
	else
		vt100_ansimode(edp, nr, MODE_SET);
}

void
vt100_resetmode(edp, nr, decmode)
	struct wsemul_vt100_emuldata *edp;
	int nr, decmode;
{
	if (decmode)
		vt100_decmode(edp, nr, MODE_RESET);
	else
		vt100_ansimode(edp, nr, MODE_RESET);
}

void
vt100_reportmode(edp, nr, decmode)
	struct wsemul_vt100_emuldata *edp;
	int nr, decmode;
{
	char buf[20];
	int res, n;

	if (decmode)
		res = vt100_decmode(edp, nr, MODE_REPORT);
	else
		res = vt100_ansimode(edp, nr, MODE_REPORT);

	n = sprintf(buf, "\033[%s%d;%d$y", (decmode ? "?" : ""), nr, res);
	wsdisplay_emulinput(edp->cbcookie, buf, n);
}
