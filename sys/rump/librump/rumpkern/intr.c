/*	$NetBSD: intr.c,v 1.4 2008/07/29 13:17:47 pooka Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/cpu.h>

struct v_dodgy {
	void	(*func)(void *);
	void	*arg;
};

void *
softint_establish(u_int flags, void (*func)(void *), void *arg)
{
	struct v_dodgy *vd;

	vd = kmem_alloc(sizeof(*vd), KM_SLEEP);
	if (vd != NULL) {
		vd->func = func;
		vd->arg = arg;
	}
	return vd;
}

void
softint_disestablish(void *arg)
{

	kmem_free(arg, sizeof(struct v_dodgy));
}

void
softint_schedule(void *arg)
{
	struct v_dodgy *vd;

	vd = arg;
	(*(vd->func))(arg);
}

bool
cpu_intr_p(void)
{

	return false;
}
