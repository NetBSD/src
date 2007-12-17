/*	$NetBSD: lex.c,v 1.36 2007/12/17 22:06:00 christos Exp $	*/

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
static char sccsid[] = "@(#)lex.c	8.2 (Berkeley) 4/20/95";
#else
__RCSID("$NetBSD: lex.c,v 1.36 2007/12/17 22:06:00 christos Exp $");
#endif
#endif /* not lint */

#include <assert.h>

#include "rcv.h"
#include <util.h>
#include "extern.h"
#ifdef USE_EDITLINE
#include "complete.h"
#endif
#include "format.h"
#include "thread.h"

/*
 * Mail -- a mail program
 *
 * Lexical processing of commands.
 */

static const char *prompt = DEFAULT_PROMPT;
static int	*msgvec;
static int	reset_on_stop;		/* do a reset() if stopped */


/*
 * Set the size of the message vector used to construct argument
 * lists to message list functions.
 */
static void
setmsize(int sz)
{
	if (msgvec != 0)
		free(msgvec);
	msgvec = ecalloc((size_t) (sz + 1), sizeof(*msgvec));
}

/*
 * Set up editing on the given file name.
 * If the first character of name is %, we are considered to be
 * editing the file, otherwise we are reading our mail which has
 * signficance for mbox and so forth.
 */
PUBLIC int
setfile(const char *name)
{
	FILE *ibuf;
	int i, fd;
	struct stat stb;
	char isedit = *name != '%' || getuserid(myname) != (int)getuid();
	const char *who = name[1] ? name + 1 : myname;
	static int shudclob;
	char tempname[PATHSIZE];

	if ((name = expand(name)) == NULL)
		return -1;

	if ((ibuf = Fopen(name, "r")) == NULL) {
		if (!isedit && errno == ENOENT)
			goto nomail;
		warn("%s", name);
		return -1;
	}

	if (fstat(fileno(ibuf), &stb) < 0) {
		warn("fstat");
		(void)Fclose(ibuf);
		return -1;
	}

	switch (stb.st_mode & S_IFMT) {
	case S_IFDIR:
		(void)Fclose(ibuf);
		errno = EISDIR;
		warn("%s", name);
		return -1;

	case S_IFREG:
		break;

	default:
		(void)Fclose(ibuf);
		errno = EINVAL;
		warn("%s", name);
		return -1;
	}

	/*
	 * Looks like all will be well.  We must now relinquish our
	 * hold on the current set of stuff.  Must hold signals
	 * while we are reading the new file, else we will ruin
	 * the message[] data structure.
	 */

	holdsigs();
	if (shudclob)
		quit();

	/*
	 * Copy the messages into /tmp
	 * and set pointers.
	 */

	readonly = 0;
	if ((i = open(name, O_WRONLY)) < 0)
		readonly++;
	else
		(void)close(i);
	if (shudclob) {
		(void)fclose(itf);
		(void)fclose(otf);
	}
	shudclob = 1;
	edit = isedit;
	(void)strcpy(prevfile, mailname);
	if (name != mailname)
		(void)strcpy(mailname, name);
	mailsize = fsize(ibuf);
	(void)snprintf(tempname, sizeof(tempname),
	    "%s/mail.RxXXXXXXXXXX", tmpdir);
	if ((fd = mkstemp(tempname)) == -1 ||
	    (otf = fdopen(fd, "w")) == NULL)
		err(1, "%s", tempname);
	(void)fcntl(fileno(otf), F_SETFD, FD_CLOEXEC);
	if ((itf = fopen(tempname, "r")) == NULL)
		err(1, "%s", tempname);
	(void)fcntl(fileno(itf), F_SETFD, FD_CLOEXEC);
	(void)rm(tempname);
	setptr(ibuf, (off_t)0);
	setmsize(get_abs_msgCount());
	/*
	 * New mail may have arrived while we were reading
	 * the mail file, so reset mailsize to be where
	 * we really are in the file...
	 */
	mailsize = ftell(ibuf);
	(void)Fclose(ibuf);
	relsesigs();
	sawcom = 0;
	if (!edit && get_abs_msgCount() == 0) {
nomail:
		(void)fprintf(stderr, "No mail for %s\n", who);
		return -1;
	}
	return 0;
}

