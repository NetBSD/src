/*	$NetBSD: collect.c,v 1.42.14.1 2009/05/13 19:19:56 jym Exp $	*/

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
#if 0
static char sccsid[] = "@(#)collect.c	8.2 (Berkeley) 4/19/94";
#else
__RCSID("$NetBSD: collect.c,v 1.42.14.1 2009/05/13 19:19:56 jym Exp $");
#endif
#endif /* not lint */

/*
 * Mail -- a mail program
 *
 * Collect input from standard input, handling
 * ~ escapes.
 */

#include <assert.h>
#include <util.h>

#include "rcv.h"
#include "extern.h"
#include "format.h"
#ifdef MIME_SUPPORT
#include "mime.h"
#endif
#include "sig.h"
#include "thread.h"


/*
 * Read a message from standard input and return a read file to it
 * or NULL on error.
 */

/*
 * The following hokiness with global variables is so that on
 * receipt of an interrupt signal, the partial message can be salted
 * away on dead.letter.
 */
static	FILE	*collf;			/* File for saving away */
static	int	hadintr;		/* Have seen one SIGINT so far */

static 	jmp_buf	abort_jmpbuf;		/* To end collection with error */
static 	jmp_buf	reset_jmpbuf;		/* To get back to work */
static	int	reset_on_stop;		/* To do job control longjmp. */

/*
 * Write a file, ex-like if f set.
 */
static int
exwrite(const char name[], FILE *fp, int f)
{
	FILE *of;
	int c;
	long cc;
	int lc;
	struct stat junk;

	if (f) {
		(void)printf("\"%s\" ", name);
		(void)fflush(stdout);
	}
	if (stat(name, &junk) >= 0 && S_ISREG(junk.st_mode)) {
		if (!f)
			(void)fprintf(stderr, "%s: ", name);
		(void)fprintf(stderr, "File exists\n");
		return -1;
	}
	if ((of = Fopen(name, "w")) == NULL) {
		warn("%s", name);
		return -1;
	}
	lc = 0;
	cc = 0;
	while ((c = getc(fp)) != EOF) {
		cc++;
		if (c == '\n')
			lc++;
		(void)putc(c, of);
		if (ferror(of)) {
			warn("%s", name);
			(void)Fclose(of);
			return -1;
		}
	}
	(void)Fclose(of);
	(void)printf("%d/%ld\n", lc, cc);
	(void)fflush(stdout);
	return 0;
}

/*
 * Edit the message being collected on fp.
 * On return, make the edit file the new temp file.
 */
static void
mesedit(FILE *fp, int c)
{
	struct sigaction osa;
	sigset_t oset;
	FILE *nf;

	sig_check();
	(void)sig_ignore(SIGINT, &osa, &oset);
	nf = run_editor(fp, (off_t)-1, c, 0);
	if (nf != NULL) {
		(void)fseek(nf, 0L, 2);
		collf = nf;
		(void)Fclose(fp);
	}
	(void)sig_restore(SIGINT, &osa, &oset);
	sig_check();
}

/*
 * Pipe the message through the command.
 * Old message is on stdin of command;
 * New message collected from stdout.
 * Sh -c must return 0 to accept the new message.
 */
static void
mespipe(FILE *fp, char cmd[])
{
	FILE *nf;
	struct sigaction osa;
	sigset_t oset;
	const char *shellcmd;
	int fd;
	char tempname[PATHSIZE];

	sig_check();
	(void)sig_ignore(SIGINT, &osa, &oset);

	(void)snprintf(tempname, sizeof(tempname),
	    "%s/mail.ReXXXXXXXXXX", tmpdir);
	if ((fd = mkstemp(tempname)) == -1 ||
	    (nf = Fdopen(fd, "w+")) == NULL) {
		if (fd != -1)
			(void)close(fd);
		warn("%s", tempname);
		goto out;
	}
	(void)unlink(tempname);
	/*
	 * stdin = current message.
	 * stdout = new message.
	 */
	if ((shellcmd = value(ENAME_SHELL)) == NULL)
		shellcmd = _PATH_CSHELL;
	if (run_command(shellcmd,
	    NULL, fileno(fp), fileno(nf), "-c", cmd, NULL) < 0) {
		(void)Fclose(nf);
		goto out;
	}
	if (fsize(nf) == 0) {
		(void)fprintf(stderr, "No bytes from \"%s\" !?\n", cmd);
		(void)Fclose(nf);
		goto out;
	}
	/*
	 * Take new files.
	 */
	(void)fseek(nf, 0L, 2);
	collf = nf;
	(void)Fclose(fp);
 out:
	(void)sig_restore(SIGINT, &osa, &oset);
	sig_check();
}

