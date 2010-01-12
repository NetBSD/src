/*	$NetBSD: complete.c,v 1.20 2010/01/12 14:44:24 christos Exp $	*/

/*-
 * Copyright (c) 1997-2000,2005,2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * Most of this is derived or copied from src/usr.bin/ftp/complete.c (1.41).
 */

#ifdef USE_EDITLINE

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: complete.c,v 1.20 2010/01/12 14:44:24 christos Exp $");
#endif /* not lint */

/*
 * FTP user program - command and file completion routines
 */

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <dirent.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stringlist.h>
#include <termcap.h>
#include <util.h>

#include <sys/param.h>
#include <sys/stat.h>

#include "rcv.h"			/* includes "glob.h" */
#include "extern.h"
#include "complete.h"
#ifdef MIME_SUPPORT
#include "mime.h"
#endif
#include "sig.h"
#ifdef THREAD_SUPPORT
#include "thread.h"
#endif

#define BELL 0x7

/*
 * Global variables
 */
static int doglob = 1;			/* glob local file names */

#define ttyout stdout
#define ttywidth screenwidth		/* in "glob.h" */
#define ttyheight screenheight		/* in "glob.h" */

/************************************************************************/
/* from src/usr.bin/ftp/utils.h (1.135) - begin */

/*
 * List words in stringlist, vertically arranged
 */
static void
list_vertical(StringList *sl)
{
	int k;
	size_t i, j, columns, lines;
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
	k = 0;
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
		if (ttyheight > 2 && ++k == ttyheight - 2) {
			int ch;
			k = 0;
			(void)fputs("--more--", ttyout);
			while ((ch = getchar()) != EOF && ch != ' ' && ch != 'q')
				(void)putc(BELL, ttyout);
			(void)fputs("\r        \r", ttyout);
			if (ch == 'q')
				break;
		}
	}
}

/*
 * Copy characters from src into dst, \ quoting characters that require it
 */
static void
ftpvis(char *dst, size_t dstlen, const char *src, size_t srclen)
{
	size_t	di, si;

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
		err(EXIT_FAILURE, "Unable to allocate memory for stringlist");
	return p;
}


/*
 * sl_add() with inbuilt error checking
 */
static void
mail_sl_add(StringList *sl, char *i)
{

	if (sl_add(sl, i) == -1)
		err(EXIT_FAILURE, "Unable to add `%s' to stringlist", i);
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
		return NULL;
	}
	p = estrdup(gl.gl_pathv[0]);
	globfree(&gl);
	return p;
}

/* from src/usr.bin/ftp/utils.h (1.135) - end */
/************************************************************************/

static int
comparstr(const void *a, const void *b)
{
	return strcmp(*(const char * const *)a, *(const char * const *)b);
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
	size_t i, j, matchlen, wordlen;

	wordlen = strlen(word);
	if (words->sl_cur == 0)
		return CC_ERROR;	/* no choices available */

	if (words->sl_cur == 1) {	/* only once choice available */
		p = words->sl_str[0] + wordlen;
		if (*p == '\0')		/* at end of word? */
			return CC_REFRESH;
		ftpvis(insertstr, sizeof(insertstr), p, strlen(p));
		if (el_insertstr(el, insertstr) == -1)
			return CC_ERROR;
		else
			return CC_REFRESH;
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
		if (matchlen >= wordlen) {
			ftpvis(insertstr, sizeof(insertstr),
			    lastmatch + wordlen, matchlen - wordlen);
			if (el_insertstr(el, insertstr) == -1)
				return CC_ERROR;
			else
				return CC_REFRESH_BEEP;
		}
	}

	(void)putc('\n', ttyout);
	qsort(words->sl_str, words->sl_cur, sizeof(char *), comparstr);

	list_vertical(words);
	return CC_REDISPLAY;
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
	return rv;
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
			return CC_ERROR;
		(void)estrlcpy(dir, p, sizeof(dir));
		free(p);
	}

	if ((dd = opendir(dir)) == NULL)
		return CC_ERROR;

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
	return rv;
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
	if (rv == CC_REFRESH) {
		if (el_insertstr(el, " ") == -1)
			rv = CC_ERROR;
	}
	sl_free(words, 1);

	return rv;
}


