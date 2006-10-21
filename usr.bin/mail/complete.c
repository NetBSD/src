/*	$NetBSD: complete.c,v 1.9 2006/10/21 21:37:20 christos Exp $	*/

/*-
 * Copyright (c) 1997-2000,2005,2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
 *
 * Additions by Anon Ymous. (2006)
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Most of this is derived or copied from src/usr.bin/ftp/complete.c (1.41). */


#ifdef USE_EDITLINE
#undef NO_EDITCOMPLETE

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: complete.c,v 1.9 2006/10/21 21:37:20 christos Exp $");
#endif /* not lint */

/*
 * FTP user program - command and file completion routines
 */

#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <dirent.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <stringlist.h>
#include <util.h>

#include "rcv.h"			/* includes "glob.h" */
#include "extern.h"
#include "complete.h"
#ifdef MIME_SUPPORT
#include "mime.h"
#endif

/*
 * Global variables
 */
static int doglob = 1;			/* glob local file names */

#define ttyout stdout
#define ttywidth screenwidth		/* in "glob.h" */
#define ttyheight screenheight		/* in "glob.h" */


/* This should find the first command that matches the name given or
 * NULL if none.  Use the routine from mail in lex.c.
 */
#define getcmd(w) lex(w)

/************************************************************************/
/* from src/usr.bin/ftp/utils.h (1.135) - begin */

/*
 * List words in stringlist, vertically arranged
 */
static void
list_vertical(StringList *sl)
{
	int i, j;
	int columns, lines;
	char *p;
	size_t w, width;

	width = 0;

	for (i = 0; i < sl->sl_cur; i++) {
		w = strlen(sl->sl_str[i]);
		if (w > width)
			width = w;
	}
	width = (width + 8) &~ 7;

	columns = ttywidth / width;
	if (columns == 0)
		columns = 1;
	lines = (sl->sl_cur + columns - 1) / columns;
	for (i = 0; i < lines; i++) {
		for (j = 0; j < columns; j++) {
			p = sl->sl_str[j * lines + i];
			if (p)
				(void)fputs(p, ttyout);
			if (j * lines + i + lines >= sl->sl_cur) {
				(void)putc('\n', ttyout);
				break;
			}
			if (p) {
				w = strlen(p);
				while (w < width) {
					w = (w + 8) &~ 7;
					(void)putc('\t', ttyout);
				}
			}
		}
	}
}

/*
 * Copy characters from src into dst, \ quoting characters that require it
 */
static void
ftpvis(char *dst, size_t dstlen, const char *src, size_t srclen)
{
	int	di, si;

	for (di = si = 0;
	    src[si] != '\0' && di < dstlen && si < srclen;
	    di++, si++) {
		switch (src[si]) {
		case '\\':
		case ' ':
		case '\t':
		case '\r':
		case '\n':
		case '"':
			dst[di++] = '\\';
			if (di >= dstlen)
				break;
			/* FALLTHROUGH */
		default:
			dst[di] = src[si];
		}
	}
	dst[di] = '\0';
}

/*
 * sl_init() with inbuilt error checking
 */
static StringList *
mail_sl_init(void)
{
	StringList *p;

	p = sl_init();
	if (p == NULL)
		err(1, "Unable to allocate memory for stringlist");
	return (p);
}


/*
 * sl_add() with inbuilt error checking
 */
static void
mail_sl_add(StringList *sl, char *i)
{

	if (sl_add(sl, i) == -1)
		err(1, "Unable to add `%s' to stringlist", i);
}


/*
 * Glob a local file name specification with the expectation of a single
 * return value. Can't control multiple values being expanded from the
 * expression, we return only the first.
 * Returns NULL on error, or a pointer to a buffer containing the filename
 * that's the caller's responsiblity to free(3) when finished with.
 */
static char *
globulize(const char *pattern)
{
	glob_t gl;
	int flags;
	char *p;

	if (!doglob)
		return estrdup(pattern);

	flags = GLOB_BRACE|GLOB_NOCHECK|GLOB_TILDE;
	(void)memset(&gl, 0, sizeof(gl));
	if (glob(pattern, flags, NULL, &gl) || gl.gl_pathc == 0) {
		warnx("%s: not found", pattern);
		globfree(&gl);
		return (NULL);
	}
	p = estrdup(gl.gl_pathv[0]);
	globfree(&gl);
	return (p);
}

