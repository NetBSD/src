/*	$NetBSD: prop_kern.c,v 1.27 2025/04/23 02:58:52 thorpej Exp $	*/

/*-
 * Copyright (c) 2006, 2009, 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#if defined(__NetBSD__)

#include <sys/types.h>
#include <sys/ioctl.h>

#include <prop/proplib.h>

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef RUMP_ACTION
#include <rump/rump_syscalls.h>
#define ioctl(a,b,c) rump_sys_ioctl(a,b,c)
#endif

/*
 * prop_object_externalize_to_pref --
 *	Externalize an object into a plistref for sending to the kernel.
 */
static int
_prop_object_externalize_to_pref(prop_object_t obj, struct plistref *pref,
	       			 char **bufp)
{
	char *buf = prop_object_externalize(obj);

	if (buf == NULL) {
		/* Assume we ran out of memory. */
		return (ENOMEM);
	}
	pref->pref_plist = buf;
	pref->pref_len = strlen(buf) + 1;

	if (bufp != NULL) {
		*bufp = buf;
	}

	return (0);
}

static bool
prop_object_externalize_to_pref(prop_object_t obj, struct plistref *prefp)
{
	int rv = _prop_object_externalize_to_pref(obj, prefp, NULL);
	if (rv != 0)
		errno = rv;	/* pass up error value in errno */
	return (rv == 0);
}

bool
prop_array_externalize_to_pref(prop_array_t array, struct plistref *prefp)
{
	return prop_object_externalize_to_pref(array, prefp);
}
__strong_alias(prop_dictionary_externalize_to_pref,
	       prop_array_externalize_to_pref)

int
prop_object_send_syscall(prop_object_t obj, struct plistref *prefp)
{
	if (prop_object_externalize_to_pref(obj, prefp))
		return 0;
	else
		return errno;
}
__strong_alias(prop_array_send_syscall, prop_object_send_syscall)
__strong_alias(prop_dictionary_send_syscall, prop_object_send_syscall)

/*
 * prop_object_send_ioctl --
 *	Send an array to the kernel using the specified ioctl.
 */
static int
_prop_object_send_ioctl(prop_object_t obj, int fd, unsigned long cmd)
{
	struct plistref pref;
	char *buf;
	int error;

	error = _prop_object_externalize_to_pref(obj, &pref, &buf);
	if (error)
		return (error);

	if (ioctl(fd, cmd, &pref) == -1)
		error = errno;
	else
		error = 0;

	free(buf);

	return (error);
}

int
prop_object_send_ioctl(prop_object_t obj, int fd, unsigned long cmd)
{
	int rv;

	rv = _prop_object_send_ioctl(obj, fd, cmd);
	if (rv != 0) {
		errno = rv;	/* pass up error value in errno */
		return rv;
	} else
		return 0;
}
__strong_alias(prop_array_send_ioctl, prop_object_send_ioctl)
__strong_alias(prop_dictionary_send_ioctl, prop_object_send_ioctl)

/*
 * prop_object_internalize_from_pref --
 * 	Internalize a pref into an object.
 */
static int
_prop_object_internalize_from_pref_with_type(const struct plistref *pref,
					     prop_object_t *objp,
					     prop_type_t type)
{
	prop_object_t obj = NULL;
	char *buf;
	int error = 0;

	if (pref->pref_len == 0) {
		/*
		 * This should never happen; we should always get the XML
		 * for an empty dictionary if it's really empty.
		 */
		error = EIO;
		goto out;
	} else {
		buf = pref->pref_plist;
		buf[pref->pref_len - 1] = '\0';	/* extra insurance */
		obj = prop_object_internalize(buf);
		(void) munmap(buf, pref->pref_len);
		if (obj != NULL && type != PROP_TYPE_UNKNOWN &&
		    prop_object_type(obj) != type) {
			prop_object_release(obj);
			obj = NULL;
		}
		if (obj == NULL)
			error = EIO;
	}

 out:
	if (error == 0)
		*objp = obj;
	return (error);
}

