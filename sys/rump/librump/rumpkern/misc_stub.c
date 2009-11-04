/*	$NetBSD: misc_stub.c,v 1.25 2009/11/04 18:11:11 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
__KERNEL_RCSID(0, "$NetBSD: misc_stub.c,v 1.25 2009/11/04 18:11:11 pooka Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/evcnt.h>
#include <sys/event.h>
#include <sys/kauth.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/syscallvar.h>
#include <sys/xcall.h>

#ifdef __sparc__
 /* 
  * XXX Least common denominator - smallest sparc pagesize.
  * Could just be declared, pooka says rump doesn't use ioctl.
  */
int nbpg = 4096;
#endif

int
syscall_establish(const struct emul *em, const struct syscall_package *sp)
{
	extern struct sysent rump_sysent[];
	int i;

	KASSERT(em == NULL || em == &emul_netbsd);

	for (i = 0; sp[i].sp_call; i++)
		rump_sysent[sp[i].sp_code].sy_call = sp[i].sp_call;

	return 0;
}

int
syscall_disestablish(const struct emul *em, const struct syscall_package *sp)
{

	return 0;
}

/* crosscalls not done, no other hardware CPUs */
uint64_t
xc_broadcast(u_int flags, xcfunc_t func, void *arg1, void *arg2)
{

	return -1;
}

void
xc_wait(uint64_t where)
{

}