static unsigned char
complete_set(EditLine *el, char *word, int dolist)
{
	struct var *vp;
	const char **ap;
	const char **p;
	int h;
	int s;
	size_t len = strlen(word);
	StringList *words;
	unsigned char rv;

	words = sl_init();

	/* allocate space for variables table */
	s = 1;
	for (h = 0; h < HSHSIZE; h++)
		for (vp = variables[h]; vp != NULL; vp = vp->v_link)
			s++;
	ap = salloc(s * sizeof(*ap));

	/* save the pointers */
	for (h = 0, p = ap; h < HSHSIZE; h++)
		for (vp = variables[h]; vp != NULL; vp = vp->v_link)
			*p++ = vp->v_name;
	*p = NULL;
	sort(ap);
	for (p = ap; *p != NULL; p++)
		if (len == 0 || strncmp(*p, word, len) == 0)
			mail_sl_add(words, estrdup(*p));

	rv = complete_ambiguous(el, word, dolist, words);

	sl_free(words, 1);

	return rv;
}


static unsigned char
complete_alias(EditLine *el, char *word, int dolist)
{
	struct grouphead *gh;
	const char **ap;
	const char **p;
	int h;
	int s;
	size_t len = strlen(word);
	StringList *words;
	unsigned char rv;

	words = sl_init();

	/* allocate space for alias table */
	s = 1;
	for (h = 0; h < HSHSIZE; h++)
		for (gh = groups[h]; gh != NULL; gh = gh->g_link)
			s++;
	ap = salloc(s * sizeof(*ap));

	/* save pointers */
	p = ap;
	for (h = 0; h < HSHSIZE; h++)
		for (gh = groups[h]; gh != NULL; gh = gh->g_link)
			*p++ = gh->g_name;

	*p = NULL;

	sort(ap);
	for (p = ap; *p != NULL; p++)
		if (len == 0 || strncmp(*p, word, len) == 0)
			mail_sl_add(words, estrdup(*p));

	rv = complete_ambiguous(el, word, dolist, words);
	if (rv == CC_REFRESH) {
		if (el_insertstr(el, " ") == -1)
			rv = CC_ERROR;
	}
	sl_free(words, 1);
	return rv;
}


static unsigned char
complete_smopts(EditLine *el, char *word, int dolist)
{
	struct grouphead *gh;
	struct smopts_s *sp;
	const char **ap;
	const char **p;
	int h;
	int s1;
	int s2;
	size_t len;
	StringList *words;
	unsigned char rv;

	len = strlen(word);
	words = sl_init();

	/* count the entries in the smoptstbl and groups (alias) tables */
	s1 = 1;
	s2 = 1;
	for (h = 0; h < HSHSIZE; h++) {
		for (sp = smoptstbl[h]; sp != NULL; sp = sp->s_link)
			s1++;
		for (gh = groups[h]; gh != NULL; gh = gh->g_link)
			s2++;
	}

	/* allocate sufficient space for the pointers */
	ap = salloc(MAX(s1, s2) * sizeof(*ap));

	/*
	 * First do the smoptstbl pointers. (case _insensitive_)
	 */
	p = ap;
	for (h = 0; h < HSHSIZE; h++)
		for (sp = smoptstbl[h]; sp != NULL; sp = sp->s_link)
			*p++ = sp->s_name;
	*p = NULL;
	sort(ap);
	for (p = ap; *p != NULL; p++)
		if (len == 0 || strncasecmp(*p, word, len) == 0)
			mail_sl_add(words, estrdup(*p));

	/*
	 * Now do the groups (alias) pointers. (case sensitive)
	 */
	p = ap;
	for (h = 0; h < HSHSIZE; h++)
		for (gh = groups[h]; gh != NULL; gh = gh->g_link)
			*p++ = gh->g_name;
	*p = NULL;
	sort(ap);
	for (p = ap; *p != NULL; p++)
		if (len == 0 || strncmp(*p, word, len) == 0)
			mail_sl_add(words, estrdup(*p));

	rv = complete_ambiguous(el, word, dolist, words);

	sl_free(words, 1);

	return rv;
}