/*
 * Incorporate any new mail that has arrived since we first
 * started reading mail.
 */
PUBLIC int
incfile(void)
{
	off_t newsize;
	int omsgCount;
	FILE *ibuf;
	int rval;

	omsgCount = get_abs_msgCount();

	ibuf = Fopen(mailname, "r");
	if (ibuf == NULL)
		return -1;
	holdsigs();
	newsize = fsize(ibuf);
	if (newsize == 0 ||		/* mail box is now empty??? */
	    newsize < mailsize) {	/* mail box has shrunk??? */
		rval = -1;
		goto done;
	}
	if (newsize == mailsize) {
		rval = 0;               /* no new mail */
		goto done;
	}
	setptr(ibuf, mailsize);		/* read in new mail */
	setmsize(get_abs_msgCount());	/* get the new message count */
	mailsize = ftell(ibuf);
	rval = get_abs_msgCount() - omsgCount;
 done:
	(void)Fclose(ibuf);
	relsesigs();
	return rval;
}

/*
 * Return a pointer to the comment character, respecting quoting as
 * done in getrawlist().  The comment character is ignored inside
 * quotes.
 */
static char *
comment_char(char *line)
{
	char *p;
	char quotec;
	quotec = '\0';
	for (p = line; *p; p++) {
		if (quotec != '\0') {
			if (*p == quotec)
				quotec = '\0';
		}
		else if (*p == '"' || *p == '\'')
			quotec = *p;
		else if (*p == COMMENT_CHAR)
			return p;
	}
	return NULL;
}

/*
 * When we wake up after ^Z, reprint the prompt.
 */
static void
stop(int s)
{
	sig_t old_action = signal(s, SIG_DFL);
	sigset_t nset;

	(void)sigemptyset(&nset);
	(void)sigaddset(&nset, s);
	(void)sigprocmask(SIG_UNBLOCK, &nset, NULL);
	(void)kill(0, s);
	(void)sigprocmask(SIG_BLOCK, &nset, NULL);
	(void)signal(s, old_action);
	if (reset_on_stop) {
		reset_on_stop = 0;
		reset(0);
	}
}



/*
 * Signal handler is hooked by setup_piping().
 * Respond to a broken pipe signal --
 * probably caused by quitting more.
 */
static jmp_buf	pipestop;

/*ARGSUSED*/
static void
brokpipe(int signo __unused)
{
	longjmp(pipestop, 1);
}

/*
 * Check the command line for any requested piping or redirection,
 * depending on the value of 'c'.  If "enable-pipes" is set, search
 * the command line (cp) for the first occurrence of the character 'c'
 * that is not in a quote or (parenthese) group.
 */
PUBLIC char *
shellpr(char *cp)
{
	int quotec;
	int level;

	if (cp == NULL || value(ENAME_ENABLE_PIPES) == NULL)
		return NULL;

	level = 0;
	quotec = 0;
	for (/*EMPTY*/; *cp != '\0'; cp++) {
		if (quotec) {
			if (*cp == quotec)
				quotec = 0;
			if (*cp == '\\' &&
			    (cp[1] == quotec || cp[1] == '\\'))
				cp++;
		}
		else {
			switch (*cp) {
			case '|':
			case '>':
				if (level == 0)
					return cp;
				break;
			case '(':
				level++;
				break;
			case ')':
				level--;
				break;
			case '"':
			case '\'':
				quotec = *cp;
				break;
			default:
				break;
			}
		}
	}
	return NULL;
}

/*
 * Setup any pipe or redirection that the command line indicates.
 * If none, then setup the pager unless "pager-off" is defined.
 */