/* from src/usr.bin/ftp/utils.h (1.135) - end */
/************************************************************************/

static int
comparstr(const void *a, const void *b)
{
	return (strcmp(*(const char * const *)a, *(const char * const *)b));
}

/*
 * Determine if complete is ambiguous. If unique, insert.
 * If no choices, error. If unambiguous prefix, insert that.
 * Otherwise, list choices. words is assumed to be filtered
 * to only contain possible choices.
 * Args:
 *	word	word which started the match
 *	dolist	list by default
 *	words	stringlist containing possible matches
 * Returns a result as per el_set(EL_ADDFN, ...)
 */
static unsigned char
complete_ambiguous(EditLine *el, char *word, int dolist, StringList *words)
{
	char insertstr[MAXPATHLEN];
	char *lastmatch, *p;
	int i, j;
	size_t matchlen, wordlen;

	wordlen = strlen(word);
	if (words->sl_cur == 0)
		return (CC_ERROR);	/* no choices available */

	if (words->sl_cur == 1) {	/* only once choice available */
		p = words->sl_str[0] + wordlen;
		if (*p == '\0')		/* at end of word? */
			return (CC_REFRESH);
		ftpvis(insertstr, sizeof(insertstr), p, strlen(p));
		if (el_insertstr(el, insertstr) == -1)
			return (CC_ERROR);
		else
			return (CC_REFRESH);
	}

	if (!dolist) {
		matchlen = 0;
		lastmatch = words->sl_str[0];
		matchlen = strlen(lastmatch);
		for (i = 1; i < words->sl_cur; i++) {
			for (j = wordlen; j < strlen(words->sl_str[i]); j++)
				if (lastmatch[j] != words->sl_str[i][j])
					break;
			if (j < matchlen)
				matchlen = j;
		}
		if (matchlen > wordlen) {
			ftpvis(insertstr, sizeof(insertstr),
			    lastmatch + wordlen, matchlen - wordlen);
			if (el_insertstr(el, insertstr) == -1)
				return (CC_ERROR);
			else
				return (CC_REFRESH_BEEP);
		}
	}

	(void)putc('\n', ttyout);
	qsort(words->sl_str, words->sl_cur, sizeof(char *), comparstr);
	list_vertical(words);
	return (CC_REDISPLAY);
}

/*
 * Complete a mail command.
 */
static unsigned char
complete_command(EditLine *el, char *word, int dolist)
{
	const struct cmd *c;
	StringList *words;
	size_t wordlen;
	unsigned char rv;

	words = mail_sl_init();
	wordlen = strlen(word);

	for (c = cmdtab; c->c_name != NULL; c++) {
		if (wordlen > strlen(c->c_name))
			continue;
		if (strncmp(word, c->c_name, wordlen) == 0)
			mail_sl_add(words, __UNCONST(c->c_name));
	}

	rv = complete_ambiguous(el, word, dolist, words);
	if (rv == CC_REFRESH) {
		if (el_insertstr(el, " ") == -1)
			rv = CC_ERROR;
	}
	sl_free(words, 0);
	return (rv);
}

/*
 * Complete a local filename.
 */
static unsigned char
complete_filename(EditLine *el, char *word, int dolist)
{
	StringList *words;
	char dir[MAXPATHLEN];
	char *fname;
	DIR *dd;
	struct dirent *dp;
	unsigned char rv;
	size_t len;

	if ((fname = strrchr(word, '/')) == NULL) {
		dir[0] = '.';
		dir[1] = '\0';
		fname = word;
	} else {
		if (fname == word) {
			dir[0] = '/';
			dir[1] = '\0';
		} else {
			len = fname - word + 1;
			(void)estrlcpy(dir, word, sizeof(dir));
			dir[len] = '\0';
		}
		fname++;
	}
	if (dir[0] == '~') {
		char *p;

		if ((p = globulize(dir)) == NULL)
			return (CC_ERROR);
		(void)estrlcpy(dir, p, sizeof(dir));
		free(p);
	}

	if ((dd = opendir(dir)) == NULL)
		return (CC_ERROR);

	words = mail_sl_init();
	len = strlen(fname);

	for (dp = readdir(dd); dp != NULL; dp = readdir(dd)) {
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

#if defined(DIRENT_MISSING_D_NAMLEN)
		if (len > strlen(dp->d_name))
			continue;
#else
		if (len > dp->d_namlen)
			continue;
#endif
		if (strncmp(fname, dp->d_name, len) == 0) {
			char *tcp;

			tcp = estrdup(dp->d_name);
			mail_sl_add(words, tcp);
		}
	}
	(void)closedir(dd);

	rv = complete_ambiguous(el, fname, dolist, words);
	if (rv == CC_REFRESH) {
		struct stat sb;
		char path[MAXPATHLEN];

		(void)estrlcpy(path, dir,		sizeof(path));
		(void)estrlcat(path, "/",		sizeof(path));
		(void)estrlcat(path, words->sl_str[0],	sizeof(path));

		if (stat(path, &sb) >= 0) {
			char suffix[2] = " ";

			if (S_ISDIR(sb.st_mode))
				suffix[0] = '/';
			if (el_insertstr(el, suffix) == -1)
				rv = CC_ERROR;
		}
	}
	sl_free(words, 1);
	return (rv);
}

