/*	$NetBSD: misc_stub.c,v 1.3.22.3 2008/01/19 12:15:39 bouyer Exp $	*/

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
#include <sys/module.h>

#ifdef __sparc__
 /* 
  * XXX Least common denominator - smallest sparc pagesize.
  * Could just be declared, pooka says rump doesn't use ioctl.
  */
int nbpg = 4096;
#endif

void
preempt()
{

	return;
}

void
knote(struct klist *list, long hint)
{

	return;
}

/* not tonight, honey */
int
sysctl_createv(struct sysctllog **log, int cflags,
	const struct sysctlnode **rnode, const struct sysctlnode **cnode,
	int flags, int type, const char *namep, const char *desc,
	sysctlfn func, u_quad_t qv, void *newp, size_t newlen, ...)
{

	return 0;
}

int
sysctl_notavail(SYSCTLFN_ARGS)
{

	return EOPNOTSUPP;
}

int
sysctl_lookup(SYSCTLFN_ARGS)
{

	return ENOSYS;
}

int
sysctl_query(SYSCTLFN_ARGS)
{

	return ENOSYS;
}

void
sysctl_lock(bool write)
{

}

void
sysctl_relock(void)
{

}

void
sysctl_unlock(void)
{

}

void
module_init_class(modclass_t mc)
{

}
