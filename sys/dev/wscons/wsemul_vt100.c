/* $NetBSD: wsemul_vt100.c,v 1.2 1998/06/20 21:52:50 drochner Exp $ */

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
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/fcntl.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsemulvar.h>
#include <dev/wscons/wsemul_vt100var.h>
#include <dev/wscons/ascii.h>

#include "opt_wskernattr.h"

void	*wsemul_vt100_cnattach __P((const struct wsscreen_descr *, void *,
				  int, int, long));
void	*wsemul_vt100_attach __P((int console, const struct wsscreen_descr *,
				  void *, int, int, void *, long));
void	wsemul_vt100_output __P((void *cookie, const u_char *data, u_int count,
				 int));
void	wsemul_vt100_detach __P((void *cookie, u_int *crowp, u_int *ccolp));

const struct wsemul_ops wsemul_vt100_ops = {
	"vt100",
	wsemul_vt100_cnattach,
	wsemul_vt100_attach,
	wsemul_vt100_output,
	wsemul_vt100_translate,
	wsemul_vt100_detach,
};

struct wsemul_vt100_emuldata wsemul_vt100_console_emuldata;

static void wsemul_vt100_init __P((struct wsemul_vt100_emuldata *,
				   const struct wsscreen_descr *,
				   void *, int, int, long));

static u_int wsemul_vt100_output_normal __P((struct wsemul_vt100_emuldata *,
					     u_char, int));
typedef u_int vt100_handler __P((struct wsemul_vt100_emuldata *, u_char));
static vt100_handler
wsemul_vt100_output_esc,
wsemul_vt100_output_csi,
wsemul_vt100_output_csi_qm,
wsemul_vt100_output_scs94,
wsemul_vt100_output_scs94_percent,
wsemul_vt100_output_scs96,
wsemul_vt100_output_scs96_percent,
wsemul_vt100_output_hash,
wsemul_vt100_output_esc_spc,
wsemul_vt100_output_csi_gt,
wsemul_vt100_output_string,
wsemul_vt100_output_string_esc,
wsemul_vt100_output_csi_dq,
wsemul_vt100_output_dcs,
wsemul_vt100_output_csi_dollar,
wsemul_vt100_output_csi_qm_dollar,
wsemul_vt100_output_csi_amp,
wsemul_vt100_output_csi_em,
wsemul_vt100_output_dcs_dollar;

#define	VT100_EMUL_STATE_NORMAL		0	/* normal processing */
#define	VT100_EMUL_STATE_ESC		1	/* got ESC */
#define	VT100_EMUL_STATE_CSI		2	/* got CSI (ESC[) */
#define	VT100_EMUL_STATE_CSI_QM		3	/* got CSI? */
#define	VT100_EMUL_STATE_SCS94		4	/* got ESC{()*+} */
#define	VT100_EMUL_STATE_SCS94_PERCENT	5	/* got ESC{()*+}% */
#define	VT100_EMUL_STATE_SCS96		6	/* got ESC{-./} */
#define	VT100_EMUL_STATE_SCS96_PERCENT	7	/* got ESC{-./}% */
#define	VT100_EMUL_STATE_HASH		8	/* got ESC# */
#define	VT100_EMUL_STATE_ESC_SPC	9	/* got ESC<SPC> */
#define	VT100_EMUL_STATE_CSI_GT		10	/* got CSI> */
#define	VT100_EMUL_STATE_STRING		11	/* waiting for ST (ESC\) */
#define	VT100_EMUL_STATE_STRING_ESC	12	/* waiting for ST, got ESC */
#define	VT100_EMUL_STATE_CSI_DQ		13	/* got CSI<p>" */
#define	VT100_EMUL_STATE_DCS		14	/* got DCS (ESC P) */
#define	VT100_EMUL_STATE_CSI_DOLLAR	15	/* got CSI<p>$ */
#define	VT100_EMUL_STATE_CSI_QM_DOLLAR	16	/* got CSI?<p>$ */
#define	VT100_EMUL_STATE_CSI_AMP	17	/* got CSI& */
#define	VT100_EMUL_STATE_CSI_EM		18	/* got CSI! */
#define	VT100_EMUL_STATE_DCS_DOLLAR	19	/* got DCS<p>$ */

