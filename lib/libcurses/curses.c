/*	$NetBSD: curses.c,v 1.21 2003/08/07 16:44:20 agc Exp $	*/

/*
 * Copyright (c) 1981, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)curses.c	8.3 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: curses.c,v 1.21 2003/08/07 16:44:20 agc Exp $");
#endif
#endif				/* not lint */

#include "curses.h"

/* Private. */
int	__echoit = 1;			 /* If stty indicates ECHO. */
int	__pfast;
int	__rawmode = 0;			 /* If stty indicates RAW mode. */
int	__noqch = 0;			 
					 /* If terminal doesn't have
					 * insert/delete line capabilities
					 * for quick change on refresh.
					 */

char	PC;
char	__tc_am, __tc_bs, __tc_cc, __tc_da, __tc_eo,
	__tc_hc, __tc_hl, __tc_in, __tc_mi, __tc_ms,
	__tc_nc, __tc_ns, __tc_os, __tc_ul, __tc_ut,
	__tc_xb, __tc_xn, __tc_xt, __tc_xs, __tc_xx;
char	__CA;
int	__tc_pa, __tc_Co, __tc_NC;
char	*__tc_ac, *__tc_AB, *__tc_ae, *__tc_AF, *__tc_AL,
	*__tc_al, *__tc_as, *__tc_bc, *__tc_bl, *__tc_bt,
	*__tc_cd, *__tc_ce, *__tc_cl, *__tc_cm, *__tc_cr,
	*__tc_cs, *__tc_dc, *__tc_DL, *__tc_dl, *__tc_dm,
	*__tc_DO, *__tc_do, *__tc_eA, *__tc_ed, *__tc_ei,
	*__tc_ho, *__tc_Ic, *__tc_ic, *__tc_im, *__tc_Ip,
	*__tc_ip, *__tc_k0, *__tc_k1, *__tc_k2, *__tc_k3,
	*__tc_k4, *__tc_k5, *__tc_k6, *__tc_k7, *__tc_k8,
	*__tc_k9, *__tc_kd, *__tc_ke, *__tc_kh, *__tc_kl,
	*__tc_kr, *__tc_ks, *__tc_ku, *__tc_LE, *__tc_ll,
	*__tc_ma, *__tc_mb, *__tc_md, *__tc_me, *__tc_mh,
	*__tc_mk, *__tc_mm, *__tc_mo, *__tc_mp, *__tc_mr,
	*__tc_nd, *__tc_nl, *__tc_oc, *__tc_op,
	*__tc_rc, *__tc_RI, *__tc_Sb, *__tc_sc, *__tc_se,
	*__tc_SF, *__tc_Sf, *__tc_sf, *__tc_so, *__tc_sp,
	*__tc_SR, *__tc_sr, *__tc_ta, *__tc_te, *__tc_ti,
	*__tc_uc, *__tc_ue, *__tc_UP, *__tc_up, *__tc_us,
	*__tc_vb, *__tc_ve, *__tc_vi, *__tc_vs;

/*
 * Public.
 *
 * XXX
 * UPPERCASE isn't used by libcurses, and is left for backward
 * compatibility only.
 */
WINDOW	*curscr;			/* Current screen. */
WINDOW	*stdscr;			/* Standard screen. */
WINDOW	*__virtscr;			/* Virtual screen (for doupdate()). */
SCREEN  *_cursesi_screen;               /* the current screen we are using */
int	 COLS;				/* Columns on the screen. */
int	 LINES;				/* Lines on the screen. */
int	 COLORS;			/* Maximum colors on the screen */
int	 COLOR_PAIRS = 0;		/* Maximum color pairs on the screen */
int	 My_term = 0;			/* Use Def_term regardless. */
const char	*Def_term = "unknown";	/* Default terminal type. */
char	 __GT;				/* Gtty indicates tabs. */
char	 __NONL;			/* Term can't hack LF doing a CR. */
char	 __UPPERCASE;			/* Terminal is uppercase only. */
