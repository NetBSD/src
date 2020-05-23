/*	$NetBSD: procfs_limit.c,v 1.4 2020/05/23 23:42:43 ad Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: procfs_limit.c,v 1.4 2020/05/23 23:42:43 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/resource.h>
#include <miscfs/procfs/procfs.h>

#define MAXBUFFERSIZE (256 * 1024)

static size_t
prl(char *buf, size_t len, intmax_t lim, char sep)
{
	if (lim == RLIM_INFINITY)
		return snprintf(buf, len, "%#20jx%c", lim, sep);
	else
		return snprintf(buf, len, "%20jd%c", lim, sep);
}

int
procfs_dolimit(struct lwp *curl, struct proc *p, struct pfsnode *pfs,
     struct uio *uio)
{
	static const char *label[] = RLIM_STRINGS;
	int error;
	char *buffer;
	size_t bufsize, pos, i;
	struct rlimit rl[RLIM_NLIMITS];

	if (uio->uio_rw != UIO_READ)
		return EOPNOTSUPP;

	mutex_enter(&proc_lock);
	mutex_enter(p->p_lock);
	memcpy(rl, p->p_rlimit, sizeof(rl));
	mutex_exit(p->p_lock);
	mutex_exit(&proc_lock);

	error = 0;

	bufsize = (64 * 3) * __arraycount(rl);
	buffer = malloc(bufsize, M_TEMP, M_WAITOK);
	pos = 0;
	for (i = 0; i < __arraycount(rl); i++) {
		pos += snprintf(buffer + pos, bufsize - pos, "%20.20s ",
		    label[i]);
		pos += prl(buffer + pos, bufsize - pos, rl[i].rlim_cur, ' ');
		pos += prl(buffer + pos, bufsize - pos, rl[i].rlim_max, '\n');
	}

	if ((uintmax_t)uio->uio_offset < pos)
		error = uiomove(buffer + uio->uio_offset,
		    pos - uio->uio_offset, uio);
	else
		error = 0;

	if (buffer != NULL)
		free(buffer, M_TEMP);

	return error;
}