static bool
_prop_object_internalize_from_pref(const struct plistref *prefp,
				   prop_object_t *objp,
				   prop_type_t type)
{
	int rv;

	rv = _prop_object_internalize_from_pref_with_type(prefp, objp, type);
	if (rv != 0)
		errno = rv;     /* pass up error value in errno */
	return (rv == 0);
}

bool
prop_array_internalize_from_pref(const struct plistref *prefp,
				 prop_array_t *arrayp)
{
	return _prop_object_internalize_from_pref(prefp,
	    (prop_object_t *)arrayp, PROP_TYPE_ARRAY);
}

bool
prop_dictionary_internalize_from_pref(const struct plistref *prefp,
				      prop_dictionary_t *dictp)
{
	return _prop_object_internalize_from_pref(prefp,
	    (prop_object_t *)dictp, PROP_TYPE_DICTIONARY);
}

static int
_prop_object_recv_syscall(const struct plistref *prefp,
			  prop_object_t *objp, prop_type_t type)
{
	if (_prop_object_internalize_from_pref_with_type(prefp, objp, type)) {
		return 0;
	} else {
		return errno;
	}
}

int
prop_object_recv_syscall(const struct plistref *prefp,
			 prop_object_t *objp)
{
	return _prop_object_recv_syscall(prefp, objp, PROP_TYPE_UNKNOWN);
}

int
prop_array_recv_syscall(const struct plistref *prefp,
			prop_array_t *arrayp)
{
	return _prop_object_recv_syscall(prefp, (prop_object_t *)arrayp,
	    PROP_TYPE_ARRAY);
}

int
prop_dictionary_recv_syscall(const struct plistref *prefp,
			     prop_dictionary_t *dictp)
{
	return _prop_object_recv_syscall(prefp, (prop_object_t *)dictp,
	    PROP_TYPE_DICTIONARY);
}

/*
 * prop_object_recv_ioctl --
 *	Receive an array from the kernel using the specified ioctl.
 */
static int
_prop_object_recv_ioctl(int fd, unsigned long cmd, prop_object_t *objp,
    prop_type_t type)
{
	int rv;
	struct plistref pref;

	rv = ioctl(fd, cmd, &pref);
	if (rv == -1)
		return errno;

	rv = _prop_object_internalize_from_pref_with_type(&pref, objp, type);
	if (rv != 0) {
		errno = rv;     /* pass up error value in errno */
		return rv;
	} else
		return 0;
}

int
prop_object_recv_ioctl(int fd, unsigned long cmd, prop_object_t *objp)
{
	return _prop_object_recv_ioctl(fd, cmd, objp, PROP_TYPE_UNKNOWN);
}

int
prop_array_recv_ioctl(int fd, unsigned long cmd, prop_array_t *arrayp)
{
	return _prop_object_recv_ioctl(fd, cmd,
	    (prop_object_t *)arrayp, PROP_TYPE_ARRAY);
}

int
prop_dictionary_recv_ioctl(int fd, unsigned long cmd, prop_dictionary_t *dictp)
{
	return _prop_object_recv_ioctl(fd, cmd,
	    (prop_object_t *)dictp, PROP_TYPE_DICTIONARY);
}

/*
 * prop_object_sendrecv_ioctl --
 *	Combination send/receive an object to/from the kernel using
 *	the specified ioctl.
 */
static int
_prop_object_sendrecv_ioctl(prop_object_t obj, int fd,
			    unsigned long cmd, prop_object_t *objp,
			    prop_type_t type)
{
	struct plistref pref;
	char *buf;
	int error;

	error = _prop_object_externalize_to_pref(obj, &pref, &buf);
	if (error != 0) {
		errno = error;
		return error;
	}

	if (ioctl(fd, cmd, &pref) == -1)
		error = errno;
	else
		error = 0;

	free(buf);

	if (error != 0)
		return error;

	error = _prop_object_internalize_from_pref_with_type(&pref, objp, type);
	if (error != 0) {
		errno = error;     /* pass up error value in errno */
		return error;
	} else
		return 0;
}