vt100_handler *vt100_output[] = {
	wsemul_vt100_output_esc,
	wsemul_vt100_output_csi,
	wsemul_vt100_output_csi_qm,
	wsemul_vt100_output_scs94,
	wsemul_vt100_output_scs94_percent,
	wsemul_vt100_output_scs96,
	wsemul_vt100_output_scs96_percent,
	wsemul_vt100_output_hash,
	wsemul_vt100_output_esc_spc,
	wsemul_vt100_output_csi_gt,
	wsemul_vt100_output_string,
	wsemul_vt100_output_string_esc,
	wsemul_vt100_output_csi_dq,
	wsemul_vt100_output_dcs,
	wsemul_vt100_output_csi_dollar,
	wsemul_vt100_output_csi_qm_dollar,
	wsemul_vt100_output_csi_amp,
	wsemul_vt100_output_csi_em,
	wsemul_vt100_output_dcs_dollar,
};

static void
wsemul_vt100_init(edp, type, cookie, ccol, crow, defattr)
	struct wsemul_vt100_emuldata *edp;
	const struct wsscreen_descr *type;
	void *cookie;
	int ccol, crow;
	long defattr;
{
	edp->emulops = type->textops;
	edp->emulcookie = cookie;
	edp->scrcapabilities = type->capabilities;
	edp->nrows = type->nrows;
	edp->ncols = type->ncols;
	edp->crow = crow;
	edp->ccol = ccol;
	edp->defattr = defattr;
}

void *
wsemul_vt100_cnattach(type, cookie, ccol, crow, defattr)
	const struct wsscreen_descr *type;
	void *cookie;
	int ccol, crow;
	long defattr;
{
	struct wsemul_vt100_emuldata *edp;
	int res;

	edp = &wsemul_vt100_console_emuldata;
	wsemul_vt100_init(edp, type, cookie, ccol, crow, defattr);
#ifdef DIAGNOSTIC
	edp->console = 1;
#endif
	edp->cbcookie = NULL;

#if defined(WS_KERNEL_FG) || defined(WS_KERNEL_BG) || \
  defined(WS_KERNEL_COLATTR) || defined(WS_KERNEL_MONOATTR)
#ifndef WS_KERNEL_FG
#define WS_KERNEL_FG WSCOL_WHITE
#endif
#ifndef WS_KERNEL_BG
#define WS_KERNEL_BG WSCOL_BLACK
#endif
#ifndef WS_KERNEL_COLATTR
#define WS_KERNEL_COLATTR 0
#endif
#ifndef WS_KERNEL_MONOATTR
#define WS_KERNEL_MONOATTR 0
#endif
	if (type->capabilities & WSSCREEN_WSCOLORS)
		res = (*edp->emulops->alloc_attr)(cookie,
					    WS_KERNEL_FG, WS_KERNEL_BG,
					    WS_KERNEL_COLATTR | WSATTR_WSCOLORS,
					    &edp->kernattr);
	else
		res = (*edp->emulops->alloc_attr)(cookie, 0, 0,
					    WS_KERNEL_MONOATTR,
					    &edp->kernattr);
	if (res)
#endif
	edp->kernattr = defattr;

	edp->tabs = 0;
	edp->dcsarg = 0;
	wsemul_vt100_reset(edp);
	return (edp);
}

void *
wsemul_vt100_attach(console, type, cookie, ccol, crow, cbcookie, defattr)
	int console;
	const struct wsscreen_descr *type;
	void *cookie;
	int ccol, crow;
	void *cbcookie;
	long defattr;
{
	struct wsemul_vt100_emuldata *edp;

	if (console) {
		edp = &wsemul_vt100_console_emuldata;
#ifdef DIAGNOSTIC
		KASSERT(edp->console == 1);
#endif
	} else {
		edp = malloc(sizeof *edp, M_DEVBUF, M_WAITOK);
		wsemul_vt100_init(edp, type, cookie, ccol, crow, defattr);
#ifdef DIAGNOSTIC
		edp->console = 0;
#endif
	}
	edp->cbcookie = cbcookie;

	edp->tabs = malloc(type->ncols, M_DEVBUF, M_NOWAIT);
	edp->dcsarg = malloc(DCS_MAXLEN, M_DEVBUF, M_NOWAIT);
	wsemul_vt100_reset(edp);
	return (edp);
}

