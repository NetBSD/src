/*	$NetBSD: intrctl_io.c,v 1.3.12.1 2018/06/25 07:26:12 pgoyette Exp $	*/

/*
 * Copyright (c) 2015 Internet Initiative Japan Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: intrctl_io.c,v 1.3.12.1 2018/06/25 07:26:12 pgoyette Exp $");

#include <sys/sysctl.h>
#include <sys/intrio.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "intrctl_io.h"

/*
 * To support increasing the number of interrupts (devices are dynamically
 * attached), retry sysctl(3) "retry" times.
 */
void *
intrctl_io_alloc(int retry)
{
	size_t buf_size;
	int i, error;
	void *buf = NULL;

	error = sysctlbyname("kern.intr.list", NULL, &buf_size, NULL, 0);
	if (error < 0) {
		goto error;
	}

	buf = malloc(buf_size);
	if (buf == NULL) {
		goto error;
	}

	for (i = 0; i < retry; i++) {
		error = sysctlbyname("kern.intr.list", buf, &buf_size, NULL, 0);
		if (error >= 0)
			return buf;
		else if (errno == ENOMEM) {
			void *temp;

			temp = realloc(buf, buf_size);
			if (temp == NULL) {
				goto error;
			}
			buf = temp;
		} else {
			goto error;
		}
	}
error:
	if (buf != NULL)
		free(buf);
	return NULL;
}

void
intrctl_io_free(void *handle)
{

	free(handle);
}

int
intrctl_io_ncpus(void *handle)
{
	struct intrio_list *list = handle;

	return list->il_ncpus;
}

int
intrctl_io_nintrs(void *handle)
{
	struct intrio_list *list = handle;

	return list->il_nintrs;
}

struct intrio_list_line *
intrctl_io_firstline(void *handle)
{
	struct intrio_list *list = handle;
	struct intrio_list_line *next;
	char *buf_end;

	buf_end = (char *)list + list->il_bufsize;
	next = (struct intrio_list_line *)((char *)list + list->il_lineoffset);
	if ((char *)next >= buf_end)
		return NULL;

	return next;
}

struct intrio_list_line *
intrctl_io_nextline(void *handle, struct intrio_list_line *cur)
{
	struct intrio_list *list;
	struct intrio_list_line *next;
	size_t line_size;
	char *buf_end;

	list = handle;
	buf_end = (char *)list + list->il_bufsize;

	line_size = list->il_linesize;
	next = (struct intrio_list_line *)((char *)cur + line_size);
	if ((char *)next >= buf_end)
		return NULL;

	return next;
}
