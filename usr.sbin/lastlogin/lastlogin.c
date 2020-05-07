/*	$NetBSD: lastlogin.c,v 1.15.44.1 2020/05/07 18:19:28 martin Exp $	*/
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
__RCSID("$NetBSD: lastlogin.c,v 1.15.44.1 2020/05/07 18:19:28 martin Exp $");
#endif

#include <sys/types.h>
#include <err.h>
#include <db.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#ifdef SUPPORT_UTMP
#include <utmp.h>
#endif
#ifdef SUPPORT_UTMPX
#include <utmpx.h>
#endif
#include <unistd.h>
#include <util.h>

#ifndef UT_NAMESIZE
# define UT_NAMESIZE	8
#endif
#ifndef UT_LINESIZE
# define UT_LINESIZE	8
#endif
#ifndef UT_HOSTSIZE
# define UT_HOSTSIZE	16
#endif

#ifndef UTX_USERSIZE
# define UTX_USERSIZE	64
#endif
#ifndef UTX_LINESIZE
# define UTX_LINESIZE	64
#endif
#ifndef UTX_HOSTSIZE
# define UTX_HOSTSIZE	256
#endif

/*
 * Fields in the structure below are 1 byte longer than the maximum possible
 * for NUL-termination.
 */
struct output {
	struct timeval	 o_tv;
	char		 o_name[UTX_USERSIZE+1];
	char		 o_line[UTX_LINESIZE+1];
	char		 o_host[UTX_HOSTSIZE+1];
	struct output	*next;
};

#define SORT_NONE	0x0000
#define SORT_REVERSE	0x0001
#define SORT_TIME	0x0002
#define DOSORT(x)	((x) & (SORT_TIME))
static	int sortlog = SORT_NONE;
static	struct output *outstack = NULL;
static	struct output *outstack_p = NULL;

static int fixed = 0;
#define FIXED_NAMELEN	UT_NAMESIZE
#define FIXED_LINELEN	UT_LINESIZE
/*
 * This makes the "fixed" output fit in 79 columns.
 * Using UT_HOSTSIZE (16) seems too conservative.
 */
#define FIXED_HOSTLEN	32

static int numeric = 0;
static size_t namelen = 0;
static size_t linelen = 0;
static size_t hostlen = 0;
#define SIZECOLUMNS	(!(namelen && linelen && hostlen))

static	int		comparelog(const void *, const void *);
static	void		output_record(struct output *);
#ifdef SUPPORT_UTMP
static	void		process_entry(struct passwd *, struct lastlog *);
static	void		dolastlog(const char *, int, char *[]);
#endif
#ifdef SUPPORT_UTMPX
static	void		process_entryx(struct passwd *, struct lastlogx *);
static	void		dolastlogx(const char *, int, char *[]);
#endif
static	void		append_record(struct output *);
static	void		sizecolumns(struct output *);
static	void		output_stack(struct output *);
static	void		sort_and_output_stack(struct output *);
__dead static	void	usage(void);

