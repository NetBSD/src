/*	$NetBSD: setterm.c,v 1.26 2000/06/12 21:04:08 jdc Exp $	*/

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
__RCSID("$NetBSD: setterm.c,v 1.26 2000/06/12 21:04:08 jdc Exp $");
#endif
#endif /* not lint */

#include <sys/ioctl.h>		/* TIOCGWINSZ on old systems. */

#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "curses.h"
#include "curses_private.h"

static int zap(struct tinfo *tinfo);

struct tinfo *_cursesi_genbuf;

static char	*sflags[] = {
		/*	 am   ut   bs   cc   da   eo   hc   hl  */
			&AM, &UT, &BS, &CC, &DA, &EO, &HC, &HL,
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
		/*	 me   mh   mk   mm   mo   mp   mr   nd   nl  */
			&ME, &MH, &MK, &MM, &MO, &MP, &MR, &ND, &NL,
		/*	 oc   op    pc   rc   sc   se   SF   so   sp */
			&OC, &OP, &_PC, &RC, &SC, &SE, &SF, &SO, &SP,
		/*	 SR   ta   te   ti   uc   ue   up   us   vb  */
			&SR, &TA, &TE, &TI, &UC, &UE, &UP, &US, &VB,
		/*	 vi   vs   ve   AB   AF   al   dl   Ic   Ip  */
			&VI, &VS, &VE, &ab, &af, &al, &dl, &iC, &iP,
		/*	 Sb   Sf   sf   sr   AL        DL        UP  */
			&sB, &sF, &sf, &sr, &AL_PARM, &DL_PARM, &UP_PARM, 
		/*	   DO        LE          RI                  */
			&DOWN_PARM, &LEFT_PARM, &RIGHT_PARM,
		};

static char	*aoftspace;		/* Address of _tspace for relocation */
static char	*tspace;		/* Space for capability strings */
static size_t   tspace_size;            /* size of tspace */

char	*ttytype;
attr_t	 __mask_OP, __mask_ME, __mask_UE, __mask_SE;