static FILE *fp_stop = NULL;
static int oldfd1 = -1;
static int
setup_piping(char *cmdline, int c_pipe)
{
	FILE *fout;
	FILE *last_file;
	char *cp;

	last_file = last_registered_file(0);

	fout = NULL;
	if ((cp = shellpr(cmdline)) != NULL) {
		char c;
		c = *cp;
		*cp = '\0';
		cp++;

		if (c == '|') {
			if ((fout = Popen(cp, "w")) == NULL) {
				warn("Popen: %s", cp);
				return -1;
			}
		}
		else {
			const char *mode;
			assert(c == '>');
			mode = *cp == '>' ? "a" : "w";
			if (*cp == '>')
				cp++;

			cp = skip_WSP(cp);
			if ((fout = Fopen(cp, mode)) == NULL) {
				warn("Fopen: %s", cp);
				return -1;
			}
		}

	}
	else if (value(ENAME_PAGER_OFF) == NULL && (c_pipe & C_PIPE_PAGER ||
		(c_pipe & C_PIPE_CRT && value(ENAME_CRT) != NULL))) {
		const char *pager;
		pager = value(ENAME_PAGER);
		if (pager == NULL || *pager == '\0')
			pager = _PATH_MORE;

		if ((fout = Popen(pager, "w")) == NULL) {
			warn("Popen: %s", pager);
			return -1;
		}
	}

	if (fout) {
		(void)signal(SIGPIPE, brokpipe);
		(void)fflush(stdout);
		if ((oldfd1 = dup(STDOUT_FILENO)) == -1)
			err(EXIT_FAILURE, "dup failed");
		if (dup2(fileno(fout), STDOUT_FILENO) == -1)
			err(EXIT_FAILURE, "dup2 failed");
		fp_stop = last_file;
	}
	return 0;
}

/*
 * This will close any piping started by setup_piping().
 */
static void
close_piping(void)
{
	if (oldfd1 != -1) {
		(void)fflush(stdout);
		if (fileno(stdout) != oldfd1 &&
		    dup2(oldfd1, STDOUT_FILENO) == -1)
			err(EXIT_FAILURE, "dup2 failed");

		(void)signal(SIGPIPE, SIG_IGN);
		close_top_files(fp_stop);
		fp_stop = NULL;
		(void)close(oldfd1);
		oldfd1 = -1;
		(void)signal(SIGPIPE, SIG_DFL);
	}
}

/*
 * Determine if as1 is a valid prefix of as2.
 * Return true if yep.
 */
static int
isprefix(char *as1, const char *as2)
{
	char *s1;
	const char *s2;

	s1 = as1;
	s2 = as2;
	while (*s1++ == *s2)
		if (*s2++ == '\0')
			return 1;
	return *--s1 == '\0';
}

/*
 * Find the correct command in the command table corresponding
 * to the passed command "word"
 */
PUBLIC const struct cmd *
lex(char word[])
{
	const struct cmd *cp;

	for (cp = &cmdtab[0]; cp->c_name != NULL; cp++)
		if (isprefix(word, cp->c_name))
			return cp;
	return NULL;
}

PUBLIC char *
get_cmdname(char *buf)
{
	char *cp;
	char *cmd;
	size_t len;

	for (cp = buf; *cp; cp++)
		if (strchr(" \t0123456789$^.:/-+*'\">|", *cp) != NULL)
			break;
	/* XXX - Don't miss the pipe command! */
	if (cp == buf && *cp == '|')
		cp++;
	len = cp - buf + 1;
	cmd = salloc(len);
	(void)strlcpy(cmd, buf, len);
	return cmd;
}

/*
 * Execute a single command.
 * Command functions return 0 for success, 1 for error, and -1
 * for abort.  A 1 or -1 aborts a load or source.  A -1 aborts
 * the interactive command loop.
 * execute_contxt_e is in extern.h.
 */
