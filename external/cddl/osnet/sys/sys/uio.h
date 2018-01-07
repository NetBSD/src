/*	$NetBSD: uio.h,v 1.10 2018/01/07 20:02:52 christos Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * $FreeBSD: src/sys/compat/opensolaris/sys/uio.h,v 1.1 2007/04/06 01:09:06 pjd Exp $
 */

#ifndef _OPENSOLARIS_SYS_UIO_H_
#define	_OPENSOLARIS_SYS_UIO_H_

#include_next <sys/uio.h>
#include <sys/sysmacros.h>


#ifndef _KERNEL
#include <assert.h>
#include <string.h>

#define	FOF_OFFSET	1	/* Use the offset in uio argument */

struct uio {
	struct	iovec *uio_iov;
	int	uio_iovcnt;
	off_t	uio_offset;
	int	uio_resid;
	enum	uio_seg uio_segflg;
	enum	uio_rw uio_rw;
	void	*uio_td;
};
#endif

struct xuio {
	struct uio xu_uio;
	int	xuio_rw;
	void *xuio_priv;
};

#define XUIO_XUZC_PRIV(xuio)	((xuio)->xuio_priv)
#define XUIO_XUZC_RW(xuio)	((xuio)->xuio_rw)

typedef	struct uio	uio_t;
typedef struct xuio	xuio_t;
typedef	struct iovec	iovec_t;

typedef enum uio_seg    uio_seg_t;

#define	uio_loffset	uio_offset

int	uiomove(void *, size_t, struct uio *);

static __inline int
zfs_uiomove(void *cp, size_t n, enum uio_rw dir, uio_t *uio)
{

	assert(uio->uio_rw == dir);
	return (uiomove(cp, n, uio));
}

static __inline int
zfs_uiocopy(void *cp, size_t n, enum uio_rw dir, uio_t *uio, size_t *cbytes)
{
	uio_t uio2;
	int err;
	
	memcpy(&uio2, uio, sizeof(*uio));
	assert(uio->uio_rw == dir);
	if ((err = uiomove(cp, n, &uio2)) != 0)
		return err;

	*cbytes = (size_t)(uio->uio_resid - uio2.uio_resid);

	return (0);
}

static __inline void
zfs_uioskip(uio_t *uiop, size_t n)
{
	if (n > (size_t)uiop->uio_resid)
		return;
	while (n != 0) {
		iovec_t        *iovp = uiop->uio_iov;
		size_t         niovb = MIN(iovp->iov_len, n);

		if (niovb == 0) {
			uiop->uio_iov++;
			uiop->uio_iovcnt--;
			continue;
		}
		iovp->iov_base = (char *)iovp->iov_base + niovb;
		uiop->uio_offset += (off_t)niovb;
		iovp->iov_len -= niovb;
		uiop->uio_resid -= (int)niovb;
		n -= niovb;
	}
	
}

#define	uiomove(cp, n, dir, uio)	zfs_uiomove((cp), (n), (dir), (uio))
#define uiocopy(cp, n, dir, uio, cbytes) 	zfs_uiocopy((cp), (n), (dir), (uio), (cbytes))
#define uioskip(uio, size) 		zfs_uioskip((uio), (size))

#endif	/* !_OPENSOLARIS_SYS_UIO_H_ */
