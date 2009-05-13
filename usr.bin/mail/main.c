/*	$NetBSD: main.c,v 1.29.6.1 2009/05/13 19:19:56 jym Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c	8.2 (Berkeley) 4/20/95";
#else
__RCSID("$NetBSD: main.c,v 1.29.6.1 2009/05/13 19:19:56 jym Exp $");
#endif
#endif /* not lint */

#define EXTERN
#include "rcv.h"
#undef EXTERN
#include <assert.h>
#include <util.h>

#include "extern.h"
#include "sig.h"

#ifdef USE_EDITLINE
#include "complete.h"
#endif
#include "format.h"
#ifdef MIME_SUPPORT
#include "mime.h"
#endif
#ifdef THREAD_SUPPORT
#include "thread.h"
#endif

/*
 * Mail -- a mail program
 *
 * Startup -- interface with user.
 */

__dead
static void
usage(void)
{
#ifdef MIME_SUPPORT
	(void)fputs("\
Usage: mail [-EiInv] [-r rcfile] [-s subject] [-a file] [-c cc-addr]\n\
            [-b bcc-addr] to-addr ... [- sendmail-options ...]\n\
       mail [-EiInNv] [-H[colon-modifier]] -f [name]\n\
       mail [-EiInNv] [-H[colon-modifier]] [-u user]\n",
				stderr);
#else /* MIME_SUPPORT */
	(void)fputs("\
Usage: mail [-EiInv] [-r rcfile] [-s subject] [-c cc-addr] [-b bcc-addr]\n\
            to-addr ... [- sendmail-options ...]\n\
       mail [-EiInNv] [-H[colon-modifier]] -f [name]\n\
       mail [-EiInNv] [-H[colon-modifier]] [-u user]\n",
				stderr);
#endif /* MIME_SUPPORT */
	exit(1);
}

/*
 * Compute what the screen size for printing headers should be.
 * We use the following algorithm for the height:
 *	If baud rate < 1200, use  9
 *	If baud rate = 1200, use 14
 *	If baud rate > 1200, use 24 or ws_row
 * Width is either 80 or ws_col;
 */
PUBLIC void
setscreensize(void)
{
	struct termios tbuf;
	struct winsize ws;
	speed_t ospeed;
	char *cp;

	if (ioctl(1, TIOCGWINSZ, &ws) < 0)
		ws.ws_col = ws.ws_row = 0;
	if (tcgetattr(1, &tbuf) < 0)
		ospeed = 9600;
	else
		ospeed = cfgetospeed(&tbuf);
	if (ospeed < 1200)
		screenheight = 9;
	else if (ospeed == 1200)
		screenheight = 14;
	else if (ws.ws_row != 0)
		screenheight = ws.ws_row;
	else
		screenheight = 24;
	if ((realscreenheight = ws.ws_row) == 0)
		realscreenheight = 24;
	if ((screenwidth = ws.ws_col) == 0)
		screenwidth = 80;
	/*
	 * Possible overrides from the rcfile.
	 */
	if ((cp = value(ENAME_SCREENWIDTH)) != NULL) {
		int width;
		width = *cp ? atoi(cp) : 0;
		if (width >= 0)
			screenwidth = width;
	}
	if ((cp = value(ENAME_SCREENHEIGHT)) != NULL) {
		int height;
		height = *cp ? atoi(cp) : 0;
		if (height >= 0) {
			realscreenheight = height;
			screenheight = height;
		}
	}
}

/*
 * Break up a white-space or comma delimited name list so that aliases
 * can get expanded.  Without this, the CC: or BCC: list is broken too
 * late for alias expansion to occur.
 */
PUBLIC struct name *
lexpand(char *str, int ntype)
{
	char *list;
	struct name *np = NULL;
	char *word, *p;

	list = estrdup(str);
	word = list;
	for (word = list; *word; word = p) {
		word = skip_WSP(word);
		for (p = word;
		     *p && !is_WSP(*p) && *p != ',';
		     p++)
			continue;
		if (*p)
			*p++ = '\0';
		np = cat(np, nalloc(word, ntype));
	}

	free(list);
	return np;
}

