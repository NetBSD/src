/*	$NetBSD: flush.c,v 1.11 2007/07/19 12:52:28 pooka Exp $	*/

/*
 * Copyright (c) 2007  Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: flush.c,v 1.11 2007/07/19 12:52:28 pooka Exp $");
#endif /* !lint */

/*
 * Flushing / invalidation routines
 */

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <puffs.h>
#include <stdio.h>

#include "puffs_priv.h"

#if 0
int
puffs_inval_namecache_node(struct puffs_usermount *pu, void *cookie,
	const char *name)
{

	return EOPNOTSUPP;
}
#endif

int
puffs_inval_namecache_dir(struct puffs_usermount *pu, void *cookie)
{
	struct puffs_flush pf;

	pf.pf_op = PUFFS_INVAL_NAMECACHE_DIR;
	pf.pf_cookie = cookie;

	return ioctl(pu->pu_fd, PUFFSFLUSHOP, &pf);
}

int
puffs_inval_namecache_all(struct puffs_usermount *pu)
{
	struct puffs_flush pf;

	pf.pf_op = PUFFS_INVAL_NAMECACHE_ALL;
	pf.pf_cookie = NULL;

	return ioctl(pu->pu_fd, PUFFSFLUSHOP, &pf);
}

int
puffs_inval_pagecache_node(struct puffs_usermount *pu, void *cookie)
{
	struct puffs_flush pf;

	pf.pf_op = PUFFS_INVAL_PAGECACHE_NODE_RANGE;
	pf.pf_cookie = cookie;
	pf.pf_start = 0;
	pf.pf_end = 0;

	return ioctl(pu->pu_fd, PUFFSFLUSHOP, &pf);
}

int
puffs_inval_pagecache_node_range(struct puffs_usermount *pu, void *cookie,
	off_t start, off_t end)
{
	struct puffs_flush pf;

	pf.pf_op = PUFFS_INVAL_PAGECACHE_NODE_RANGE;
	pf.pf_cookie = cookie;
	pf.pf_start = start;
	pf.pf_end = end;

	return ioctl(pu->pu_fd, PUFFSFLUSHOP, &pf);
}

int
puffs_flush_pagecache_node(struct puffs_usermount *pu, void *cookie)
{
	struct puffs_flush pf;

	pf.pf_op = PUFFS_FLUSH_PAGECACHE_NODE_RANGE;
	pf.pf_cookie = cookie;
	pf.pf_start = 0;
	pf.pf_end = 0;

	return ioctl(pu->pu_fd, PUFFSFLUSHOP, &pf);
}

int
puffs_flush_pagecache_node_range(struct puffs_usermount *pu, void *cookie,
	off_t start, off_t end)
{
	struct puffs_flush pf;

	pf.pf_op = PUFFS_FLUSH_PAGECACHE_NODE_RANGE;
	pf.pf_cookie = cookie;
	pf.pf_start = start;
	pf.pf_end = end;

	return ioctl(pu->pu_fd, PUFFSFLUSHOP, &pf);
}