void
wsemul_vt100_detach(cookie, crowp, ccolp)
	void *cookie;
	u_int *crowp, *ccolp;
{
	struct wsemul_vt100_emuldata *edp = cookie;

	*crowp = edp->crow;
	*ccolp = edp->ccol;
	if (edp->tabs) {
		free(edp->tabs, M_DEVBUF);
		edp->tabs = 0;
	}
	if (edp->dcsarg) {
		free(edp->dcsarg, M_DEVBUF);
		edp->dcsarg = 0;
	}
	if (edp != &wsemul_vt100_console_emuldata)
		free(edp, M_DEVBUF);
}

void
wsemul_vt100_reset(edp)
	struct wsemul_vt100_emuldata *edp;
{
	int i;

	edp->state = VT100_EMUL_STATE_NORMAL;
	edp->flags = VTFL_DECAWM | VTFL_CURSORON;
	edp->curattr = edp->defattr;
	edp->attrflags = 0;
	edp->fgcol = WSCOL_WHITE;
	edp->bgcol = WSCOL_BLACK;
	edp->scrreg_startrow = 0;
	edp->scrreg_nrows = edp->nrows;
	if (edp->tabs) {
		bzero(edp->tabs, edp->ncols);
		for (i = 1; i < edp->ncols; i += 8 - (i & 7))
			edp->tabs[i] = 1;
	}
	edp->dcspos = 0;
	edp->dcstype = 0;
}

/*
 * now all the state machine bits
 */

static u_int
wsemul_vt100_output_normal(edp, c, kernel)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
	int kernel;
{
	u_int newstate = VT100_EMUL_STATE_NORMAL;
	u_int n;

	switch (c) {
	    case ASCII_NUL:
		/* ignore */
		break;
	    case ASCII_BEL:
		wsdisplay_emulbell(edp->cbcookie);
		break;
	    case ASCII_BS:
		if (edp->ccol > 0) {
			edp->ccol--;
			edp->flags &= ~VTFL_LASTCHAR;
		}
		break;
	    case ASCII_CR:
		edp->ccol = 0;
		edp->flags &= ~VTFL_LASTCHAR;
		break;
	    case ASCII_HT:
		if (edp->tabs) {
			if (edp->ccol >= edp->ncols - 1)
				break;
			for (n = edp->ccol + 1; n < edp->ncols - 1; n++)
				if (edp->tabs[n])
					break;
		} else {
			n = edp->ccol + min(8 - (edp->ccol & 7), COLS_LEFT);
		}
		(*edp->emulops->erasecols)(edp->emulcookie, edp->crow,
					   edp->ccol, n - edp->ccol,
				     kernel ? edp->kernattr : edp->curattr);
		edp->ccol = n;
		break;
	    case ASCII_SI: /* LS0 */
#ifdef VT100_PRINTNOTIMPL
		printf("SI ignored\n");
#endif
		break;
	    case ASCII_SO: /* LS1 */
#ifdef VT100_PRINTNOTIMPL
		printf("SO ignored\n");
#endif
		break;
	    case ASCII_ESC:
#ifdef DIAGNOSTIC
		if (kernel)
			panic("ESC in kernel output");
#endif
		newstate = VT100_EMUL_STATE_ESC;
		break;
#if 0
	    case CSI: /* 8-bit */
		edp->nargs = 0;
		bzero(edp->args, sizeof (edp->args));
		newstate = VT100_EMUL_STATE_CSI;
		break;
	    case DCS: /* 8-bit */
		edp->nargs = 0;
		bzero(edp->args, sizeof (edp->args));
		newstate = VT100_EMUL_STATE_DCS;
		break;
#endif
	    default: /* normal character */
		if ((edp->flags & (VTFL_LASTCHAR | VTFL_DECAWM)) ==
		    (VTFL_LASTCHAR | VTFL_DECAWM)) {
			if (ROWS_BELOW > 0)
				edp->crow++;
			else
				wsemul_vt100_scrollup(edp, 1);
			edp->ccol = 0;
			edp->flags &= ~VTFL_LASTCHAR;
		}

		if ((edp->flags & VTFL_INSERTMODE) &&
		    edp->ccol < edp->ncols - 1) {
			(*edp->emulops->copycols)(edp->emulcookie, edp->crow,
						  edp->ccol, edp->ccol + 1,
						  edp->ncols - edp->ccol - 1);
		}

		(*edp->emulops->putchar)(edp->emulcookie, edp->crow, edp->ccol,
					 c, kernel ? edp->kernattr : edp->curattr);

		if (edp->ccol < edp->ncols - 1)
			edp->ccol++;
		else
			edp->flags |= VTFL_LASTCHAR;
		break;
	    case ASCII_LF:
	    case ASCII_VT:
	    case ASCII_FF:
		if (ROWS_BELOW > 0)
			edp->crow++;
		else
			wsemul_vt100_scrollup(edp, 1);
		break;
	}

	return (newstate);
}

