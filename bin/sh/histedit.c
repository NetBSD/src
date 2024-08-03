/*	$NetBSD: histedit.c,v 1.73 2024/08/03 03:46:23 kre Exp $	*/

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
__RCSID("$NetBSD: histedit.c,v 1.73 2024/08/03 03:46:23 kre Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
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
#include "redir.h"

#ifndef SMALL		/* almost all the rest of this file */

#include "eval.h"
#include "memalloc.h"
#include "show.h"

#define MAXHISTLOOPS	4	/* max recursions through fc */
#define DEFEDITOR	"ed"	/* default editor *should* be $EDITOR */

History *hist;	/* history cookie */
EditLine *el;	/* editline cookie */
int displayhist;
static FILE *el_in, *el_out;
static int curpos;

static char *HistFile = NULL;
static const char *HistFileOpen = NULL;
FILE *HistFP = NULL;
static int History_fd;

#ifdef DEBUG
extern FILE *tracefile;
#endif

static const char *fc_replace(const char *, char *, char *);
static int not_fcnumber(const char *);
static int str_to_event(const char *, int);
static int comparator(const void *, const void *);
static char **sh_matches(const char *, int, int);
static unsigned char sh_complete(EditLine *, int);
static FILE *Hist_File_Open(const char *);

/*
 * Set history and editing status.  Called whenever the status may
 * have changed (figures out what to do).
 */
void
histedit(void)
{
	FILE *el_err;

#define editing (Eflag || Vflag)

	CTRACE(DBG_HISTORY, ("histedit: %cE%cV %sinteractive\n",
	    Eflag ? '-' : '+',  Vflag ? '-' : '+', iflag ? "" : "not "));

	if (iflag == 1) {
		if (!hist) {
			/*
			 * turn history on
			 */
			INTOFF;
			hist = history_init();
			INTON;

			if (hist != NULL) {
				sethistsize(histsizeval(), histsizeflags());
				sethistfile(histfileval(), histfileflags());
			} else
				out2str("sh: can't initialize history\n");
		}
		if (editing && !el && isatty(0)) { /* && isatty(2) ??? */
			/*
			 * turn editing on
			 */
			char *term;

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
			 * environment to the shell) which is only ever
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
			el = el_init("sh", el_in, el_out, el_err);
			VTRACE(DBG_HISTORY, ("el_init() %sed\n",
			    el != NULL ? "succeed" : "fail"));
			if (el != NULL) {
				if (hist)
					el_set(el, EL_HIST, history, hist);

				set_prompt_lit(lookupvar("PSlit"), 0);
				el_set(el, EL_SIGNAL, 1);
				el_set(el, EL_SAFEREAD, 1);
				el_set(el, EL_ALIAS_TEXT, alias_text, NULL);
				el_set(el, EL_ADDFN, "rl-complete",
				    "ReadLine compatible completion function",
				    sh_complete);
			} else {
 bad:;
				out2str("sh: can't initialize editing\n");
			}
			INTON;
		} else if (!editing && el) {
			INTOFF;
			el_end(el);
			el = NULL;
			VTRACE(DBG_HISTORY, ("line editing disabled\n"));
			INTON;
		}
		if (el) {
			INTOFF;
			if (Vflag)
				el_set(el, EL_EDITOR, "vi");
			else if (Eflag)
				el_set(el, EL_EDITOR, "emacs");
			VTRACE(DBG_HISTORY, ("reading $EDITRC\n"));
			el_source(el, lookupvar("EDITRC"));
			el_set(el, EL_BIND, "^I",
			    tabcomplete ? "rl-complete" : "ed-insert", NULL);
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
		VTRACE(DBG_HISTORY, ("line editing & history disabled\n"));
	}
}

void
set_prompt_lit(char *lit_ch, int flags __unused)
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
set_editrc(char *fname, int flags)
{
	INTOFF;
	if (iflag && editing && el && !(flags & VUNSET))
		el_source(el, fname);
	INTON;
}

void
sethistsize(char *hs, int flags)
{
	int histsize;
	HistEvent he;

	CTRACE(DBG_HISTORY, ("Set HISTSIZE=%s [%x] %s\n",
	    (hs == NULL ? "''" : hs), flags, "!hist" + (hist != NULL)));

	if (hs != NULL && *hs != '\0' && (flags & VUNSAFE) && !is_number(hs))
		hs = NULL;

	if (hs == NULL || *hs == '\0' || (flags & VUNSET) ||
	    (histsize = number(hs)) < 0)
		histsize = 100;

	if (hist != NULL) {
		INTOFF;
		/* H_SETSIZE actually sets n-1 as the limit */
		history(hist, &he, H_SETSIZE, histsize + 1);
		history(hist, &he, H_SETUNIQUE, 1);
		INTON;
	}
}

void
sethistfile(char *hs, int flags)
{
	const char *file;
	HistEvent he;

	CTRACE(DBG_HISTORY, ("Set HISTFILE=%s [%x] %s\n",
	    (hs == NULL ? "''" : hs), flags, "!hist" + (hist != NULL)));

	if (hs == NULL || *hs == '\0' || (flags & VUNSET)) {
		if (HistFP != NULL) {
			fclose(HistFP);
			HistFP = NULL;
		}
		if (HistFile != NULL) {
			free(HistFile);
			HistFile = NULL;
			HistFileOpen = NULL;
		}
		return;
	}

	if (hist != NULL) {
		file = expandvar(hs, flags);
		if (file == NULL || *file == '\0')
			return;

		INTOFF;

		history(hist, &he, H_LOAD, file);

		/*
		 * This is needed so sethistappend() can work
		 * on the current (new) filename, not the previous one.
		 */
		if (HistFile != NULL)
			free(HistFile);

		HistFile = strdup(hs);
		/*
		 * We need to ensure that HistFile & HistFileOpen
		 * are not equal .. we know HistFile has just altered.
		 * If they happen to be equal (both NULL perhaps, or
		 * the strdup() just above happned to return the same
		 * buffer as was freed the line before) then simply
		 * set HistFileOpen to something which cannot be the
		 * same as anything allocated, or NULL.   Its only
		 * use is to compare against HistFile.
		 */
		if (HistFile == HistFileOpen)
			HistFileOpen = "";

		sethistappend((histappflags() & VUNSET) ? NULL : histappval(),
			~VUNSET & 0xFFFF);

		INTON;
	}
}

void
sethistappend(char *s, int flags __diagused)
{
	CTRACE(DBG_HISTORY, ("Set HISTAPPEND=%s [%x] %s ",
	    (s == NULL ? "''" : s), flags, "!hist" + (hist != NULL)));

	INTOFF;
	if (flags & VUNSET || !boolstr(s)) {
		CTRACE(DBG_HISTORY, ("off"));

		if (HistFP != NULL) {
			CTRACE(DBG_HISTORY, (" closing"));

			fclose(HistFP);
			HistFP = NULL;
			HistFileOpen = NULL;
		}
	} else {
		CTRACE(DBG_HISTORY, ("on"));

		if (HistFileOpen != HistFile || HistFP == NULL) {
			if (HistFP != NULL) {
				CTRACE(DBG_HISTORY, (" closing prev"));
				fclose(HistFP);
				HistFP = NULL;
			}
			if (hist != NULL &&
			    HistFile != NULL &&
			    HistFP == NULL) {
				CTRACE(DBG_HISTORY, ("\n"));

				save_sh_history();

				CTRACE(DBG_HISTORY, ("opening: "));

				HistFP = Hist_File_Open(HistFile);
				if (HistFP != NULL)
					HistFileOpen = HistFile;
				else {
					CTRACE(DBG_HISTORY, ("open failed"));
				}
			}
		}
	}
	INTON;

	CTRACE(DBG_HISTORY, ("\n"));
}

static void
History_FD_Renumbered(int from, int to)
{
	if (History_fd == from)
		History_fd = to;

	VTRACE(DBG_HISTORY, ("History_FD_Renumbered(%d,%d)-> %d\n",
	    from, to, History_fd));
}

/*
 * The callback functions for the FILE* returned by funopen2()
 */
static ssize_t
Hist_Write(void *cookie, const void *buf, size_t len)
{
	if (cookie != (void *)&History_fd) {
		errno = EINVAL;
		return -1;
	}

	return write(History_fd, buf, len);
}

static int
Hist_Close(void *cookie)
{
	if (cookie == (void *)&History_fd) {
		sh_close(History_fd);
		History_fd = -1;
		return 0;
	}

	VTRACE(DBG_HISTORY, ("HistClose(%p) != %p\n", cookie, &History_fd));

	errno = EINVAL;
	return -1;
}

static off_t
Hist_Seek(void *cookie, off_t pos, int whence)
{
	if (cookie != (void *)&History_fd) {
		errno = EINVAL;
		return -1;
	}

	return lseek(History_fd, pos, whence);
}

/*
 * a variant of open() for history files.
 */
static int
open_history_file(const char *name, int mode)
{
	int fd;
	struct stat statb;

	fd = open(name, mode, S_IWUSR|S_IRUSR);

	VTRACE(DBG_HISTORY, ("open_history_file(\"%s\", %#x) -> %d\n",
	    name, mode, fd));

	if (fd == -1)
		return -1;

	if (fstat(fd, &statb) == -1) {
		VTRACE(DBG_HISTORY, ("history file fstat(%d) failed [%d]\n",
		    fd, errno));
		close(fd);
		return -1;
	}

	if (statb.st_uid != getuid()) {
		VTRACE(DBG_HISTORY,
		    ("history file wrong user (uid=%d file=%d)\n",
			getuid(), statb.st_uid));
		close(fd);
		return -1;
	}

	return fd;
}

static FILE *
Hist_File_Open(const char *name)
{
	FILE *fd;
	int n;

	INTOFF;

	n = open_history_file(name, O_WRONLY|O_CREAT|O_APPEND|O_CLOEXEC);

	VTRACE(DBG_HISTORY, ("History_File_Open(\"%s\") -> %d", name, n));
	if (n == -1) {
		VTRACE(DBG_HISTORY, (" [%d]\n", errno));
		INTON;
		return NULL;
	}

	n = to_upper_fd(n);
	(void) lseek(n, 0, SEEK_END);
	VTRACE(DBG_HISTORY, (" -> %d", n));

	History_fd = n;
	register_sh_fd(n, History_FD_Renumbered);

	if ((fd =
	    funopen2(&History_fd, NULL, Hist_Write, Hist_Seek, NULL,
		     Hist_Close)) == NULL) {

		VTRACE(DBG_HISTORY, ("; funopen2 failed[%d]\n", errno));

		sh_close(n);
		History_fd = -1;
		INTON;
		return NULL;
	}
	setlinebuf(fd);

	VTRACE(DBG_HISTORY, (" fd:%p\n", fd));

	INTON;

	return fd;
}

void
save_sh_history(void)
{
	char *var;
	const char *file;
	int fd;
	FILE *fp;
	HistEvent he;

	if (HistFP != NULL) {
		/* don't close, just make sure nothing in buffer */
		(void) fflush(HistFP);
		return;
	}

	if (hist == NULL)
		return;

	var = histfileval();
	if ((histfileflags() & VUNSET) || *var == '\0')
		return;

	file = expandvar(var, histfileflags());

	VTRACE(DBG_HISTORY,
	    ("save_sh_history('%s')\n", file == NULL ? "" : file));

	if (file == NULL || *file == '\0')
		return;

	INTOFF;
	fd = open_history_file(file, O_WRONLY|O_CREAT|O_TRUNC);
	if (fd != -1) {
		fp = fdopen(fd, "w");
		if (fp != NULL) {
			(void) history(hist, &he, H_SAVE_FP, fp);
			fclose(fp);
		} else
			close(fd);
	}
	INTON;
}

void
setterm(char *term, int flags __unused)
{
	INTOFF;
	if (el != NULL && term != NULL && *term != '\0')
		if (el_set(el, EL_TERMINAL, term) != 0) {
			outfmt(out2, "sh: Can't set terminal type %s\n", term);
			outfmt(out2, "sh: Using dumb terminal settings.\n");
		}
	INTON;
}

/*
 * The built-in sh commands supported by this file
 */
int
inputrc(int argc, char **argv)
{
	CTRACE(DBG_HISTORY, ("inputrc (%d arg%s)", argc-1, argc==2?"":"s"));
	if (argc != 2) {
		CTRACE(DBG_HISTORY, (" -- bad\n"));
		out2str("usage: inputrc file\n");
		return 1;
	}
	CTRACE(DBG_HISTORY, (" file: \"%s\"\n", argv[1]));
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
	volatile int lflg = 0, nflg = 0, rflg = 0, sflg = 0, zflg = 0;
	int i, retval;
	const char *firststr, *laststr;
	int first, last, direction;

	char * volatile pat = NULL;	/* ksh "fc old=new" crap */
	char * volatile repl;

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

	CTRACE(DBG_HISTORY, ("histcmd (fc) %d arg%s\n", argc, argc==1?"":"s"));
	if (argc == 1)
		error("missing history argument");

	optreset = 1; optind = 1; /* initialize getopt */
	while (not_fcnumber(argv[optind]) &&
	      (ch = getopt(argc, argv, ":e:lnrsz")) != -1)
		switch ((char)ch) {
		case 'e':
			editor = optarg;
			VTRACE(DBG_HISTORY, ("histcmd -e %s\n", editor));
			break;
		case 'l':
			lflg = 1;
			VTRACE(DBG_HISTORY, ("histcmd -l\n"));
			break;
		case 'n':
			nflg = 1;
			VTRACE(DBG_HISTORY, ("histcmd -n\n"));
			break;
		case 'r':
			rflg = 1;
			VTRACE(DBG_HISTORY, ("histcmd -r\n"));
			break;
		case 's':
			sflg = 1;
			VTRACE(DBG_HISTORY, ("histcmd -s\n"));
			break;
		case 'z':
			zflg = 1;
			VTRACE(DBG_HISTORY, ("histcmd -z\n"));
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

	if (zflg) {
		if (argc != 0 || (lflg|nflg|rflg|sflg) != 0)
			error("Usage: fc -z");

		history(hist, &he, H_CLEAR);
		return 0;
	}


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
			if (*editfile) {
				VTRACE(DBG_HISTORY,
				    ("histcmd err jump unlink temp \"%s\"\n",
				    editfile));
				unlink(editfile);
			}
			handler = savehandler;
			longjmp(handler->loc, 1);
		}
		handler = &jmploc;
		VTRACE(DBG_HISTORY, ("histcmd is active %d(++)\n", active));
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
			VTRACE(DBG_HISTORY, ("histcmd using %s as editor\n",
			    editor == NULL ? "-nothing-" : editor));
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
		VTRACE(DBG_HISTORY, ("histcmd replace old=\"%s\" new=\"%s\""
		    " (%d args)\n", pat, repl, argc));
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
		if (lflg) {
			firststr = "-16";
			laststr = "-1";
		} else
			firststr = laststr = "-1";	/* the exact same str */
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

	if (first == -1 || last == -1) {
		if (lflg)   /* no history exists, that's OK */
			return 0;
		if (first == -1 && last == -1) {
			if (firststr != laststr)
				error("history events %s to %s do not exist",
				    firststr, laststr);
			else
				error("history event %s does not exist",
				    firststr);
		} else {
			error("history event %s does not exist",
			    first == -1 ? firststr : laststr);
		}
	}

	if (rflg) {
		i = last;
		last = first;
		first = i;
	}
	VTRACE(DBG_HISTORY, ("histcmd%s first=\"%s\" (#%d) last=\"%s\" (#%d)\n",
	    rflg ? " reversed" : "", rflg ? laststr : firststr, first,
	    rflg ? firststr : laststr, last));

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
		snprintf(editfile, sizeof(editfile),
		    "%s_shXXXXXX", _PATH_TMP);
		if ((fd = mkstemp(editfile)) < 0)
			error("can't create temporary file %s", editfile);
		if ((efp = fdopen(fd, "w")) == NULL) {
			close(fd);
			error("can't allocate stdio buffer for temp");
		}
		VTRACE(DBG_HISTORY, ("histcmd created \"%s\" for edit buffer"
		    " fd=%d\n", editfile, fd));
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
	for ( ; retval != -1; retval = history(hist, &he, direction)) {
		if (lflg) {
			if (!nflg)
				out1fmt("%5d ", he.num);
			out1str(he.str);
		} else {
			const char *s = pat ?
			   fc_replace(he.str, pat, repl) : he.str;

			if (sflg) {
				VTRACE(DBG_HISTORY, ("histcmd -s \"%s\"\n", s));
				if (displayhist) {
					out2str(s);
				}

				evalstring(s, 0);

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
		VTRACE(DBG_HISTORY, ("histcmd editing: \"%s\"\n", editcmd));
		evalstring(editcmd, 0);	/* XXX - should use no JC command */
		stunalloc(editcmd);
		VTRACE(DBG_HISTORY, ("histcmd read cmds from %s\n", editfile));
		readcmdfile(editfile);	/* XXX - should read back - quick tst */
		VTRACE(DBG_HISTORY, ("histcmd unlink %s\n", editfile));
		unlink(editfile);
		editfile[0] = '\0';
		INTON;
	}

	if (lflg == 0 && active > 0)
		--active;
	if (displayhist)
		displayhist = 0;
	return 0;
}

/*
 * and finally worker functions for those built-ins
 */

static const char *
fc_replace(const char *s, char *p, char *r)
{
	char *dest;
	int plen = strlen(p);

	VTRACE(DBG_HISTORY, ("histcmd s/%s/%s/ in \"%s\" -> ", p, r, s));
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
	STPUTC('\0', dest);
	dest = grabstackstr(dest);
	VTRACE(DBG_HISTORY, ("\"%s\"\n", dest));

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
static char **
sh_matches(const char *text, int start, int end)
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
			if (entry->d_type == DT_UNKNOWN ||
			    entry->d_type == DT_LNK) {
				if (fstatat(dfd, entry->d_name, &statb, 0)
				    == -1)
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
 out:;
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
			return -1;
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

#else	/* defined(SMALL) */

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

#endif	/* SMALL */
