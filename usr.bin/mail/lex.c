/*	$NetBSD: lex.c,v 1.19 2002/03/04 03:16:10 wiz Exp $	*/

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
static char sccsid[] = "@(#)lex.c	8.2 (Berkeley) 4/20/95";
#else
__RCSID("$NetBSD: lex.c,v 1.19 2002/03/04 03:16:10 wiz Exp $");
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * Lexical processing of commands.
 */

extern char *version;
extern const struct cmd cmdtab[];
extern char *tempMesg;

char	*prompt = "& ";

/*
 * Set up editing on the given file name.
 * If the first character of name is %, we are considered to be
 * editing the file, otherwise we are reading our mail which has
 * signficance for mbox and so forth.
 */
int
setfile(char *name)
{
	FILE *ibuf;
	int i;
	struct stat stb;
	char isedit = *name != '%' || getuserid(myname) != getuid();
	char *who = name[1] ? name + 1 : myname;
	static int shudclob;

	if ((name = expand(name)) == NULL)
		return -1;

	if ((ibuf = Fopen(name, "r")) == NULL) {
		if (!isedit && errno == ENOENT)
			goto nomail;
		perror(name);
		return(-1);
	}

	if (fstat(fileno(ibuf), &stb) < 0) {
		perror("fstat");
		Fclose(ibuf);
		return (-1);
	}

	switch (stb.st_mode & S_IFMT) {
	case S_IFDIR:
		Fclose(ibuf);
		errno = EISDIR;
		perror(name);
		return (-1);

	case S_IFREG:
		break;

	default:
		Fclose(ibuf);
		errno = EINVAL;
		perror(name);
		return (-1);
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
	if ((i = open(name, 1)) < 0)
		readonly++;
	else
		close(i);
	if (shudclob) {
		fclose(itf);
		fclose(otf);
	}
	shudclob = 1;
	edit = isedit;
	strcpy(prevfile, mailname);
	if (name != mailname)
		strcpy(mailname, name);
	mailsize = fsize(ibuf);
	if ((otf = fopen(tempMesg, "w")) == NULL) {
		perror(tempMesg);
		exit(1);
	}
	(void) fcntl(fileno(otf), F_SETFD, 1);
	if ((itf = fopen(tempMesg, "r")) == NULL) {
		perror(tempMesg);
		exit(1);
	}
	(void) fcntl(fileno(itf), F_SETFD, 1);
	rm(tempMesg);
	setptr(ibuf, 0);
	setmsize(msgCount);
	/*
	 * New mail may have arrived while we were reading
	 * the mail file, so reset mailsize to be where
	 * we really are in the file...
	 */
	mailsize = ftell(ibuf);
	Fclose(ibuf);
	relsesigs();
	sawcom = 0;
	if (!edit && msgCount == 0) {
nomail:
		fprintf(stderr, "No mail for %s\n", who);
		return -1;
	}
	return(0);
}

/*
 * Incorporate any new mail that has arrived since we first
 * started reading mail.
 */
int
incfile(void)
{
	int newsize;
	int omsgCount = msgCount;
	FILE *ibuf;

	ibuf = Fopen(mailname, "r");
	if (ibuf == NULL)
		return -1;
	holdsigs();
	newsize = fsize(ibuf);
	if (newsize == 0)
		return -1;		/* mail box is now empty??? */
	if (newsize < mailsize)
		return -1;              /* mail box has shrunk??? */
	if (newsize == mailsize)
		return 0;               /* no new mail */
	setptr(ibuf, mailsize);
	setmsize(msgCount);
	mailsize = ftell(ibuf);
	Fclose(ibuf);
	relsesigs();
	return(msgCount - omsgCount);
}

int	*msgvec;
int	reset_on_stop;			/* do a reset() if stopped */

/*
 * Interpret user commands one by one.  If standard input is not a tty,
 * print no prompt.
 */
void
commands(void)
{
	int eofloop = 0;
	int n;
	char linebuf[LINESIZE];
#if __GNUC__
	/* Avoid longjmp clobbering */
	(void) &eofloop;
#endif

	if (!sourcing) {
		if (signal(SIGINT, SIG_IGN) != SIG_IGN)
			signal(SIGINT, intr);
		if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
			signal(SIGHUP, hangup);
		signal(SIGTSTP, stop);
		signal(SIGTTOU, stop);
		signal(SIGTTIN, stop);
	}
	setexit();
	for (;;) {
		/*
		 * Print the prompt, if needed.  Clear out
		 * string space, and flush the output.
		 */
		if (!sourcing && value("interactive") != NULL) {
			if ((value("autoinc") != NULL) && (incfile() > 0))
				printf("New mail has arrived.\n");
			reset_on_stop = 1;
			printf("%s", prompt);
		}
		fflush(stdout);
		sreset();
		/*
		 * Read a line of commands from the current input
		 * and handle end of file specially.
		 */
		n = 0;
		for (;;) {
			if (readline(input, &linebuf[n], LINESIZE - n) < 0) {
				if (n == 0)
					n = -1;
				break;
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
				unstack();
				continue;
			}
			if (value("interactive") != NULL &&
			    value("ignoreeof") != NULL &&
			    ++eofloop < 25) {
				printf("Use \"quit\" to quit.\n");
				continue;
			}
			break;
		}
		eofloop = 0;
		if (execute(linebuf, 0))
			break;
	}
}

/*
 * Execute a single command.
 * Command functions return 0 for success, 1 for error, and -1
 * for abort.  A 1 or -1 aborts a load or source.  A -1 aborts
 * the interactive command loop.
 * Contxt is non-zero if called while composing mail.
 */
int
execute(char linebuf[], int contxt)
{
	char word[LINESIZE];
	char *arglist[MAXARGC];
	const struct cmd *com = NULL;
	char *cp, *cp2;
	int c;
	int muvec[2];
	int e = 1;

	/*
	 * Strip the white space away from the beginning
	 * of the command, then scan out a word, which
	 * consists of anything except digits and white space.
	 *
	 * Handle ! escapes differently to get the correct
	 * lexical conventions.
	 */

	for (cp = linebuf; isspace((unsigned char)*cp); cp++)
		;
	if (*cp == '!') {
		if (sourcing) {
			printf("Can't \"!\" while sourcing\n");
			goto out;
		}
		shell(cp+1);
		return(0);
	}
	cp2 = word;
	while (*cp && strchr(" \t0123456789$^.:/-+*'\"", *cp) == NULL)
		*cp2++ = *cp++;
	*cp2 = '\0';

	/*
	 * Look up the command; if not found, bitch.
	 * Normally, a blank command would map to the
	 * first command in the table; while sourcing,
	 * however, we ignore blank lines to eliminate
	 * confusion.
	 */

	if (sourcing && *word == '\0')
		return(0);
	com = lex(word);
	if (com == NULL) {
		printf("Unknown command: \"%s\"\n", word);
		goto out;
	}

	/*
	 * See if we should execute the command -- if a conditional
	 * we always execute it, otherwise, check the state of cond.
	 */

	if ((com->c_argtype & F) == 0)
		if ((cond == CRCV && !rcvmode) || (cond == CSEND && rcvmode))
			return(0);

	/*
	 * Process the arguments to the command, depending
	 * on the type he expects.  Default to an error.
	 * If we are sourcing an interactive command, it's
	 * an error.
	 */

	if (!rcvmode && (com->c_argtype & M) == 0) {
		printf("May not execute \"%s\" while sending\n",
		    com->c_name);
		goto out;
	}
	if (sourcing && com->c_argtype & I) {
		printf("May not execute \"%s\" while sourcing\n",
		    com->c_name);
		goto out;
	}
	if (readonly && com->c_argtype & W) {
		printf("May not execute \"%s\" -- message file is read only\n",
		   com->c_name);
		goto out;
	}
	if (contxt && com->c_argtype & R) {
		printf("Cannot recursively invoke \"%s\"\n", com->c_name);
		goto out;
	}
	switch (com->c_argtype & ~(F|P|I|M|T|W|R)) {
	case MSGLIST:
		/*
		 * A message list defaulting to nearest forward
		 * legal message.
		 */
		if (msgvec == 0) {
			printf("Illegal use of \"message list\"\n");
			break;
		}
		if ((c = getmsglist(cp, msgvec, com->c_msgflag)) < 0)
			break;
		if (c  == 0) {
			*msgvec = first(com->c_msgflag,
				com->c_msgmask);
			msgvec[1] = 0;
		}
		if (*msgvec == 0) {
			printf("No applicable messages\n");
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
			printf("Illegal use of \"message list\"\n");
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
		while (isspace((unsigned char)*cp))
			cp++;
		e = (*com->c_func)(cp);
		break;

	case RAWLIST:
		/*
		 * A vector of strings, in shell style.
		 */
		if ((c = getrawlist(cp, arglist,
				sizeof arglist / sizeof *arglist)) < 0)
			break;
		if (c < com->c_minargs) {
			printf("%s requires at least %d arg(s)\n",
				com->c_name, com->c_minargs);
			break;
		}
		if (c > com->c_maxargs) {
			printf("%s takes no more than %d arg(s)\n",
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
			unstack();
		return 0;
	}
	if (com == NULL)
		return(0);
	if (value("autoprint") != NULL && com->c_argtype & P)
		if ((dot->m_flag & MDELETED) == 0) {
			muvec[0] = dot - &message[0] + 1;
			muvec[1] = 0;
			type(muvec);
		}
	if (!sourcing && (com->c_argtype & T) == 0)
		sawcom = 1;
	return(0);
}

/*
 * Set the size of the message vector used to construct argument
 * lists to message list functions.
 */
void
setmsize(int sz)
{

	if (msgvec != 0)
		free((char *) msgvec);
	msgvec = (int *) calloc((unsigned) (sz + 1), sizeof *msgvec);
}

/*
 * Find the correct command in the command table corresponding
 * to the passed command "word"
 */

const struct cmd *
lex(char word[])
{
	const struct cmd *cp;

	for (cp = &cmdtab[0]; cp->c_name != NULL; cp++)
		if (isprefix(word, cp->c_name))
			return(cp);
	return(NULL);
}

/*
 * Determine if as1 is a valid prefix of as2.
 * Return true if yep.
 */
int
isprefix(char *as1, char *as2)
{
	char *s1, *s2;

	s1 = as1;
	s2 = as2;
	while (*s1++ == *s2)
		if (*s2++ == '\0')
			return(1);
	return(*--s1 == '\0');
}

/*
 * The following gets called on receipt of an interrupt.  This is
 * to abort printout of a command, mainly.
 * Dispatching here when command() is inactive crashes rcv.
 * Close all open files except 0, 1, 2, and the temporary.
 * Also, unstack all source files.
 */

int	inithdr;			/* am printing startup headers */

/*ARGSUSED*/
void
intr(int s)
{

	noreset = 0;
	if (!inithdr)
		sawcom++;
	inithdr = 0;
	while (sourcing)
		unstack();

	close_all_files();

	if (image >= 0) {
		close(image);
		image = -1;
	}
	fprintf(stderr, "Interrupt\n");
	reset(0);
}

/*
 * When we wake up after ^Z, reprint the prompt.
 */
void
stop(int s)
{
	sig_t old_action = signal(s, SIG_DFL);
	sigset_t nset;

	sigemptyset(&nset);
	sigaddset(&nset, s);
	sigprocmask(SIG_UNBLOCK, &nset, NULL);
	kill(0, s);
	sigprocmask(SIG_BLOCK, &nset, NULL);
	signal(s, old_action);
	if (reset_on_stop) {
		reset_on_stop = 0;
		reset(0);
	}
}

/*
 * Branch here on hangup signal and simulate "exit".
 */
/*ARGSUSED*/
void
hangup(int s)
{

	/* nothing to do? */
	exit(1);
}

/*
 * Announce the presence of the current Mail version,
 * give the message count, and print a header listing.
 */
void
announce(void)
{
	int vec[2], mdot;

	mdot = newfileinfo(0);
	vec[0] = mdot;
	vec[1] = 0;
	dot = &message[mdot - 1];
	if (msgCount > 0 && value("noheader") == NULL) {
		inithdr++;
		headers(vec);
		inithdr = 0;
	}
}

/*
 * Announce information about the file we are editing.
 * Return a likely place to set dot.
 */
int
newfileinfo(int omsgCount)
{
	struct message *mp;
	int u, n, mdot, d, s, l;
	char fname[PATHSIZE], zname[PATHSIZE], *ename;

	for (mp = &message[omsgCount]; mp < &message[msgCount]; mp++)
		if (mp->m_flag & MNEW)
			break;
	if (mp >= &message[msgCount])
		for (mp = &message[omsgCount]; mp < &message[msgCount]; mp++)
			if ((mp->m_flag & MREAD) == 0)
				break;
	if (mp < &message[msgCount])
		mdot = mp - &message[0] + 1;
	else
		mdot = omsgCount + 1;
	s = d = 0;
	for (mp = &message[0], n = 0, u = 0; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW)
			n++;
		if ((mp->m_flag & MREAD) == 0)
			u++;
		if (mp->m_flag & MDELETED)
			d++;
		if (mp->m_flag & MSAVED)
			s++;
	}
	ename = mailname;
	if (getfold(fname) >= 0) {
		l = strlen(fname);
		if (l < PATHSIZE - 1)
			fname[l++] = '/';
		if (strncmp(fname, mailname, l) == 0) {
			snprintf(zname, PATHSIZE, "+%s",
			    mailname + l);
			ename = zname;
		}
	}
	printf("\"%s\": ", ename);
	if (msgCount == 1)
		printf("1 message");
	else
		printf("%d messages", msgCount);
	if (n > 0)
		printf(" %d new", n);
	if (u-n > 0)
		printf(" %d unread", u);
	if (d > 0)
		printf(" %d deleted", d);
	if (s > 0)
		printf(" %d saved", s);
	if (readonly)
		printf(" [Read only]");
	printf("\n");
	return(mdot);
}

/*
 * Print the current version number.
 */

/*ARGSUSED*/
int
pversion(void *v)
{
	printf("Version %s\n", version);
	return(0);
}

/*
 * Load a file of user definitions.
 */
void
load(char *name)
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
	Fclose(in);
}