#ifdef THREAD_SUPPORT
static unsigned char
complete_thread_key(EditLine *el, char *word, int dolist)
{
	const char **ap;
	const char **p;
	const char *name;
	size_t len;
	StringList *words;
	unsigned char rv;
	int cnt;
	const void *cookie;

	len = strlen(word);
	words = sl_init();

	/* count the entries in the table */
	/* XXX - have a function return this rather than counting? */
	cnt = 1;	/* count the NULL terminator */
	cookie = NULL;
	while (thread_next_key_name(&cookie) != NULL)
		cnt++;

	/* allocate sufficient space for the pointers */
	ap = salloc(cnt * sizeof(*ap));

	/* load the array */
	p = ap;
	cookie = NULL;
	while ((name = thread_next_key_name(&cookie)) != NULL)
		*p++ = name;
	*p = NULL;
	sort(ap);
	for (p = ap; *p != NULL; p++)
		if (len == 0 || strncmp(*p, word, len) == 0)
			mail_sl_add(words, estrdup(*p));

	rv = complete_ambiguous(el, word, dolist, words);

	sl_free(words, 1);

	return rv;
}
#endif /* THREAD_SUPPORT */

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
emacs_ctrl_d(EditLine *el, const LineInfo *lf, int ch)
{
	static char delunder[3] = { CTRL('f'), CTRL('h'), '\0' };

	if (ch == CTRL('d') && is_emacs_mode(el)) {	/* CTRL-D is special */
		if (lf->buffer == lf->lastchar)
			return CC_EOF;
		if (lf->cursor != lf->lastchar) { /* delete without using ^D */
			el_push(el, delunder); /* ^F^H */
			return CC_NORM;
		}
	}
	return -1;
}
#endif /* EMACS_CTRL_D_BINDING_HACK */

/*
 * Check if this is the second request made for this line indicating
 * the need to list all the completion possibilities.
 */
static int
get_dolist(const LineInfo *lf)
{
	static char last_line[LINESIZE];
	static char *last_cursor_pos;
	char *cursor_pos;
	int dolist;
	size_t len;

	len = lf->lastchar - lf->buffer;
	if (len >= sizeof(last_line) - 1)
		return -1;

	cursor_pos = last_line + (lf->cursor - lf->buffer);
	dolist =
	    cursor_pos == last_cursor_pos &&
	    strncmp(last_line, lf->buffer, len) == 0;

	(void)strlcpy(last_line, lf->buffer, len + 1);
	last_cursor_pos = cursor_pos;

	return dolist;
}

/*
 * Take the full line (lf) including the command and split it into a
 * sub-line (returned) and a completion context (cmplarray).
 */
static LineInfo *
split_line(const char **cmplarray, const LineInfo *lf)
{
	static LineInfo li;
	const struct cmd *c;
	char *cmdname;
	char line[LINESIZE];
	char *cp;
	size_t len;

	len = lf->cursor - lf->buffer;
	if (len + 1 > sizeof(line))
		return NULL;

	(void)strlcpy(line, lf->buffer, len + 1);

	li.cursor   = line + len;
	li.lastchar = line + len;

	cp = skip_WSP(line);
	cmdname = get_cmdname(cp);
	cp += strlen(cmdname);

	if (cp == li.cursor) {
		*cmplarray = "c";
		li.buffer = cmdname;
		return &li;
	}

	c = lex(cmdname);
	if (c == NULL)
		return NULL;

	*cmplarray = c->c_complete;
	if (c->c_pipe) {
		char *cp2;
		if ((cp2 = shellpr(cp)) != NULL) {
			cp = cp2;
# define XX(a)  ((a) + ((a)[1] == '>' ? 2 : 1))
			while ((cp2 = shellpr(XX(cp))) != NULL)
				cp = cp2;

			if (*cp == '|') {
				*cmplarray = "xF";
				cp = skip_WSP(cp + 1);
			}
			else {
				assert(*cp == '>');
				cp = skip_WSP(XX(cp));
				*cmplarray = "f";
			}
# undef XX
		}
	}
	li.buffer = cp;
	return &li;
}

/*
 * Split a sub-line and a completion context into a word and a
 * completion type.  Use the editline tokenizer to handle the quoting
 * and splitting.
 */
