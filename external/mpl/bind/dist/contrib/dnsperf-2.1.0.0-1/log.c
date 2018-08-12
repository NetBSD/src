/*	$NetBSD: log.c,v 1.2 2018/08/12 13:02:32 christos Exp $	*/

/*
 * Copyright (C) 2011 - 2015 Nominum, Inc.
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

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "util.h"

pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

static void
vlog(FILE *stream, const char *prefix, const char *fmt, va_list args)
{
	LOCK(&log_lock);
	fflush(stdout);
	if (prefix != NULL)
		fprintf(stream, "%s: ", prefix);
	vfprintf(stream, fmt, args);
	fprintf(stream, "\n");
	UNLOCK(&log_lock);
}

void
perf_log_printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vlog(stdout, NULL, fmt, args);
}

void
perf_log_fatal(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vlog(stderr, "Error", fmt, args);
	exit(1);
}

void
perf_log_warning(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vlog(stderr, "Warning", fmt, args);
}
