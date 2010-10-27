/*	$NetBSD: rumpcopy.c,v 1.6 2010/10/27 20:44:49 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rumpcopy.c,v 1.6 2010/10/27 20:44:49 pooka Exp $");

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

	strlcpy(kdaddr, kfaddr, len);
	if (done)
		*done = strlen(kdaddr)+1; /* includes termination */
	return 0;
}

int
copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{

	if (curproc->p_vmspace == &vmspace0)
		strlcpy(kaddr, uaddr, len);
	else
		rumpuser_sp_copyin(uaddr, kaddr, len);
	if (done)
		*done = strlen(kaddr)+1; /* includes termination */
	return 0;
}

int
copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done)
{

	if (curproc->p_vmspace == &vmspace0)
		strlcpy(uaddr, kaddr, len);
	else
		rumpuser_sp_copyout(kaddr, uaddr, len);
	if (done)
		*done = strlen(uaddr)+1; /* includes termination */
	return 0;
}

int
kcopy(const void *src, void *dst, size_t len)
{

	memcpy(dst, src, len);
	return 0;
}
