/* $NetBSD: copyinoutstr.c,v 1.2 2000/06/26 14:59:03 mrg Exp $ */

/*-
 * Copyright (c) 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * copyinoutstr.c -- copy strings with sanity checking
 */

#include <sys/param.h>

__RCSID("$NetBSD: copyinoutstr.c,v 1.2 2000/06/26 14:59:03 mrg Exp $");

#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <machine/pcb.h>
#include <uvm/uvm_param.h>

/*
 * FIXME: These are bogus.  They should use LDRT and friends for user
 * accesses.
 */

int copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{
	int err;

	/* Check if this is an easy case */
	if (uaddr < (void *)VM_MIN_ADDRESS ||
	    uaddr >= (void *)VM_MAXUSER_ADDRESS)
		return EFAULT;
	if (uaddr + len < (void *)VM_MAXUSER_ADDRESS)
		return copystr(uaddr, kaddr, len, done);
	err = copystr(uaddr, kaddr, (void *)VM_MAXUSER_ADDRESS - uaddr, done);
	if (err == ENAMETOOLONG)
		return EFAULT;
	return err;
}

int copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done)
{
	int err;

	/* Check if this is an easy case */
	if (uaddr < (void *)VM_MIN_ADDRESS ||
	    uaddr > (void *)VM_MAXUSER_ADDRESS)
		return EFAULT;
	if (uaddr + len < (void *)VM_MAXUSER_ADDRESS)
		return copystr(kaddr, uaddr, len, done);
	err = copystr(kaddr, uaddr, (void *)VM_MAXUSER_ADDRESS - uaddr, done);
	if (err == ENAMETOOLONG)
		return EFAULT;
	return err;
}

int copystr(const void *src, void *dest, size_t len, size_t *done)
{
	label_t here;
	size_t count;
	int c;
	char const *s;
	char *d;

	if (setjmp(&here) == 0) {
		s = src; d = dest;
		curproc->p_addr->u_pcb.pcb_onfault = &here;
		c = -1;
		*done = count = 0;
		while (count < len) {
			c = d[count] = s[count];
			*done = ++count;
			if (c == 0)
				break;
		}
		curproc->p_addr->u_pcb.pcb_onfault = NULL;
		if (c == 0)
			return 0;
		return ENAMETOOLONG;
	} else {
		curproc->p_addr->u_pcb.pcb_onfault = NULL;
		return EFAULT;
	}
}