PUBLIC int
execute(char linebuf[], enum execute_contxt_e contxt)
{
	char *word;
	char *arglist[MAXARGC];
	const struct cmd *com = NULL;
	char *volatile cp;
	int c;
	int e = 1;

	/*
	 * Strip the white space away from the beginning
	 * of the command, then scan out a word, which
	 * consists of anything except digits and white space.
	 *
	 * Handle ! escapes differently to get the correct
	 * lexical conventions.
	 */

	cp = skip_space(linebuf);
	if (*cp == '!') {
		if (sourcing) {
			(void)printf("Can't \"!\" while sourcing\n");
			goto out;
		}
		(void)shell(cp + 1);
		return 0;
	}

	word = get_cmdname(cp);
	cp += strlen(word);

	/*
	 * Look up the command; if not found, bitch.
	 * Normally, a blank command would map to the
	 * first command in the table; while sourcing,
	 * however, we ignore blank lines to eliminate
	 * confusion.
	 */

	if (sourcing && *word == '\0')
		return 0;
	com = lex(word);
	if (com == NULL) {
		(void)printf("Unknown command: \"%s\"\n", word);
		goto out;
	}

	/*
	 * See if we should execute the command -- if a conditional
	 * we always execute it, otherwise, check the state of cond.
	 */

	if ((com->c_argtype & F) == 0 && (cond & CSKIP))
		return 0;

	/*
	 * Process the arguments to the command, depending
	 * on the type he expects.  Default to an error.
	 * If we are sourcing an interactive command, it's
	 * an error.
	 */

	if (mailmode == mm_sending && (com->c_argtype & M) == 0) {
		(void)printf("May not execute \"%s\" while sending\n",
		    com->c_name);
		goto out;
	}
	if (sourcing && com->c_argtype & I) {
		(void)printf("May not execute \"%s\" while sourcing\n",
		    com->c_name);
		goto out;
	}
	if (readonly && com->c_argtype & W) {
		(void)printf("May not execute \"%s\" -- message file is read only\n",
		   com->c_name);
		goto out;
	}
	if (contxt == ec_composing && com->c_argtype & R) {
		(void)printf("Cannot recursively invoke \"%s\"\n", com->c_name);
		goto out;
	}

	if (!sourcing && com->c_pipe && value(ENAME_INTERACTIVE) != NULL) {
		if (setjmp(pipestop))
			goto out;

		if (setup_piping(cp, com->c_pipe) == -1)
			goto out;
	}
	switch (com->c_argtype & ARGTYPE_MASK) {
	case MSGLIST:
		/*
		 * A message list defaulting to nearest forward
		 * legal message.
		 */
		if (msgvec == 0) {
			(void)printf("Illegal use of \"message list\"\n");
			break;
		}
		if ((c = getmsglist(cp, msgvec, com->c_msgflag)) < 0)
			break;
		if (c  == 0) {
			*msgvec = first(com->c_msgflag,	com->c_msgmask);
			msgvec[1] = 0;
		}
		if (*msgvec == 0) {
			(void)printf("No applicable messages\n");
			break;
		}
		e = (*com->c_func)(msgvec);
		break;

	case NDMLIST:
		/*
		 * A message list with no defaults, but no error
		 * if none exist.
		 */
		if (msgvec == 0) {
			(void)printf("Illegal use of \"message list\"\n");
			break;
		}
		if (getmsglist(cp, msgvec, com->c_msgflag) < 0)
			break;
		e = (*com->c_func)(msgvec);
		break;

	case STRLIST:
		/*
		 * Just the straight string, with
		 * leading blanks removed.
		 */
		cp = skip_space(cp);
		e = (*com->c_func)(cp);
		break;

	case RAWLIST:
		/*
		 * A vector of strings, in shell style.
		 */
		if ((c = getrawlist(cp, arglist,
				sizeof(arglist) / sizeof(*arglist))) < 0)
			break;
		if (c < com->c_minargs) {
			(void)printf("%s requires at least %d arg(s)\n",
				com->c_name, com->c_minargs);
			break;
		}
		if (c > com->c_maxargs) {
			(void)printf("%s takes no more than %d arg(s)\n",
				com->c_name, com->c_maxargs);
			break;
		}
		e = (*com->c_func)(arglist);
		break;

	case NOLIST:
		/*
		 * Just the constant zero, for exiting,
		 * eg.
		 */
		e = (*com->c_func)(0);
		break;

	default:
		errx(1, "Unknown argtype");
	}

out:
	close_piping();

	/*
	 * Exit the current source file on
	 * error.
	 */
	if (e) {
		if (e < 0)
			return 1;
		if (loading)
			return 1;
		if (sourcing)
			(void)unstack();
		return 0;
	}
	if (com == NULL)
		return 0;
	if (contxt != ec_autoprint && com->c_argtype & P &&
	    value(ENAME_AUTOPRINT) != NULL && (dot->m_flag & MDELETED) == 0)
		(void)execute(__UNCONST("print ."), ec_autoprint);
	if (!sourcing && (com->c_argtype & T) == 0)
		sawcom = 1;
	return 0;
}


