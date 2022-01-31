/*	$NetBSD: histedit.c,v 1.58 2022/01/31 16:54:28 kre Exp $	*/

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
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
static char sccsid[] = "@(#)histedit.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: histedit.c,v 1.58 2022/01/31 16:54:28 kre Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/*
 * Editline and history functions (and glue).
 */
#include "shell.h"
#include "parser.h"
#include "var.h"
#include "options.h"
#include "builtins.h"
#include "main.h"
#include "output.h"
#include "mystring.h"
#include "myhistedit.h"
#include "error.h"
#include "alias.h"
#ifndef SMALL
#include "eval.h"
#include "memalloc.h"

#define MAXHISTLOOPS	4	/* max recursions through fc */
#define DEFEDITOR	"ed"	/* default editor *should* be $EDITOR */

History *hist;	/* history cookie */
EditLine *el;	/* editline cookie */
int displayhist;
static FILE *el_in, *el_out;
static int curpos;

#ifdef DEBUG
extern FILE *tracefile;
#endif

static const char *fc_replace(const char *, char *, char *);
static int not_fcnumber(const char *);
static int str_to_event(const char *, int);
static int comparator(const void *, const void *);
static char **sh_matches(const char *, int, int);
static unsigned char sh_complete(EditLine *, int);

/*
 * Set history and editing status.  Called whenever the status may
 * have changed (figures out what to do).
 */
void
histedit(void)
{
	FILE *el_err;

#define editing (Eflag || Vflag)

	if (iflag == 1) {
		if (!hist) {
			/*
			 * turn history on
			 */
			INTOFF;
			hist = history_init();
			INTON;

			if (hist != NULL)
				sethistsize(histsizeval());
			else
				out2str("sh: can't initialize history\n");
		}
		if (editing && !el && isatty(0)) { /* && isatty(2) ??? */
			/*
			 * turn editing on
			 */
			char *term, *shname;

			INTOFF;
			if (el_in == NULL)
				el_in = fdopen(0, "r");
			if (el_out == NULL)
				el_out = fdopen(2, "w");
			if (el_in == NULL || el_out == NULL)
				goto bad;
			el_err = el_out;
#if DEBUG
			if (tracefile)
				el_err = tracefile;
#endif
			/*
			 * This odd piece of code doesn't affect the shell
			 * at all, the environment modified here is the
			 * stuff accessed via "environ" (the incoming
			 * envoironment to the shell) which is only ever
			 * touched at sh startup time (long before we get
			 * here) and ignored thereafter.
			 *
			 * But libedit calls getenv() to discover TERM
			 * and that searches the "environ" environment,
			 * not the shell's internal variable data struct,
			 * so we need to make sure that TERM in there is
			 * correct.
			 *
			 * This sequence copies TERM from the shell into
			 * the old "environ" environment.
			 */
			term = lookupvar("TERM");
			if (term)
				setenv("TERM", term, 1);
			else
				unsetenv("TERM");
			shname = arg0;
			if (shname[0] == '-')
				shname++;
			el = el_init(shname, el_in, el_out, el_err);
			if (el != NULL) {
				if (hist)
					el_set(el, EL_HIST, history, hist);

				set_prompt_lit(lookupvar("PSlit"));
				el_set(el, EL_SIGNAL, 1);
				el_set(el, EL_SAFEREAD, 1);
				el_set(el, EL_ALIAS_TEXT, alias_text, NULL);
				el_set(el, EL_ADDFN, "rl-complete",
				    "ReadLine compatible completion function",
				    sh_complete);
			} else {
bad:
				out2str("sh: can't initialize editing\n");
			}
			INTON;
		} else if (!editing && el) {
			INTOFF;
			el_end(el);
			el = NULL;
			INTON;
		}
		if (el) {
			INTOFF;
			if (Vflag)
				el_set(el, EL_EDITOR, "vi");
			else if (Eflag)
				el_set(el, EL_EDITOR, "emacs");
			el_set(el, EL_BIND, "^I", 
			    tabcomplete ? "rl-complete" : "ed-insert", NULL);
			el_source(el, lookupvar("EDITRC"));
			INTON;
		}
	} else {
		INTOFF;
		if (el) {	/* no editing if not interactive */
			el_end(el);
			el = NULL;
		}
		if (hist) {
			history_end(hist);
			hist = NULL;
		}
		INTON;
	}
}

