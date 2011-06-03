/* $NetBSD: arrayalloc.c,v 1.1.2.1 2011/06/03 13:27:43 cherry Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cherry G. Mathew <cherry@NetBSD.org>
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

__RCSID("$NetBSD: arrayalloc.c,v 1.1.2.1 2011/06/03 13:27:43 cherry Exp $");

#include <sys/param.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/bitops.h>
#include <sys/mman.h>

#include "arrayalloc.h"
#include "system.h"

#define MMAPFILE  "/tmp/arrayalloc.c.file"

static size_t mapoffset = 0;

static int fd;

void arrayalloc_init(char *arrayp)
{

	fd = open(MMAPFILE, O_RDWR | O_CREAT, 666);

	if (fd == -1) {
		perror("open(2)");
		abort();
	}
	
	if (unlink(MMAPFILE) == -1) {
		perror("unlink(2)");
		abort();
	}

	return;
}

void arrayalloc_fini(void)
{
	close(fd);
	return;
}

void *arrayalloc(size_t sz)
{
	void *maddr;

	assert(sz != 0);

	size_t i = 0;
	int a[sz];

	sz = roundup(sz, PAGE_SIZE);

	while (i < sz) {

		ssize_t written;
		written = write(fd, a, sz);
		assert(written > 0);
		if (written == -1) {
			perror("write(2)");
			abort();
		}

		i += written;
	}

	maddr = mmap(NULL, sz, PROT_READ | PROT_WRITE, 
		     MAP_ALIGNED(ilog2(PAGE_SIZE)) | 
		     MAP_HASSEMAPHORE | MAP_SHARED | MAP_WIRED,
		     fd, mapoffset);

	if (maddr == MAP_FAILED) {
		perror("mmap(2)");
		abort();
	}

	mapoffset += sz;

	return maddr;
}

void arrayfree(void *ptr, size_t sz)
{
	if (munmap(ptr, sz) == -1) {
		perror("munmap()\n");
		abort();
	}

	/* 
	 * XXX: Ignore releasing the underlying offset for now. This
	 * is a test framework anyway... :-)
	 */
	return;
}