static char *
split_word(int *cmpltype, const char *cmplarray, LineInfo *li)
{
	static Tokenizer *t = NULL;
	const char **argv;
	char *word;
	int argc;
	int cursorc;
	int cursoro;
	int arraylen;

	if (t != NULL)
		tok_reset(t);
	else {
		if ((t = tok_init(NULL)) == NULL)
			err(EXIT_FAILURE, "tok_init");
	}
	if (tok_line(t, li, &argc, &argv, &cursorc, &cursoro) == -1)
		err(EXIT_FAILURE, "tok_line");

	if (cursorc >= argc)
		word = __UNCONST("");
	else {
		word = salloc((size_t)cursoro + 1);
		(void)strlcpy(word, argv[cursorc], (size_t)cursoro + 1);
	}

	/* check for 'continuation' completes (which are uppercase) */
	arraylen = (int)strlen(cmplarray);
	if (cursorc >= arraylen &&
	    arraylen > 0 &&
	    isupper((unsigned char)cmplarray[arraylen - 1]))
		cursorc = arraylen - 1;

	if (cursorc >= arraylen)
		return NULL;

	*cmpltype = cmplarray[cursorc];
	return word;
}

/*
 * A generic complete routine for the mail command line.
 */
static unsigned char
mail_complete(EditLine *el, int ch)
{
	LineInfo *li;
	const LineInfo *lf;
	const char *cmplarray;
	int dolist;
	int cmpltype;
	char *word;

	lf = el_line(el);

#ifdef EMACS_CTRL_D_BINDING_HACK
	{
		int cc_ret;
		if ((cc_ret = emacs_ctrl_d(el, lf, ch)) != -1)
			return cc_ret;
	}
#endif /* EMACS_CTRL_D_BINDING_HACK */

	if ((dolist = get_dolist(lf)) == -1)
		return CC_ERROR;

	if ((li = split_line(&cmplarray, lf)) == NULL)
		return CC_ERROR;

	if ((word = split_word(&cmpltype, cmplarray, li)) == NULL)
		return CC_ERROR;

	switch (cmpltype) {
	case 'a':			/* alias complete */
	case 'A':
		return complete_alias(el, word, dolist);

	case 'c':			/* command complete */
	case 'C':
		return complete_command(el, word, dolist);

	case 'f':			/* filename complete */
	case 'F':
		return complete_filename(el, word, dolist);

	case 'm':
	case 'M':
		return complete_smopts(el, word, dolist);

	case 'n':			/* no complete */
	case 'N':			/* no complete */
		return CC_ERROR;

	case 's':
	case 'S':
		return complete_set(el, word, dolist);
#ifdef THREAD_SUPPORT
	case 't':
	case 'T':
		return complete_thread_key(el, word, dolist);
#endif
		case 'x':			/* executable complete */
	case 'X':
		return complete_executable(el, word, dolist);

	default:
		warnx("unknown complete type `%c'", cmpltype);
#if 0
		assert(/*CONSTCOND*/0);
#endif
		return CC_ERROR;
	}
	/* NOTREACHED */
}


/*
 * A generic file completion routine.
 */
static unsigned char
file_complete(EditLine *el, int ch)
{
	static char word[LINESIZE];
	const LineInfo *lf;
	size_t word_len;
	int dolist;

	lf = el_line(el);

#ifdef EMACS_CTRL_D_BINDING_HACK
	{
		int cc_ret;
		if ((cc_ret = emacs_ctrl_d(el, lf, ch)) != -1)
			return cc_ret;
	}
#endif /* EMACS_CTRL_D_BINDING_HACK */

	word_len = lf->cursor - lf->buffer;
	if (word_len + 1 > sizeof(word))
		return CC_ERROR;

	(void)strlcpy(word, lf->buffer, word_len + 1);	/* do not use estrlcpy here! */

	if ((dolist = get_dolist(lf)) == -1)
		return CC_ERROR;

	return complete_filename(el, word, dolist);
}


#ifdef MIME_SUPPORT
/*
 * Complete mime_transfer_encoding type.
 */