void
set_prompt_lit(const char *lit_ch)
{
	wchar_t wc;

	if (!(iflag && editing && el))
		return;

	if (lit_ch == NULL) {
		el_set(el, EL_PROMPT, getprompt);
		return;
	}

	mbtowc(&wc, NULL, 1);		/* state init */

	INTOFF;
	if (mbtowc(&wc, lit_ch, strlen(lit_ch)) <= 0)
		el_set(el, EL_PROMPT, getprompt);
	else
		el_set(el, EL_PROMPT_ESC, getprompt, (int)wc);
	INTON;
}

void
set_editrc(const char *fname)
{
	INTOFF;
	if (iflag && editing && el)
		el_source(el, fname);
	INTON;
}

void
sethistsize(const char *hs)
{
	int histsize;
	HistEvent he;

	if (hist != NULL) {
		if (hs == NULL || *hs == '\0' || *hs == '-' ||
		   (histsize = number(hs)) < 0)
			histsize = 100;
		INTOFF;
		history(hist, &he, H_SETSIZE, histsize);
		history(hist, &he, H_SETUNIQUE, 1);
		INTON;
	}
}

void
setterm(const char *term)
{
	INTOFF;
	if (el != NULL && term != NULL)
		if (el_set(el, EL_TERMINAL, term) != 0) {
			outfmt(out2, "sh: Can't set terminal type %s\n", term);
			outfmt(out2, "sh: Using dumb terminal settings.\n");
		}
	INTON;
}

int
inputrc(int argc, char **argv)
{
	if (argc != 2) {
		out2str("usage: inputrc file\n");
		return 1;
	}
	if (el != NULL) {
		INTOFF;
		if (el_source(el, argv[1])) {
			INTON;
			out2str("inputrc: failed\n");
			return 1;
		}
		INTON;
		return 0;
	} else {
		out2str("sh: inputrc ignored, not editing\n");
		return 1;
	}
}

/*
 *  This command is provided since POSIX decided to standardize
 *  the Korn shell fc command.  Oh well...
 */
