/*	$NetBSD: sys_process_lwpstatus.c,v 1.1 2020/01/04 03:46:19 kamil Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: sys_process_lwpstatus.c,v 1.1 2020/01/04 03:46:19 kamil Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/lwp.h>
#include <sys/ptrace.h>


void
ptrace_read_lwpstatus(struct lwp *l, struct ptrace_lwpstatus *pls)
{

	KASSERT(l->l_lid == pls->pl_lwpid);

	memcpy(&pls->pl_sigmask, &l->l_sigmask, sizeof(pls->pl_sigmask));
	memcpy(&pls->pl_sigpend, &l->l_sigpend.sp_set, sizeof(pls->pl_sigpend));

	if (l->l_name == NULL)
		memset(&pls->pl_name, 0, PL_LNAMELEN);
	else {
		KASSERT(strlen(l->l_name) < PL_LNAMELEN);
		strncpy(pls->pl_name, l->l_name, PL_LNAMELEN);
	}

#ifdef PTRACE_LWP_GETPRIVATE
	pls->pl_private = (void *)(intptr_t)PTRACE_LWP_GETPRIVATE(l);
#else
	pls->pl_private = l->l_private;
#endif
}

void
process_read_lwpstatus(struct lwp *l, struct ptrace_lwpstatus *pls)
{

	pls->pl_lwpid = l->l_lid;

	ptrace_read_lwpstatus(l, pls);
}