static int
find_execs(char *word, char *path, StringList *list)
{
	char *sep;
	char *dir=path;
	DIR *dd;
	struct dirent *dp;
	size_t len = strlen(word);
	uid_t uid = getuid();
	gid_t gid = getgid();

	for (sep = dir; sep; dir = sep + 1) {
		if ((sep=strchr(dir, ':')) != NULL) {
			*sep=0;
		}
		
		if ((dd = opendir(dir)) == NULL) {
			perror("dir");
			return -1;
		}
		
		for (dp = readdir(dd); dp != NULL; dp = readdir(dd)) {

			if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
				continue;
			
#if defined(DIRENT_MISSING_D_NAMLEN)
			if (len > strlen(dp->d_name))
				continue;
#else
			if (len > dp->d_namlen)
				continue;
#endif

			if (strncmp(word, dp->d_name, len) == 0) {
				struct stat sb;
				char pathname[ MAXPATHLEN ];
				unsigned mask;

				(void)snprintf(pathname, sizeof(pathname),
				    "%s/%s", dir, dp->d_name);
				if (stat(pathname, &sb) != 0) {
					perror(pathname);
					continue;
				}

				mask = 0001;
				if (sb.st_uid == uid)	mask |= 0100;
				if (sb.st_gid == gid)	mask |= 0010;

				if ((sb.st_mode & mask) != 0) {
					char *tcp;
					tcp = estrdup(dp->d_name);
					mail_sl_add(list, tcp);
				}
			}
		
		}

		(void)closedir(dd);
	}

	return 0;
}


/*
 * Complete a local executable
 */
static unsigned char
complete_executable(EditLine *el, char *word, int dolist)
{
	StringList *words;
	char dir[ MAXPATHLEN ];
	char *fname;
	unsigned char rv;
	size_t len;
	int error;

	if ((fname = strrchr(word, '/')) == NULL) {
		dir[0] = '\0';		/* walk the path */
		fname = word;
	} else {
		if (fname == word) {
			dir[0] = '/';
			dir[1] = '\0';
		} else {
			len = fname - word;
			(void)strncpy(dir, word, len);
			dir[fname - word] = '\0';
		}
		fname++;
	}

	words = sl_init();

	if (*dir == '\0') {		/* walk path */
		char *env;
		char *path;
		env = getenv("PATH");
		len = strlen(env);
		path = salloc(len + 1);
		(void)strcpy(path, env);
		error = find_execs(word, path, words);
	}
	else {		/* check specified dir only */
		error = find_execs(word, dir, words);
	}
	if (error != 0)
		return CC_ERROR;

	rv = complete_ambiguous(el, fname, dolist, words);

	if (rv == CC_REFRESH)
		if (el_insertstr(el, " ") == -1)
			rv = CC_ERROR;

	sl_free(words, 1);

	return (rv);
}


