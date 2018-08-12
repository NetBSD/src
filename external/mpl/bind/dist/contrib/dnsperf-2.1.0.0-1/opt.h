/*	$NetBSD: opt.h,v 1.1.1.1 2018/08/12 12:07:51 christos Exp $	*/

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

#ifndef PERF_OPT_H
#define PERF_OPT_H 1

typedef enum {
	perf_opt_string,
	perf_opt_boolean,
	perf_opt_uint,
	perf_opt_timeval,
	perf_opt_double,
	perf_opt_port,
} perf_opttype_t;

void
perf_opt_add(char c, perf_opttype_t type, const char *desc, const char *help,
	     const char *defval, void *valp);

void
perf_opt_usage(void);

void
perf_opt_parse(int argc, char **argv);

#endif
