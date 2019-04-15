/*	$NetBSD: ufetchstore_tester.c,v 1.1 2019/04/15 23:41:23 christos Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/types.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include "common.h"

static struct tester_ctx {
	struct sysctllog *ctx_sysctllog;
} tester_ctx;

static int
test_ufetch(void * const uaddr, struct ufetchstore_test_args * const args)
{
	int error = 0;

	switch (args->size) {
	case 8:
		args->fetchstore_error = ufetch_8(uaddr, &args->val8);
		break;
	case 16:
		args->fetchstore_error = ufetch_16(uaddr, &args->val16);
		break;
	case 32:
		args->fetchstore_error = ufetch_32(uaddr, &args->val32);
		break;
#ifdef _LP64
	case 64:
		args->fetchstore_error = ufetch_64(uaddr, &args->val64);
		break;
#endif /* _LP64 */
	default:
		error = EINVAL;
	}

	return error;
}

static int
test_ustore(void * const uaddr, struct ufetchstore_test_args * const args)
{
	int error = 0;

	switch (args->size) {
	case 8:
		args->fetchstore_error = ustore_8(uaddr, args->val8);
		break;
	case 16:
		args->fetchstore_error = ustore_16(uaddr, args->val16);
		break;
	case 32:
		args->fetchstore_error = ustore_32(uaddr, args->val32);
		break;
#ifdef _LP64
	case 64:
		args->fetchstore_error = ustore_64(uaddr, args->val64);
		break;
#endif /* _LP64 */
	default:
		error = EINVAL;
	}

	return error;
}

static int
test_ucas(void * const uaddr, struct ufetchstore_test_args * const args)
{
	int error = 0;

	switch (args->size) {
	case 32:
		args->fetchstore_error = ucas_32(uaddr,
		    args->ea_val32, args->val32, &args->ea_val32);
		break;
#ifdef _LP64
	case 64:
		args->fetchstore_error = ucas_64(uaddr,
		    args->ea_val64, args->val64, &args->ea_val64);
		break;
#endif /* _LP64 */
	default:
		error = EINVAL;
	}

	return error;
}

static int
do_ufetchstore_test(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct ufetchstore_test_args *uargs, args;
	uint64_t args64;
	int error;

	node = *rnode;

	uargs = NULL;
	node.sysctl_data = &args64;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error)
		return error;
	if (newp == NULL)
		return EINVAL;

	uargs = (void *)(uintptr_t)args64;

	error = copyin(uargs, &args, sizeof(args));
	if (error)
		return error;

	args.fetchstore_error = EBADF;	/* poison */

	void *uaddr = (void *)(uintptr_t)args.uaddr64;

	switch (args.test_op) {
	case OP_LOAD:
		error = test_ufetch(uaddr, &args);
		break;

	case OP_STORE:
		error = test_ustore(uaddr, &args);
		break;

	case OP_CAS:
		error = test_ucas(uaddr, &args);
		break;

	default:
		error = EINVAL;
	}

	if (error == 0)
		error = copyout(&args, uargs, sizeof(args));
	return error;
}

static int
ufetchstore_tester_init(void)
{
	struct sysctllog **log = &tester_ctx.ctx_sysctllog;
	const struct sysctlnode *rnode, *cnode;
	int error;

	error = sysctl_createv(log, 0, NULL, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "ufetchstore_test",
	    SYSCTL_DESCR("ufetchstore testing interface"),
	    NULL, 0, NULL, 0, CTL_KERN, CTL_CREATE, CTL_EOL);
	if (error)
		goto return_error;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    /*
	     * It's really a pointer to our argument structure, because
	     * we want to have precise control over when copyin / copyout
	     * happens.
	     */
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_QUAD, "test",
	    SYSCTL_DESCR("execute a ufetchstore test"),
	    do_ufetchstore_test, 0,
	    (void *)&tester_ctx, 0, CTL_CREATE, CTL_EOL);

 return_error:
 	if (error)
		sysctl_teardown(log);
	return error;
}

static int
ufetchstore_tester_fini(void)
{
	sysctl_teardown(&tester_ctx.ctx_sysctllog);
	return 0;
}

static int
ufetchstore_tester_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = ufetchstore_tester_init();
		break;
	
	case MODULE_CMD_FINI:
		error = ufetchstore_tester_fini();
		break;
	
	case MODULE_CMD_STAT:
	default:
		error = ENOTTY;
	}

	return error;
}

MODULE(MODULE_CLASS_MISC, ufetchstore_tester, NULL);
