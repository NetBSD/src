/*	$NetBSD: fiq.c,v 1.2.2.3 2001/09/13 01:13:11 thorpej Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: fiq.c,v 1.2.2.3 2001/09/13 01:13:11 thorpej Exp $");

#include <sys/systm.h>

#include <machine/fiq.h>

int fiq_claimed;

extern char fiqhandler[];

void (*fiq_downgrade_handler)(void);

int
fiq_claim(void *handler, size_t size)
{
	int s;

	if (size > 0x100)
		return -1;
	s = splhigh();
	if (fiq_claimed) {
		splx(s);
		return -1;
	}
	fiq_claimed = 1;
	splx(s);
	fiq_installhandler(handler, size);
	return 0;
}

void
fiq_installhandler(void *handler, size_t size)
{

	memcpy(fiqhandler, handler, size);
}

void
fiq_release()
{

	KASSERT(fiq_claimed);
	fiq_claimed = 0;
}


