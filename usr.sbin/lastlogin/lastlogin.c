/*	$NetBSD: lastlogin.c,v 1.7 2003/08/28 15:54:41 elric Exp $	*/
/*
 * Copyright (c) 1996 John M. Vinopal
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project
 *	by John M. Vinopal.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: lastlogin.c,v 1.7 2003/08/28 15:54:41 elric Exp $");
#endif

#include <sys/types.h>
#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <utmp.h>
#include <unistd.h>

struct output {
	char		 o_name[UT_NAMESIZE];
	char		 o_line[UT_LINESIZE];
	char		 o_host[UT_HOSTSIZE];
	time_t		 o_time;
	struct output	*next;
};

static	char *logfile = _PATH_LASTLOG;

#define SORT_NONE	0x0000
#define SORT_REVERSE	0x0001
#define SORT_TIME	0x0002
#define DOSORT(x)	((x) & (SORT_TIME))
static	int sortlog = SORT_NONE;
static	struct output *outstack = NULL;

	int	main __P((int, char **));
static	int	comparelog __P((const void *, const void *));
static	void	output __P((struct output *));
static	void	process_entry __P((struct passwd *, struct lastlog *));
static	void	push __P((struct output *));
static	void	sortoutput __P((struct output *));
static	void	usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int	ch, i;
	FILE	*fp;
	struct passwd	*passwd;
	struct lastlog	last;

	while ((ch = getopt(argc, argv, "rt")) != -1) {
		switch (ch) {
		case 'r':
			sortlog |= SORT_REVERSE;
			break;
		case 't':
			sortlog |= SORT_TIME;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	fp = fopen(logfile, "r");
	if (fp == NULL)
		err(1, "%s", logfile);

	setpassent(1);	/* Keep passwd file pointers open */

	/* Process usernames given on the command line. */
	if (argc > 1) {
		long offset;
		for (i = 1; i < argc; ++i) {
			if ((passwd = getpwnam(argv[i])) == NULL) {
				warnx("user '%s' not found", argv[i]);
				continue;
			}
			/* Calculate the offset into the lastlog file. */
			offset = (long)(passwd->pw_uid * sizeof(last));
			if (fseek(fp, offset, SEEK_SET)) {
				warn("fseek error");
				continue;
			}
			if (fread(&last, sizeof(last), 1, fp) != 1) {
				warnx("fread error on '%s'", passwd->pw_name);
				clearerr(fp);
				continue;
			}
			process_entry(passwd, &last);
		}
	}
	/* Read all lastlog entries, looking for active ones */
	else {
		for (i = 0; fread(&last, sizeof(last), 1, fp) == 1; i++) {
			if (last.ll_time == 0)
				continue;
			if ((passwd = getpwuid(i)) != NULL)
				process_entry(passwd, &last);
		}
		if (ferror(fp))
			warnx("fread error");
	}

	setpassent(0);	/* Close passwd file pointers */

	if (DOSORT(sortlog))
		sortoutput(outstack);

	fclose(fp);
	exit(0);
}

static void
process_entry(struct passwd *p, struct lastlog *l)
{
	struct output	o;

	strncpy(o.o_name, p->pw_name, UT_NAMESIZE);
	strncpy(o.o_line, l->ll_line, UT_LINESIZE);
	strncpy(o.o_host, l->ll_host, UT_HOSTSIZE);
	o.o_time = l->ll_time;
	o.next = NULL;

	/*
	 * If we are sorting it, we need all the entries in memory so
	 * push the current entry onto a stack.  Otherwise, we can just
	 * output it.
	 */
	if (DOSORT(sortlog))
		push(&o);
	else
		output(&o);
}

static void
push(struct output *o)
{
	struct output	*out;

	out = malloc(sizeof(*out));
	if (!out)
		err(EXIT_FAILURE, "malloc failed");
	memcpy(out, o, sizeof(*out));
	out->next = NULL;

	if (outstack) {
		out->next = outstack;
		outstack = out;
	} else {
		outstack = out;
	}
}

static void
sortoutput(struct output *o)
{
	struct	output **outs;
	struct	output *tmpo;
	int	num;
	int	i;

	/* count the number of entries to display */
	for (num=0, tmpo = o; tmpo; tmpo=tmpo->next, num++)
		;

	outs = malloc(sizeof(*outs) * num);
	if (!outs)
		err(EXIT_FAILURE, "malloc failed");
	for (i=0, tmpo = o; i < num; tmpo=tmpo->next, i++)
		outs[i] = tmpo;

	mergesort(outs, num, sizeof(*outs), comparelog);

	for (i=0; i < num; i++)
		output(outs[i]);
}

static int
comparelog(const void *left, const void *right)
{
	struct output *l = *(struct output **)left;
	struct output *r = *(struct output **)right;
	int order = (sortlog&SORT_REVERSE)?-1:1;

	if (l->o_time < r->o_time)
		return 1 * order;
	if (l->o_time == r->o_time)
		return 0;
	return -1 * order;
}

/* Duplicate the output of last(1) */
static void
output(struct output *o)
{

	printf("%-*.*s  %-*.*s %-*.*s   %s",
		UT_NAMESIZE, UT_NAMESIZE, o->o_name,
		UT_LINESIZE, UT_LINESIZE, o->o_line,
		UT_HOSTSIZE, UT_HOSTSIZE, o->o_host,
		(o->o_time) ? ctime(&(o->o_time)) : "Never logged in\n");
}

static void
usage()
{
	fprintf(stderr, "usage: %s [-rt] [user ...]\n", getprogname());
	exit(1);
}
