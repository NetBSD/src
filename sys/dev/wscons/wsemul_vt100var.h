/* $NetBSD: wsemul_vt100var.h,v 1.1 1998/06/20 19:17:47 drochner Exp $ */

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

#define	VT100_EMUL_NARGS	10	/* max # of args to a command */

struct wsemul_vt100_emuldata {
	const struct wsdisplay_emulops *emulops;
	void *emulcookie;
	int scrcapabilities;
	u_int nrows, ncols, crow, ccol;
	long defattr;			/* default attribute */

	long kernattr;			/* attribute for kernel output */
	void *cbcookie;
#ifdef DIAGNOSTIC
	int console;
#endif

	u_int state;			/* processing state */
	int flags;
#define VTFL_LASTCHAR 1		/* printed last char on line (below cursor) */
#define VTFL_INSERTMODE 2
#define VTFL_APPLKEYPAD 4
#define VTFL_APPLCURSOR 8
#define VTFL_DECOM 16		/* origin mode */
#define VTFL_DECAWM 32		/* auto wrap */
#define VTFL_CURSORON 64
	long curattr;			/* currently used attribute */
	int attrflags, fgcol, bgcol;	/* properties of curattr */
	int scrreg_startrow;
	int scrreg_nrows;
	char *tabs;

	int nargs;
	u_int args[VT100_EMUL_NARGS];	/* command args */

	char designating;	/* substate in VT100_EMUL_STATE_DESIGNATE */

	int dcstype;		/* substate in VT100_EMUL_STATE_STRING */
	char *dcsarg;
	int dcspos;
#define DCS_MAXLEN 256 /* ??? */
#define DCSTYPE_TABRESTORE 1 /* DCS2$t */

	int savedcursor_row, savedcursor_col;
};

/* some useful utility macros */
#define	ARG(n)			(edp->args[(n)])
#define	DEF1_ARG(n)	(ARG(n) ? ARG(n) : 1)
#define	COLS_LEFT		(edp->ncols - edp->ccol - 1)
#define ROWS_ABOVE		(edp->crow - edp->scrreg_startrow)
#define	ROWS_BELOW		(edp->scrreg_startrow + edp->scrreg_nrows \
					- edp->crow - 1)

/*
 * response to primary DA request
 * operating level: 61 = VT100, 62 = VT200, 63 = VT300
 * extensions: 1 = 132 cols, 2 = printer port, 6 = selective erase,
 *	7 = soft charset, 8 = UDKs, 9 = NRC sets
 * VT100 = "033[?1;2c"
 */
#define WSEMUL_VT_ID1 "\033[?62;6c"
/*
 * response to secondary DA request
 * ident code: 24 = VT320
 * firmware version
 * hardware options: 0 = no options
 */
#define WSEMUL_VT_ID2 "\033[>24;20;0c"

void wsemul_vt100_reset __P((struct wsemul_vt100_emuldata *));
void wsemul_vt100_scrollup __P((struct wsemul_vt100_emuldata *, int));
void wsemul_vt100_scrolldown __P((struct wsemul_vt100_emuldata *, int));
void wsemul_vt100_ed __P((struct wsemul_vt100_emuldata *, int));
void wsemul_vt100_el __P((struct wsemul_vt100_emuldata *, int));
void wsemul_vt100_handle_csi __P((struct wsemul_vt100_emuldata *, u_char));
void wsemul_vt100_handle_csi_qm __P((struct wsemul_vt100_emuldata *,
				     u_char));
void wsemul_vt100_handle_dcs __P((struct wsemul_vt100_emuldata *));
void vt100_setmode __P((struct wsemul_vt100_emuldata *, int, int));
void vt100_resetmode __P((struct wsemul_vt100_emuldata *, int, int));
void vt100_reportmode __P((struct wsemul_vt100_emuldata *, int, int));

int	wsemul_vt100_translate __P((void *cookie, keysym_t, char **));