/*
 * The following gets called on receipt of an interrupt.  This is
 * to abort printout of a command, mainly.
 * Dispatching here when command() is inactive crashes rcv.
 * Close all open files except 0, 1, 2, and the temporary.
 * Also, unstack all source files.
 */
static int	inithdr;	/* am printing startup headers */

/*ARGSUSED*/
static void
intr(int s __unused)
{
	noreset = 0;
	if (!inithdr)
		sawcom++;
	inithdr = 0;
	while (sourcing)
		(void)unstack();

	close_piping();
	close_all_files();

	if (image >= 0) {
		(void)close(image);
		image = -1;
	}
	(void)fprintf(stderr, "Interrupt\n");
	reset(0);
}

/*
 * Branch here on hangup signal and simulate "exit".
 */
/*ARGSUSED*/
static void
hangup(int s __unused)
{
	/* nothing to do? */
	exit(1);
}

/*
 * Interpret user commands one by one.  If standard input is not a tty,
 * print no prompt.
 */
PUBLIC void
commands(void)
{
	int n;
	char linebuf[LINESIZE];
	int eofloop;

	if (!sourcing) {
		if (signal(SIGINT, SIG_IGN) != SIG_IGN)
			(void)signal(SIGINT, intr);
		if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
			(void)signal(SIGHUP, hangup);
		(void)signal(SIGTSTP, stop);
		(void)signal(SIGTTOU, stop);
		(void)signal(SIGTTIN, stop);
	}
	setexit();	/* defined as (void)setjmp(srbuf) in def.h */
	eofloop = 0;	/* initialize this after a possible longjmp */
	for (;;) {
		(void)fflush(stdout);
		sreset();
		/*
		 * Print the prompt, if needed.  Clear out
		 * string space, and flush the output.
		 */
		if (!sourcing && value(ENAME_INTERACTIVE) != NULL) {
			if ((prompt = value(ENAME_PROMPT)) == NULL)
				prompt = DEFAULT_PROMPT;
			prompt = smsgprintf(prompt, dot);
			if ((value(ENAME_AUTOINC) != NULL) && (incfile() > 0))
				(void)printf("New mail has arrived.\n");
			reset_on_stop = 1;
#ifndef USE_EDITLINE
			(void)printf("%s", prompt);
#endif
		}
		/*
		 * Read a line of commands from the current input
		 * and handle end of file specially.
		 */
		n = 0;
		for (;;) {
#ifdef USE_EDITLINE
			if (!sourcing) {
				char *line;
				if ((line = my_gets(&elm.command, prompt, NULL)) == NULL) {
					if (n == 0)
						n = -1;
					break;
				}
				(void)strlcpy(linebuf, line, sizeof(linebuf));
				setscreensize();	/* so we can resize a window */
			}
			else {
				if (mail_readline(input, &linebuf[n], LINESIZE - n) < 0) {
					if (n == 0)
						n = -1;
					break;
				}
			}
#else /* USE_EDITLINE */
			if (mail_readline(input, &linebuf[n], LINESIZE - n) < 0) {
				if (n == 0)
					n = -1;
				break;
			}
#endif /* USE_EDITLINE */

			if (sourcing) {  /* allow comments in source files */
				char *ptr;
				if ((ptr = comment_char(linebuf)) != NULL)
					*ptr = '\0';
			}
			if ((n = strlen(linebuf)) == 0)
				break;
			n--;
			if (linebuf[n] != '\\')
				break;
			linebuf[n++] = ' ';
		}
		reset_on_stop = 0;
		if (n < 0) {
				/* eof */
			if (loading)
				break;
			if (sourcing) {
				(void)unstack();
				continue;
			}
#ifdef USE_EDITLINE
			{
				char *p;
				if (value(ENAME_INTERACTIVE) != NULL &&
				    (p = value(ENAME_IGNOREEOF)) != NULL &&
				    ++eofloop < (*p == '\0' ? 25 : atoi(p))) {
					(void)printf("Use \"quit\" to quit.\n");
					continue;
				}
			}
#else
			if (value(ENAME_INTERACTIVE) != NULL &&
			    value(ENAME_IGNOREEOF) != NULL &&
			    ++eofloop < 25) {
				(void)printf("Use \"quit\" to quit.\n");
				continue;
			}
#endif
			break;
		}
		eofloop = 0;
		if (execute(linebuf, ec_normal))
			break;
	}
}

