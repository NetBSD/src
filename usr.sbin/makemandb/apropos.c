/*	$NetBSD: apropos.c,v 1.26 2022/05/19 04:08:03 gutteridge Exp $	*/
/*-
 * Copyright (c) 2011 Abhinav Upadhyay <er.abhinav.upadhyay@gmail.com>
 * All rights reserved.
 *
 * This code was developed as part of Google's Summer of Code 2011 program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: apropos.c,v 1.26 2022/05/19 04:08:03 gutteridge Exp $");

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "apropos-utils.h"

typedef struct apropos_flags {
	char **sections;
	int nresults;
	int pager;
	int no_context;
	query_format format;
	int legacy;
	const char *machine;
	const char *manconf;
} apropos_flags;

typedef struct callback_data {
	int count;
	FILE *out;
	apropos_flags *aflags;
} callback_data;

static char *remove_stopwords(const char *);
static int query_callback(query_callback_args *);
__dead static void usage(void);

#define _PATH_PAGER	"/usr/bin/more -s"
#define SECTIONS_ARGS_LENGTH 4;

static void
parseargs(int argc, char **argv, struct apropos_flags *aflags)
{
	int ch;
	size_t sections_offset = 0;
	size_t sections_size = 0;
	char **sections = NULL;
	char *section;
	aflags->manconf = MANCONF;

#define RESIZE_SECTIONS(newsize) \
	if (sections == NULL || sections_offset > sections_size - 1) { \
		sections_size += newsize; \
		sections = erealloc(sections, sections_size * sizeof(*sections)); \
	}

	while ((ch = getopt(argc, argv, "123456789C:hilMmn:PprS:s:")) != -1) {
		switch (ch) {
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			section = emalloc(2);
			section[0] = ch;
			section[1] = 0;
			RESIZE_SECTIONS(SECTIONS_ARGS_LENGTH)
			sections[sections_offset++] = section;
			break;
		case 'C':
			aflags->manconf = optarg;
			break;
		case 'h':
			aflags->format = APROPOS_HTML;
			break;
		case 'i':
			aflags->format = APROPOS_TERM;
			break;
		case 'l':
			aflags->legacy = 1;
			aflags->no_context = 1;
			aflags->format = APROPOS_NONE;
			break;
		case 'M':
			aflags->no_context = 1;
			break;
		case 'm':
			aflags->no_context = 0;
			break;
		case 'n':
			aflags->nresults = atoi(optarg);
			break;
		case 'p':	// user wants a pager
			aflags->pager = 1;
			/*FALLTHROUGH*/
		case 'P':
			aflags->format = APROPOS_PAGER;
			break;
		case 'r':
			aflags->format = APROPOS_NONE;
			break;
		case 'S':
			aflags->machine = optarg;
			break;
		case 's':
			RESIZE_SECTIONS(SECTIONS_ARGS_LENGTH)
			sections[sections_offset++] = estrdup(optarg);
			break;
		case '?':
		default:
			usage();
		}
	}
	if (sections) {
		RESIZE_SECTIONS(1)
		sections[sections_offset] = NULL;
	}
	aflags->sections = sections;
}