static u_int
wsemul_vt100_output_esc(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	u_int newstate = VT100_EMUL_STATE_NORMAL;

	switch (c) {
	    case '[': /* CSI */
		edp->nargs = 0;
		bzero(edp->args, sizeof (edp->args));
		newstate = VT100_EMUL_STATE_CSI;
		break;
	    case '7': /* DECSC */
		edp->savedcursor_row = edp->crow;
		edp->savedcursor_col = edp->ccol;
		/* save attributes! */
		break;
	    case '8': /* DECRC */
		edp->crow = edp->savedcursor_row;
		edp->ccol = edp->savedcursor_col;
		/* restore attributes! */
		break;
	    case '=': /* DECKPAM application mode */
		edp->flags |= VTFL_APPLKEYPAD;
		break;
	    case '>': /* DECKPNM numeric mode */
		edp->flags &= ~VTFL_APPLKEYPAD;
		break;
	    case 'E': /* NEL */
		edp->ccol = 0;
		/* FALLTHRU */
	    case 'D': /* IND */
		if (ROWS_BELOW > 0) {
			edp->crow++;
			break;
		}
		wsemul_vt100_scrollup(edp, 1);
		break;
	    case 'H': /* HTS */
		KASSERT(edp->tabs != 0);
		edp->tabs[edp->ccol] = 1;
		break;
	    case '~': /* LS1R */
	    case 'n': /* LS2 */
	    case '}': /* LS2R */
	    case 'o': /* LS3 */
	    case '|': /* LS3R */
#ifdef VT100_PRINTNOTIMPL
		printf("ESC %c ignored\n", c);
#endif
		break;
	    case 'N': /* SS2 */
	    case 'O': /* SS3 */
#ifdef VT100_PRINTNOTIMPL
		printf("ESC %c ignored\n", c);
#endif
		break;
	    case 'M': /* RI */
		if (ROWS_ABOVE > 0) {
			edp->crow--;
			break;
		}
		wsemul_vt100_scrolldown(edp, 1);
		break;
	    case 'P': /* DCS */
		edp->nargs = 0;
		bzero(edp->args, sizeof (edp->args));
		newstate = VT100_EMUL_STATE_DCS;
		break;
	    case 'c': /* RIS */
		wsemul_vt100_reset(edp);
		edp->ccol = edp->crow = 0;
		break;
	    case '(': case ')': case '*': case '+': /* SCS */
		edp->designating = c;
		newstate = VT100_EMUL_STATE_SCS94;
		break;
	    case '-': case '.': case '/': /* SCS */
		edp->designating = c;
		newstate = VT100_EMUL_STATE_SCS96;
		break;
	    case '#':
		newstate = VT100_EMUL_STATE_HASH;
		break;
	    case ' ': /* 7/8 bit */
		newstate = VT100_EMUL_STATE_ESC_SPC;
		break;
	    case ']': /* OSC operating system command */
	    case '^': /* PM privacy message */
	    case '_': /* APC application program command */
		/* ignored */
		newstate = VT100_EMUL_STATE_STRING;
		break;
	    case '<': /* exit VT52 mode - ignored */
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("ESC%c unknown\n", c);
#endif
		break;
	}

	return (newstate);
}