int
histcmd(volatile int argc, char ** volatile argv)
{
	int ch;
	const char * volatile editor = NULL;
	HistEvent he;
	volatile int lflg = 0, nflg = 0, rflg = 0, sflg = 0;
	int i, retval;
	const char *firststr, *laststr;
	int first, last, direction;
	char * volatile pat = NULL, * volatile repl;	/* ksh "fc old=new" crap */
	static int active = 0;
	struct jmploc jmploc;
	struct jmploc *volatile savehandler;
	char editfile[MAXPATHLEN + 1];
	FILE * volatile efp;
#ifdef __GNUC__
	repl = NULL;	/* XXX gcc4 */
	efp = NULL;	/* XXX gcc4 */
#endif

	if (hist == NULL)
		error("history not active");

	if (argc == 1)
		error("missing history argument");

	optreset = 1; optind = 1; /* initialize getopt */
	while (not_fcnumber(argv[optind]) &&
	      (ch = getopt(argc, argv, ":e:lnrs")) != -1)
		switch ((char)ch) {
		case 'e':
			editor = optionarg;
			break;
		case 'l':
			lflg = 1;
			break;
		case 'n':
			nflg = 1;
			break;
		case 'r':
			rflg = 1;
			break;
		case 's':
			sflg = 1;
			break;
		case ':':
			error("option -%c expects argument", optopt);
			/* NOTREACHED */
		case '?':
		default:
			error("unknown option: -%c", optopt);
			/* NOTREACHED */
		}
	argc -= optind, argv += optind;

	/*
	 * If executing...
	 */
	if (lflg == 0 || editor || sflg) {
		lflg = 0;	/* ignore */
		editfile[0] = '\0';
		/*
		 * Catch interrupts to reset active counter and
		 * cleanup temp files.
		 */
		savehandler = handler;
		if (setjmp(jmploc.loc)) {
			active = 0;
			if (*editfile)
				unlink(editfile);
			handler = savehandler;
			longjmp(handler->loc, 1);
		}
		handler = &jmploc;
		if (++active > MAXHISTLOOPS) {
			active = 0;
			displayhist = 0;
			error("called recursively too many times");
		}
		/*
		 * Set editor.
		 */
		if (sflg == 0) {
			if (editor == NULL &&
			    (editor = bltinlookup("FCEDIT", 1)) == NULL &&
			    (editor = bltinlookup("EDITOR", 1)) == NULL)
				editor = DEFEDITOR;
			if (editor[0] == '-' && editor[1] == '\0') {
				sflg = 1;	/* no edit */
				editor = NULL;
			}
		}
	}

	/*
	 * If executing, parse [old=new] now
	 */
	if (lflg == 0 && argc > 0 &&
	     ((repl = strchr(argv[0], '=')) != NULL)) {
		pat = argv[0];
		*repl++ = '\0';
		argc--, argv++;
	}

	/*
	 * If -s is specified, accept only one operand
	 */
	if (sflg && argc >= 2)
		error("too many args");

	/*
	 * determine [first] and [last]
	 */
	switch (argc) {
	case 0:
		firststr = lflg ? "-16" : "-1";
		laststr = "-1";
		break;
	case 1:
		firststr = argv[0];
		laststr = lflg ? "-1" : argv[0];
		break;
	case 2:
		firststr = argv[0];
		laststr = argv[1];
		break;
	default:
		error("too many args");
		/* NOTREACHED */
	}
	/*
	 * Turn into event numbers.
	 */
	first = str_to_event(firststr, 0);
	last = str_to_event(laststr, 1);

	if (rflg) {
		i = last;
		last = first;
		first = i;
	}
	/*
	 * XXX - this should not depend on the event numbers
	 * always increasing.  Add sequence numbers or offset
	 * to the history element in next (diskbased) release.
	 */
	direction = first < last ? H_PREV : H_NEXT;

	/*
	 * If editing, grab a temp file.
	 */
	if (editor) {
		int fd;
		INTOFF;		/* easier */
		snprintf(editfile, sizeof(editfile), "%s_shXXXXXX", _PATH_TMP);
		if ((fd = mkstemp(editfile)) < 0)
			error("can't create temporary file %s", editfile);
		if ((efp = fdopen(fd, "w")) == NULL) {
			close(fd);
			error("can't allocate stdio buffer for temp");
		}
	}

	/*
	 * Loop through selected history events.  If listing or executing,
	 * do it now.  Otherwise, put into temp file and call the editor
	 * after.
	 *
	 * The history interface needs rethinking, as the following
	 * convolutions will demonstrate.
	 */
	history(hist, &he, H_FIRST);
	retval = history(hist, &he, H_NEXT_EVENT, first);
	for (;retval != -1; retval = history(hist, &he, direction)) {
		if (lflg) {
			if (!nflg)
				out1fmt("%5d ", he.num);
			out1str(he.str);
		} else {
			const char *s = pat ?
			   fc_replace(he.str, pat, repl) : he.str;

			if (sflg) {
				if (displayhist) {
					out2str(s);
				}

				evalstring(strcpy(stalloc(strlen(s) + 1), s), 0);
				if (displayhist && hist) {
					/*
					 *  XXX what about recursive and
					 *  relative histnums.
					 */
					history(hist, &he, H_ENTER, s);
				}

				break;
			} else
				fputs(s, efp);
		}
		/*
		 * At end?  (if we were to lose last, we'd sure be
		 * messed up).
		 */
		if (he.num == last)
			break;
	}
	if (editor) {
		char *editcmd;
		size_t cmdlen;

		fclose(efp);
		cmdlen = strlen(editor) + strlen(editfile) + 2;
		editcmd = stalloc(cmdlen);
		snprintf(editcmd, cmdlen, "%s %s", editor, editfile);
		evalstring(editcmd, 0);	/* XXX - should use no JC command */
		stunalloc(editcmd);
		readcmdfile(editfile);	/* XXX - should read back - quick tst */
		unlink(editfile);
		INTON;
	}

	if (lflg == 0 && active > 0)
		--active;
	if (displayhist)
		displayhist = 0;
	return 0;
}

static const char *
fc_replace(const char *s, char *p, char *r)
{
	char *dest;
	int plen = strlen(p);

	STARTSTACKSTR(dest);
	while (*s) {
		if (*s == *p && strncmp(s, p, plen) == 0) {
			while (*r)
				STPUTC(*r++, dest);
			s += plen;
			*p = '\0';	/* so no more matches */
		} else
			STPUTC(*s++, dest);
	}
	STACKSTRNUL(dest);
	dest = grabstackstr(dest);

	return dest;
}


/*
 * Comparator function for qsort(). The use of curpos here is to skip
 * characters that we already know to compare equal (common prefix).
 */
