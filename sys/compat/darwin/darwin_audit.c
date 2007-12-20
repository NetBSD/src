/*	$NetBSD: darwin_audit.c,v 1.7 2007/12/20 23:02:45 dsl Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: darwin_audit.c,v 1.7 2007/12/20 23:02:45 dsl Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/ktrace.h>
#include <sys/syscallargs.h>

#include <compat/sys/signal.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_audit.h>
#include <compat/darwin/darwin_syscallargs.h>

int
darwin_sys_audit(struct lwp *l, const struct darwin_sys_audit_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) record;
		syscallarg(size_t) len;
	} */

	uap = NULL; /* Shut up a warning */
	printf("unimplemented darwin_sys_audit()\n");

	return 0;
}

int
darwin_sys_auditon(struct lwp *l, const struct darwin_sys_auditon_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) cmd;
		syscallarg(void *) data;
		syscallarg(size_t) len;
	} */

	uap = NULL; /* Shut up a warning */
	printf("unimplemented darwin_sys_auditon()\n");

	return 0;
}

int
darwin_sys_getauid(struct lwp *l, const struct darwin_sys_getauid_args *uap, register_t *retval)
{
	/* {
		syscallarg(darwin_au_id_t *) auid;
	} */

	uap = NULL; /* Shut up a warning */
	printf("unimplemented darwin_sys_getauid()\n");

	return 0;
}

int
darwin_sys_setauid(struct lwp *l, const struct darwin_sys_setauid_args *uap, register_t *retval)
{
	/* {
		syscallarg(darwin_au_id_t *) auid;
	} */

	uap = NULL; /* Shut up a warning */
	printf("unimplemented darwin_sys_setauid()\n");

	return 0;
}

int
darwin_sys_getauditinfo(struct lwp *l, const struct darwin_sys_getauditinfo_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct darwin_auditinfo *) auditinfo;
	} */

	uap = NULL; /* Shut up a warning */
	printf("unimplemented darwin_sys_getauditinfo()\n");

	return 0;
}

int
darwin_sys_setauditinfo(struct lwp *l, const struct darwin_sys_setauditinfo_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct darwin_auditinfo *) auditinfo;
	} */

	uap = NULL; /* Shut up a warning */
	printf("unimplemented darwin_sys_setauditinfo()\n");

	return 0;
}

int
darwin_sys_getaudit_addr(struct lwp *l, const struct darwin_sys_getaudit_addr_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct darwin_audit_addr *) auditinfo_addr;
		syscallarg(int) len;
	} */

	uap = NULL; /* Shut up a warning */
	printf("unimplemented darwin_sys_getaudit_addr()\n");

	return 0;
}

int
darwin_sys_setaudit_addr(struct lwp *l, const struct darwin_sys_setaudit_addr_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct darwin_audit_addr *) auditinfo_addr;
		syscallarg(int) len;
	} */

	uap = NULL; /* Shut up a warning */
	printf("unimplemented darwin_sys_setaudit_addr()\n");

	return 0;
}

int
darwin_sys_auditctl(struct lwp *l, const struct darwin_sys_auditctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) path;
	} */

	uap = NULL; /* Shut up a warning */
	printf("unimplemented darwin_sys_auditctl()\n");

	return 0;
}