static u_int
wsemul_vt100_output_csi_qm(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	u_int newstate = VT100_EMUL_STATE_CSI_QM;

	switch (c) {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		/* argument digit */
		if (edp->nargs > VT100_EMUL_NARGS - 1)
			break;
		edp->args[edp->nargs] = (edp->args[edp->nargs] * 10) +
		    (c - '0');
		break;
	    case ';': /* argument terminator */
		edp->nargs++;
		break;
	    case '$': /* request mode */
		newstate = VT100_EMUL_STATE_CSI_QM_DOLLAR;
		break;
	    default: /* end of escape sequence */
		edp->nargs++;
		if (edp->nargs > VT100_EMUL_NARGS) {
#ifdef VT100_DEBUG
			printf("vt100: too many arguments\n");
#endif
			edp->nargs = VT100_EMUL_NARGS;
		}
		wsemul_vt100_handle_csi_qm(edp, c);
		newstate = VT100_EMUL_STATE_NORMAL;
		break;
	}
	return (newstate);
}

static u_int
wsemul_vt100_output_csi_qm_dollar(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	u_int newstate = VT100_EMUL_STATE_NORMAL;

	switch (c) {
	    case 'p': /* DECRQM request DEC private mode */
		vt100_reportmode(edp, ARG(0), 1);
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("CSI?%c unknown\n", c);
#endif
		break;
	}

	return (newstate);
}

static u_int
wsemul_vt100_output_csi_amp(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	u_int newstate = VT100_EMUL_STATE_NORMAL;

	switch (c) {
	    case 'u': /* DECRQUPSS request user preferred supplemental set */
		wsdisplay_emulinput(edp->emulcookie, "\033P0!u%5\033\\", 9);
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("CSI&%c unknown\n", c);
#endif
		break;
	}

	return (newstate);
}

static u_int
wsemul_vt100_output_scs94(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	u_int newstate = VT100_EMUL_STATE_NORMAL;

	switch (c) {
	    case '%': /* probably DEC supplemental graphic */
		newstate = VT100_EMUL_STATE_SCS94_PERCENT;
		break;
	    case 'B': /* ASCII */
	    case 'A': /* ISO-latin-1 supplemental */
	    case '<': /* user preferred supplemental */
	    case '0': /* DEC special graphic */
#ifdef VT100_PRINTNOTIMPL
		printf("ESC%c%c ignored\n", edp->designating, c);
#endif
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("ESC%c%c unknown\n", edp->designating, c);
#endif
		break;
	}
	return (newstate);
}

static u_int
wsemul_vt100_output_scs94_percent(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	switch (c) {
	    case '5': /* DEC supplemental graphic */
#ifdef VT100_PRINTNOTIMPL
		printf("ESC%c%%5 ignored\n", edp->designating);
#endif
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("ESC%c%%%c unknown\n", edp->designating, c);
#endif
		break;
	}
	return (VT100_EMUL_STATE_NORMAL);
}

static u_int
wsemul_vt100_output_scs96(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	u_int newstate = VT100_EMUL_STATE_NORMAL;

	switch (c) {
	    case '%': /* probably portugese */
		newstate = VT100_EMUL_STATE_SCS96_PERCENT;
		break;
	    case 'A': /* british */
	    case '4': /* dutch */
	    case '5': case 'C': /* finnish */
	    case 'R': /* french */
	    case 'Q': /* french canadian */
	    case 'K': /* german */
	    case 'Y': /* italian */
	    case 'E': case '6': /* norwegian / danish */
	    case 'Z': /* spanish */
	    case '7': case 'H': /* swedish */
	    case '=': /* swiss */
#ifdef VT100_PRINTNOTIMPL
		printf("ESC%c%c ignored\n", edp->designating, c);
#endif
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("ESC%c%c unknown\n", edp->designating, c);
#endif
		break;
	}
	return (newstate);
}

