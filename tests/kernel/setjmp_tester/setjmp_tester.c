/*	$NetBSD: setjmp_tester.c,v 1.1 2025/04/27 16:22:26 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: setjmp_tester.c,v 1.1 2025/04/27 16:22:26 riastradh Exp $");

#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

MODULE(MODULE_CLASS_MISC, setjmp_tester, NULL);

static struct sysctllog *setjmp_tester_sysctllog;
static const struct sysctlnode *setjmp_tester_sysctlnode;
static kmutex_t setjmp_tester_lock;
static bool setjmp_tester_done;
static label_t setjmp_tester_label;

__noinline
static void
setjmp_tester_subroutine(void)
{

	printf("%s: call longjmp\n", __func__);
	setjmp_tester_done = true;
	longjmp(&setjmp_tester_label);
	printf("%s: unreachable\n", __func__);
}

static int
setjmp_tester_test(void)
{
	int result;

	mutex_enter(&setjmp_tester_lock);

	setjmp_tester_done = false;
	result = setjmp(&setjmp_tester_label);
	if (!setjmp_tester_done) {
		printf("%s: setjmp returned %d at first\n", __func__, result);
		if (result != 0) {
			result = -1;
			goto out;
		}
		setjmp_tester_subroutine();
		/*NOTREACHED*/
		printf("%s: setjmp_tester_subroutine returned\n", __func__);
		result = -1;
	} else {
		printf("%s: setjmp returned %d at second\n", __func__, result);
		if (result == 0) {
			result = -2;
			goto out;
		}
	}

out:	mutex_exit(&setjmp_tester_lock);
	return result;
}

static int
setjmp_tester_test_sysctl(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	int v = 0;
	int error;

	if (newp == NULL) {
		error = ENOENT;
		goto out;
	}
	error = sysctl_copyin(curlwp, newp, &v, sizeof(v));
	if (error)
		goto out;
	switch (v) {
	case 1:
		v = setjmp_tester_test();
		break;
	default:
		error = EINVAL;
		break;
	}
	node.sysctl_data = &v;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

out:	return error;
}

static int
setjmp_tester_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		mutex_init(&setjmp_tester_lock, MUTEX_DEFAULT, IPL_NONE);
		error = sysctl_createv(&setjmp_tester_sysctllog, 0,
		    NULL, &setjmp_tester_sysctlnode,
		    CTLFLAG_PERMANENT, CTLTYPE_NODE, "setjmp_tester",
		    SYSCTL_DESCR("setjmp/longjmp testing interface"),
		    NULL, 0, NULL, 0,
		    CTL_KERN, CTL_CREATE, CTL_EOL);
		if (error)
			goto fini;
		error = sysctl_createv(&setjmp_tester_sysctllog, 0,
		    &setjmp_tester_sysctlnode, NULL,
		    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "test",
		    SYSCTL_DESCR("setjmp/longjmp test trigger"),
		    &setjmp_tester_test_sysctl, 0, NULL, 0,
		    CTL_CREATE, CTL_EOL);
		if (error)
			goto fini;
		return error;
	case MODULE_CMD_FINI:
	fini:
		sysctl_teardown(&setjmp_tester_sysctllog);
		mutex_destroy(&setjmp_tester_lock);
		return error;
	default:
		return ENOTTY;
	}
}
