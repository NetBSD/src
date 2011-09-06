/*	$NetBSD: tipout.c,v 1.15 2011/09/06 18:33:01 joerg Exp $	*/

/*
 * Copyright (c) 1983, 1993
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
#include <poll.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)tipout.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: tipout.c,v 1.15 2011/09/06 18:33:01 joerg Exp $");
#endif /* not lint */

#include "tip.h"
/*
 * tip
 *
 * lower fork of tip -- handles passive side
 *  reading from the remote host
 */

void	intEMT(void);
void	intIOT(void);
void	intSYS(void);
__dead static void	intTERM(int);

/*
 * TIPOUT wait state routine --
 *   sent by TIPIN when it wants to posses the remote host
 */
void
intIOT(void)
{

	(void)write(repdes[1],&ccc,1);	/* We got the message */
	(void)read(fildes[0], &ccc,1);	/* Now wait for coprocess */
}

/*
 * Scripting command interpreter --
 *  accepts script file name over the pipe and acts accordingly
 */
void
intEMT(void)
{
	char c, line[256];
	char *pline = line;
	char reply;

	(void)read(fildes[0], &c, 1);		/* We got the message */
	while (c != '\n' && line + sizeof line - pline > 0) {
		*pline++ = c;
		(void)read(fildes[0], &c, 1);
	}
	*pline = '\0';
	if (boolean(value(SCRIPT)) && fscript != NULL)
		(void)fclose(fscript);
	if (pline == line) {
		setboolean(value(SCRIPT), FALSE);
		reply = 'y';
	} else {
		if ((fscript = fopen(line, "a")) == NULL)
			reply = 'n';
		else {
			reply = 'y';
			setboolean(value(SCRIPT), TRUE);
		}
	}
	(void)write(repdes[1], &reply, 1);	/* Now coprocess waits for us */
}

static void
/*ARGSUSED*/
intTERM(int dummy __unused)
{

	if (boolean(value(SCRIPT)) && fscript != NULL)
		(void)fclose(fscript);
	exit(0);
}

void
intSYS(void)
{

	setboolean(value(BEAUTIFY), !boolean(value(BEAUTIFY)));
}

/*
 * ****TIPOUT   TIPOUT****
 */
void
tipout(void)
{
	char buf[BUFSIZ];
	char *cp;
	int cnt;
	int omask;
	struct pollfd pfd[2];

	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGHUP, intTERM);	/* for dial-ups */
	(void)signal(SIGTERM, intTERM);	/* time to go signal*/

	pfd[0].fd = attndes[0];
	pfd[0].events = POLLIN;
	pfd[1].fd = FD;
	pfd[1].events = POLLIN|POLLHUP;

	for (omask = 0;; (void)sigsetmask(omask)) {

	if (poll(pfd, 2, -1) > 0) {

	if (pfd[0].revents & POLLIN)
		if (read(attndes[0], &ccc, 1) > 0) {
			switch(ccc) {
			case 'W':
				intIOT();	/* TIPIN wants us to wait */
				break;
			case 'S':
				intEMT();	/* TIPIN wants us to script */
				break;
			case 'B':
				intSYS();	/* "Beautify" value toggle */
				break;
			default:
				break;
			}
		}
	}

	if (pfd[1].revents & (POLLIN|POLLHUP)) {
			cnt = read(FD, buf, BUFSIZ);
			if (cnt <= 0) {
				/* lost carrier || EOF */
				if ((cnt < 0 && errno == EIO) || (cnt == 0)) {
					(void)sigblock(sigmask(SIGTERM));
					intTERM(0);
					/*NOTREACHED*/
				}
				continue;
			}
			omask = sigblock(SIGTERM);
			for (cp = buf; cp < buf + cnt; cp++)
				*cp &= STRIP_PAR;
			(void)write(1, buf, (size_t)cnt);
			if (boolean(value(SCRIPT)) && fscript != NULL) {
				if (!boolean(value(BEAUTIFY))) {
					(void)fwrite(buf, 1, (size_t)cnt, fscript);
					continue;
				}
				for (cp = buf; cp < buf + cnt; cp++)
					if ((*cp >= ' ' && *cp <= '~') ||
					    any(*cp, value(EXCEPTIONS)))
						(void)putc(*cp, fscript);
			}
		}
	}
}
