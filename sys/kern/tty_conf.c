/*	$NetBSD: tty_conf.c,v 1.31.2.2 2001/08/24 00:11:41 nathanw Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *
 *	@(#)tty_conf.c	8.5 (Berkeley) 1/9/95
 */

#include "opt_compat_freebsd.h"
#include "opt_compat_43.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/ioctl.h>
#include <sys/ttycom.h>
#include <sys/conf.h>
#include <sys/malloc.h>

#include "tb.h"
#if NTB > 0
int	tbopen __P((dev_t dev, struct tty *tp));
int	tbclose __P((struct tty *tp, int flags));
int	tbread __P((struct tty *tp, struct uio *uio, int flags));
int	tbtioctl __P((struct tty *tp, u_long cmd, caddr_t data,
			int flag, struct proc *p));
int	tbinput __P((int c, struct tty *tp));
#endif

#include "sl.h"
#if NSL > 0
int	slopen __P((dev_t dev, struct tty *tp));
int	slclose __P((struct tty *tp, int flags));
int	sltioctl __P((struct tty *tp, u_long cmd, caddr_t data,
			int flag, struct proc *p));
int	slinput __P((int c, struct tty *tp));
int	slstart __P((struct tty *tp));
#endif

#include "ppp.h"
#if NPPP > 0
int	pppopen __P((dev_t dev, struct tty *tp));
int	pppclose __P((struct tty *tp, int flags));
int	ppptioctl __P((struct tty *tp, u_long cmd, caddr_t data,
			int flag, struct proc *p));
int	pppinput __P((int c, struct tty *tp));
int	pppstart __P((struct tty *tp));
int	pppread __P((struct tty *tp, struct uio *uio, int flag));
int	pppwrite __P((struct tty *tp, struct uio *uio, int flag));
#endif

#include "strip.h"
#if NSTRIP > 0
int	stripopen __P((dev_t dev, struct tty *tp));
int	stripclose __P((struct tty *tp, int flags));
int	striptioctl __P((struct tty *tp, u_long cmd, caddr_t data,
			int flag, struct proc *p));
int	stripinput __P((int c, struct tty *tp));
int	stripstart __P((struct tty *tp));
#endif


struct  linesw termios_disc =
	{ "termios", 0, ttylopen, ttylclose, ttread, ttwrite, nullioctl,
	  ttyinput, ttstart, ttymodem, ttpoll };	/* 0- termios */
struct  linesw defunct_disc =
	{ "defunct", 1, ttynodisc, ttyerrclose, ttyerrio, ttyerrio, nullioctl,
	  ttyerrinput, ttyerrstart, nullmodem, ttyerrpoll }; /* 1- defunct */
#if defined(COMPAT_43) || defined(COMPAT_FREEBSD)
struct  linesw ntty_disc =
	{ "ntty", 2, ttylopen, ttylclose, ttread, ttwrite, nullioctl,
	  ttyinput, ttstart, ttymodem, ttpoll };	/* 2- old NTTYDISC */
#endif
#if NTB > 0
struct  linesw table_disc =
	{ "tablet", 3, tbopen, tbclose, tbread, ttyerrio, tbtioctl,
	  tbinput, ttstart, nullmodem, ttyerrpoll };	/* 3- TABLDISC */
#endif
#if NSL > 0
struct  linesw slip_disc =
	{ "slip", 4, slopen, slclose, ttyerrio, ttyerrio, sltioctl,
	  slinput, slstart, nullmodem, ttyerrpoll };	/* 4- SLIPDISC */
#endif
#if NPPP > 0
struct  linesw ppp_disc =
	{ "ppp", 5, pppopen, pppclose, pppread, pppwrite, ppptioctl,
	  pppinput, pppstart, ttymodem, ttpoll };	/* 5- PPPDISC */
#endif
#if NSTRIP > 0
struct  linesw strip_disc =
	{ "strip", 6, stripopen, stripclose, ttyerrio, ttyerrio, striptioctl,
	  stripinput, stripstart, nullmodem, ttyerrpoll }; /* 6- STRIPDISC */
#endif

/*
 * Registered line disciplines.  Don't use this
 * it will go away.
 */
#define LSWITCHBRK	20
struct	linesw **linesw = NULL;
int	nlinesw = 0;
int	slinesw = 0;