int
setterm(char *type)
{
	static char __ttytype[128], cm_buff[1024], tc[1024], *tcptr;
	int unknown;
	size_t limit;
	struct winsize win;
	char *p;

#ifdef DEBUG
	__CTRACE("setterm: (\"%s\")\nLINES = %d, COLS = %d\n",
	    type, LINES, COLS);
#endif
	if (type[0] == '\0')
		type = "xx";
	unknown = 0;
	if (t_getent(&_cursesi_genbuf, type) != 1) {
		unknown++;
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
		if (unknown) {
			LINES = -1;
			COLS = -1;
		} else {
			LINES = t_getnum(_cursesi_genbuf, "li");
			COLS = t_getnum(_cursesi_genbuf, "co");
		}
		
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
	if (!unknown) {
		if (zap(_cursesi_genbuf) == ERR) /* Get terminal description.*/
			return ERR;
	}
	
	/* If we can't tab, we can't backtab, either. */
	if (!GT)
		BT = NULL;

	/*
	 * Test for cursor motion capability.
	 *
	 */
	if (t_goto(NULL, CM, 0, 0, cm_buff, 1023) < 0) {
		CA = 0;
		CM = 0;
	} else
		CA = 1;

	PC = _PC ? _PC[0] : 0;
	aoftspace = tspace;
	if (unknown) {
		strcpy(ttytype, "dumb");
	} else {
		tcptr = tc;
		limit = 1023;
		if (t_getterm(_cursesi_genbuf, &tcptr, &limit) < 0)
			return ERR;
		ttytype = __longname(tc, __ttytype);
	}
	
	/* If no scrolling commands, no quick change. */
	__noqch =
  	    (CS == NULL || HO == NULL ||
	    (SF == NULL && sf == NULL) || (SR == NULL && sr == NULL)) &&
	    ((AL == NULL && al == NULL) || (DL == NULL && dl == NULL));

	/* Precalculate conflict info for color/attribute end commands. */
	__mask_OP = __ATTRIBUTES & ~__COLOR;
	if (OP != NULL) {
		if (SE != NULL && !strcmp(OP, SE))
			__mask_OP &= ~__STANDOUT;
		if (UE != NULL && !strcmp(OP, UE))
			__mask_OP &= ~__UNDERSCORE;
		if (ME != NULL && !strcmp(OP, ME))
			__mask_OP &= ~__TERMATTR;
	}
	__mask_ME = __ATTRIBUTES & ~__TERMATTR;
	if (ME != NULL) {
		if (SE != NULL && !strcmp(ME, SE))
			__mask_ME &= ~__STANDOUT;
		if (UE != NULL && !strcmp(ME, UE))
			__mask_ME &= ~__UNDERSCORE;
		if (OP != NULL && !strcmp(ME, OP))
			__mask_ME &= ~__COLOR;
	}
	__mask_UE = __ATTRIBUTES & ~__UNDERSCORE;
	if (UE != NULL) {
		if (SE != NULL && !strcmp(UE, SE))
			__mask_UE &= ~__STANDOUT;
		if (ME != NULL && !strcmp(UE, ME))
			__mask_UE &= ~__TERMATTR;
		if (OP != NULL && !strcmp(UE, OP))
			__mask_UE &= ~__COLOR;
	}
	__mask_SE = __ATTRIBUTES & ~__STANDOUT;
	if (SE != NULL) {
		if (UE != NULL && !strcmp(SE, UE))
			__mask_SE &= ~__UNDERSCORE;
		if (ME != NULL && !strcmp(SE, ME))
			__mask_SE &= ~__TERMATTR;
		if (OP != NULL && !strcmp(SE, OP))
			__mask_SE &= ~__COLOR;
	}

	return (unknown ? ERR : OK);
}

/*
 * zap --
 *	Gets all the terminal flags from the termcap database.
 */
static int
zap(struct tinfo *tinfo)
{
	const char *nampstr, *namp;
        char ***sp;
	int  **vp;
	char **fp;
	char tmp[3];
	size_t i;
#ifdef DEBUG
	char	*cp;
#endif
	tmp[2] = '\0';

	namp = "amutbsccdaeohchlinmimsncnsosulxbxnxtxsxx";
	fp = sflags;
	do {
		*tmp = *namp;
		*(tmp + 1) = *(namp + 1);
		*(*fp++) = t_getflag(tinfo, tmp);
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
		*(*vp++) = t_getnum(tinfo, tmp);
#ifdef DEBUG
		__CTRACE("%2.2s = %d\n", namp, *vp[-1]);
#endif
		namp += 2;

	} while (*namp);

	  /* calculate the size of tspace.... */
	nampstr = "acaeALasbcblbtcdceclcmcrcsdcDLdmdoeAedeik0k1k2k3k4k5k6k7k8k9hoicimipkdkekhklkrkskullmambmdmemhmkmmmompmrndnlocoppcprscseSFsospSRtatetiucueupusvbvivsveABAFaldlIcIpSbSfsfsrALDLUPDOLERI";
	namp = nampstr;
	tspace_size = 0;
	do {
		*tmp = *namp;
		*(tmp + 1) = *(namp + 1);
		t_getstr(tinfo, tmp, NULL, &i);
		tspace_size += i + 1;
		namp += 2;
	} while (*namp);

	if ((tspace = (char *) malloc(tspace_size)) == NULL)
		return ERR;
#ifdef DEBUG
	__CTRACE("Allocated %d (0x%x) size buffer for tspace\n", tspace_size,
		 tspace_size);
#endif
	aoftspace = tspace;
	
	namp = nampstr;
	sp = sstrs;
	do {
		*tmp = *namp;
		*(tmp + 1) = *(namp + 1);
		*(*sp++) = t_getstr(tinfo, tmp, &aoftspace, NULL);
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
		if (t_getnum(tinfo, "sg") > 0)
			SO = NULL;
		if (t_getnum(tinfo, "ug") > 0)
			US = NULL;
		if (!SO && US) {
			SO = US;
			SE = UE;
		}
	}

	return OK;
}

/*
 * getcap --
 *	Return a capability from termcap.
 */
char	*
getcap(char *name)
{
	size_t ent_size, offset;
	char *new_tspace;

	  /* verify cap exists and grab size of it at the same time */
	t_getstr(_cursesi_genbuf, name, NULL, &ent_size);

	  /* grow tspace to hold the new cap */
	if ((new_tspace = realloc(tspace, ent_size + tspace_size)) == NULL)
		return NULL;

	  /* point aoftspace to the same place in the newly allocated buffer */
	offset = aoftspace - tspace;
	tspace = new_tspace + offset;
	
	return (t_getstr(_cursesi_genbuf, name, &aoftspace, NULL));
}
