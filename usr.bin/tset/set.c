/*	$NetBSD: set.c,v 1.13 2010/02/03 15:34:46 roy Exp $	*/

/*-
 * Copyright (c) 1991, 1993
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
static char sccsid[] = "@(#)set.c	8.2 (Berkeley) 2/28/94";
#endif
__RCSID("$NetBSD: set.c,v 1.13 2010/02/03 15:34:46 roy Exp $");
#endif /* not lint */

#include <stdio.h>
#include <term.h>
#include <termios.h>
#include <unistd.h>
#include "extern.h"

#define	CHK(val, dft)	(val <= 0 ? dft : val)

int	set_tabs __P((void));

/*
 * Reset the terminal mode bits to a sensible state.  Very useful after
 * a child program dies in raw mode.
 */
void
reset_mode()
{
	tcgetattr(STDERR_FILENO, &mode);

#if defined(VDISCARD) && defined(CDISCARD)
	mode.c_cc[VDISCARD] = CHK(mode.c_cc[VDISCARD], CDISCARD);
#endif
	mode.c_cc[VEOF] = CHK(mode.c_cc[VEOF], CEOF);
	mode.c_cc[VERASE] = CHK(mode.c_cc[VERASE], CERASE);
#if defined(VFLUSH) && defined(CFLUSH)
	mode.c_cc[VFLUSH] = CHK(mode.c_cc[VFLUSH], CFLUSH);
#endif
	mode.c_cc[VINTR] = CHK(mode.c_cc[VINTR], CINTR);
	mode.c_cc[VKILL] = CHK(mode.c_cc[VKILL], CKILL);
#if defined(VLNEXT) && defined(CLNEXT)
	mode.c_cc[VLNEXT] = CHK(mode.c_cc[VLNEXT], CLNEXT);
#endif
	mode.c_cc[VQUIT] = CHK(mode.c_cc[VQUIT], CQUIT);
#if defined(VREPRINT) && defined(CRPRNT)
	mode.c_cc[VREPRINT] = CHK(mode.c_cc[VREPRINT], CRPRNT);
#endif
	mode.c_cc[VSTART] = CHK(mode.c_cc[VSTART], CSTART);
	mode.c_cc[VSTOP] = CHK(mode.c_cc[VSTOP], CSTOP);
	mode.c_cc[VSUSP] = CHK(mode.c_cc[VSUSP], CSUSP);
#if defined(VWERASE) && defined(CWERASE)
	mode.c_cc[VWERASE] = CHK(mode.c_cc[VWERASE], CWERASE);
#endif

	mode.c_iflag &= ~(IGNBRK | PARMRK | INPCK | ISTRIP | INLCR | IGNCR
#ifdef IUCLC
			  | IUCLC
#endif
#ifdef IXANY
			  | IXANY
#endif
			  | IXOFF);

	mode.c_iflag |= (BRKINT | IGNPAR | ICRNL | IXON
#ifdef IMAXBEL
			 | IMAXBEL
#endif
			 );

	mode.c_oflag &= ~(0
#ifdef OLCUC
			  | OLCUC
#endif
#ifdef OCRNL
			  | OCRNL
#endif
#ifdef ONOCR
			  | ONOCR
#endif
#ifdef ONLRET
			  | ONLRET
#endif
#ifdef OFILL
			  | OFILL
#endif
#ifdef OFDEL
			  | OFDEL
#endif
#ifdef NLDLY
			  | NLDLY | CRDLY | TABDLY | BSDLY | VTDLY | FFDLY
#endif
			  );

	mode.c_oflag |= (OPOST
#ifdef ONLCR
			 | ONLCR
#endif
			 );

	mode.c_cflag &= ~(CSIZE | CSTOPB | PARENB | PARODD);
	mode.c_cflag |= (CS8 | CREAD);
	mode.c_lflag &= ~(ECHONL | NOFLSH | TOSTOP
#ifdef ECHOPTR
			  | ECHOPRT
#endif
#ifdef XCASE
			  | XCASE
#endif
			  );

	mode.c_lflag |= (ISIG | ICANON | ECHO | ECHOE | ECHOK
#ifdef ECHOCTL
			 | ECHOCTL
#endif
#ifdef ECHOKE
			 | ECHOKE
#endif
 			 );

	tcsetattr(STDERR_FILENO, TCSADRAIN, &mode);
}