int
prop_object_sendrecv_ioctl(prop_object_t obj, int fd,
			   unsigned long cmd, prop_object_t *objp)
{
	return _prop_object_sendrecv_ioctl(obj, fd, cmd, objp,
	    PROP_TYPE_UNKNOWN);
}

int
prop_dictionary_sendrecv_ioctl(prop_dictionary_t dict, int fd,
			       unsigned long cmd, prop_dictionary_t *dictp)
{
	return _prop_object_sendrecv_ioctl(dict, fd, cmd,
	    (prop_object_t *)dictp, PROP_TYPE_DICTIONARY);
}
#endif /* !_KERNEL && !_STANDALONE */

#if defined(_KERNEL)
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/resource.h>
#include <sys/pool.h>

#include <uvm/uvm_extern.h>

#include "prop_object_impl.h"

/* Arbitrary limit ioctl input to 128KB */
unsigned int prop_object_copyin_limit = 128 * 1024;

/* initialize proplib for use in the kernel */
void
prop_kern_init(void)
{
	__link_set_decl(prop_linkpools, struct prop_pool_init);
	struct prop_pool_init * const *pi;

	__link_set_foreach(pi, prop_linkpools)
		pool_init((*pi)->pp, (*pi)->size, 0, 0, 0, (*pi)->wchan,
		    &pool_allocator_nointr, IPL_NONE);
}

static int
_prop_object_copyin_size(const struct plistref *pref, prop_object_t *objp,
    size_t lim, prop_type_t type)
{
	prop_object_t obj = NULL;
	char *buf;
	int error;

	if (pref->pref_len >= lim)
		return E2BIG;

	/*
	 * Allocate an extra byte so we can guarantee NUL-termination.
	 */
	buf = malloc(pref->pref_len + 1, M_TEMP, M_WAITOK);
	if (buf == NULL)
		return (ENOMEM);
	error = copyin(pref->pref_plist, buf, pref->pref_len);
	if (error) {
		free(buf, M_TEMP);
		return (error);
	}
	buf[pref->pref_len] = '\0';

	obj = prop_object_internalize(buf);
	if (obj != NULL &&
	    type != PROP_TYPE_UNKNOWN && prop_object_type(obj) != type) {
		prop_object_release(obj);
		obj = NULL;
	}

	free(buf, M_TEMP);
	if (obj == NULL) {
		error = EIO;
	} else {
		*objp = obj;
	}
	return (error);
}

int
prop_object_copyin_size(const struct plistref *pref,
    prop_object_t *objp, size_t lim)
{
	return _prop_object_copyin_size(pref, objp, lim, PROP_TYPE_UNKNOWN);
}

int
prop_array_copyin_size(const struct plistref *pref,
    prop_array_t *arrayp, size_t lim)
{
	return _prop_object_copyin_size(pref, (prop_object_t *)arrayp, lim,
	    PROP_TYPE_ARRAY);
}

int
prop_dictionary_copyin_size(const struct plistref *pref,
    prop_dictionary_t *dictp, size_t lim)
{
	return _prop_object_copyin_size(pref, (prop_object_t *)dictp, lim,
	    PROP_TYPE_DICTIONARY);
}

int
prop_object_copyin(const struct plistref *pref, prop_object_t *objp)
{
	return _prop_object_copyin_size(pref, objp, prop_object_copyin_limit,
	    PROP_TYPE_UNKNOWN);
}

int
prop_array_copyin(const struct plistref *pref, prop_array_t *arrayp)
{
	return _prop_object_copyin_size(pref, (prop_object_t *)arrayp,
	    prop_object_copyin_limit, PROP_TYPE_ARRAY);
}

