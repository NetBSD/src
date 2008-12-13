/*	$NetBSD: misc_stub.c,v 1.10.2.2 2008/12/13 01:15:34 haad Exp $	*/

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

#include <sys/param.h>
#include <sys/event.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/syscallvar.h>
#include <sys/cpu.h>
#include <sys/evcnt.h>

#ifdef __sparc__
 /* 
  * XXX Least common denominator - smallest sparc pagesize.
  * Could just be declared, pooka says rump doesn't use ioctl.
  */
int nbpg = 4096;
#endif

void
yield(void)
{

	/*
	 * Do nothing - doesn't really make sense as we're being
	 * scheduled anyway.
	 */
	return;
}

void
preempt()
{

	/* see yield */
	return;
}

void
knote(struct klist *list, long hint)
{

	return;
}

struct cpu_info *
cpu_lookup(u_int index)
{
	extern struct cpu_info rump_cpu;

	return &rump_cpu;
}

void
evcnt_attach_dynamic(struct evcnt *ev, int type, const struct evcnt *parent,
    const char *group, const char *name)
{

}

void
evcnt_detach(struct evcnt *ev)
{

}

int
syscall_establish(const struct emul *em, const struct syscall_package *sp)
{

	return 0;
}

int
syscall_disestablish(const struct emul *em, const struct syscall_package *sp)
{

	return 0;
}
