/*	$NetBSD: db_lwp.c,v 1.8 2026/05/11 13:44:35 thorpej Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_lwp.c,v 1.8 2026/05/11 13:44:35 thorpej Exp $");

#ifndef _KERNEL
#include <stdbool.h>
#endif

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/cpu.h>

#include <machine/pcb.h>

#include <ddb/ddb.h>

#if !defined(_KERNEL) && !defined(USPACE)
#include <sys/sysctl.h>
#include <unistd.h>

/*
 * USPACE is needed by KSTACK_SIZE.
 *
 * XXX Rather than getting it from the running system, we should exctract
 * it from the kernel crash dump.
 */
static int
getuspace(void)
{
	static int uspace;

	int mib[2];
	size_t size;

	if (uspace) {
		return uspace;
	}

	mib[0] = CTL_VM;
	mib[1] = VM_USPACE;
	size = sizeof(uspace);
	if (sysctl(mib, 2, &uspace, &size, NULL, 0)) {
		uspace = getpagesize();
	}

	return uspace;
}
#define	USPACE		(getuspace())
#endif /* ! _KERNEL && ! USPACE */

lwp_t *
db_lwp_first(void)
{

	return db_read_ptr("alllwp");
}

lwp_t *
db_lwp_next(lwp_t *l)
{

	db_read_bytes((db_addr_t)&l->l_list.le_next, sizeof(l), (char *)&l);
	return l;
}

void
db_lwp_whatis(uintptr_t addr, void (*pr)(const char *, ...))
{
	uintptr_t stack;
	lwp_t l, *lp;

	for (lp = db_lwp_first(); lp != NULL; lp = db_lwp_next(lp)) {
		db_read_bytes((db_addr_t)lp, sizeof(l), (char *)&l);
		stack = (uintptr_t)KSTACK_LOWEST_ADDR((&l));
		if (addr < stack || stack + KSTACK_SIZE <= addr) {
			continue;
		}
		(*pr)("%p is %p+%zu, LWP %p's stack\n",
		    (void *)addr, (void *)stack,
		    (size_t)(addr - stack), lp);
	}
}