int
prop_dictionary_copyin(const struct plistref *pref, prop_dictionary_t *dictp)
{
	return _prop_object_copyin_size(pref, (prop_object_t *)dictp,
	    prop_object_copyin_limit, PROP_TYPE_DICTIONARY);
}

static int
_prop_object_copyin_ioctl_size(const struct plistref *pref,
    const u_long cmd, prop_object_t *objp, size_t lim, prop_type_t type)
{
	if ((cmd & IOC_IN) == 0)
		return (EFAULT);

	return _prop_object_copyin_size(pref, objp, lim, type);
}

int
prop_object_copyin_ioctl_size(const struct plistref *pref,
    const u_long cmd, prop_object_t *objp, size_t lim)
{
	return _prop_object_copyin_ioctl_size(pref, cmd, objp, lim,
	    PROP_TYPE_UNKNOWN);
}

int
prop_array_copyin_ioctl_size(const struct plistref *pref,
    const u_long cmd, prop_array_t *arrayp, size_t lim)
{
	return _prop_object_copyin_ioctl_size(pref, cmd,
	    (prop_object_t *)arrayp, lim, PROP_TYPE_ARRAY);
}

int
prop_dictionary_copyin_ioctl_size(const struct plistref *pref,
    const u_long cmd, prop_dictionary_t *dictp, size_t lim)
{
	return _prop_object_copyin_ioctl_size(pref, cmd,
	    (prop_object_t *)dictp, lim, PROP_TYPE_DICTIONARY);
}

int
prop_object_copyin_ioctl(const struct plistref *pref,
    const u_long cmd, prop_object_t *objp)
{
	return _prop_object_copyin_ioctl_size(pref, cmd, objp,
	    prop_object_copyin_limit, PROP_TYPE_UNKNOWN);
}

int
prop_array_copyin_ioctl(const struct plistref *pref,
    const u_long cmd, prop_array_t *arrayp)
{
	return _prop_object_copyin_ioctl_size(pref, cmd,
	    (prop_object_t *)arrayp, prop_object_copyin_limit,
	    PROP_TYPE_ARRAY);
}

int
prop_dictionary_copyin_ioctl(const struct plistref *pref,
    const u_long cmd, prop_dictionary_t *dictp)
{
	return _prop_object_copyin_ioctl_size(pref, cmd,
	    (prop_object_t *)dictp, prop_object_copyin_limit,
	    PROP_TYPE_DICTIONARY);
}

int
prop_object_copyout(struct plistref *pref, prop_object_t obj)
{
	struct lwp *l = curlwp;		/* XXX */
	struct proc *p = l->l_proc;
	char *buf;
	void *uaddr;
	size_t len, rlen;
	int error = 0;

	buf = prop_object_externalize(obj);
	if (buf == NULL)
		return (ENOMEM);

	len = strlen(buf) + 1;
	rlen = round_page(len);
	uaddr = NULL;
	error = uvm_mmap_anon(p, &uaddr, rlen);
	if (error == 0) {
		error = copyout(buf, uaddr, len);
		if (error == 0) {
			pref->pref_plist = uaddr;
			pref->pref_len   = len;
		}
	}

	free(buf, M_TEMP);

	return (error);
}
__strong_alias(prop_array_copyout, prop_object_copyout)
__strong_alias(prop_dictionary_copyout, prop_object_copyout)

int
prop_object_copyout_ioctl(struct plistref *pref, const u_long cmd,
			  prop_object_t obj)
{
	if ((cmd & IOC_OUT) == 0)
		return (EFAULT);
	return prop_object_copyout(pref, obj);
}
__strong_alias(prop_array_copyout_ioctl, prop_object_copyout_ioctl)
__strong_alias(prop_dictionary_copyout_ioctl, prop_object_copyout_ioctl)

#endif /* _KERNEL */

#endif /* __NetBSD__ */
