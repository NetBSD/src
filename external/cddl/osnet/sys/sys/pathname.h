/*	$NetBSD: pathname.h,v 1.3.44.1 2018/06/25 07:25:26 pgoyette Exp $	*/

/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/cddl/compat/opensolaris/sys/pathname.h 314194 2017-02-24 07:53:56Z avg $
 */

#ifndef _OPENSOLARIS_SYS_PATHNAME_H_
#define	_OPENSOLARIS_SYS_PATHNAME_H_

#ifdef _KERNEL

#include <sys/param.h>
#include <sys/vnode.h>

typedef struct pathname {
	char	*pn_buf;		/* underlying storage */
	char	*pn_path;		/* remaining pathname */
	size_t	pn_pathlen;		/* remaining length */
	size_t	pn_bufsize;		/* total size of pn_buf */
} pathname_t;

#define	NO_FOLLOW	NOFOLLOW

int lookupname(char *, enum uio_seg, enum symfollow, vnode_t **, vnode_t **);
int lookupnameat(char *, enum uio_seg, enum symfollow, vnode_t **, vnode_t **,
    vnode_t *);

#endif	/* _KERNEL */

#endif	/* _OPENSOLARIS_SYS_PATHNAME_H_ */