static unsigned
char complete_set(EditLine *el, char *word, int dolist)
{
	struct var *vp;
	char **ap;
	char **p;
	int h;
	int s;
	size_t len = strlen(word);
	StringList *words;
	unsigned char rv;

	words = sl_init();

	/* allocate space for variables table */
	for (h = 0, s = 1; h < HSHSIZE; h++)
		for (vp = variables[h]; vp != NULL; vp = vp->v_link)
			s++;
	ap = (char **) salloc(s * sizeof *ap);

	/* save pointers */
	for (h = 0, p = ap; h < HSHSIZE; h++)
		for (vp = variables[h]; vp != NULL; vp = vp->v_link)
			*p++ = vp->v_name;
	*p = NULL;

	/* sort pointers */
	sort(ap);

	/* complete on list */
	for (p = ap; *p != NULL; p++) {
		if (len == 0 || strncmp(*p, word, len) == 0) {
			char *tcp;
			tcp = estrdup(*p);
			mail_sl_add(words, tcp);
		}
	}

	rv = complete_ambiguous(el, word, dolist, words);

	sl_free(words, 1);

	return(rv);
}





static unsigned char
complete_alias(EditLine *el, char *word, int dolist)
{
	struct grouphead *gh;
	char **ap;
	char **p;
	int h;
	int s;
	size_t len = strlen(word);
	StringList *words;
	unsigned char rv;

	words = sl_init();

	/* allocate space for alias table */
	for (h = 0, s = 1; h < HSHSIZE; h++)
		for (gh = groups[h]; gh != NULL; gh = gh->g_link)
			s++;
	ap = (char **) salloc(s * sizeof *ap);

	/* save pointers */
	for (h = 0, p = ap; h < HSHSIZE; h++)
		for (gh = groups[h]; gh != NULL; gh = gh->g_link)
			*p++ = gh->g_name;
	*p = NULL;

	/* sort pointers */
	sort(ap);

	/* complete on list */
	for (p = ap; *p != NULL; p++) {
		if (len == 0 || strncmp(*p, word, len) == 0) {
			char *tcp;
			tcp = estrdup(*p);
			mail_sl_add(words, tcp);
		}
	}

	rv = complete_ambiguous(el, word, dolist, words);
	if (rv == CC_REFRESH)
		if (el_insertstr(el, " ") == -1)
			rv = CC_ERROR;

	sl_free(words, 1);

	return(rv);
}



static char	*stringbase;	/* current scan point in line buffer */
static char	*argbase;	/* current storage point in arg buffer */
static StringList *marg_sl;	/* stringlist containing margv */
static int	margc;		/* count of arguments on input line */
#define margv (marg_sl->sl_str)	/* args parsed from input line */

static char *cursor_pos;
static int   cursor_argc;
static int   cursor_argo;

static void
init_complete(void)
{
	marg_sl = sl_init();
	return;
}



/************************************************************************/
/* from /usr/src/usr.bin/ftp/main.c(1.101) - begin */

static int   slrflag;
#ifdef NEED_ALTARG
static char *altarg;		/* argv[1] with no shell-like preprocessing  */
#endif

#ifdef NO_EDITCOMPLETE
#define	INC_CHKCURSOR(x)	(x)++
#else  /* !NO_EDITCOMPLETE */
#define	INC_CHKCURSOR(x)				\
	do {						\
		(x)++;					\
		if (x == cursor_pos) {			\
			cursor_argc = margc;		\
			cursor_argo = ap - argbase;	\
			cursor_pos  = NULL;		\
		}					\
	} while(/* CONSTCOND */ 0)
#endif /* !NO_EDITCOMPLETE */

/*
 * Parse string into argbuf;
 * implemented with FSM to
 * handle quoting and strings
 */
