/*	$NetBSD: citrus_mmap.c,v 1.1 2003/06/25 09:51:37 tshiozak Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: citrus_mmap.c,v 1.1 2003/06/25 09:51:37 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>

#include "citrus_namespace.h"
#include "citrus_region.h"
#include "citrus_mmap.h"

int
_citrus_map_file(struct _citrus_region * __restrict r,
		 const char * __restrict path)
{
	int fd, ret;
	struct stat st;
	void *head;

	_DIAGASSERT(r != NULL);

	_region_init(r, NULL, 0);
	fd = open(path, O_RDONLY);
	if (fd<0)
		return (errno);
	if (fstat(fd, &st)) {
		ret = errno;
		close(fd);
		return ret;
	}
	if ((st.st_mode & S_IFMT) != S_IFREG) {
		close(fd);
		return EOPNOTSUPP;
	}
	head = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_FILE|MAP_PRIVATE,
		    fd, (off_t)0);
	if (!head) {
		ret = errno;
		close(fd);
		return ret;
	}
	_region_init(r, head, (size_t)st.st_size);
	close(fd);

	return 0;
}

void
_citrus_unmap_file(struct _citrus_region *r)
{

	_DIAGASSERT(r != NULL);

	if (_region_head(r) != NULL) {
		munmap(_region_head(r), _region_size(r));
		_region_init(r, NULL, 0);
	}
}