static unsigned char
mime_enc_complete(EditLine *el, int ch)
{
	static char word[LINESIZE];
	StringList *words;
	unsigned char rv;
	const LineInfo *lf;
	size_t word_len;
	int dolist;

	lf = el_line(el);

#ifdef EMACS_CTRL_D_BINDING_HACK
	{
		int cc_ret;
		if ((cc_ret = emacs_ctrl_d(el, lf, ch)) != -1)
			return cc_ret;
	}
#endif /* EMACS_CTRL_D_BINDING_HACK */

	word_len = lf->cursor - lf->buffer;
	if (word_len >= sizeof(word) - 1)
		return CC_ERROR;

	words = mail_sl_init();
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
	(void)strlcpy(word, lf->buffer, word_len + 1);

	if ((dolist = get_dolist(lf)) == -1)
		return CC_ERROR;

	rv = complete_ambiguous(el, word, dolist, words);

	sl_free(words, 1);
	return rv;
}
#endif /* MIME_SUPPORT */


/*************************************************************************
 * Our public interface to el_gets():
 *
 * init_editline()
 *    Initializes of all editline and completion data strutures.
 *
 * my_gets()
 *    Displays prompt, calls el_gets() and deals with history.
 *    Returns the next line of input as a NULL termnated string
 *    without the trailing newline, or NULL if el_gets() sees is an
 *    error or signal.
 */

static const char *el_prompt;

/*ARGSUSED*/
static const char *
show_prompt(EditLine *e __unused)
{
	return el_prompt;
}

/*
 * Write the current INTR character to fp in a friendly form.
 */
static void
echo_INTR(void *p)
{
	struct termios ttybuf;
	char buf[5];
	FILE *fp;

	fp = p;
	if (tcgetattr(fileno(stdin), &ttybuf) == -1)
		warn("tcgetattr");
	else {
		(void)vis(buf, ttybuf.c_cc[VINTR], VIS_SAFE | VIS_NOSLASH, 0);
		(void)fprintf(fp, "%s", buf);
		(void)fflush(fp);
	}
}

static sig_t old_sigint;
static void
comp_intr(int signo)
{

	echo_INTR(stdout);
	old_sigint(signo);
}

PUBLIC char *
my_gets(el_mode_t *em, const char *prompt, char *string)
{
	static char line[LINE_MAX];
	size_t len;
	int cnt;
	const char *buf;
	HistEvent ev;

	sig_check();

	el_prompt = prompt;
	if (string)
		el_push(em->el, string);

	/*
	 * Let el_gets() deal with flow control.  Also, make sure we
	 * output a ^C when we get a SIGINT as el_gets() doesn't echo
	 * one.
	 */
	old_sigint = sig_signal(SIGINT, comp_intr);
	buf = el_gets(em->el, &cnt);
	(void)sig_signal(SIGINT, old_sigint);

	if (buf == NULL) {
		sig_check();
		return NULL;
	}

	if (cnt > 0) {
		if (buf[cnt - 1] == '\n')
			cnt--;	/* trash the trailing LF */

		len = MIN(sizeof(line) - 1, (size_t)cnt);
		(void)memcpy(line, buf, len);
	}
	line[cnt] = '\0';

	/* enter non-empty lines into history */
	if (em->hist) {
		const char *p;

		p = skip_WSP(line);
		if (*p && history(em->hist, &ev, H_ENTER, line) == 0)
			(void)printf("Failed history entry: %s", line);
	}
	sig_check();
	return line;
}

static el_mode_t
init_el_mode(
	const char *el_editor,
	unsigned char (*completer)(EditLine *, int),
	struct name *keys,
	int history_size)
{
	FILE *nullfp;
	el_mode_t em;

	(void)memset(&em, 0, sizeof(em));

	if ((nullfp = fopen(_PATH_DEVNULL, "w")) == NULL)
		err(EXIT_FAILURE, "Cannot open `%s'", _PATH_DEVNULL);

	if ((em.el = el_init(getprogname(), stdin, stdout, nullfp)) == NULL) {
		warn("el_init");
		return em;
	}
	(void)fflush(nullfp);
	(void)dup2(STDERR_FILENO, fileno(nullfp));

	(void)el_set(em.el, EL_PROMPT, show_prompt);
	(void)el_set(em.el, EL_SIGNAL, 1); /* editline handles the signals. */

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
		if ((em.hist = history_init()) == NULL) {
			warn("history_init");
			return em;
		}
		if (history(em.hist, &ev, H_SETSIZE, history_size) == -1)
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

PUBLIC void
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
	return;
}

#endif /* USE_EDITLINE */