static u_int
wsemul_vt100_output_scs96_percent(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	switch (c) {
	    case '6': /* portugese */
#ifdef VT100_PRINTNOTIMPL
		printf("ESC%c%%6 ignored\n", edp->designating);
#endif
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("ESC%c%%%c unknown\n", edp->designating, c);
#endif
		break;
	}
	return (VT100_EMUL_STATE_NORMAL);
}

static u_int
wsemul_vt100_output_esc_spc(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	switch (c) {
	    case 'F': /* 7-bit controls */
	    case 'G': /* 8-bit controls */
#ifdef VT100_PRINTNOTIMPL
		printf("ESC<SPC>%c ignored\n", c);
#endif
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("ESC<SPC>%c unknown\n", c);
#endif
		break;
	}
	return (VT100_EMUL_STATE_NORMAL);
}

static u_int
wsemul_vt100_output_string(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	switch (c) {
	    case ASCII_ESC: /* might be a string end */
		return (VT100_EMUL_STATE_STRING_ESC);
#if 0
	    case ST: /* string end 8-bit */
		wsemul_vt100_handle_dcs(edp);
		return (VT100_EMUL_STATE_NORMAL);
#endif
	    default:
		if (edp->dcstype && edp->dcspos < DCS_MAXLEN)
			edp->dcsarg[edp->dcspos++] = c;
	}
	return (VT100_EMUL_STATE_STRING);
}

static u_int
wsemul_vt100_output_string_esc(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	if (c == '\\') { /* ST complete */
		wsemul_vt100_handle_dcs(edp);
		return (VT100_EMUL_STATE_NORMAL);
	} else
		return (VT100_EMUL_STATE_STRING);
}

static u_int
wsemul_vt100_output_csi_em(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	switch (c) {
	    case 'p': /* DECSTR soft reset VT300 only */
		wsemul_vt100_reset(edp);
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("ESC!%c unknown\n", c);
#endif
		break;
	}
	return (VT100_EMUL_STATE_NORMAL);
}

static u_int
wsemul_vt100_output_dcs(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	u_int newstate = VT100_EMUL_STATE_DCS;

	switch (c) {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		/* argument digit */
		if (edp->nargs > VT100_EMUL_NARGS - 1)
			break;
		edp->args[edp->nargs] = (edp->args[edp->nargs] * 10) +
		    (c - '0');
		break;
	    case ';': /* argument terminator */
		edp->nargs++;
		break;
	    default:
		edp->nargs++;
		if (edp->nargs > VT100_EMUL_NARGS) {
#ifdef VT100_DEBUG
			printf("vt100: too many arguments\n");
#endif
			edp->nargs = VT100_EMUL_NARGS;
		}
		newstate = VT100_EMUL_STATE_STRING;
		switch (c) {
		    case '$':
			newstate = VT100_EMUL_STATE_DCS_DOLLAR;
			break;
		    case '{': /* DECDLD soft charset */
		    case '!': /* DECRQUPSS user preferred supplemental set */
			/* 'u' must follow - need another state */
		    case '|': /* DECUDK program F6..F20 */
#ifdef VT100_PRINTNOTIMPL
			printf("DCS%c ignored\n", c);
#endif
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("DCS%c (%d, %d) unknown\n", c, ARG(0), ARG(1));
#endif
			break;
		}
	}

	return (newstate);
}