int
main(int argc, char *argv[])
{
	query_args args;
	char *query = NULL;	// the user query
	char *errmsg = NULL;
	char *str;
	int pc = 0;
	int rc = 0;
	size_t i;
	int s;
	callback_data cbdata;
	cbdata.out = stdout;		// the default output stream
	cbdata.count = 0;
	apropos_flags aflags;
	aflags.sections = NULL;
	cbdata.aflags = &aflags;
	sqlite3 *db;
	setprogname(argv[0]);
	if (argc < 2)
		usage();

	memset(&aflags, 0, sizeof(aflags));

	if (!isatty(STDOUT_FILENO))
		aflags.format = APROPOS_NONE;
	else
		aflags.format = APROPOS_TERM;

	if ((str = getenv("APROPOS")) != NULL) {
		char **ptr = emalloc((strlen(str) + 2) * sizeof(*ptr));
#define WS "\t\n\r "
		ptr[0] = __UNCONST(getprogname());
		for (s = 1, str = strtok(str, WS); str;
		    str = strtok(NULL, WS), s++)
			ptr[s] = str;
		ptr[s] = NULL;
		parseargs(s, ptr, &aflags);
		free(ptr);
		optreset = 1;
		optind = 1;
	}

	parseargs(argc, argv, &aflags);

	argc -= optind;
	argv += optind;

	if (!argc)
		usage();

	str = NULL;
	while (argc--)
		concat(&str, *argv++);
	query = remove_stopwords(lower(str));

	/*
	 * If the query consisted only of stopwords and we removed all of
	 * them, use the original query.
	 */
	if (query == NULL)
		query = str;
	else
		free(str);

	if ((db = init_db(MANDB_READONLY, aflags.manconf)) == NULL)
		exit(EXIT_FAILURE);

	/* If user wants to page the output, then set some settings */
	if (aflags.pager) {
		const char *pager = getenv("PAGER");
		if (pager == NULL)
			pager = _PATH_PAGER;

		/* Don't get killed by a broken pipe */
		signal(SIGPIPE, SIG_IGN);

		/* Open a pipe to the pager */
		if ((cbdata.out = popen(pager, "w")) == NULL) {
			close_db(db);
			err(EXIT_FAILURE, "pipe failed");
		}
	}

	args.search_str = query;
	args.sections = aflags.sections;
	args.legacy = aflags.legacy;
	args.nrec = aflags.nresults ? aflags.nresults : -1;
	args.offset = 0;
	args.machine = aflags.machine;
	args.callback = &query_callback;
	args.callback_data = &cbdata;
	args.errmsg = &errmsg;

	if (aflags.format == APROPOS_HTML) {
		fprintf(cbdata.out, "<html>\n<header>\n<title>apropos results "
		    "for %s</title></header>\n<body>\n<table cellpadding=\"4\""
		    "style=\"border: 1px solid #000000; border-collapse:"
		    "collapse;\" border=\"1\">\n", query);
	}
	rc = run_query(db, aflags.format, &args);
	if (aflags.format == APROPOS_HTML)
		fprintf(cbdata.out, "</table>\n</body>\n</html>\n");

	if (aflags.pager)
		pc = pclose(cbdata.out);
	free(query);

	if (aflags.sections) {
		for(i = 0; aflags.sections[i]; i++)
			free(aflags.sections[i]);
		free(aflags.sections);
	}

	close_db(db);
	if (errmsg) {
		warnx("%s", errmsg);
		free(errmsg);
		exit(EXIT_FAILURE);
	}

	if (pc == -1)
		err(EXIT_FAILURE, "pclose error");

	/* 
	 * Something wrong with the database, writing output, or a non-existent
	 * pager.
	 */
	if (rc < 0)
		exit(EXIT_FAILURE);

	if (cbdata.count == 0) {
		warnx("No relevant results obtained.\n"
		    "Please make sure that you spelled all the terms correctly "
		    "or try using different keywords.");
	}
	return 0;
}

/*
 * query_callback --
 *  Callback function for run_query.
 *  It simply outputs the results from run_query. If the user specified the -p
 *  option, then the output is sent to a pager, otherwise stdout is the default
 *  output stream.
 */
static int
query_callback(query_callback_args *qargs)
{
	callback_data *cbdata = (callback_data *) qargs->other_data;
	FILE *out = cbdata->out;
	cbdata->count++;
	if (cbdata->aflags->format != APROPOS_HTML) {
	    fprintf(out, cbdata->aflags->legacy ? "%s(%s) - %s\n" :
		"%s (%s)\t%s\n", qargs->name, qargs->section, qargs->name_desc);
	    if (cbdata->aflags->no_context == 0)
		    fprintf(out, "%s\n\n", qargs->snippet);
	} else {
	    fprintf(out, "<tr><td>%s(%s)</td><td>%s</td></tr>\n", qargs->name,
		qargs->section, qargs->name_desc);
	    if (cbdata->aflags->no_context == 0)
		    fprintf(out, "<tr><td colspan=2>%s</td></tr>\n", qargs->snippet);
	}

	return fflush(out);
}

#include "stopwords.c"

/*
 * remove_stopwords--
 *  Scans the query and removes any stop words from it.
 *  Returns the modified query or NULL, if it contained only stop words.
 */

static char *
remove_stopwords(const char *query)
{
	size_t len, idx;
	char *output, *buf;
	const char *sep, *next;

	output = buf = emalloc(strlen(query) + 1);

	for (; query[0] != '\0'; query = next) {
		sep = strchr(query, ' ');
		if (sep == NULL) {
			len = strlen(query);
			next = query + len;
		} else {
			len = sep - query;
			next = sep + 1;
		}
		if (len == 0)
			continue;
		idx = stopwords_hash(query, len);
		if (memcmp(stopwords[idx], query, len) == 0 &&
		    stopwords[idx][len] == '\0')
			continue;
		memcpy(buf, query, len);
		buf += len;
		*buf++ = ' ';
	}

	if (output == buf) {
		free(output);
		return NULL;
	}
	buf[-1] = '\0';
	return output;
}

/*
 * usage --
 *	print usage message and die
 */
static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-123456789ilMmpr] [-C path] [-n results] "
	    "[-S machine] [-s section] query\n",
	    getprogname());
	exit(1);
}