static int
comparator(const void *a, const void *b)
{
	return strcmp(*(char *const *)a + curpos,
		*(char *const *)b + curpos);
}

/*
 * This function is passed to libedit's fn_complete(). The library will
 * use it instead of its standard function to find matches, which
 * searches for files in current directory. If we're at the start of the
 * line, we want to look for available commands from all paths in $PATH.
 */
static char
**sh_matches(const char *text, int start, int end)
{
	char *free_path = NULL, *dirname, *path;
	char **matches = NULL;
	size_t i = 0, size = 16;

	if (start > 0)
		return NULL;
	curpos = end - start;
	if ((free_path = path = strdup(pathval())) == NULL)
		goto out;
	if ((matches = malloc(size * sizeof(matches[0]))) == NULL)
		goto out;
	while ((dirname = strsep(&path, ":")) != NULL) {
		struct dirent *entry;
		DIR *dir;
		int dfd;

		if ((dir = opendir(dirname)) == NULL)
			continue;
		if ((dfd = dirfd(dir)) == -1)
			continue;
		while ((entry = readdir(dir)) != NULL) {
			struct stat statb;

			if (strncmp(entry->d_name, text, curpos) != 0)
				continue;
			if (entry->d_type == DT_UNKNOWN || entry->d_type == DT_LNK) {
				if (fstatat(dfd, entry->d_name, &statb, 0) == -1)
					continue;
				if (!S_ISREG(statb.st_mode))
					continue;
			} else if (entry->d_type != DT_REG)
				continue;
			if (++i >= size - 1) {
				size *= 2;
				if (reallocarr(&matches, size,
				    sizeof(*matches)))
				{
					closedir(dir);
					goto out;
				}
			}
			matches[i] = strdup(entry->d_name);
		}
		closedir(dir);
	}
out:
	free(free_path);
	if (i == 0) {
		free(matches);
		return NULL;
	}
	if (i == 1) {
		matches[0] = strdup(matches[1]);
		matches[i + 1] = NULL;
	} else {
		size_t j, k;

		qsort(matches + 1, i, sizeof(matches[0]), comparator);
		for (j = 1, k = 2; k <= i; k++)
			if (strcmp(matches[j] + curpos, matches[k] + curpos)
			    == 0)
				free(matches[k]);
			else
				matches[++j] = matches[k];
		matches[0] = strdup(text);
		matches[j + 1] = NULL;
	}
	return matches;
}

/*
 * This is passed to el_set(el, EL_ADDFN, ...) so that it's possible to
 * bind a key (tab by default) to execute the function.
 */
unsigned char
sh_complete(EditLine *sel, int ch __unused)
{
	return (unsigned char)fn_complete2(sel, NULL, sh_matches,
		L" \t\n\"\\'`@$><=;|&{(", NULL, NULL, (size_t)100,
		NULL, &((int) {0}), NULL, NULL, FN_QUOTE_MATCH);
}

static int
not_fcnumber(const char *s)
{
	if (s == NULL)
		return 0;
        if (*s == '-')
                s++;
	return !is_number(s);
}

static int
str_to_event(const char *str, int last)
{
	HistEvent he;
	const char *s = str;
	int relative = 0;
	int i, retval;

	retval = history(hist, &he, H_FIRST);
	switch (*s) {
	case '-':
		relative = 1;
		/*FALLTHROUGH*/
	case '+':
		s++;
	}
	if (is_number(s)) {
		i = number(s);
		if (relative) {
			while (retval != -1 && i--) {
				retval = history(hist, &he, H_NEXT);
			}
			if (retval == -1)
				retval = history(hist, &he, H_LAST);
		} else {
			retval = history(hist, &he, H_NEXT_EVENT, i);
			if (retval == -1) {
				/*
				 * the notion of first and last is
				 * backwards to that of the history package
				 */
				retval = history(hist, &he,
						last ? H_FIRST : H_LAST);
			}
		}
		if (retval == -1)
			error("history number %s not found (internal error)",
			       str);
	} else {
		/*
		 * pattern
		 */
		retval = history(hist, &he, H_PREV_STR, str);
		if (retval == -1)
			error("history pattern not found: %s", str);
	}
	return he.num;
}
#else
int
histcmd(int argc, char **argv)
{
	error("not compiled with history support");
	/* NOTREACHED */
}
int
inputrc(int argc, char **argv)
{
	error("not compiled with history support");
	/* NOTREACHED */
}
#endif
