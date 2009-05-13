/*	$NetBSD: tty.c,v 1.27.26.1 2009/05/13 19:19:56 jym Exp $	*/

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
static char sccsid[] = "@(#)tty.c	8.2 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: tty.c,v 1.27.26.1 2009/05/13 19:19:56 jym Exp $");
#endif
#endif /* not lint */

/*
 * Mail -- a mail program
 *
 * Generally useful tty stuff.
 */

#include "rcv.h"
#include "extern.h"
#ifdef USE_EDITLINE
# include "complete.h"
#endif
#include "sig.h"

static	jmp_buf	tty_jmpbuf;		/* Place to go when interrupted */

#ifndef USE_EDITLINE
static	cc_t	c_erase;		/* Current erase char */
static	cc_t	c_kill;			/* Current kill char */
# ifndef TIOCSTI
static	int	ttyset;			/* We must now do erase/kill */
# endif
#endif /* USE_EDITLINE */

/*
 * Read up a header from standard input.
 * The source string has the preliminary contents to
 * be read.
 *
 * Returns: an salloc'ed copy of the line read if successful, or NULL
 * if no characters were read or if an error occurred.
 *
 */
#ifdef USE_EDITLINE
static char *
readtty(const char *pr, char *src)
{
	char *line;

	line = my_gets(&elm.string, pr, src);
	sig_check();
	if (line == NULL)
		(void)putc('\n', stdout);

	return line ? savestr(line) : __UNCONST("");
}

#else /* not USE_EDITLINE */

static char *
readtty(const char *pr, char *src)
{
	char canonb[LINESIZE];
	char *cp, *cp2;
	int c;
#ifdef TIOCSTI
	char ch;
#endif

	(void)fputs(pr, stdout);
	(void)fflush(stdout);
	if (src != NULL && strlen(src) > sizeof(canonb) - 2) {
		(void)printf("too long to edit\n");
		return src;
	}
#ifndef TIOCSTI
	if (src != NULL)
		cp = copy(src, canonb);
	else
		cp = copy(__UNCONST(""), canonb);
	c = *cp;
	(void)fputs(canonb, stdout);
	(void)fflush(stdout);
#else
	cp = src == NULL ? __UNCONST("") : src;
	while ((c = *cp++) != '\0') {
		if ((c_erase != _POSIX_VDISABLE && c == c_erase) ||
		    (c_kill != _POSIX_VDISABLE && c == c_kill)) {
			ch = '\\';
			(void)ioctl(0, TIOCSTI, &ch);
		}
		ch = c;
		(void)ioctl(0, TIOCSTI, &ch);
	}
	cp = canonb;
	*cp = '\0';
#endif
	clearerr(stdin);
	cp2 = cp;
	while (cp2 < canonb + sizeof(canonb) - 1) {
		c = getc(stdin);
		sig_check();
		if (c == EOF) {
			if (feof(stdin))
				(void)putc('\n', stdout);
			break;
		}
		if (c == '\n')
			break;
		*cp2++ = c;
	}
	*cp2 = '\0';

	if (c == EOF && ferror(stdin)) {
		cp = strlen(canonb) > 0 ? canonb : NULL;
		clearerr(stdin);
		return readtty(pr, cp);
	}
#ifndef TIOCSTI
	if (cp == NULL || *cp == '\0')
		return src;
	if (ttyset == 0)
		return strlen(canonb) > 0 ? savestr(canonb) : NULL;

	/*
	 * Do erase and kill.
	 */
	cp2 = cp;
	while (*cp != '\0') {
		c = *cp++;
		if (c_erase != _POSIX_VDISABLE && c == c_erase) {
			if (cp2 == canonb)
				continue;
			if (cp2[-1] == '\\') {
				cp2[-1] = c;
				continue;
			}
			cp2--;
			continue;
		}
		if (c_kill != _POSIX_VDISABLE && c == c_kill) {
			if (cp2 == canonb)
				continue;
			if (cp2[-1] == '\\') {
				cp2[-1] = c;
				continue;
			}
			cp2 = canonb;
			continue;
		}
		*cp2++ = c;
	}
	*cp2 = '\0';
#endif
	if (canonb[0] == '\0')
		return __UNCONST("");
	return savestr(canonb);
}
#endif /* USE_EDITLINE */

#ifdef USE_EDITLINE
# define save_erase_and_kill(t)		0
#else
static int
save_erase_and_kill(struct termios *t)
{

# ifndef TIOCSTI
	ttyset = 0;
#endif
	if (tcgetattr(fileno(stdin), t) == -1) {
		warn("tcgetattr");
		return -1;
	}
	c_erase = t->c_cc[VERASE];
	c_kill = t->c_cc[VKILL];
	return 0;
}
#endif