/*
 * Interpolate the named messages into the current
 * message, preceding each line with a tab.
 * Return a count of the number of characters now in
 * the message, or -1 if an error is encountered writing
 * the message temporary.  The flag argument is 'm' if we
 * should shift over and 'f' if not.
 */
static int
interpolate(char ms[], FILE *fp, char *fn, int f)
{
	int *msgvec;
	struct ignoretab *ig;
	const char *tabst;
#ifdef MIME_SUPPORT
	struct mime_info *mip;
	int retval;
#endif
	msgvec = salloc((get_msgCount() + 1) * sizeof(*msgvec));
	if (msgvec == NULL)
		return 0;
	if (getmsglist(ms, msgvec, 0) < 0)
		return 0;
	if (*msgvec == 0) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == 0) {
			(void)printf("No appropriate messages\n");
			return 0;
		}
		msgvec[1] = 0;
	}
	if (f == 'f' || f == 'F')
		tabst = NULL;
	else if ((tabst = value(ENAME_INDENTPREFIX)) == NULL)
		tabst = "\t";
	ig = isupper(f) ? NULL : ignore;
	(void)printf("Interpolating:");
	for (/*EMPTY*/; *msgvec != 0; msgvec++) {
		struct message *mp;
		char *fmtstr;

		mp = get_message(*msgvec);
		touch(mp);
		(void)printf(" %d", *msgvec);
		(void)fflush(stdout);	/* flush stdout and the above */

		if (tabst && (fmtstr = value(ENAME_INDENT_PREAMBLE)) != NULL)
			fmsgprintf(collf, fmtstr, mp);
#ifdef MIME_SUPPORT
		mip = NULL;
		if (value(ENAME_MIME_DECODE_MSG)) {
			if ((tabst == NULL && value(ENAME_MIME_DECODE_INSERT)) ||
			    (tabst != NULL && value(ENAME_MIME_DECODE_QUOTE)))
				mip = mime_decode_open(mp);
		}
		retval = mime_sendmessage(mp, fp, ig, tabst, mip);
		mime_decode_close(mip);
		if (retval < 0)
#else
		if (sendmessage(mp, fp, ig, tabst, NULL) < 0)
#endif
		{
			warn("%s", fn);
			return -1;
		}
		if (tabst && (fmtstr = value(ENAME_INDENT_POSTSCRIPT)) != NULL)
			fmsgprintf(collf, fmtstr, mp);
	}
	(void)printf("\n");
	return 0;
}

/*
 * Append the contents of the file to the end of the deadletter file.
 */
PUBLIC void
savedeadletter(FILE *fp)
{
	FILE *dbuf;
	mode_t m;
	int c;
	const char *cp;

	if (fsize(fp) == 0)
		return;
	cp = getdeadletter();
	m = umask(077);
	dbuf = Fopen(cp, "a");
	(void)umask(m);
	if (dbuf == NULL)
		return;
	(void)printf("Saving message body to `%s'.\n", cp);
	while ((c = getc(fp)) != EOF)
		(void)putc(c, dbuf);
	(void)Fclose(dbuf);
	rewind(fp);
}

/*
 * On interrupt, come here to save the partial message in ~/dead.letter.
 * Then jump out of the collection loop.
 */
static void
coll_int(int signo)
{

	/*
	 * the control flow is subtle, because we can be called from ~q.
	 */
	if (!hadintr) {
		if (value(ENAME_IGNORE) != NULL) {
			(void)puts("@");
			(void)fflush(stdout);
			clearerr(stdin);
			return;
		}
		hadintr = 1;
		longjmp(reset_jmpbuf, signo);
	}
	rewind(collf);
	if (value(ENAME_NOSAVE) == NULL)
		savedeadletter(collf);
	longjmp(abort_jmpbuf, signo);
}

