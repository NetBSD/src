/*	$NetBSD: linux_module.c,v 1.3 2014/07/16 20:56:25 riastradh Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__KERNEL_RCSID(0, "$NetBSD: linux_module.c,v 1.3 2014/07/16 20:56:25 riastradh Exp $");

#include <sys/module.h>
#ifndef _MODULE
#include <sys/once.h>
#endif

#include <linux/highmem.h>
#include <linux/idr.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>

MODULE(MODULE_CLASS_MISC, drmkms_linux, NULL);

static int
linux_init(void)
{
	int error;

	error = linux_idr_module_init();
	if (error) {
		printf("linux: unable to initialize idr: %d\n", error);
		goto fail0;
	}

	error = linux_kmap_init();
	if (error) {
		printf("linux: unable to initialize kmap: %d\n", error);
		goto fail1;
	}

	error = linux_workqueue_init();
	if (error) {
		printf("linux: unable to initialize workqueues: %d\n", error);
		goto fail2;
	}

	error = linux_writecomb_init();
	if (error) {
		printf("linux: unable to initialize write-combining: %d\n",
		    error);
		goto fail3;
	}

	return 0;

fail4: __unused
	linux_writecomb_fini();
fail3:	linux_workqueue_fini();
fail2:	linux_kmap_fini();
fail1:	linux_idr_module_fini();
fail0:	return error;
}

int	linux_guarantee_initialized(void); /* XXX */
int
linux_guarantee_initialized(void)
{
#ifdef _MODULE
	return 0;
#else
	static ONCE_DECL(linux_init_once);

	return RUN_ONCE(&linux_init_once, &linux_init);
#endif
}

static void
linux_fini(void)
{

	linux_writecomb_fini();
	linux_workqueue_fini();
	linux_kmap_fini();
}

static int
drmkms_linux_modcmd(modcmd_t cmd, void *arg __unused)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		return linux_init();
#else
		return linux_guarantee_initialized();
#endif
	case MODULE_CMD_FINI:
		linux_fini();
		return 0;
	default:
		return ENOTTY;
	}
}