PUBLIC int
main(int argc, char *argv[])
{
	jmp_buf jmpbuf;
	struct sigaction sa;
	struct name *to, *cc, *bcc, *smopts;
#ifdef MIME_SUPPORT
	struct name *attach_optargs;
	struct name *attach_end;
#endif
	char *subject;
	const char *ef;
	char nosrc = 0;
	const char *rc;
	int Hflag;
	int i;

	/*
	 * For portability, call setprogname() early, before
	 * getprogname() is called.
	 */
	(void)setprogname(argv[0]);

	/*
	 * Set up a reasonable environment.
	 * Figure out whether we are being run interactively,
	 * start the SIGCHLD catcher, and so forth.
	 * (Other signals are setup later by sig_setup().)
	 */
	(void)sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = sigchild;
	(void)sigaction(SIGCHLD, &sa, NULL);

	if (isatty(0))
		assign(ENAME_INTERACTIVE, "");
	image = -1;

	/*
	 * Now, determine how we are being used.
	 * We successively pick off - flags.
	 * If there is anything left, it is the base of the list
	 * of users to mail to.  Argp will be set to point to the
	 * first of these users.
	 */
	rc = NULL;
	ef = NULL;
	to = NULL;
	cc = NULL;
	bcc = NULL;
	smopts = NULL;
	subject = NULL;
	Hflag = 0;
#ifdef MIME_SUPPORT
	attach_optargs = NULL;
	attach_end = NULL;
	while ((i = getopt(argc, argv, ":~EH:INT:a:b:c:dfinr:s:u:v")) != -1)
#else
	while ((i = getopt(argc, argv, ":~EH:INT:b:c:dfinr:s:u:v")) != -1)
#endif
	{
		switch (i) {
		case 'T':
			/*
			 * Next argument is temp file to write which
			 * articles have been read/deleted for netnews.
			 */
			Tflag = optarg;
			if ((i = creat(Tflag, 0600)) < 0) {
				warn("%s", Tflag);
				exit(1);
			}
			(void)close(i);
			break;
#ifdef MIME_SUPPORT
		case 'a': {
			struct name *np;
			np = nalloc(optarg, 0);
			if (attach_end == NULL)
				attach_optargs = np;
			else {
				np->n_blink = attach_end;
				attach_end->n_flink = np;
			}
			attach_end = np;
			break;
		}
#endif
		case 'u':
			/*
			 * Next argument is person to pretend to be.
			 */
			myname = optarg;
			(void)unsetenv("MAIL");
			break;
		case 'i':
			/*
			 * User wants to ignore interrupts.
			 * Set the variable "ignore"
			 */
			assign(ENAME_IGNORE, "");
			break;
		case 'd':
			debug++;
			break;
		case 'r':
			rc = optarg;
			break;
		case 's':
			/*
			 * Give a subject field for sending from
			 * non terminal
			 */
			subject = optarg;
			break;
		case 'f':
			/*
			 * User is specifying file to "edit" with Mail,
			 * as opposed to reading system mailbox.
			 * If no argument is given after -f, we read his
			 * mbox file.
			 *
			 * getopt() can't handle optional arguments, so here
			 * is an ugly hack to get around it.
			 */
			if ((argv[optind]) && (argv[optind][0] != '-'))
				ef = argv[optind++];
			else
				ef = "&";
			break;
		case 'H':
			/*
			 * Print out the headers and quit.
			 */
			Hflag = get_Hflag(argv);
			break;
		case 'n':
			/*
			 * User doesn't want to source /usr/lib/Mail.rc
			 */
			nosrc++;
			break;
		case 'N':
			/*
			 * Avoid initial header printing.
			 */
			assign(ENAME_NOHEADER, "");
			break;
		case 'v':
			/*
			 * Send mailer verbose flag
			 */
			assign(ENAME_VERBOSE, "");
			break;
		case 'I':
		case '~':
			/*
			 * We're interactive
			 */
			assign(ENAME_INTERACTIVE, "");
			break;
		case 'c':
			/*
			 * Get Carbon Copy Recipient list
			 */
			cc = cat(cc, lexpand(optarg, GCC));
			break;
		case 'b':
			/*
			 * Get Blind Carbon Copy Recipient list
			 */
			bcc = cat(bcc, lexpand(optarg, GBCC));

			break;
		case 'E':
			/*
			 * Don't send empty files.
			 */
			assign(ENAME_DONTSENDEMPTY, "");
			break;
		case ':':
			/*
			 * An optarg was expected but not found.
			 */
			if (optopt == 'H') {
				Hflag = get_Hflag(NULL);
				break;
			}
			(void)fprintf(stderr,
			    "%s: option requires an argument -- %c\n",
			    getprogname(), optopt);

			/* FALLTHROUGH */
		case '?':
			/*
			 * An unknown option flag.  We need to do the
			 * error message.
			 */
			if (optopt != '?')
				(void)fprintf(stderr,
				    "%s: unknown option -- %c\n", getprogname(),
				    optopt);
			usage();	/* print usage message and die */
			/*NOTREACHED*/
		}
	}
	for (i = optind; (argv[i]) && (*argv[i] != '-'); i++)
		to = cat(to, nalloc(argv[i], GTO));
	for (/*EMPTY*/; argv[i]; i++)
		smopts = cat(smopts, nalloc(argv[i], GSMOPTS));
	/*
	 * Check for inconsistent arguments.
	 */
	if (to == NULL && (subject != NULL || cc != NULL || bcc != NULL))
		errx(1, "You must specify direct recipients with -s, -c, or -b.");
	if (ef != NULL && to != NULL) {
		errx(1, "Cannot give -f and people to send to.");
	}
	if (Hflag != 0 && to != NULL)
		errx(EXIT_FAILURE, "Cannot give -H and people to send to.");
#ifdef MIME_SUPPORT
	if (attach_optargs != NULL && to == NULL)
		errx(EXIT_FAILURE, "Cannot give -a without people to send to.");
#endif
	tinit();	/* must be done before loading the rcfile */
	input = stdin;
	mailmode = Hflag ? mm_hdrsonly :
	    to ? mm_sending : mm_receiving;

	spreserve();
	if (!nosrc)
		load(_PATH_MASTER_RC);
	/*
	 * Expand returns a savestr, but load only uses the file name
	 * for fopen, so it's safe to do this.
	 */
	if (rc == NULL && (rc = getenv("MAILRC")) == NULL)
		rc = "~/.mailrc";
	load(expand(rc));
	setscreensize();	/* do this after loading the rcfile */

#ifdef USE_EDITLINE
	/*
	 * This is after loading the MAILRC so we can use value().
	 * Avoid editline in mm_hdrsonly mode or pipelines will screw
	 * up.  XXX - there must be a better way!
	 */
	if (mailmode != mm_hdrsonly)
		init_editline();
#endif

	sig_setup();

	switch (mailmode) {
	case mm_sending:
		(void)mail(to, cc, bcc, smopts, subject,
		    mime_attach_optargs(attach_optargs));
		/*
		 * why wait?
		 */
		exit(senderr);
		break;	/* XXX - keep lint happy */

	case mm_receiving:
	case mm_hdrsonly:
		/*
		 * Ok, we are reading mail.
		 * Decide whether we are editing a mailbox or reading
		 * the system mailbox, and open up the right stuff.
		 */
		if (ef == NULL)
			ef = "%";
		if (setfile(ef) < 0)
			exit(1);		/* error already reported */
		if (value(ENAME_QUIET) == NULL)
			(void)printf("Mail version %s.  Type ? for help.\n",
			    version);
		if (mailmode == mm_hdrsonly)
			show_headers_and_exit(Hflag);	/* NORETURN */
		announce();
		(void)fflush(stdout);

		if (setjmp(jmpbuf) != 0) {
			/* Return here if quit() fails below. */
			(void)printf("Use 'exit' to quit without saving changes.\n");
		}
		commands();

		/* Ignore these signals from now on! */
		(void)signal(SIGHUP, SIG_IGN);
		(void)signal(SIGINT, SIG_IGN);
		(void)signal(SIGQUIT, SIG_IGN);
		quit(jmpbuf);
		break;

	default:
		assert(/*CONSTCOND*/0);
		break;
	}

	return 0;
}
