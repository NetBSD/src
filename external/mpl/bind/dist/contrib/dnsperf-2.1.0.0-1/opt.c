/*	$NetBSD: opt.c,v 1.2 2018/08/12 13:02:32 christos Exp $	*/

/*
 * Copyright (C) 2012 - 2015 Nominum, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND NOMINUM DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL NOMINUM BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>

#include <isc/file.h>
#include <isc/parseint.h>
#include <isc/result.h>

#include "log.h"
#include "opt.h"
#include "util.h"

#define MAX_OPTS 64
#define LINE_LENGTH 80

typedef struct {
	char c;
	perf_opttype_t type;
	const char *desc;
	const char *help;
	const char *defval;
	char defvalbuf[32];
	union {
		void *valp;
		char **stringp;
		isc_boolean_t *boolp;
		unsigned int *uintp;
		isc_uint64_t *uint64p;
		double *doublep;
		in_port_t *portp;
	} u;
} opt_t;

static opt_t opts[MAX_OPTS];
static unsigned int nopts;
static char optstr[MAX_OPTS * 2 + 2];
static const char *progname;

void
perf_opt_add(char c, perf_opttype_t type, const char *desc, const char *help,
	     const char *defval, void *valp)
{
	opt_t *opt;
	char s[3];

	if (nopts == MAX_OPTS)
		perf_log_fatal("too many defined options");
	opt = &opts[nopts++];
	opt->c = c;
	opt->type = type;
	opt->desc = desc;
	opt->help = help;
	if (defval != NULL) {
		strncpy(opt->defvalbuf, defval, sizeof(opt->defvalbuf));
		opt->defval = opt->defvalbuf;
	} else {
		opt->defval = NULL;
	}
	opt->u.valp = valp;

	sprintf(s, "%c%s", c, (type == perf_opt_boolean ? "" : ":"));
	strcat(optstr, s);
}

void
perf_opt_usage(void)
{
	unsigned int prefix_len, position, arg_len, i, j;

	prefix_len = fprintf(stderr, "Usage: %s", progname);
	position = prefix_len;
	for (i = 0; i < nopts; i++) {
		arg_len = 6;
		if (opts[i].desc != NULL)
			arg_len += strlen(opts[i].desc) + 1;
		if (LINE_LENGTH - position - 1 < arg_len) {
			fprintf(stderr, "\n");
			for (j = 0; j < prefix_len; j++)
				fprintf(stderr, " ");
			position = prefix_len;
		}
		fprintf(stderr, " [-%c", opts[i].c);
		if (opts[i].desc != NULL)
			fprintf(stderr, " %s", opts[i].desc);
		fprintf(stderr, "]");
		position += arg_len;
	}
	fprintf(stderr, "\n");

	for (i = 0; i < nopts; i++) {
		fprintf(stderr, "  -%c %s", opts[i].c, opts[i].help);
		if (opts[i].defval)
			fprintf(stderr, " (default: %s)", opts[i].defval);
		fprintf(stderr, "\n");
	}
}

static isc_uint32_t
parse_uint(const char *desc, const char *str,
	   unsigned int min, unsigned int max)
{
	isc_uint32_t val;
	isc_result_t result;

	val = 0;
	result = isc_parse_uint32(&val, str, 10);
	if (result != ISC_R_SUCCESS || val < min || val > max) {
		fprintf(stderr, "invalid %s: %s\n", desc, str);
		perf_opt_usage();
		exit(1);
	}
	return val;
}

static isc_uint64_t
parse_timeval(const char *desc, const char *str)
{
	const char *s;
	char c;
	isc_boolean_t seen_dot = ISC_FALSE;

	s = str;
	while (*s != 0) {
		c = *s++;
		if (c == '.') {
			if (seen_dot)
				goto fail;
			seen_dot = ISC_TRUE;
		} else if (c < '0' || c > '9') {
			goto fail;
		}
	}

	return MILLION * atof(str);
 fail:
	fprintf(stderr, "invalid %s: %s\n", desc, str);
	perf_opt_usage();
	exit(1);
}

static double
parse_double(const char *desc, const char *str) {
	char c;
	int index = 0;
	isc_boolean_t seen_dot = ISC_FALSE;

	while (str[index] != 0) {
		c = str[index];
		if (c == '.') {
			if (seen_dot)
				goto fail;
			seen_dot = ISC_TRUE;
		} else if (c < '0' || c > '9') {
			goto fail;
		}
		index++;
	}

	return atof(str);
 fail:
	fprintf(stderr, "invalid %s: %s\n", desc, str);
	perf_opt_usage();
	exit(1);
}

void
perf_opt_parse(int argc, char **argv)
{
	int c;
	opt_t *opt;
	unsigned int i;

	progname = isc_file_basename(argv[0]);

	perf_opt_add('h', perf_opt_boolean, NULL, "print this help",
		     NULL, NULL);

	while ((c = getopt(argc, argv, optstr)) != -1) {
		for (i = 0; i < nopts; i++) {
			if (opts[i].c == c)
				break;
		}
		if (i == nopts) {
			perf_opt_usage();
			exit(1);
		}
		if (c == 'h') {
			perf_opt_usage();
			exit(0);
		}
		opt = &opts[i];
		switch (opt->type) {
		case perf_opt_string:
			*opt->u.stringp = optarg;
			break;
		case perf_opt_boolean:
			*opt->u.boolp = ISC_TRUE;
			break;
		case perf_opt_uint:
			*opt->u.uintp = parse_uint(opt->desc, optarg,
						   1, 0xFFFFFFFF);
			break;
		case perf_opt_timeval:
			*opt->u.uint64p = parse_timeval(opt->desc, optarg);
			break;
		case perf_opt_double:
			*opt->u.doublep = parse_double(opt->desc, optarg);
			break;
		case perf_opt_port:
			*opt->u.portp = parse_uint(opt->desc, optarg,
						   0, 0xFFFF);
			break;
		}
	}
	if (optind != argc) {
		fprintf(stderr, "unexpected argument %s\n", argv[optind]);
		perf_opt_usage();
		exit(1);
	}
}