/*
 * Determine the erase, interrupt, and kill characters from the termcap
 * entry and command line and update their values in 'mode'.
 */
void
set_control_chars(int erasechar, int intrchar, int killchar)
{
	
	if (mode.c_cc[VERASE] == 0 || erasechar != 0) {
		if (erasechar == 0) {
			if (over_strike &&
			    key_backspace != NULL &&
			    key_backspace[1] == '\0')
				mode.c_cc[VERASE] = key_backspace[1];
			else
				mode.c_cc[VERASE] = CERASE;
		} else
			mode.c_cc[VERASE] = erasechar;
	}

	if (mode.c_cc[VINTR] == 0 || intrchar != 0)
		 mode.c_cc[VINTR] = intrchar ? intrchar : CINTR;

	if (mode.c_cc[VKILL] == 0 || killchar != 0)
		mode.c_cc[VKILL] = killchar ? killchar : CKILL;
}

/*
 * Set up various conversions in 'mode', including parity, tabs, returns,
 * echo, and case, according to the termcap entry.  If the program we're
 * running was named with a leading upper-case character, map external
 * uppercase to internal lowercase.
 */
void
set_conversions(int usingupper)
{

#ifdef ONLCR
	mode.c_oflag |= ONLCR;
#endif
	mode.c_iflag |= ICRNL;
	mode.c_lflag |= ECHO;
	mode.c_oflag |= OXTABS;
	if (newline != NULL && newline[0] == '\n' && !newline[1]) {			/* Newline, not linefeed. */
#ifdef ONLCR
		mode.c_oflag &= ~ONLCR;
#endif
		mode.c_iflag &= ~ICRNL;
	}
	if (tab)	/* Print tabs. */
		mode.c_oflag &= ~OXTABS;
	mode.c_lflag |= (ECHOE | ECHOK);
}

/* Output startup string. */
void
set_init()
{
	const char *bp;
	int settle;

#ifdef TAB3
	if (oldmode.c_oflag & (TAB3 | ONLCR | OCRNL | ONLRET)) {
		oldmode.c_oflag &= (TAB3 | ONLCR | OCRNL | ONLRET);
		tcsetattr(STDERR_FILENO, TCSADRAIN, &oldmode);
	}
#endif
	settle = set_tabs();

	if (isreset) {
		if (reset_1string) {
			tputs(reset_1string, 0, outc);
			settle = 1;
		}
		if (reset_2string) {
			tputs(reset_2string, 0, outc);
			settle = 1;
		}
		if ((bp = reset_file) || (bp = init_file)) {
			tputs(bp, 0, outc);
			settle = 1;
		}
	}

	if (settle) {
		(void)putc('\r', stderr);
		(void)fflush(stderr);
		(void)sleep(1);			/* Settle the terminal. */
	}
}

/*
 * Set the hardware tabs on the terminal, using the ct (clear all tabs),
 * st (set one tab) and ch (horizontal cursor addressing) capabilities.
 * This is done before if and is, so they can patch in case we blow this.
 * Return nonzero if we set any tab stops, zero if not.
 */
int
set_tabs()
{
	int c;
	char *out;

	if (set_tab) {
		if (clear_all_tabs) {
			(void)putc('\r', stderr);   /* Force to left margin. */
			tputs(clear_all_tabs, 0, outc);
		}
		
		for (c = 8; c < ncolumns; c += 8) {
			if (column_address)
				out = vtparm(column_address, c);
			else
				out = NULL;
			if (out)
				tputs(out, 1, outc);
			else
				(void)fprintf(stderr, "%s", "        ");
			/* Set the tab. */
			tputs(set_tab, 0, outc);
		}
		putc('\r', stderr);
		return (1);
	}
	return (0);
}
