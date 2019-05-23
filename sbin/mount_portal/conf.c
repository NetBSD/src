/*	$NetBSD: conf.c,v 1.14 2019/05/23 04:34:25 kre Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 *
 *	from: Id: conf.c,v 1.2 1992/05/27 07:09:27 jsp Exp
 *	@(#)conf.c	8.2 (Berkeley) 3/27/94
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: conf.c,v 1.14 2019/05/23 04:34:25 kre Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <regex.h>

#include "portald.h"

#define	ALLOC(ty)	(xmalloc(sizeof(ty)))

typedef struct path path;
struct path {
	qelem p_q;		/* 2-way linked list */
	int p_lno;		/* Line number of this record */
	char *p_args;		/* copy of arg string (malloc) */
	char *p_key;		/* Pathname to match (also p_argv[0]) */
	regex_t p_re;		/* RE to match against pathname (malloc) */
	int p_use_re;		/* true if entry is RE */
	int p_argc;		/* number of elements in arg string */
	char **p_argv;		/* argv[] pointers into arg string (malloc) */
};

static	void	ins_que(qelem *, qelem *);
static	path   *palloc(char *, int, const char *);
static	void	pfree(path *);
static	int	pinsert(path *, qelem *);
static	void	preplace(qelem *, qelem *);
static	void	readfp(qelem *, FILE *, const char *);
static	void	rem_que(qelem *);
static	void   *xmalloc(size_t);

/*
 * Add an element to a 2-way list,
 * just after (pred)
 */
static void
ins_que(qelem *elem, qelem *pred)
{
	qelem *p = pred->q_forw;
	elem->q_back = pred;
	elem->q_forw = p;
	pred->q_forw = elem;
	p->q_back = elem;
}

/*
 * Remove an element from a 2-way list
 */
static void
rem_que(qelem *elem)
{
	qelem *p = elem->q_forw;
	qelem *p2 = elem->q_back;
	p2->q_forw = p;
	p->q_back = p2;
}

/*
 * Error checking malloc
 */
static void *
xmalloc(size_t siz)
{
	void *p = malloc(siz);

	if (p)
		return p;

	syslog(LOG_ERR, "malloc: failed to get %lu bytes", (u_long)siz);
	exit(1);
}

/*
 * Insert the path in the list.
 * If there is already an element with the same key then
 * the *second* one is ignored (return 0).  If the key is
 * not found then the path is added to the end of the list
 * and 1 is returned.
 */
static int
pinsert(path *p0, qelem *q0)
{
	qelem *q;

	if (p0->p_argc == 0)
		return 0;

	for (q = q0->q_forw; q != q0; q = q->q_forw) {
		path *p = (path *)q;

		if (strcmp(p->p_key, p0->p_key) == 0)
			return 0;
	}
	ins_que(&p0->p_q, q0->q_back);
	return 1;
	
}

static path *
palloc(char *cline, int lno, const char *conf_file)
{
	int c, errcode;
	char *s;
	char *key;
	path *p;
	char **ap;

	/*
	 * Do a pass through the string to count the number
	 * of arguments.   Stop if we encounter a comment.
	 */
	c = 0;
	key = strdup(cline);
	for (s = key; s != NULL; ) {
		char *val;

		if (*s == '#') {	/* '#" at beginning of word */
			cline[s-key] = '\0';	/* delete comment -> EOL */
			break;
		}

		while ((val = strsep(&s, " \t\n")) != NULL && *val == '\0')
			;
		if (val)
			c++;
	}
	c++;
	free(key);

	if (c <= 1)
		return 0;

	/*
	 * Now do another pass and generate a new path structure
	 */
	p = ALLOC(path);
	p->p_argc = 0;
	p->p_argv = xmalloc(c * sizeof(char *));
	p->p_args = strdup(cline);
	ap = p->p_argv;
	for (s = p->p_args; s != NULL; ) {
		char *val;

		while ((val = strsep(&s, " \t\n")) != NULL && *val == '\0')
			;
		if (val) {
			*ap++ = val;
			p->p_argc++;
		}
	}
	*ap = 0;

#ifdef DEBUG
	for (c = 0; c < p->p_argc; c++)
		printf("%sv[%d] = %s\n", c?"\t":"", c, p->p_argv[c]);
#endif

	p->p_key = p->p_argv[0];
	p->p_use_re = 0;
	if (strpbrk(p->p_key, RE_CHARS)) {
		errcode = regcomp(&p->p_re, p->p_key, REG_EXTENDED|REG_NOSUB);
		if (errcode == 0)
			p->p_use_re = 1;
		else {
			char buf[200];
			regerror(errcode, &p->p_re, buf, sizeof(buf));
			
			syslog(LOG_WARNING, "%s, line %d: regcomp \"%s\": %s",
	  		  conf_file, p->p_lno, p->p_key, buf);
		}
	}
	p->p_lno = lno;

	return p;
}

/*
 * Free a path structure
 */
static void
pfree(path *p)
{
	free(p->p_args);
	free((char *)p->p_argv);
	if (p->p_use_re)
		regfree(&p->p_re);
	free((char *)p);
}

/*
 * Discard all currently held path structures on q0.
 * and add all the ones on xq.
 */
static void
preplace(qelem *q0, qelem *xq)
{
	/*
	 * While the list is not empty,
	 * take the first element off the list
	 * and free it.
	 */
	while (q0->q_forw != q0) {
		qelem *q = q0->q_forw;
		rem_que(q);
		pfree((path *)q);
	}
	while (xq->q_forw != xq) {
		qelem *q = xq->q_forw;
		rem_que(q);
		ins_que(q, q0);
	}
}

/*
 * Read the lines from the configuration file and
 * add them to the list of paths.
 */
static void
readfp(qelem *q0, FILE *fp, const char *conf_file)
{
	char cline[LINE_MAX];
	int nread = 0;
	qelem q;

	/*
	 * Make a new empty list.
	 */
	q.q_forw = q.q_back = &q;

	/*
	 * Read the lines from the configuration file.
	 */
	while (fgets(cline, sizeof(cline), fp)) {
		path *p = palloc(cline, nread+1, conf_file);

		if (p && !pinsert(p, &q))
			pfree(p);
		nread++;
	}

	/*
	 * If some records were read, then throw
	 * away the old list and replace with the
	 * new one.
	 */
	if (nread)
		preplace(q0, &q);
}

/*
 * Read the configuration file (conf) and replace
 * the existing path list with the new version.
 * If the file is not readable, then no changes take place
 */
int
conf_read(qelem *q, const char *conf)
{
	FILE *fp = fopen(conf, "r");

	if (fp) {
		readfp(q, fp, conf);
		(void)fclose(fp);
		return 0;
	} else {
		int sverrno = errno;

		syslog(LOG_WARNING, "open config file \"%s\": %m", conf);
		errno = sverrno;
		return -1;
	}
}


char **
conf_match(qelem *q0, char *key)
{
	qelem *q;

	for (q = q0->q_forw; q != q0; q = q->q_forw) {
		path *p = (path *)q;

		if (p->p_use_re) {
			if (regexec(&p->p_re, key, 0, NULL, 0) == 0)
				return p->p_argv + 1;
		} else {
			if (strncmp(p->p_key, key, strlen(p->p_key)) == 0)
				return p->p_argv + 1;
		}
	}

	return 0;
}
