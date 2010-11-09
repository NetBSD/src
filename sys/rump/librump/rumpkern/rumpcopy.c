/*	$NetBSD: rumpcopy.c,v 1.8 2010/11/09 15:22:47 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rumpcopy.c,v 1.8 2010/11/09 15:22:47 pooka Exp $");

#include <sys/param.h>
#include <sys/lwp.h>
#include <sys/systm.h>

#include <rump/rump.h>

#include "rump_private.h"

int
copyin(const void *uaddr, void *kaddr, size_t len)
{

	if (__predict_false(uaddr == NULL && len)) {
		return EFAULT;
	}

	if (curproc->p_vmspace == &vmspace0) {
		memcpy(kaddr, uaddr, len);
	} else {
		rumpuser_sp_copyin(uaddr, kaddr, len);
	}
	return 0;
}

int
copyout(const void *kaddr, void *uaddr, size_t len)
{

	if (__predict_false(uaddr == NULL && len)) {
		return EFAULT;
	}

	if (curproc->p_vmspace == &vmspace0) {
		memcpy(uaddr, kaddr, len);
	} else {
		rumpuser_sp_copyout(kaddr, uaddr, len);
	}
	return 0;
}

int
subyte(void *uaddr, int byte)
{

	if (curproc->p_vmspace == &vmspace0)
		*(char *)uaddr = byte;
	else
		rumpuser_sp_copyout(&byte, uaddr, 1);
	return 0;
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

	if (curproc->p_vmspace == &vmspace0)
		return copystr(uaddr, kaddr, len, done);

	if ((rv = rumpuser_sp_copyin(uaddr, kaddr, len)) != 0)
		return rv;

	/* figure out if we got a terminate string or not */
	to = (uint8_t *)kaddr + len;
	while (to != kaddr) {
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

	if (curproc->p_vmspace == &vmspace0)
		return copystr(kaddr, uaddr, len, done);

	slen = strlen(kaddr)+1;
	if (slen > len)
		return ENAMETOOLONG;

	rumpuser_sp_copyout(kaddr, uaddr, slen);
	if (done)
		*done = slen;

	return 0;
}

int
kcopy(const void *src, void *dst, size_t len)
{

	memcpy(dst, src, len);
	return 0;
}
