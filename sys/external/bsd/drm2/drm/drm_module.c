/*	$NetBSD: drm_module.c,v 1.1.2.7 2013/12/30 04:50:12 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: drm_module.c,v 1.1.2.7 2013/12/30 04:50:12 riastradh Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/systm.h>

#include <linux/highmem.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>

#include <drm/drmP.h>

/*
 * XXX I2C stuff should be moved to a separate drm2edid module.
 *
 * XXX PCI stuff should be moved to a separate drm2pci module.
 *
 * XXX Other Linux stuff should be moved to a linux compatibility
 * module on which this one depends.
 */
MODULE(MODULE_CLASS_MISC, drm2, "iic,pci");

#ifdef _MODULE
#include "ioconf.c"
#endif

extern const struct cdevsw drm_cdevsw; /* XXX Put this in a header file?  */

static int
drm2_modcmd(modcmd_t cmd, void *arg __unused)
{
#ifdef _MODULE
	devmajor_t bmajor = NODEVMAJOR, cmajor = NODEVMAJOR;
	int error;
#endif

	switch (cmd) {
	case MODULE_CMD_INIT:
		linux_mutex_init(&drm_global_mutex);
		error = linux_kmap_init();
		if (error) {
			aprint_error("drm: unable to initialize linux kmap:"
			    " %d", error);
			return error;
		}
		error = linux_workqueue_init();
		if (error) {
			aprint_error("drm: unable to initialize workqueues:"
			    " %d", error);
			linux_kmap_fini();
			return error;
		}
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_drm,
		    cfattach_ioconf_drm, cfdata_ioconf_drm);
		if (error) {
			aprint_error("drm: unable to init component: %d\n",
			    error);
			linux_workqueue_fini();
			linux_kmap_fini();
			return error;
		}
		error = devsw_attach("drm", NULL, &bmajor,
		    &drm_cdevsw, &cmajor);
		if (error) {
			aprint_error("drm: unable to attach devsw: %d\n",
			    error);
			(void)config_fini_component(cfdriver_ioconf_drm,
			    cfattach_ioconf_drm, cfdata_ioconf_drm);
			linux_workqueue_fini();
			linux_kmap_fini();
			return error;
		}
#endif
		return 0;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = devsw_detach(NULL, &drm_cdevsw);
		if (error)
			return error;
		error = config_fini_component(cfdriver_ioconf_drm,
		    cfattach_ioconf_drm, cfdata_ioconf_drm);
		if (error)
			/* XXX Now what?  Reattach the devsw?  */
			return error;
#endif
		linux_workqueue_fini();
		linux_kmap_fini();
		linux_mutex_destroy(&drm_global_mutex);
		return 0;

	default:
		return ENOTTY;
	}
}
