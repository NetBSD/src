/*	$NetBSD: setterm.c,v 1.15 2000/04/12 21:45:30 jdc Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
static char sccsid[] = "@(#)setterm.c	8.8 (Berkeley) 10/25/94";
#else
__RCSID("$NetBSD: setterm.c,v 1.15 2000/04/12 21:45:30 jdc Exp $");
#endif
#endif /* not lint */

#include <sys/ioctl.h>		/* TIOCGWINSZ on old systems. */

#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "curses.h"
#include "curses_private.h"

static void zap __P((void));

static char	*sflags[] = {
		/*	 am   be   bs   cc   da   eo   hc   hl  */
			&AM, &BE, &BS, &CC, &DA, &EO, &HC, &HL,
		/*	 in   mi   ms   nc   ns   os   ul   xb  */
			&IN, &MI, &MS, &NC, &NS, &OS, &UL, &XB,
		/*	 xn   xt   xs   xx  */
			&XN, &XT, &XS, &XX
		};

static int	*svals[] = {
		/*	 pa   Co */
			&PA, &cO, &nc
		};
static char	*_PC,
		**sstrs[] = {
		/*	 AC   AE   AL   AS   bc   bl   bt   cd   ce  */
			&AC, &AE, &AL, &AS, &BC, &BL, &BT, &CD, &CE,
		/*	 cl   cm   cr   cs   dc   DL   dm   do   eA  */
			&CL, &CM, &CR, &CS, &DC, &DL, &DM, &DO, &Ea,
		/*	 ed   ei   k0   k1   k2   k3   k4   k5   k6  */
			&ED, &EI, &K0, &K1, &K2, &K3, &K4, &K5, &K6,
		/*	 k7   k8   k9   ho   ic   im   ip   kd   ke  */
			&K7, &K8, &K9, &HO, &IC, &IM, &IP, &KD, &KE,
		/*	 kh   kl   kr   ks   ku   ll   ma   mb   md  */
			&KH, &KL, &KR, &KS, &KU, &LL, &MA, &MB, &MD,
		/*	 me   mh   mk   mp   mr   nd   nl   oc   op  */
			&ME, &MH, &MK, &MP, &MR, &ND, &NL, &OC, &OP,
		/*	  pc   rc   sc   se   SF   so   sp   SR   ta */
			&_PC, &RC, &SC, &SE, &SF, &SO, &SP, &SR, &TA,
		/*	 te   ti   uc   ue   up   us   vb   vs   ve  */
			&TE, &TI, &UC, &UE, &UP, &US, &VB, &VS, &VE,
		/*	 AB   AF   al   dl   Ic   Ip   Sb   Sf   sf  */
			&ab, &af, &al, &dl, &iC, &iP, &sB, &sF, &sf,
		/*	 sr   AL        DL        UP        DO  */
			&sr, &AL_PARM, &DL_PARM, &UP_PARM, &DOWN_PARM,
		/*	 LE          RI */
			&LEFT_PARM, &RIGHT_PARM,
		};

static char	*aoftspace;		/* Address of _tspace for relocation */
static char	tspace[4096];		/* Space for capability strings */

char *ttytype;

