/*	$NetBSD: rumpcopy.c,v 1.23 2019/04/06 03:06:28 thorpej Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: rumpcopy.c,v 1.23 2019/04/06 03:06:28 thorpej Exp $");

#define	__UFETCHSTORE_PRIVATE
#define	__UCAS_PRIVATE

#include <sys/param.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#include <sys/uio.h>

#include <rump-sys/kern.h>

#include <rump/rumpuser.h>

int
copyin(const void *uaddr, void *kaddr, size_t len)
{
	int error = 0;

	if (__predict_false(uaddr == NULL && len)) {
		return EFAULT;
	}

	if (RUMP_LOCALPROC_P(curproc)) {
		memcpy(kaddr, uaddr, len);
	} else if (len) {
		error = rump_sysproxy_copyin(RUMP_SPVM2CTL(curproc->p_vmspace),
		    uaddr, kaddr, len);
	}

	return error;
}

int
copyout(const void *kaddr, void *uaddr, size_t len)
{
	int error = 0;

	if (__predict_false(uaddr == NULL && len)) {
		return EFAULT;
	}

	if (RUMP_LOCALPROC_P(curproc)) {
		memcpy(uaddr, kaddr, len);
	} else if (len) {
		error = rump_sysproxy_copyout(RUMP_SPVM2CTL(curproc->p_vmspace),
		    kaddr, uaddr, len);
	}
	return error;
}

int
copystr(const void *kfaddr, void *kdaddr, size_t len, size_t *done)
{
	uint8_t *to = kdaddr;
	const uint8_t *from = kfaddr;
	size_t actlen = 0;

	while (len-- > 0 && (*to++ = *from++) != 0)
		actlen++;

	if (len+1 == 0 && *(to-1) != 0)
		return ENAMETOOLONG;

	if (done)
		*done = actlen+1; /* + '\0' */
	return 0;
}

int
copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{
	uint8_t *to;
	int rv;

	if (len == 0)
		return 0;

	if (__predict_false(uaddr == NULL)) {
		return EFAULT;
	}

	if (RUMP_LOCALPROC_P(curproc))
		return copystr(uaddr, kaddr, len, done);

	if ((rv = rump_sysproxy_copyinstr(RUMP_SPVM2CTL(curproc->p_vmspace),
	    uaddr, kaddr, &len)) != 0)
		return rv;

	/* figure out if we got a terminated string or not */
	to = (uint8_t *)kaddr + (len-1);
	while (to >= (uint8_t *)kaddr) {
		if (*to == 0)
			goto found;
		to--;
	}
	return ENAMETOOLONG;

 found:
	if (done)
		*done = strlen(kaddr)+1; /* includes termination */

	return 0;
}

int
copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done)
{
	size_t slen;
	int error;

	if (__predict_false(uaddr == NULL && len)) {
		return EFAULT;
	}

	if (RUMP_LOCALPROC_P(curproc))
		return copystr(kaddr, uaddr, len, done);

	slen = strlen(kaddr)+1;
	if (slen > len)
		return ENAMETOOLONG;

	error = rump_sysproxy_copyoutstr(RUMP_SPVM2CTL(curproc->p_vmspace),
	    kaddr, uaddr, &slen);
	if (done)
		*done = slen;

	return error;
}

int
kcopy(const void *src, void *dst, size_t len)
{

	memcpy(dst, src, len);
	return 0;
}

/*
 * Low-level I/O routine.  This is used only when "all else fails",
 * i.e. the current thread does not have an appropriate vm context.
 */
int
uvm_io(struct vm_map *vm, struct uio *uio, int flag)
{
	int error = 0;

	/* loop over iovecs one-by-one and copyout */
	for (; uio->uio_resid && uio->uio_iovcnt;
	    uio->uio_iovcnt--, uio->uio_iov++) {
		struct iovec *iov = uio->uio_iov;
		size_t curlen = MIN(uio->uio_resid, iov->iov_len);

		if (__predict_false(curlen == 0))
			continue;

		if (uio->uio_rw == UIO_READ) {
			error = rump_sysproxy_copyin(RUMP_SPVM2CTL(vm),
			    (void *)(vaddr_t)uio->uio_offset, iov->iov_base,
			    curlen);
		} else {
			error = rump_sysproxy_copyout(RUMP_SPVM2CTL(vm),
			    iov->iov_base, (void *)(vaddr_t)uio->uio_offset,
			    curlen);
		}
		if (error)
			break;

		iov->iov_base = (uint8_t *)iov->iov_base + curlen;
		iov->iov_len -= curlen;

		uio->uio_resid -= curlen;
		uio->uio_offset += curlen;
	}

	return error;
}

int
_ucas_32(volatile uint32_t *uaddr, uint32_t old, uint32_t new, uint32_t *ret)
{
	uint32_t *uva = ((void *)(uintptr_t)uaddr);
	int error;

	/* XXXXJRT do we need a MP CPU gate? */

	kpreempt_disable();
	error = _ufetch_32(uva, ret);
	if (error == 0 && *ret == old) {
		error = _ustore_32(uva, new);
	}
	kpreempt_enable();

	return error;
}

#ifdef _LP64
int
_ucas_64(volatile uint64_t *uaddr, uint64_t old, uint64_t new, uint64_t *ret)
{
	uint64_t *uva = ((void *)(uintptr_t)uaddr);
	int error;

	/* XXXXJRT do we need a MP CPU gate? */

	kpreempt_disable();
	error = _ufetch_64(uva, ret);
	if (error == 0 && *ret == old) {
		error = _ustore_64(uva, new);
	}
	kpreempt_enable();

	return error;
}
#endif /* _LP64 */

#define	UFETCH(sz)							\
int									\
_ufetch_ ## sz(const uint ## sz ##_t *uaddr, uint ## sz ## _t *valp)	\
{									\
	int error = 0;							\
									\
	if (RUMP_LOCALPROC_P(curproc)) {				\
		*valp = *uaddr;						\
	} else {							\
		error = rump_sysproxy_copyin(				\
		    RUMP_SPVM2CTL(curproc->p_vmspace),			\
		    uaddr, valp, sizeof(*valp));			\
	}								\
	return error;							\
}

UFETCH(8)
UFETCH(16)
UFETCH(32)
#ifdef _LP64
UFETCH(64)
#endif

#undef UFETCH

#define	USTORE(sz)							\
int									\
_ustore_ ## sz(uint ## sz ## _t *uaddr, uint ## sz ## _t val)		\
{									\
	int error = 0;							\
									\
	if (RUMP_LOCALPROC_P(curproc)) {				\
		*uaddr = val;						\
	} else {							\
		error = rump_sysproxy_copyout(				\
		    RUMP_SPVM2CTL(curproc->p_vmspace),			\
		    &val, uaddr, sizeof(val));				\
	}								\
	return error;							\
}

USTORE(8)
USTORE(16)
USTORE(32)
#ifdef _LP64
USTORE(64)
#endif

#undef USTORE