/*
 * Do nothing specific version of line
 * discipline specific ioctl command.
 */
/*ARGSUSED*/
int
nullioctl(tp, cmd, data, flags, p)
	struct tty *tp;
	u_long cmd;
	char *data;
	int flags;
	struct proc *p;
{

#ifdef lint
	tp = tp; data = data; flags = flags; p = p;
#endif
	return (-1);
}

/*
 * Register a line discipline, optionally providing a
 * specific discipline number for compatibility, -1 allocates
 * a new one.  Returns a discipline number, or -1 on
 * failure.
 */
int
ttyldisc_add(disc, no) 
	struct linesw *disc;
	int no;
{

	/* You are not allowed to exceed TTLINEDNAMELEN */
	if (strlen(disc->l_name) > TTLINEDNAMELEN)
		return (-1);

	/* 
	 * You are not allowed to specify a line switch 
	 * compatibility number greater than 10.
	 */
	if (no > 10) 
		return (-1);

	if (linesw == NULL)
		panic("adding uninitialized linesw");

#ifdef DEBUG
	/*
	 * XXX: For the benefit of LKMs
	 */
	if (disc->l_poll == NULL)
		panic("line discipline must now provide l_poll() entry point");
#endif

	if (no == -1) {
		/* Hunt for any slot */

		for (no = slinesw; no-- > 0;)
			if (linesw[no] == NULL)
				break;
		/* if no == -1 we should realloc linesw, but for now... */
		if (no == -1)
			return (-1);
	}

	/* Need a specific slot */
	if (linesw[no] != NULL)
		return (-1);
	
	linesw[no] = disc;
	disc->l_no = no;

	/* Define size of table */
	if (no >= nlinesw)
		nlinesw = no + 1;
	
	return (no);
}

/*
 * Remove a line discipline by its name.  Returns the
 * discipline on success or NULL on failure.
 */
struct linesw *
ttyldisc_remove(name) 
	char *name;
{
	struct linesw *disc;
	int i;

	if (linesw == NULL)
		panic("removing uninitialized linesw");

	for (i = 0; i < nlinesw; i++) {
		if (linesw[i] && (strcmp(name, linesw[i]->l_name) == 0)) {
			disc = linesw[i];
			linesw[i] = NULL;
			
			if (nlinesw == i + 1) {
				/* Need to fix up array sizing */
				while (i-- > 0 && linesw[i] == NULL)
					continue;
				nlinesw = i + 1;
			}
			return (disc);
		}
	}
	return (NULL);
}

/*
 * Look up a line discipline by its name.
 */
struct linesw *
ttyldisc_lookup(name)
	char *name;
{
	int i;

	for (i = 0; i < nlinesw; i++)
		if (linesw[i] && (strcmp(name, linesw[i]->l_name) == 0))
			return (linesw[i]);
	return (NULL);
}

#define TTYLDISCINIT(s, v) \
	do { \
		if (ttyldisc_add(&(s), (v)) != (v)) \
			panic("ttyldisc_init: " __STRING(s)); \
	} while (0)

/*
 * Register the basic line disciplines.
 */	
void
ttyldisc_init()
{

	/* Only initialize once */
	if (linesw)
		return;

	slinesw = LSWITCHBRK;
	linesw = malloc(slinesw * sizeof(struct linesw *), 
	    M_TTYS, M_WAITOK);
	memset(linesw, 0, slinesw * sizeof(struct linesw *));

	TTYLDISCINIT(termios_disc, 0);
	/* Do we really need this one? */
	TTYLDISCINIT(defunct_disc, 1);

	/* 
	 * The following should really be moved to
	 * initialization code for each module.
	 */

#if defined(COMPAT_43) || defined(COMPAT_FREEBSD)
	TTYLDISCINIT(ntty_disc, 2);
#endif
#if NTB > 0
	TTYLDISCINIT(table_disc, 3);
#endif
#if NSL > 0
	TTYLDISCINIT(slip_disc, 4);
#endif
#if NPPP > 0
	TTYLDISCINIT(ppp_disc, 5);
#endif
#if NSTRIP > 0
	TTYLDISCINIT(strip_disc, 6);
#endif
}