static char *
slurpstring(void)
{
	int got_one = 0;
	char *sb = stringbase;
	char *ap = argbase;
	char *tmp = argbase;		/* will return this if token found */

	if (*sb == '!' || *sb == '$') {	/* recognize ! as a token for shell */
		switch (slrflag) {	/* and $ as token for macro invoke */
			case 0:
				slrflag++;
				INC_CHKCURSOR(stringbase);
				return __UNCONST((*sb == '!') ? "!" : "$");
				/* NOTREACHED */
			case 1:
				slrflag++;
#ifdef NEED_ALTARG
				altarg = stringbase;
#endif
				break;
			default:
				break;
		}
	}

S0:
	switch (*sb) {

	case '\0':
		goto OUT;

	case ' ':
	case '\t':
		INC_CHKCURSOR(sb);
		goto S0;

	default:
		switch (slrflag) {
			case 0:
				slrflag++;
				break;
			case 1:
				slrflag++;
#ifdef NEED_ALTARG
				altarg = sb;
#endif
				break;
			default:
				break;
		}
		goto S1;
	}

S1:
	switch (*sb) {

	case ' ':
	case '\t':
	case '\0':
		goto OUT;	/* end of token */

	case '\\':
		INC_CHKCURSOR(sb);
		goto S2;	/* slurp next character */

	case '"':
		INC_CHKCURSOR(sb);
		goto S3;	/* slurp quoted string */

	default:
		/* the first arg (command) is special - see execute() in lex.c */
		if (margc == 0 && got_one &&
		    strchr(" \t0123456789$^.:/-+*'\"", *sb))
			goto OUT;

		*ap = *sb;	/* add character to token */
		ap++;
		INC_CHKCURSOR(sb);
		got_one = 1;
		goto S1;
	}

S2:
	switch (*sb) {

	case '\0':
		goto OUT;

	default:
		*ap = *sb;
		ap++;
		INC_CHKCURSOR(sb);
		got_one = 1;
		goto S1;
	}

S3:
	switch (*sb) {

	case '\0':
		goto OUT;

	case '"':
		INC_CHKCURSOR(sb);
		goto S1;

	default:
		*ap = *sb;
		ap++;
		INC_CHKCURSOR(sb);
		got_one = 1;
		goto S3;
	}

OUT:
	if (got_one)
		*ap++ = '\0';
	argbase = ap;			/* update storage pointer */
	stringbase = sb;		/* update scan pointer */
	if (got_one) {
		return (tmp);
	}
	switch (slrflag) {
		case 0:
			slrflag++;
			break;
		case 1:
			slrflag++;
#ifdef NEED_ALTARG
			altarg = NULL;
#endif
			break;
		default:
			break;
	}
	return (NULL);
}


/*
 * Slice a string up into argc/argv.
 */
static void
makeargv(char *line)
{
	static char argbuf[ LINESIZE ];	/* argument storage buffer */
	char *argp;

	stringbase = line;		/* scan from first of buffer */
	argbase = argbuf;		/* store from first of buffer */
	slrflag = 0;
	marg_sl->sl_cur = 0;		/* reset to start of marg_sl */
	for (margc = 0; /* EMPTY */; margc++) {
		argp = slurpstring();
		mail_sl_add(marg_sl, argp);
		if (argp == NULL)
			break;
	}
#ifndef NO_EDITCOMPLETE
	if (cursor_pos == line) {
		cursor_argc = 0;
		cursor_argo = 0;
	} else if (cursor_pos != NULL) {
		cursor_argc = margc;
		cursor_argo = strlen(margv[margc-1]);
	}
#endif /* !NO_EDITCOMPLETE */
}

/* from /usr/src/usr.bin/ftp/main.c(1.101) - end */
/************************************************************************/

/* Some people like to bind file completion to CTRL-D.  In emacs mode,
 * CTRL-D is also used to delete the current character, we have to
 * special case this situation.
 */
#define EMACS_CTRL_D_BINDING_HACK

#ifdef EMACS_CTRL_D_BINDING_HACK
static int
is_emacs_mode(EditLine *el)
{
	char *mode;
	if (el_get(el, EL_EDITOR, &mode) == -1)
		return 0;
	return equal(mode, "emacs");
}

static int
emacs_ctrl_d(EditLine *el, const LineInfo *lf, int ch, size_t len)
{
	static char delunder[3] = { CTRL('f'), CTRL('h'), '\0' };
	if (ch == CTRL('d') && is_emacs_mode(el)) {	/* CTRL-D is special */
		if (len == 0) 
			return (CC_EOF);
		if (lf->lastchar != lf->cursor) { /* delete without using ^D */
			el_push(el, delunder); /* ^F^H */
			return (CC_NORM);
		}
	}
	return -1;
}
#endif /* EMACS_CTRL_D_BINDING_HACK */

/*
 * Generic complete routine
 */