int
setterm(type)
	char *type;
{
	static char genbuf[2048];
	static char __ttytype[128];
	int unknown;
	struct winsize win;
	char *p;

#ifdef DEBUG
	__CTRACE("setterm: (\"%s\")\nLINES = %d, COLS = %d\n",
	    type, LINES, COLS);
#endif
	if (type[0] == '\0')
		type = "xx";
	unknown = 0;
	if (tgetent(genbuf, type) != 1) {
		unknown++;
		(void)strncpy(genbuf, "xx|dumb:", sizeof(genbuf) - 1);
	}
#ifdef DEBUG
	__CTRACE("setterm: tty = %s\n", type);
#endif

	/* Try TIOCGWINSZ, and, if it fails, the termcap entry. */
	if (ioctl(STDERR_FILENO, TIOCGWINSZ, &win) != -1 &&
	    win.ws_row != 0 && win.ws_col != 0) {
		LINES = win.ws_row;
		COLS = win.ws_col;
	}  else {
		LINES = tgetnum("li");
		COLS = tgetnum("co");
	}

	/* POSIX 1003.2 requires that the environment override. */
	if ((p = getenv("LINES")) != NULL)
		LINES = (int) strtol(p, NULL, 10);
	if ((p = getenv("COLUMNS")) != NULL)
		COLS = (int) strtol(p, NULL, 10);

	/*
	 * Want cols > 4, otherwise things will fail.
	 */
	if (COLS <= 4)
		return (ERR);

#ifdef DEBUG
	__CTRACE("setterm: LINES = %d, COLS = %d\n", LINES, COLS);
#endif
	aoftspace = tspace;
	zap();			/* Get terminal description. */

	/* If we can't tab, we can't backtab, either. */
	if (!GT)
		BT = NULL;

	/*
	 * Test for cursor motion capbility.
	 *
	 * XXX
	 * This is truly stupid -- historically, tgoto returns "OOPS" if it
	 * can't do cursor motions.  Some systems have been fixed to return
	 * a NULL pointer.
	 */
	if ((p = tgoto(CM, 0, 0)) == NULL || *p == 'O') {
		CA = 0;
		CM = 0;
	} else
		CA = 1;

	PC = _PC ? _PC[0] : 0;
	aoftspace = tspace;
	ttytype = __longname(genbuf, __ttytype);

	/* If no scrolling commands, no quick change. */
	__noqch =
	    (CS == NULL || HO == NULL ||
	    (SF == NULL && sf == NULL) || (SR == NULL && sr == NULL)) &&
	    ((AL == NULL && al == NULL) || (DL == NULL && dl == NULL));

	return (unknown ? ERR : OK);
}

/*
 * zap --
 *	Gets all the terminal flags from the termcap database.
 */
static void
zap()
{
	char *namp, ***sp;
	int  **vp;
	char **fp;
	char tmp[3];
#ifdef DEBUG
	char	*cp;
#endif
	tmp[2] = '\0';

	namp = "ambebsccdaeohchlinmimsncnsosulxbxnxtxsxx";
	fp = sflags;
	do {
		*tmp = *namp;
		*(tmp + 1) = *(namp + 1);
		*(*fp++) = tgetflag(tmp);
#ifdef DEBUG
		__CTRACE("%2.2s = %s\n", namp, *fp[-1] ? "TRUE" : "FALSE");
#endif
		namp += 2;

	} while (*namp);
	namp = "paCoNC";
	vp = svals;
	do {
		*tmp = *namp;
		*(tmp + 1) = *(namp + 1);
		*(*vp++) = tgetnum(tmp);
#ifdef DEBUG
		__CTRACE("%2.2s = %d\n", namp, *vp[-1]);
#endif
		namp += 2;

	} while (*namp);
	namp = "acaeALasbcblbtcdceclcmcrcsdcDLdmdoeAedeik0k1k2k3k4k5k6k7k8k9hoicimipkdkekhklkrkskullmambmdmemhmkmpmrndnlocoppcprscseSFsospSRtatetiucueupusvbvsveABAFaldlIcIpSbSfsfsrALDLUPDOLERI";
	sp = sstrs;
	do {
		*tmp = *namp;
		*(tmp + 1) = *(namp + 1);
		*(*sp++) = tgetstr(tmp, &aoftspace);
#ifdef DEBUG
		__CTRACE("%2.2s = %s", namp, *sp[-1] == NULL ? "NULL\n" : "\"");
		if (*sp[-1] != NULL) {
			for (cp = *sp[-1]; *cp; cp++)
				__CTRACE("%s", unctrl(*cp));
			__CTRACE("\"\n");
		}
#endif
		namp += 2;
	} while (*namp);
	if (XS)
		SO = SE = NULL;
	else {
		if (tgetnum("sg") > 0)
			SO = NULL;
		if (tgetnum("ug") > 0)
			US = NULL;
		if (!SO && US) {
			SO = US;
			SE = UE;
		}
	}
}

/*
 * getcap --
 *	Return a capability from termcap.
 */
char	*
getcap(name)
	char	*name;
{
	return (tgetstr(name, &aoftspace));
}
