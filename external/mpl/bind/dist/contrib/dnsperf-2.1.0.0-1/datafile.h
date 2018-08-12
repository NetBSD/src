/*	$NetBSD: datafile.h,v 1.2 2018/08/12 13:02:32 christos Exp $	*/

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

#ifndef PERF_DATAFILE_H
#define PERF_DATAFILE_H 1

#include <isc/types.h>

typedef struct perf_datafile perf_datafile_t;

perf_datafile_t *
perf_datafile_open(isc_mem_t *mctx, const char *filename);

void
perf_datafile_close(perf_datafile_t **dfilep);

void
perf_datafile_setmaxruns(perf_datafile_t *dfile, unsigned int maxruns);

void
perf_datafile_setpipefd(perf_datafile_t *dfile, int pipe_fd);

isc_result_t
perf_datafile_next(perf_datafile_t *dfile, isc_buffer_t *lines,
		   isc_boolean_t is_update);

unsigned int
perf_datafile_nruns(const perf_datafile_t *dfile);

#endif
