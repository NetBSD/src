/*	$NetBSD: linux_module.c,v 1.2 2014/03/18 18:20:43 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_module.c,v 1.2 2014/03/18 18:20:43 riastradh Exp $");

#include <sys/module.h>

#include <linux/highmem.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>

MODULE(MODULE_CLASS_MISC, drmkms_linux, NULL);

#ifndef _MODULE
/*
 * XXX Mega-kludge.  See drm_init in drm_drv.c for details.
 */
int linux_suppress_init = 0;
#endif

static int
drmkms_linux_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifndef _MODULE
		if (linux_suppress_init)
			return 0;
#endif
		error = linux_kmap_init();
		if (error) {
			aprint_error("drmkms_linux:"
			    " unable to initialize linux kmap: %d",
			    error);
			goto init_fail0;
		}
		error = linux_workqueue_init();
		if (error) {
			aprint_error("drmkms_linux:"
			    " unable to initialize workqueues: %d",
			    error);
			goto init_fail1;
		}
		return 0;

init_fail1:	linux_kmap_fini();
init_fail0:	return error;

	case MODULE_CMD_FINI:
		linux_workqueue_fini();
		linux_kmap_fini();
		return 0;

	default:
		return ENOTTY;
	}
}