/*ARGSUSED*/
static void
coll_hup(int signo __unused)
{

	rewind(collf);
	savedeadletter(collf);
	/*
	 * Let's pretend nobody else wants to clean up,
	 * a true statement at this time.
	 */
	exit(EXIT_FAILURE);
}

/*
 * Print (continue) when continued after ^Z.
 */
static void
coll_stop(int signo)
{

	if (reset_on_stop) {
		reset_on_stop = 0;
		hadintr = 0;
		longjmp(reset_jmpbuf, signo);
	}
}

PUBLIC FILE *
collect(struct header *hp, int printheaders)
{
	volatile sig_t old_sigint;
	volatile sig_t old_sighup;
	volatile sig_t old_sigtstp;
	volatile sig_t old_sigttin;
	volatile sig_t old_sigttou;
	FILE *fbuf;
	int lc, cc;
	int c, fd, t;
	char linebuf[LINESIZE];
	const char *cp;
	char tempname[PATHSIZE];
	char mailtempname[PATHSIZE];
	int eofcount;
	int longline;
	int lastlong, rc;	/* So we don't make 2 or more lines
				   out of a long input line. */

	/* The following are declared volatile to avoid longjmp clobbering. */
	char volatile getsub;
	int volatile escape;

	(void)memset(mailtempname, 0, sizeof(mailtempname));
	collf = NULL;

	if (setjmp(abort_jmpbuf) || setjmp(reset_jmpbuf)) {
		(void)rm(mailtempname);
		goto err;
	}
	sig_check();

	sig_hold();
	old_sigint  = sig_signal(SIGINT,  coll_int);
	old_sighup  = sig_signal(SIGHUP,  coll_hup);
	old_sigtstp = sig_signal(SIGTSTP, coll_stop);
	old_sigttin = sig_signal(SIGTTIN, coll_stop);
	old_sigttou = sig_signal(SIGTTOU, coll_stop);
	sig_release();

	noreset++;
	(void)snprintf(mailtempname, sizeof(mailtempname),
	    "%s/mail.RsXXXXXXXXXX", tmpdir);
	if ((fd = mkstemp(mailtempname)) == -1 ||
	    (collf = Fdopen(fd, "w+")) == NULL) {
		if (fd != -1)
			(void)close(fd);
		warn("%s", mailtempname);
		goto err;
	}
	(void)rm(mailtempname);

	/*
	 * If we are going to prompt for a subject,
	 * refrain from printing a newline after
	 * the headers (since some people mind).
	 */
	t = GTO | GSUBJECT | GCC | GNL | GSMOPTS;
	getsub = 0;
	if (hp->h_subject == NULL && value(ENAME_INTERACTIVE) != NULL &&
	    (value(ENAME_ASK) != NULL || value(ENAME_ASKSUB) != NULL)) {
		t &= ~GNL;
		getsub++;
	}
	if (printheaders) {
		(void)puthead(hp, stdout, t);
		(void)fflush(stdout);
	}
	if ((cp = value(ENAME_ESCAPE)) != NULL)
		escape = *cp;
	else
		escape = ESCAPE;
	hadintr = 0;
	if (setjmp(reset_jmpbuf) == 0) {
		if (getsub)
			(void)grabh(hp, GSUBJECT);
	} else {
		/*
		 * Come here for printing the after-signal message.
		 * Duplicate messages won't be printed because
		 * the write is aborted if we get a SIGTTOU.
		 */
 cont:
		if (hadintr) {
			(void)fflush(stdout);
			(void)fprintf(stderr,
			"\n(Interrupt -- one more to kill letter)\n");
		} else {
			(void)printf("(continue)\n");
			(void)fflush(stdout);
		}
	}
	eofcount = 0;	/* reset after possible longjmp */
	longline = 0;	/* reset after possible longjmp */
	for (;;) {
		reset_on_stop = 1;
		c = readline(stdin, linebuf, LINESIZE, reset_on_stop);
		reset_on_stop = 0;

		if (c < 0) {
			char *p;

			if (value(ENAME_INTERACTIVE) != NULL &&
			    (p = value(ENAME_IGNOREEOF)) != NULL &&
			    ++eofcount < (*p == 0 ? 25 : atoi(p))) {
				(void)printf("Use \".\" to terminate letter\n");
				continue;
			}
			break;
		}
		lastlong = longline;
		longline = c == LINESIZE - 1;
		eofcount = 0;
		hadintr = 0;
		if (linebuf[0] == '.' && linebuf[1] == '\0' &&
		    value(ENAME_INTERACTIVE) != NULL && !lastlong &&
		    (value(ENAME_DOT) != NULL || value(ENAME_IGNOREEOF) != NULL))
			break;
		if (linebuf[0] != escape || value(ENAME_INTERACTIVE) == NULL ||
		    lastlong) {
			if (putline(collf, linebuf, !longline) < 0)
				goto err;
			continue;
		}
		c = linebuf[1];
		switch (c) {
		default:
			/*
			 * On double escape, just send the single one.
			 * Otherwise, it's an error.
			 */
			if (c == escape) {
				if (putline(collf, &linebuf[1], !longline) < 0)
					goto err;
				else
					break;
			}
			(void)printf("Unknown tilde escape.\n");
			break;
#ifdef MIME_SUPPORT
		case '@':
			hp->h_attach = mime_attach_files(hp->h_attach, &linebuf[2]);
			break;
#endif
		case 'C':
			/*
			 * Dump core.
			 */
			(void)core(NULL);
			break;
		case '!':
			/*
			 * Shell escape, send the balance of the
			 * line to sh -c.
			 */
			(void)shell(&linebuf[2]);
			break;
		case ':':
		case '_':
			/*
			 * Escape to command mode, but be nice!
			 */
			(void)execute(&linebuf[2], ec_composing);
			goto cont;
		case '.':
			/*
			 * Simulate end of file on input.
			 */
			goto out;
		case 'q':
			/*
			 * Force a quit of sending mail.
			 * Act like an interrupt happened.
			 */
			hadintr++;
			coll_int(SIGINT);
			exit(1);
			/*NOTREACHED*/

		case 'x':	/* exit, do not save in dead.letter */
			goto err;

		case 'h':
			/*
			 * Grab a bunch of headers.
			 */
			(void)grabh(hp, GTO | GSUBJECT | GCC | GBCC | GSMOPTS);
			goto cont;
		case 't':
			/*
			 * Add to the To list.
			 */
			hp->h_to = cat(hp->h_to, extract(&linebuf[2], GTO));
			break;
		case 's':
			/*
			 * Set the Subject list.
			 */
			cp = skip_WSP(&linebuf[2]);
			hp->h_subject = savestr(cp);
			break;
		case 'c':
			/*
			 * Add to the CC list.
			 */
			hp->h_cc = cat(hp->h_cc, extract(&linebuf[2], GCC));
			break;
		case 'b':
			/*
			 * Add stuff to blind carbon copies list.
			 */
			hp->h_bcc = cat(hp->h_bcc, extract(&linebuf[2], GBCC));
			break;
		case 'i':
		case 'A':
		case 'a':
			/*
			 * Insert named variable in message
			 */

			switch(c) {
				case 'i':
					cp = skip_WSP(&linebuf[2]);
					break;
				case 'a':
					cp = "sign";
					break;
				case 'A':
					cp = "Sign";
					break;
				default:
					goto err;
			}

			if (*cp && (cp = value(cp)) != NULL) {
				(void)printf("%s\n", cp);
				if (putline(collf, cp, 1) < 0)
					goto err;
			}

			break;

		case 'd':
			(void)strcpy(linebuf + 2, getdeadletter());
			/* FALLTHROUGH */
		case 'r':
		case '<':
			/*
			 * Invoke a file:
			 * Search for the file name,
			 * then open it and copy the contents to collf.
			 */
			cp = skip_WSP(&linebuf[2]);
			if (*cp == '\0') {
				(void)printf("Interpolate what file?\n");
				break;
			}

			cp = expand(cp);
			if (cp == NULL)
				break;

			if (*cp == '!') {	/* insert stdout of command */
				const char *shellcmd;
				int nullfd;
				int rc2;

				if ((nullfd = open("/dev/null", O_RDONLY, 0)) == -1) {
					warn("/dev/null");
					break;
				}

				(void)snprintf(tempname, sizeof(tempname),
				    "%s/mail.ReXXXXXXXXXX", tmpdir);
				if ((fd = mkstemp(tempname)) == -1 ||
				    (fbuf = Fdopen(fd, "w+")) == NULL) {
					if (fd != -1)
						(void)close(fd);
					warn("%s", tempname);
					break;
				}
				(void)unlink(tempname);

				if ((shellcmd = value(ENAME_SHELL)) == NULL)
					shellcmd = _PATH_CSHELL;

				rc2 = run_command(shellcmd, NULL, nullfd, fileno(fbuf), "-c", cp + 1, NULL);

				(void)close(nullfd);

				if (rc2 < 0) {
					(void)Fclose(fbuf);
					break;
				}

				if (fsize(fbuf) == 0) {
					(void)fprintf(stderr, "No bytes from command \"%s\"\n", cp + 1);
					(void)Fclose(fbuf);
					break;
				}

				rewind(fbuf);
			}
			else if (isdir(cp)) {
				(void)printf("%s: Directory\n", cp);
				break;
			}
			else if ((fbuf = Fopen(cp, "r")) == NULL) {
				warn("%s", cp);
				break;
			}
			(void)printf("\"%s\" ", cp);
			(void)fflush(stdout);
			lc = 0;
			cc = 0;
			while ((rc = readline(fbuf, linebuf, LINESIZE, 0)) >= 0) {
				if (rc != LINESIZE-1) lc++;
				if ((t = putline(collf, linebuf,
						 rc != LINESIZE-1)) < 0) {
					(void)Fclose(fbuf);
					goto err;
				}
				cc += t;
			}
			(void)Fclose(fbuf);
			(void)printf("%d/%d\n", lc, cc);
			break;
		case 'w':
			/*
			 * Write the message on a file.
			 */
			cp = skip_WSP(&linebuf[2]);
			if (*cp == '\0') {
				(void)fprintf(stderr, "Write what file!?\n");
				break;
			}
			if ((cp = expand(cp)) == NULL)
				break;
			rewind(collf);
			(void)exwrite(cp, collf, 1);
			break;
		case 'm':
		case 'M':
		case 'f':
		case 'F':
			/*
			 * Interpolate the named messages, if we
			 * are in receiving mail mode.  Does the
			 * standard list processing garbage.
			 * If ~f is given, we don't shift over.
			 */
			if (interpolate(linebuf + 2, collf, mailtempname, c) < 0)
				goto err;
			goto cont;
		case '?':
			cathelp(_PATH_TILDE);
			break;
		case 'p':
			/*
			 * Print out the current state of the
			 * message without altering anything.
			 */
			rewind(collf);
			(void)printf("-------\nMessage contains:\n");
			(void)puthead(hp, stdout,
			    GTO | GSUBJECT | GCC | GBCC | GSMOPTS | GNL);
			while ((t = getc(collf)) != EOF)
				(void)putchar(t);
			goto cont;
		case '|':
			/*
			 * Pipe message through command.
			 * Collect output as new message.
			 */
			rewind(collf);
			mespipe(collf, &linebuf[2]);
			goto cont;
		case 'v':
		case 'e':
			/*
			 * Edit the current message.
			 * 'e' means to use EDITOR
			 * 'v' means to use VISUAL
			 */
			rewind(collf);
			mesedit(collf, c);
			goto cont;
		}
	}
	goto out;
 err:
	if (collf != NULL) {
		(void)Fclose(collf);
		collf = NULL;
	}
 out:
	if (collf != NULL)
		rewind(collf);
	noreset--;

	sig_hold();
	(void)sig_signal(SIGINT,  old_sigint);
	(void)sig_signal(SIGHUP,  old_sighup);
	(void)sig_signal(SIGTSTP, old_sigtstp);
	(void)sig_signal(SIGTTIN, old_sigttin);
	(void)sig_signal(SIGTTOU, old_sigttou);
	sig_release();

	sig_check();
	return collf;
}
