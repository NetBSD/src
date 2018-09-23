/*	$NetBSD: nv_kern_netbsd.c,v 1.5 2018/09/23 21:35:26 rmind Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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
__RCSID("$NetBSD: nv_kern_netbsd.c,v 1.5 2018/09/23 21:35:26 rmind Exp $");

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#endif
#ifdef _KERNEL
#include <sys/param.h>
#include <sys/lwp.h>
#include <sys/kmem.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <uvm/uvm_extern.h>
#endif
#ifdef _STANDALONE
/* XXX */
extern void *alloc(unsigned int);
extern void dealloc(void *, unsigned int);
// #include "stand.h"
#else
#include <sys/ioctl.h>
#endif
#include "nv.h"
#include "nv_impl.h"

#ifndef _STANDALONE
#ifdef _KERNEL

void 
nv_free(void *buf)
{
	if (!buf) {
		return;
	}
	free(buf, M_NVLIST);
}

int
nvlist_copyin(const nvlist_ref_t *nref, nvlist_t **nvlp, size_t lim)
{
	const size_t len = nref->len;
	int flags, error;
	nvlist_t *nvl;
	void *buf;

	if (len >= lim) {
		return E2BIG;
	}
	buf = kmem_alloc(len, KM_SLEEP);
	error = copyin(nref->buf, buf, len);
	if (error) {
		kmem_free(buf, len);
		return error;
	}
	flags = nref->flags & (NV_FLAG_IGNORE_CASE | NV_FLAG_NO_UNIQUE);
	nvl = nvlist_unpack(buf, len, flags);
	kmem_free(buf, len);
	if (nvl == NULL) {
		return EINVAL;
	}
	*nvlp = nvl;
	return 0;
}

int
nvlist_copyout(nvlist_ref_t *nref, const nvlist_t *nvl)
{
	struct proc *p = curproc;
	void *buf, *uaddr;
	size_t len, rlen;
	int error;

	buf = nvlist_pack(nvl, &len);
	if (buf == NULL) {
		return ENOMEM;
	}

	/*
	 * Map the user page(s).
	 *
	 * Note: nvlist_recv_ioctl() will unmap it.
	 */
	uaddr = NULL;
	rlen = round_page(len);
	error = uvm_mmap_anon(p, &uaddr, rlen);
	if (error) {
		goto err;
	}
	error = copyout(buf, uaddr, len);
	if (error) {
		uvm_unmap(&p->p_vmspace->vm_map, (vaddr_t)uaddr,
		    (vaddr_t)uaddr + len);
		goto err;
	}
	nref->flags = nvlist_flags(nvl);
	nref->buf = uaddr;
	nref->len = len;
err:
	free(buf, M_TEMP);
	return error;
}

#else

int
nvlist_xfer_ioctl(int fd, unsigned long cmd, const nvlist_t *nvl,
    nvlist_t **nvlp)
{
	nvlist_ref_t nref;
	void *buf = NULL;

	memset(&nref, 0, sizeof(nvlist_ref_t));

	if (nvl) {
		/*
		 * Sending: serialize the name-value list.
		 */
		buf = nvlist_pack(nvl, &nref.len);
		if (buf == NULL) {
			errno = ENOMEM;
			return -1;
		}
		nref.buf = buf;
		nref.flags = nvlist_flags(nvl);
	}

	/*
	 * Exchange the nvlist reference data.
	 */
	if (ioctl(fd, cmd, &nref) == -1) {
		free(buf);
		return -1;
	}
	free(buf);

	if (nvlp) {
		nvlist_t *retnvl;

		/*
		 * Receiving: unserialize the nvlist.
		 *
		 * Note: pages are mapped by nvlist_kern_copyout() for us.
		 */
		if (nref.buf == NULL || nref.len == 0) {
			errno = EIO;
			return -1;
		}
		retnvl = nvlist_unpack(nref.buf, nref.len, nref.flags);
		munmap(nref.buf, nref.len);
		if (retnvl == NULL) {
			errno = EIO;
			return -1;
		}
		*nvlp = retnvl;
	}
	return 0;
}

int
nvlist_send_ioctl(int fd, unsigned long cmd, const nvlist_t *nvl)
{
	return nvlist_xfer_ioctl(fd, cmd, nvl, NULL);
}

int
nvlist_recv_ioctl(int fd, unsigned long cmd, nvlist_t **nvlp)
{
	return nvlist_xfer_ioctl(fd, cmd, NULL, nvlp);
}
#endif
#endif

void *
nv_calloc(size_t n, size_t s)
{
	const size_t len = n * s;
	void *buf = nv_malloc(len);
	if (buf == NULL)
		return NULL;
	memset(buf, 0, len);
	return buf;
}

char *
nv_strdup(const char *s1)
{
	size_t len = strlen(s1) + 1;
	char *s2;

	s2 = nv_malloc(len);
	if (s2) {
		memcpy(s2, s1, len);
		s2[len] = '\0';
	}
	return s2;
}

#ifdef _STANDALONE

void *
nv_malloc(size_t len)
{
	return alloc(len);
}

void 
nv_free(void *buf)
{
	if (buf == NULL)
		return;
	unsigned int *olen = (void *)((char *)buf - sizeof(unsigned int));
	dealloc(buf, *olen);
}

void *
nv_realloc(void *buf, size_t len)
{
	if (buf == NULL)
		return alloc(len);

	unsigned int *olen = (void *)((char *)buf - sizeof(unsigned int));
	if (*olen < len)
		return buf;

	void *nbuf = alloc(len);
	memcpy(nbuf, buf, *olen);
	dealloc(buf, *olen);
	return nbuf;
}
#endif