static unsigned char
mail_complete(EditLine *el, int ch)
{
	static char line[LINESIZE];	/* input line buffer */
	static char word[LINESIZE];
	static int lastc_argc, lastc_argo;

	const struct cmd *c;
	const LineInfo *lf;
	int celems, dolist, cmpltype;
	size_t len;

	lf = el_line(el);
	len = lf->lastchar - lf->buffer;

#ifdef EMACS_CTRL_D_BINDING_HACK
	{
		int cc_ret;
		if ((cc_ret = emacs_ctrl_d(el, lf, ch, len)) != -1)
			return cc_ret;
	}
#endif /* EMACS_CTRL_D_BINDING_HACK */

	if (len >= sizeof(line) - 1)
		return (CC_ERROR);

	(void)strlcpy(line, lf->buffer, len + 1); /* do not use estrlcpy here! */
	cursor_pos = line + (lf->cursor - lf->buffer);
	lastc_argc = cursor_argc;	/* remember last cursor pos */
	lastc_argo = cursor_argo;
	makeargv(line);			/* build argc/argv of current line */

	if (cursor_argo >= sizeof(word) - 1)
		return (CC_ERROR);

	dolist = 0;
	/* if cursor and word are the same, list alternatives */
	if (lastc_argc == cursor_argc && lastc_argo == cursor_argo
	    && strncmp(word, margv[cursor_argc] ? margv[cursor_argc] : "",
		(size_t)cursor_argo) == 0)
		dolist = 1;
	else if (cursor_argc < margc)
		(void)strlcpy(word, margv[cursor_argc], (size_t)cursor_argo + 1); /* do not use estrlcpy() here */
	word[cursor_argo] = '\0';

	if (cursor_argc == 0)
		return (complete_command(el, word, dolist));

	c = getcmd(margv[0]);
	if (c == NULL)
		return (CC_ERROR);
	celems = strlen(c->c_complete);

		/* check for 'continuation' completes (which are uppercase) */
	if ((cursor_argc > celems) && (celems > 0)
	    && isupper((unsigned char) c->c_complete[celems-1]))
		cursor_argc = celems;

	if (cursor_argc > celems)
		return (CC_ERROR);

	cmpltype = c->c_complete[cursor_argc - 1];
	switch (cmpltype) {
		case 'a':			/* alias complete */
		case 'A':
			return (complete_alias(el, word, dolist));

		case 'c':			/* command complete */
		case 'C':
			return (complete_command(el, word, dolist));

		case 'f':			/* filename complete */
		case 'F':
			return (complete_filename(el, word, dolist));

		case 'n':			/* no complete */
		case 'N':			/* no complete */
			return (CC_ERROR);

		case 's':
		case 'S':
			return (complete_set(el, word, dolist));

		case 'x':			/* executable complete */
		case 'X':
			return (complete_executable(el, word, dolist));

		default:
			errx(1, "unknown complete type `%c'", cmpltype);
			return (CC_ERROR);
	}
	/* NOTREACHED */
}


/*
 * Generic file completion routine
 */
static unsigned char
file_complete(EditLine *el, int ch)
{
	static char word[LINESIZE];

	const LineInfo *lf;
	size_t len, word_len;

	lf = el_line(el);
	len = lf->lastchar - lf->buffer;

#ifdef EMACS_CTRL_D_BINDING_HACK
	{
		int cc_ret;
		if ((cc_ret = emacs_ctrl_d(el, lf, ch, len)) != -1)
			return cc_ret;
	}
#endif /* EMACS_CTRL_D_BINDING_HACK */

	if (len >= sizeof(word) - 1)
		return (CC_ERROR);

	word_len = lf->cursor - lf->buffer;
	(void)strlcpy(word, lf->buffer, word_len + 1);	/* do not use estrlcpy here! */
	return complete_filename(el, word, len != word_len);
}


#ifdef MIME_SUPPORT
/*
 * Complete mime_transfer_encoding type
 */
static unsigned char
mime_enc_complete(EditLine *el, int ch)
{
	static char word[LINESIZE];
	StringList *words;
	unsigned char rv;
	const LineInfo *lf;
	size_t len, word_len;

	lf = el_line(el);
	len = lf->lastchar - lf->buffer;

#ifdef EMACS_CTRL_D_BINDING_HACK
	{
		int cc_ret;
		if ((cc_ret = emacs_ctrl_d(el, lf, ch, len)) != -1)
			return cc_ret;
	}
#endif /* EMACS_CTRL_D_BINDING_HACK */

	if (len >= sizeof(word) - 1)
		return (CC_ERROR);

	words = mail_sl_init();
	word_len = lf->cursor - lf->buffer;
	{
		const char *ename;
		const void *cookie;
		cookie = NULL;
		for (ename = mime_next_encoding_name(&cookie);
		     ename;
		     ename = mime_next_encoding_name(&cookie))
			if (word_len == 0 ||
			    strncmp(lf->buffer, ename, word_len) == 0) {
				char *cp;
				cp = estrdup(ename);
				mail_sl_add(words, cp);
			}
	}
	(void)strlcpy(word, lf->buffer, word_len + 1);	/* do not use estrlcpy here */

	rv = complete_ambiguous(el, word, len != word_len, words);

	sl_free(words, 1);
	return (rv);
}
#endif /* MIME_SUPPORT */