/*
 * Announce information about the file we are editing.
 * Return a likely place to set dot.
 */
PUBLIC int
newfileinfo(int omsgCount)
{
	struct message *mp;
	int d, n, s, t, u, mdot;
	char fname[PATHSIZE];
	char *ename;

	/*
	 * Figure out where to set the 'dot'.  Use the first new or
	 * unread message.
	 */
	for (mp = get_abs_message(omsgCount + 1); mp;
	     mp = next_abs_message(mp))
		if (mp->m_flag & MNEW)
			break;

	if (mp == NULL)
		for (mp = get_abs_message(omsgCount + 1); mp;
		     mp = next_abs_message(mp))
			if ((mp->m_flag & MREAD) == 0)
				break;
	if (mp != NULL)
		mdot = get_msgnum(mp);
	else
		mdot = omsgCount + 1;
#ifdef THREAD_SUPPORT
	/*
	 * See if the message is in the current thread.
	 */
	if (mp != NULL && get_message(1) != NULL && get_message(mdot) != mp)
		mdot = 0;
#endif
	/*
	 * Scan the message array counting the new, unread, deleted,
	 * and saved messages.
	 */
	d = n = s = t = u = 0;
	for (mp = get_abs_message(1); mp; mp = next_abs_message(mp)) {
		if (mp->m_flag & MNEW)
			n++;
		if ((mp->m_flag & MREAD) == 0)
			u++;
		if (mp->m_flag & MDELETED)
			d++;
		if (mp->m_flag & MSAVED)
			s++;
		if (mp->m_flag & MTAGGED)
			t++;
	}
	ename = mailname;
	if (getfold(fname, sizeof(fname)) >= 0) {
		char zname[PATHSIZE];
		size_t l;
		l = strlen(fname);
		if (l < sizeof(fname) - 1)
			fname[l++] = '/';
		if (strncmp(fname, mailname, l) == 0) {
			(void)snprintf(zname, sizeof(zname), "+%s",
			    mailname + l);
			ename = zname;
		}
	}
	/*
	 * Display the statistics.
	 */
	(void)printf("\"%s\": ", ename);
	{
		int cnt = get_abs_msgCount();
		(void)printf("%d message%s", cnt, cnt == 1 ? "" : "s");
	}
	if (n > 0)
		(void)printf(" %d new", n);
	if (u-n > 0)
		(void)printf(" %d unread", u);
	if (t > 0)
		(void)printf(" %d tagged", t);
	if (d > 0)
		(void)printf(" %d deleted", d);
	if (s > 0)
		(void)printf(" %d saved", s);
	if (readonly)
		(void)printf(" [Read only]");
	(void)printf("\n");

	return mdot;
}

/*
 * Announce the presence of the current Mail version,
 * give the message count, and print a header listing.
 */
PUBLIC void
announce(void)
{
	int vec[2], mdot;

	mdot = newfileinfo(0);
	vec[0] = mdot;
	vec[1] = 0;
	if ((dot = get_message(mdot)) == NULL)
		dot = get_abs_message(1); /* make sure we get something! */
	if (get_abs_msgCount() > 0 && value(ENAME_NOHEADER) == NULL) {
		inithdr++;
		(void)headers(vec);
		inithdr = 0;
	}
}

/*
 * Print the current version number.
 */

/*ARGSUSED*/
PUBLIC int
pversion(void *v __unused)
{
	(void)printf("Version %s\n", version);
	return 0;
}

/*
 * Load a file of user definitions.
 */
PUBLIC void
load(const char *name)
{
	FILE *in, *oldin;

	if ((in = Fopen(name, "r")) == NULL)
		return;
	oldin = input;
	input = in;
	loading = 1;
	sourcing = 1;
	commands();
	loading = 0;
	sourcing = 0;
	input = oldin;
	(void)Fclose(in);
}
