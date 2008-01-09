/* $NetBSD: osf1_sysv_ipc.c,v 1.5.20.1 2008/01/09 01:51:45 matt Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: osf1_sysv_ipc.c,v 1.5.20.1 2008/01/09 01:51:45 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/osf1/osf1_cvt.h>

int
osf1_sys_shmat(struct lwp *l, const struct osf1_sys_shmat_args *v, register_t *retval)
{

	/* XXX */
	return (ENOSYS);
}

int
osf1_sys_shmctl(struct lwp *l, const struct osf1_sys_shmctl_args *v, register_t *retval)
{

	/* XXX */
	return (ENOSYS);
}

int
osf1_sys_shmdt(struct lwp *l, const struct osf1_sys_shmdt_args *v, register_t *retval)
{

	/* XXX */
	return (ENOSYS);
}

int
osf1_sys_shmget(struct lwp *l, const struct osf1_sys_shmget_args *v, register_t *retval)
{

	/* XXX */
	return (ENOSYS);
}
