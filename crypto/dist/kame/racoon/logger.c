/*	$KAME: logger.c,v 1.7 2000/10/04 17:41:01 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "logger.h"
#include "var.h"

struct log *
log_open(siz, fname)
	size_t siz;
	char *fname;
{
	struct log *p;

	p = (struct log *)malloc(sizeof(*p));
	if (p == NULL)
		return NULL;
	memset(p, 0, sizeof(*p));

	p->buf = (char **)malloc(sizeof(char *) * siz);
	if (p->buf == NULL) {
		free(p);
		return NULL;
	}
	memset(p->buf, 0, sizeof(char *) * siz);

	p->tbuf = (time_t *)malloc(sizeof(time_t *) * siz);
	if (p->tbuf == NULL) {
		free(p);
		free(p->buf);
		return NULL;
	}
	memset(p->tbuf, 0, sizeof(time_t *) * siz);

	p->siz = siz;
	if (fname)
		p->fname = strdup(fname);

	return p;
}

/*
 * append string to ring buffer.
 * string must be \n-terminated (since we add timestamps).
 * even if not, we'll add \n to avoid formatting mistake (see log_close()).
 */
void
log_add(p, str)
	struct log *p;
	char *str;
{
	/* syslog if p->fname == NULL? */
	if (p->buf[p->head])
		free(p->buf[p->head]);
	p->buf[p->head] = strdup(str);
	p->tbuf[p->head] = time(NULL);
	p->head++;
	p->head %= p->siz;
}

/*
 * write out string to the log file, as is.
 * \n-termination is up to the caller.  if you don't add \n, the file
 * format may be broken.
 */
int
log_print(p, str)
	struct log *p;
	char *str;
{
	FILE *fp;

	if (p->fname == NULL)
		return -1;	/*XXX syslog?*/
	fp = fopen(p->fname, "a");
	if (fp == NULL)
		return -1;
	fprintf(fp, "%s", str);
	fclose(fp);

	return 0;
}

int
log_vprint(struct log *p, const char *fmt, ...)
{
	va_list ap;

	FILE *fp;

	if (p->fname == NULL)
		return -1;	/*XXX syslog?*/
	fp = fopen(p->fname, "a");
	if (fp == NULL)
		return -1;
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);

	fclose(fp);

	return 0;
}

int
log_vaprint(struct log *p, const char *fmt, va_list ap)
{
	FILE *fp;

	if (p->fname == NULL)
		return -1;	/*XXX syslog?*/
	fp = fopen(p->fname, "a");
	if (fp == NULL)
		return -1;
	vfprintf(fp, fmt, ap);
	fclose(fp);

	return 0;
}

/*
 * write out content of ring buffer, and reclaim the log structure
 */
int
log_close(p)
	struct log *p;
{
	FILE *fp;
	int i, j;
	char ts[256];
	struct tm *tm;

	if (p->fname == NULL)
		goto nowrite;
	fp = fopen(p->fname, "a");
	if (fp == NULL)
		goto nowrite;

	for (i = 0; i < p->siz; i++) {
		j = (p->head + i) % p->siz;
		if (p->buf[j]) {
			tm = localtime(&p->tbuf[j]);
			strftime(ts, sizeof(ts), "%B %d %T", tm);
			fprintf(fp, "%s: %s\n", ts, p->buf[j]);
			if (*(p->buf[j] + strlen(p->buf[j]) - 1) != '\n')
				fprintf(fp, "\n");
		}
	}
	fclose(fp);

nowrite:
	log_free(p);
	return 0;
}

void
log_free(p)
	struct log *p;
{
	int i;

	for (i = 0; i < p->siz; i++)
		free(p->buf[i]);
	free(p->buf);
	free(p->tbuf);
	if (p->fname)
		free(p->fname);
	free(p);
}

#ifdef TEST
struct log *l;

void
vatest(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	log_vaprint(l, fmt, ap);
	va_end(ap);
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	int i;

	l = log_open(30, "/tmp/hoge");
	if (l == NULL)
		errx(1, "hoge");

	for (i = 0; i < 50; i++) {
		log_add(l, "foo");
		log_add(l, "baa");
		log_add(l, "baz");
	}
	log_print(l, "hoge\n");
	log_vprint(l, "hoge %s\n", "this is test");
	vatest("%s %s\n", "this is", "vprint test");
	abort();
	log_free(l);
}

#endif