static u_int
wsemul_vt100_output_dcs_dollar(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	switch (c) {
	    case 'p': /* DECRSTS terminal state restore */
	    case 'q': /* DECRQSS control function request */
#ifdef VT100_PRINTNOTIMPL
		printf("DCS$%c ignored\n", c);
#endif
		break;
	    case 't': /* DECRSPS restore presentation state */
		switch (ARG(0)) {
		    case 0: /* error */
			break;
		    case 1: /* cursor information restore */
#ifdef VT100_PRINTNOTIMPL
			printf("DCS1$t ignored\n");
#endif
			break;
		    case 2: /* tab stop restore */
			edp->dcspos = 0;
			edp->dcstype = DCSTYPE_TABRESTORE;
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("DCS%d$t unknown\n", ARG(0));
#endif
			break;
		}
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("DCS$%c (%d, %d) unknown\n", c, ARG(0), ARG(1));
#endif
		break;
	}
	return (VT100_EMUL_STATE_STRING);
}

static u_int
wsemul_vt100_output_csi_gt(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	switch (c) {
	    case 'c': /* DA secondary */
		wsdisplay_emulinput(edp->cbcookie, WSEMUL_VT_ID2,
				    sizeof(WSEMUL_VT_ID2));
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("CSI>%c unknown\n", c);
#endif
		break;
	}
	return (VT100_EMUL_STATE_NORMAL);
}

static u_int
wsemul_vt100_output_csi_dollar(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	u_int newstate = VT100_EMUL_STATE_NORMAL;

	switch (c) {
	    case '}': /* DECSASD */
		/* select active status display */
		switch (ARG(0)) {
		    case 0: /* main display */
		    case 1: /* status line */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI%d$} ignored\n", ARG(0));
#endif
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d$} unknown\n", ARG(0));
#endif
			break;
		}
		break;
	    case '~': /* DECSSDD */
		/* select status line type */
		switch (ARG(0)) {
		    case 0: /* none */
		    case 1: /* indicator */
		    case 2: /* host-writable */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI%d$~ ignored\n", ARG(0));
#endif
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d$~ unknown\n", ARG(0));
#endif
			break;
		}
		break;
	    case 'p': /* DECRQM request mode ANSI */
		vt100_reportmode(edp, ARG(0), 0);
		break;
	    case 'u': /* DECRQTSR request terminal status report */
		switch (ARG(0)) {
		    case 0: /* ignored */
			break;
		    case 1: /* terminal state report */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI1$u ignored\n");
#endif
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d$u unknown\n", ARG(0));
#endif
			break;
		}
		break;
	    case 'w': /* DECRQPSR request presentation status report
			 (VT300 only) */
		switch (ARG(0)) {
		    case 0: /* error */
			break;
		    case 1: /* cursor information report */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI1$w ignored\n");
#endif
			break;
		    case 2: /* tab stop report */
			{
			int i, n;
			char buf[20];
			KASSERT(edp->tabs != 0);
			wsdisplay_emulinput(edp->cbcookie, "\033P2$u", 5);
			for (i = 0; i < edp->ncols; i++)
				if (edp->tabs[i]) {
					n = sprintf(buf, "%s%d",
						    (i ? "/" : ""), i + 1);
					wsdisplay_emulinput(edp->cbcookie,
							    buf, n);
				}
			}
			wsdisplay_emulinput(edp->cbcookie, "\033\\", 2);
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d$w unknown\n", ARG(0));
#endif
			break;
		}
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("CSI$%c unknown\n", c);
#endif
		break;
	}
	return (newstate);
}

static u_int
wsemul_vt100_output_hash(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	switch (c) {
	    case '5': /*  DECSWL single width, single height */
		break;
	    case '6': /*  DECDWL double width, single height */
	    case '3': /*  DECDHL double width, double height, top half */
	    case '4': /*  DECDHL double width, double height, bottom half */
#ifdef VT100_PRINTNOTIMPL
		printf("ESC#%c ignored\n", c);
#endif
		break;
	    case '8': { /* DECALN */
		int i, j;
		for (i = 0; i < edp->nrows; i++)
			for (j = 0; j < edp->ncols; j++)
				(*edp->emulops->putchar)(edp->emulcookie, i, j,
							 'E', edp->curattr);
		}
		edp->ccol = 0;
		edp->crow = 0;
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("ESC#%c unknown\n", c);
#endif
		break;
	}
	return (VT100_EMUL_STATE_NORMAL);
}

