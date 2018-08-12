/*	$NetBSD: datafile.c,v 1.2 2018/08/12 13:02:32 christos Exp $	*/

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

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define ISC_BUFFER_USEINLINE

#include <isc/buffer.h>
#include <isc/mem.h>

#include "datafile.h"
#include "log.h"
#include "os.h"
#include "util.h"

#define BUFFER_SIZE (64 * 1024)

struct perf_datafile {
	isc_mem_t *mctx;
	pthread_mutex_t lock;
	int pipe_fd;
	int fd;
	isc_boolean_t is_file;
	size_t size;
	isc_boolean_t cached;
	char databuf[BUFFER_SIZE + 1];
	isc_buffer_t data;
	unsigned int maxruns;
	unsigned int nruns;
	isc_boolean_t read_any;
};

static inline void
nul_terminate(perf_datafile_t *dfile)
{
	unsigned char *data;

	data = isc_buffer_used(&dfile->data);
	*data = '\0';
}

perf_datafile_t *
perf_datafile_open(isc_mem_t *mctx, const char *filename)
{
	perf_datafile_t *dfile;
	struct stat buf;

	dfile = isc_mem_get(mctx, sizeof(*dfile));
	if (dfile == NULL)
		perf_log_fatal("out of memory");

	dfile->mctx = mctx;
	MUTEX_INIT(&dfile->lock);
	dfile->pipe_fd = -1;
	dfile->is_file = ISC_FALSE;
	dfile->size = 0;
	dfile->cached = ISC_FALSE;
	dfile->maxruns = 1;
	dfile->nruns = 0;
	dfile->read_any = ISC_FALSE;
	isc_buffer_init(&dfile->data, dfile->databuf, BUFFER_SIZE);
	if (filename == NULL) {
		dfile->fd = STDIN_FILENO;
	} else {
		dfile->fd = open(filename, O_RDONLY);
		if (dfile->fd < 0)
			perf_log_fatal("unable to open file: %s", filename);
		if (fstat(dfile->fd, &buf) == 0 && S_ISREG(buf.st_mode)) {
			dfile->is_file = ISC_TRUE;
			dfile->size = buf.st_size;
		}
	}
	nul_terminate(dfile);

	return dfile;
}

void
perf_datafile_close(perf_datafile_t **dfilep)
{
	perf_datafile_t *dfile;

	ISC_INSIST(dfilep != NULL && *dfilep != NULL);

	dfile = *dfilep;
	*dfilep = NULL;

	if (dfile->fd >= 0 && dfile->fd != STDIN_FILENO)
		close(dfile->fd);
	MUTEX_DESTROY(&dfile->lock);
	isc_mem_put(dfile->mctx, dfile, sizeof(*dfile));
}

void
perf_datafile_setpipefd(perf_datafile_t *dfile, int pipe_fd)
{
	dfile->pipe_fd = pipe_fd;
}

void
perf_datafile_setmaxruns(perf_datafile_t *dfile, unsigned int maxruns)
{
	dfile->maxruns = maxruns;
}

static void
reopen_file(perf_datafile_t *dfile)
{
	if (dfile->cached) {
		isc_buffer_first(&dfile->data);
	} else {
		if (lseek(dfile->fd, 0L, SEEK_SET) < 0)
			perf_log_fatal("cannot reread input");
		isc_buffer_clear(&dfile->data);
		nul_terminate(dfile);
	}
}

static isc_result_t
read_more(perf_datafile_t *dfile)
{
	unsigned char *data;
	size_t size;
	ssize_t n;
	isc_result_t result;

	if (!dfile->is_file && dfile->pipe_fd >= 0) {
		result = perf_os_waituntilreadable(dfile->fd, dfile->pipe_fd,
						   -1);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	isc_buffer_compact(&dfile->data);
	data = isc_buffer_used(&dfile->data);
	size = isc_buffer_availablelength(&dfile->data);

	n = read(dfile->fd, data, size);
	if (n < 0)
		return (ISC_R_FAILURE);

	isc_buffer_add(&dfile->data, n);
	nul_terminate(dfile);

	if (dfile->is_file &&
	    isc_buffer_usedlength(&dfile->data) == dfile->size)
		dfile->cached = ISC_TRUE;

	return (ISC_R_SUCCESS);
}

static isc_result_t
read_one_line(perf_datafile_t *dfile, isc_buffer_t *lines)
{
	const char *cur;
	unsigned int length, curlen, nrem;
	isc_result_t result;

	while (ISC_TRUE) {
		/* Get the current line */
		cur = isc_buffer_current(&dfile->data);
		curlen = strcspn(cur, "\n");

		/*
		 * If the current line contains the rest of the buffer,
		 * we need to read more (unless the full file is cached).
		 */
		nrem = isc_buffer_remaininglength(&dfile->data);
		if (curlen == nrem) {
			if (! dfile->cached) {
				result = read_more(dfile);
				if (result != ISC_R_SUCCESS)
					return (result);
			}
			if (isc_buffer_remaininglength(&dfile->data) == 0) {
				dfile->nruns++;
				return (ISC_R_EOF);
			}
			if (isc_buffer_remaininglength(&dfile->data) > nrem)
				continue;
		}

		/* We now have a line.  Advance the buffer past it. */
		isc_buffer_forward(&dfile->data, curlen);
		if (isc_buffer_remaininglength(&dfile->data) > 0)
			isc_buffer_forward(&dfile->data, 1);

		/* If the line is empty or a comment, we need to try again. */
		if (curlen > 0 && cur[0] != ';')
			break;
	}

	length = isc_buffer_availablelength(lines);
	if (curlen > length - 1)
		curlen = length - 1;
	isc_buffer_putmem(lines, cur, curlen);
	isc_buffer_putuint8(lines, 0);

	return (ISC_R_SUCCESS);
}

isc_result_t
perf_datafile_next(perf_datafile_t *dfile, isc_buffer_t *lines,
		   isc_boolean_t is_update)
{
	const char *current;
	isc_result_t result;

	LOCK(&dfile->lock);

	if (dfile->maxruns > 0 && dfile->maxruns == dfile->nruns) {
		result = ISC_R_EOF;
		goto done;
	}

	result = read_one_line(dfile, lines);
	if (result == ISC_R_EOF) {
		if (!dfile->read_any) {
			result = ISC_R_INVALIDFILE;
			goto done;
		}
		if (dfile->maxruns != dfile->nruns) {
			reopen_file(dfile);
			result = read_one_line(dfile, lines);

		}
	}
	if (result != ISC_R_SUCCESS) {
		goto done;
	}
	dfile->read_any = ISC_TRUE;

	if (is_update) {
		while (ISC_TRUE) {
			current = isc_buffer_used(lines);
			result = read_one_line(dfile, lines);
			if (result == ISC_R_EOF &&
			    dfile->maxruns != dfile->nruns) {
				reopen_file(dfile);
			}
			if (result != ISC_R_SUCCESS ||
			    strcasecmp(current, "send") == 0)
				break;
		};
	}

	result = ISC_R_SUCCESS;
 done:
	UNLOCK(&dfile->lock);
	return (result);
}

unsigned int
perf_datafile_nruns(const perf_datafile_t *dfile)
{
	return dfile->nruns;
}
