/*	$NetBSD: reboot.c,v 1.14 1997/07/19 23:03:54 perry Exp $	*/

/*
 * Copyright (c) 1980, 1986, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1986, 1993\n"
"	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)reboot.c	8.1 (Berkeley) 6/5/93";
#else
__RCSID("$NetBSD: reboot.c,v 1.14 1997/07/19 23:03:54 perry Exp $");
#endif
#endif /* not lint */

#include <sys/reboot.h>
#include <signal.h>
#include <pwd.h>
#include <err.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

int main __P((int, char *[]));
void usage __P((void));

int dohalt;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	struct passwd *pw;
	int ch, howto, lflag, nflag, qflag, sverrno, len;
	char *p, *user, *bootstr, **av;

	p = (p = rindex(*argv, '/')) ? p + 1 : *argv;
	if (!strcmp(p, "halt") || !strcmp(p, "-halt")) {
		dohalt = 1;
		howto = RB_HALT;
	} else
		howto = 0;
	lflag = nflag = qflag = 0;
	while ((ch = getopt(argc, argv, "lnqd")) != EOF)
		switch(ch) {
		case 'l':		/* Undocumented; used by shutdown. */
			lflag = 1;
			break;
		case 'n':
			nflag = 1;
			howto |= RB_NOSYNC;
			break;
		case 'q':
			qflag = 1;
			break;
		case 'd':
			howto |= RB_DUMP;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc) {
		for (av = argv, len = 0; *av; av++)
			len += strlen(*av);
		bootstr = malloc(len + 1);
		*bootstr = '\0';
		for (av = argv; *av; av++)
			strcpy(bootstr, *av);
		howto |= RB_STRING;
	} else
		bootstr = NULL;

	if (geteuid())
		errx(1, "%s", strerror(EPERM));

	if (qflag) {
		reboot(howto, bootstr);
		err(1, "reboot");
	}

	/* Log the reboot. */
	if (!lflag)  {
		if ((user = getlogin()) == NULL)
			user = (pw = getpwuid(getuid())) ?
			    pw->pw_name : "???";
		if (dohalt) {
			openlog("halt", 0, LOG_AUTH | LOG_CONS);
			syslog(LOG_CRIT, "halted by %s", user);
		} else {
			openlog("reboot", 0, LOG_AUTH | LOG_CONS);
			if (bootstr)
				syslog(LOG_CRIT, "rebooted by %s: %s", user,
				    bootstr);
			else
				syslog(LOG_CRIT, "rebooted by %s", user);
		}
	}
	logwtmp("~", "shutdown", "");

	/*
	 * Do a sync early on, so disks start transfers while we're off
	 * killing processes.  Don't worry about writes done before the
	 * processes die, the reboot system call syncs the disks.
	 */
	if (!nflag)
		sync();

	/* Just stop init -- if we fail, we'll restart it. */
	if (kill(1, SIGTSTP) == -1)
		err(1, "SIGTSTP init");

	/* Ignore the SIGHUP we get when our parent shell dies. */
	(void)signal(SIGHUP, SIG_IGN);

	/* Send a SIGTERM first, a chance to save the buffers. */
	if (kill(-1, SIGTERM) == -1) {
		/*
		 * If ESRCH, everything's OK: we're the only non-system
		 * process!  That can happen e.g. via 'exec reboot' in
		 * single-user mode.
		 */
		if (errno != ESRCH) {
			(void)fprintf(stderr, "%s: SIGTERM processes: %s",
			    dohalt ? "halt" : "reboot", strerror(errno));
			goto restart;
		}
	}

	/*
	 * After the processes receive the signal, start the rest of the
	 * buffers on their way.  Wait 5 seconds between the SIGTERM and
	 * the SIGKILL to give everybody a chance.
	 */
	sleep(2);
	if (!nflag)
		sync();
	sleep(3);

	for (i = 1;; ++i) {
		if (kill(-1, SIGKILL) == -1) {
			if (errno == ESRCH)
				break;
			goto restart;
		}
		if (i > 5) {
			(void)fprintf(stderr,
			    "WARNING: some process(es) wouldn't die\n");
			break;
		}
		(void)sleep(2 * i);
	}

	reboot(howto, bootstr);
	/* FALLTHROUGH */

restart:
	sverrno = errno;
	errx(1, "%s%s", kill(1, SIGHUP) == -1 ? "(can't restart init): " : "",
	    strerror(sverrno));
	/* NOTREACHED */
}

void
usage()
{
	(void)fprintf(stderr, "usage: %s [-nqd] [-- <boot string>]\n", dohalt ? "halt" : "reboot");
	exit(1);
}