static u_int
wsemul_vt100_output_csi(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	u_int newstate = VT100_EMUL_STATE_CSI;

	switch (c) {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		/* argument digit */
		if (edp->nargs > VT100_EMUL_NARGS - 1)
			break;
		edp->args[edp->nargs] = (edp->args[edp->nargs] * 10) +
		    (c - '0');
		break;
	    case ';': /* argument terminator */
		edp->nargs++;
		break;
	    case '?': /* DEC specific */
		edp->nargs = 0;
		bzero(edp->args, sizeof (edp->args));
		newstate = VT100_EMUL_STATE_CSI_QM;
		break;
	    case '>': /* DA query */
		newstate = VT100_EMUL_STATE_CSI_GT;
		break;
	    case '"':
		newstate = VT100_EMUL_STATE_CSI_DQ;
		break;
	    case '$':
		newstate = VT100_EMUL_STATE_CSI_DOLLAR;
		break;
	    case '&':
		newstate = VT100_EMUL_STATE_CSI_AMP;
		break;
	    case '!':
		newstate = VT100_EMUL_STATE_CSI_EM;
		break;
	    default: /* end of escape sequence */
		edp->nargs++;
		if (edp->nargs > VT100_EMUL_NARGS) {
#ifdef VT100_DEBUG
			printf("vt100: too many arguments\n");
#endif
			edp->nargs = VT100_EMUL_NARGS;
		}
		wsemul_vt100_handle_csi(edp, c);
		newstate = VT100_EMUL_STATE_NORMAL;
		break;
	}
	return (newstate);
}

static u_int
wsemul_vt100_output_csi_dq(edp, c)
	struct wsemul_vt100_emuldata *edp;
	u_char c;
{
	switch (c) {
	    case 'p': /* DECSCL */
		switch (ARG(0)) {
		    case 61: /* VT100 mode (no further arguments!) */
			break;
		    case 62:
		    case 63: /* VT300 mode */
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d\"p unknown\n", ARG(0));
#endif
			break;
		}
		switch (ARG(1)) {
		    case 0:
		    case 2: /* 8-bit controls */
#ifdef VT100_PRINTNOTIMPL
			printf("CSI%d;%d\"p ignored\n", ARG(0), ARG(1));
#endif
			break;
		    case 1: /* 7-bit controls */
			break;
		    default:
#ifdef VT100_PRINTUNKNOWN
			printf("CSI%d;%d\"p unknown\n", ARG(0), ARG(1));
#endif
			break;
		}
		break;
	    case 'q': /* DECSCA select character attribute VT300 only */
		switch (ARG(0)) {
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
			printf("CSI%d\"q unknown\n", ARG(0));
#endif
			break;
		}
		break;
	    default:
#ifdef VT100_PRINTUNKNOWN
		printf("CSI\"%c unknown (%d, %d)\n", c, ARG(0), ARG(1));
#endif
		break;
	}
	return (VT100_EMUL_STATE_NORMAL);
}

void
wsemul_vt100_output(cookie, data, count, kernel)
	void *cookie;
	const u_char *data;
	u_int count;
	int kernel;
{
	struct wsemul_vt100_emuldata *edp = cookie;

#ifdef DIAGNOSTIC
	if (kernel && !edp->console)
		panic("wsemul_vt100_output: kernel output, not console");
#endif

	/* XXX */
	(*edp->emulops->cursor)(edp->emulcookie, 0, edp->crow, edp->ccol);
	for (; count > 0; data++, count--) {
		if (edp->state == VT100_EMUL_STATE_NORMAL || kernel) {
			edp->state = wsemul_vt100_output_normal(edp, *data,
								kernel);
			continue;
		}
#ifdef DIAGNOSTIC
		if (edp->state > sizeof(vt100_output) / sizeof(vt100_output[0]))
			panic("wsemul_vt100: invalid state %d\n", edp->state);
#endif
		edp->state = vt100_output[edp->state - 1](edp, *data);
	}
	/* XXX */
	(*edp->emulops->cursor)(edp->emulcookie, edp->flags & VTFL_CURSORON,
				edp->crow, edp->ccol);
}