/*************************************************************************
 * Our public interface to el_gets():
 *
 * init_editline()
 *    Initializes of all editline and completion data strutures.
 *
 * my_gets()
 *    Returns the next line of input as a NULL termnated string without
 *    the trailing newline.
 */

static const char *el_prompt;

/*ARGSUSED*/ 
static const char *
show_prompt(EditLine *e __unused)
{
	return el_prompt;
}

char *
my_gets(el_mode_t *em, const char *prompt, char *string)
{
	int cnt;
	size_t len;
	const char *buf;
	HistEvent ev;
	static char line[LINE_MAX];

	el_prompt = prompt;

	if (string)
		el_push(em->el, string);

	buf = el_gets(em->el, &cnt);

	if (buf == NULL || cnt <= 0) {
		if (cnt == 0)
			(void)putc('\n', stdout);
		return NULL;
	}

	cnt--;	/* trash the trailing LF */
	len = MIN(sizeof(line) - 1, cnt);
	(void)memcpy(line, buf, len);
	line[cnt] = '\0';

	/* enter non-empty lines into history */
	if (em->hist) {
		const char *p;
		p = skip_white(line);
		if (*p && history(em->hist, &ev, H_ENTER, line) == 0)
			(void)printf("Failed history entry: %s", line);
	}
	return line;
}

static el_mode_t
init_el_mode(
	const char *el_editor,
	unsigned char (*completer)(EditLine *, int),
	struct name *keys,
	int history_size)
{
	el_mode_t em;
	(void)memset(&em, 0, sizeof(em));

	em.el  = el_init(getprogname(), stdin, stdout, stderr);

	(void)el_set(em.el, EL_PROMPT, show_prompt);
	(void)el_set(em.el, EL_SIGNAL, SIGHUP);
	
	if (el_editor)
		(void)el_set(em.el, EL_EDITOR, el_editor);

	if (completer) {
		struct name *np;
		(void)el_set(em.el, EL_ADDFN, "mail-complete",
		    "Context sensitive argument completion", completer);
		for (np = keys; np; np = np->n_flink)
			(void)el_set(em.el, EL_BIND, np->n_name,
			    "mail-complete", NULL);
	}

	if (history_size) {
		HistEvent ev;
		em.hist = history_init();
		if (history(em.hist, &ev, H_SETSIZE, history_size))
			(void)printf("history: %s\n", ev.str);
		(void)el_set(em.el, EL_HIST, history, em.hist);
	}

	(void)el_source(em.el, NULL);		/* read ~/.editrc */

	return em;
}


struct el_modes_s elm = {
	.command  = { .el = NULL, .hist = NULL, },
	.string   = { .el = NULL, .hist = NULL, },
	.filec    = { .el = NULL, .hist = NULL, },
#ifdef MIME_SUPPORT
	.mime_enc = { .el = NULL, .hist = NULL, },
#endif
};

void
init_editline(void)
{
	const char *mode;
	int hist_size;
	struct name *keys;
	char *cp;

	mode = value(ENAME_EL_EDITOR);
	
	cp = value(ENAME_EL_HISTORY_SIZE);
	hist_size = cp ? atoi(cp) : 0;

	cp = value(ENAME_EL_COMPLETION_KEYS);
	keys = cp && *cp ? lexpand(cp, 0) : NULL;

	elm.command  = init_el_mode(mode, mail_complete,     keys, hist_size);
	elm.filec    = init_el_mode(mode, file_complete,     keys, 0);
	elm.string   = init_el_mode(mode, NULL,              NULL, 0);
#ifdef MIME_SUPPORT
	elm.mime_enc = init_el_mode(mode, mime_enc_complete, keys, 0);
#endif
	init_complete();

	return;
}

#endif /* USE_EDITLINE */