#if defined(USE_EDITLINE) || defined(TIOCSTI)
# define disable_erase_and_kill(t)
#else
static void
disable_erase_and_kill(struct termios *t)
{

	if (ttyset == 0) {
		ttyset = 1;
		t->c_cc[VERASE] = _POSIX_VDISABLE;
		t->c_cc[VKILL] = _POSIX_VDISABLE;
		(void)tcsetattr(fileno(stdin), TCSADRAIN, t);
	}
}
#endif

#if defined(USE_EDITLINE) || defined(TIOCSTI)
# define restore_erase_and_kill(t)
#else
static void
restore_erase_and_kill(struct termios *t)
{

	if (ttyset != 0) {
		ttyset = 0;
		t->c_cc[VERASE] = c_erase;
		t->c_cc[VKILL] = c_kill;
		(void)tcsetattr(fileno(stdin), TCSADRAIN, t);
	}
}
#endif

/*
 * Do a shell-like extraction of a line
 * and make a list of name from it.
 * Return the list or NULL if none found.
 */
static struct name *
shextract(char *line, int ntype)
{
	struct name *begin, *np, *t;
	char *argv[MAXARGC];
	size_t argc, i;

	begin = NULL;
	if (line) {
		np = NULL;
		argc = getrawlist(line, argv, (int)__arraycount(argv));
		for (i = 0; i < argc; i++) {
			t = nalloc(argv[i], ntype);
			if (begin == NULL)
				begin = t;
			else
				np->n_flink = t;
			t->n_blink = np;
			np = t;
		}
	}
	return begin;
}

/*ARGSUSED*/
static void
tty_sigint(int signo __unused)
{

	longjmp(tty_jmpbuf, 1);
}

/*
 * Read all relevant header fields.
 * Returns 0 on success; 1 if there was an error or signal.
 */
PUBLIC int
grabh(struct header *hp, int gflags)
{
	sig_t volatile old_sigint;
	int retval;
#ifndef USE_EDITLINE
	struct termios ttybuf;
# if defined(TIOCSTI) && defined(TIOCEXT)
	int extproc;
# endif

	if (save_erase_and_kill(&ttybuf))
		return -1;

# if defined(TIOCSTI) && defined(TIOCEXT)
	extproc = ((ttybuf.c_lflag & EXTPROC) ? 1 : 0);
	if (extproc) {
		int flag;

		flag = 0;
		if (ioctl(fileno(stdin), TIOCEXT, &flag) == -1)
			warn("TIOCEXT: off");
	}
# endif
#endif /* USE_EDITLINE */

	sig_check();
	old_sigint = sig_signal(SIGINT, tty_sigint);

	/* return here if we detect a SIGINT */
	if ((retval = setjmp(tty_jmpbuf)) != 0) {
		(void)putc('\n', stdout);
		goto out;
	}

	/*
	 * Do this irrespective of whether the initial string is empty.
	 * Otherwise, the editing is inconsistent.
	 */
	disable_erase_and_kill(&ttybuf);

	if (gflags & GTO) {
		hp->h_to =
		    extract(readtty("To: ", detract(hp->h_to, 0)), GTO);
	}
	if (gflags & GSUBJECT) {
		hp->h_subject = readtty("Subject: ", hp->h_subject);
	}
	if (gflags & GCC) {
		hp->h_cc =
		    extract(readtty("Cc: ", detract(hp->h_cc, 0)), GCC);
	}
	if (gflags & GBCC) {
		hp->h_bcc =
		    extract(readtty("Bcc: ", detract(hp->h_bcc, 0)), GBCC);
	}
	if (gflags & GSMOPTS) {
		hp->h_smopts =
		    shextract(readtty("Smopts: ", detract(hp->h_smopts, 0)),
			GSMOPTS);
	}
#ifdef MIME_SUPPORT
	if (gflags & GSMOPTS) {	/* XXX - Use a new flag for this? */
		if (hp->h_attach) {
			struct attachment *ap;
			int i;

			i = 0;
			for (ap = hp->h_attach; ap; ap = ap->a_flink)
				i++;
			(void)printf("Attachment%s: %d\n", i > 1 ? "s" : "", i);
		}
	}
#endif
 out:
	restore_erase_and_kill(&ttybuf);

#ifndef USE_EDITLINE
# if defined(TIOCSTI) && defined(TIOCEXT)
	if (extproc) {
		int flag;
		flag = 1;
		if (ioctl(fileno(stdin), TIOCEXT, &flag) == -1)
			warn("TIOCEXT: on");
	}
# endif
#endif
	(void)sig_signal(SIGINT, old_sigint);
	sig_check();
	return retval;
}