int
main(int argc, char *argv[])
{
	const char *logfile =
#if defined(SUPPORT_UTMPX)
	    _PATH_LASTLOGX;
#elif defined(SUPPORT_UTMP)
	    _PATH_LASTLOG;
#else
	#error "either SUPPORT_UTMP or SUPPORT_UTMPX must be defined"
#endif
	int	ch;
	size_t	len;

	while ((ch = getopt(argc, argv, "f:FH:L:nN:rt")) != -1) {
		switch (ch) {
		case 'H':
			hostlen = atoi(optarg);
			break;
		case 'f':
			logfile = optarg;
			break;
		case 'F':
			fixed++;
			break;
		case 'L':
			linelen = atoi(optarg);
			break;
		case 'n':
			numeric++;
			break;
		case 'N':
			namelen = atoi(optarg);
			break;
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

	if (fixed) {
		if (!namelen)
			namelen = FIXED_NAMELEN;
		if (!linelen)
			linelen = FIXED_LINELEN;
		if (!hostlen)
			hostlen = FIXED_HOSTLEN;
	}

	len = strlen(logfile);

	setpassent(1);	/* Keep passwd file pointers open */

#if defined(SUPPORT_UTMPX)
	if (len > 0 && logfile[len - 1] == 'x')
		dolastlogx(logfile, argc, argv);
	else
#endif
#if defined(SUPPORT_UTMP)
		dolastlog(logfile, argc, argv);
#endif

	setpassent(0);	/* Close passwd file pointers */

	if (outstack) {
		if (SIZECOLUMNS)
			sizecolumns(outstack);

		if (DOSORT(sortlog))
			sort_and_output_stack(outstack);
		else
			output_stack(outstack);
	}

	return 0;
}

#ifdef SUPPORT_UTMP
static void
dolastlog(const char *logfile, int argc, char **argv)
{
	int i;
	FILE *fp = fopen(logfile, "r");
	struct passwd	*passwd;
	struct lastlog l;

	if (fp == NULL)
		err(1, "%s", logfile);

	/* Process usernames given on the command line. */
	if (argc > 0) {
		off_t offset;
		for (i = 0; i < argc; i++) {
			if ((passwd = getpwnam(argv[i])) == NULL) {
				warnx("user '%s' not found", argv[i]);
				continue;
			}
			/* Calculate the offset into the lastlog file. */
			offset = passwd->pw_uid * sizeof(l);
			if (fseeko(fp, offset, SEEK_SET)) {
				warn("fseek error");
				continue;
			}
			if (fread(&l, sizeof(l), 1, fp) != 1) {
				warnx("fread error on '%s'", passwd->pw_name);
				clearerr(fp);
				continue;
			}
			process_entry(passwd, &l);
		}
	}
	/* Read all lastlog entries, looking for active ones */
	else {
		for (i = 0; fread(&l, sizeof(l), 1, fp) == 1; i++) {
			if (l.ll_time == 0)
				continue;
			if ((passwd = getpwuid(i)) == NULL) {
				static struct passwd p;
				static char n[32];
				snprintf(n, sizeof(n), "(%d)", i);
				p.pw_uid = i;
				p.pw_name = n;
				passwd = &p;
			}
			process_entry(passwd, &l);
		}
		if (ferror(fp))
			warnx("fread error");
	}

	(void)fclose(fp);
}

static void
process_entry(struct passwd *p, struct lastlog *l)
{
	struct output	o;

	memset(&o, 0, sizeof(o));
	if (numeric > 1)
		(void)snprintf(o.o_name, sizeof(o.o_name), "%d", p->pw_uid);
	else
		(void)strlcpy(o.o_name, p->pw_name, sizeof(o.o_name));
	(void)memcpy(o.o_line, l->ll_line, sizeof(l->ll_line));
	(void)memcpy(o.o_host, l->ll_host, sizeof(l->ll_host));
	o.o_tv.tv_sec = l->ll_time;
	o.o_tv.tv_usec = 0;
	o.next = NULL;

	/*
	 * If we are dynamically sizing the columns or sorting the log,
	 * we need all the entries in memory so push the current entry
	 * onto a stack.  Otherwise, we can just output it.
	 */
	if (SIZECOLUMNS || DOSORT(sortlog))
		append_record(&o);
	else
		output_record(&o);
}
#endif

#ifdef SUPPORT_UTMPX
static void
dolastlogx(const char *logfile, int argc, char **argv)
{
	int i = 0;
	DB *db = dbopen(logfile, O_RDONLY|O_SHLOCK, 0, DB_HASH, NULL);
	DBT key, data;
	struct lastlogx l;
	struct passwd	*passwd;

	if (db == NULL)
		err(1, "%s", logfile);

	if (argc > 0) {
		for (i = 0; i < argc; i++) {
			if ((passwd = getpwnam(argv[i])) == NULL) {
				warnx("User `%s' not found", argv[i]);
				continue;
			}
			key.data = &passwd->pw_uid;
			key.size = sizeof(passwd->pw_uid);

			switch ((*db->get)(db, &key, &data, 0)) {
			case 0:
				break;
			case 1:
				warnx("User `%s' not found", passwd->pw_name);
				continue;
			case -1:
				warn("Error looking up `%s'", passwd->pw_name);
				continue;
			default:
				abort();
			}

			if (data.size != sizeof(l)) {
				errno = EFTYPE;
				err(1, "%s", logfile);
			}
			(void)memcpy(&l, data.data, sizeof(l));

			process_entryx(passwd, &l);
		}
	}
	/* Read all lastlog entries, looking for active ones */
	else {
		switch ((*db->seq)(db, &key, &data, R_FIRST)) {
		case 0:
			break;
		case 1:
			warnx("No entries found");
			(*db->close)(db);
			return;
		case -1:
			warn("Error seeking to first entry");
			(*db->close)(db);
			return;
		default:
			abort();
		}

		do {
			uid_t uid;

			if (key.size != sizeof(uid) || data.size != sizeof(l)) {
				errno = EFTYPE;
				err(1, "%s", logfile);
			}
			(void)memcpy(&uid, key.data, sizeof(uid));

			if ((passwd = getpwuid(uid)) == NULL) {
				static struct passwd p;
				static char n[32];
				snprintf(n, sizeof(n), "(%d)", i);
				p.pw_uid = i;
				p.pw_name = n;
				passwd = &p;
			}
			(void)memcpy(&l, data.data, sizeof(l));
			process_entryx(passwd, &l);
		} while ((i = (*db->seq)(db, &key, &data, R_NEXT)) == 0);

		switch (i) {
		case 1:
			break;
		case -1:
			warn("Error seeking to last entry");
			break;
		case 0:
		default:
			abort();
		}
	}

	(*db->close)(db);
}

static void
process_entryx(struct passwd *p, struct lastlogx *l)
{
	struct output	o;

	memset(&o, 0, sizeof(o));
	if (numeric > 1)
		(void)snprintf(o.o_name, sizeof(o.o_name), "%d", p->pw_uid);
	else
		(void)strlcpy(o.o_name, p->pw_name, sizeof(o.o_name));
	(void)memcpy(o.o_line, l->ll_line, sizeof(l->ll_line));
	if (numeric)
		(void)sockaddr_snprintf(o.o_host, sizeof(o.o_host), "%a",
		    (struct sockaddr *)&l->ll_ss);
	else
		(void)memcpy(o.o_host, l->ll_host, sizeof(l->ll_host));
	o.o_tv = l->ll_tv;
	o.next = NULL;

	/*
	 * If we are dynamically sizing the columns or sorting the log,
	 * we need all the entries in memory so push the current entry
	 * onto a stack.  Otherwise, we can just output it.
	 */
	if (SIZECOLUMNS || DOSORT(sortlog))
		append_record(&o);
	else
		output_record(&o);
}
#endif

static void
append_record(struct output *o)
{
	struct output	*out;

	out = malloc(sizeof(*out));
	if (!out)
		err(EXIT_FAILURE, "malloc failed");
	(void)memcpy(out, o, sizeof(*out));
	out->next = NULL;

	if (outstack_p)
		outstack_p = outstack_p->next = out;
	else
		outstack = outstack_p = out;
}

static void
sizecolumns(struct output *stack)
{
	struct	output *o;
	size_t	len;

	if (!namelen)
		for (o = stack; o; o = o->next) {
			len = strlen(o->o_name);
			if (namelen < len)
				namelen = len;
		}

	if (!linelen)
		for (o = stack; o; o = o->next) {
			len = strlen(o->o_line);
			if (linelen < len)
				linelen = len;
		}

	if (!hostlen)
		for (o = stack; o; o = o->next) {
			len = strlen(o->o_host);
			if (hostlen < len)
				hostlen = len;
		}
}

static void
output_stack(struct output *stack)
{
	struct	output *o;

	for (o = stack; o; o = o->next)
		output_record(o);
}

static void
sort_and_output_stack(struct output *o)
{
	struct	output **outs;
	struct	output *tmpo;
	int	num;
	int	i;

	/* count the number of entries to display */
	for (num=0, tmpo = o; tmpo; tmpo = tmpo->next, num++)
		;

	outs = malloc(sizeof(*outs) * num);
	if (!outs)
		err(EXIT_FAILURE, "malloc failed");
	for (i=0, tmpo = o; i < num; tmpo=tmpo->next, i++)
		outs[i] = tmpo;

	mergesort(outs, num, sizeof(*outs), comparelog);

	for (i=0; i < num; i++)
		output_record(outs[i]);
}

static int
comparelog(const void *left, const void *right)
{
	const struct output *l = *(const struct output * const *)left;
	const struct output *r = *(const struct output * const *)right;
	int order = (sortlog&SORT_REVERSE)?-1:1;

	if (l->o_tv.tv_sec < r->o_tv.tv_sec)
		return 1 * order;
	if (l->o_tv.tv_sec == r->o_tv.tv_sec)
		return 0;
	return -1 * order;
}

/* Duplicate the output of last(1) */
static void
output_record(struct output *o)
{
	time_t t = (time_t)o->o_tv.tv_sec;
	printf("%-*.*s  %-*.*s  %-*.*s  %s",
		(int)namelen, (int)namelen, o->o_name,
		(int)linelen, (int)linelen, o->o_line,
		(int)hostlen, (int)hostlen, o->o_host,
		t ? ctime(&t) : "Never logged in\n");
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-Fnrt] [-f filename] "
	    "[-H hostsize] [-L linesize] [-N namesize] [user ...]\n",
	    getprogname());
	exit(1);
}
