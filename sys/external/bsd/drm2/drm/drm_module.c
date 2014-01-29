/*	$NetBSD: drm_module.c,v 1.1.2.9 2014/01/29 19:47:38 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_module.c,v 1.1.2.9 2014/01/29 19:47:38 riastradh Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/systm.h>

#include <linux/highmem.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>

#include <drm/drmP.h>

/*
 * XXX I2C stuff should be moved to a separate drmkms_edid module.
 *
 * XXX PCI stuff should be moved to a separate drmkms_pci module.
 *
 * XXX Other Linux stuff should be moved to a linux compatibility
 * module on which this one depends.
 */
MODULE(MODULE_CLASS_DRIVER, drmkms, "iic,pci");

#ifdef _MODULE
#include "ioconf.c"
extern const struct cdevsw drmkms_cdevsw; /* XXX */
#endif

static int
drmkms_modcmd(modcmd_t cmd, void *arg __unused)
{
#ifdef _MODULE
	devmajor_t bmajor = NODEVMAJOR, cmajor = NODEVMAJOR;
	int error;
#endif

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		linux_mutex_init(&drm_global_mutex);
		error = linux_kmap_init();
		if (error) {
			aprint_error("drmkms: unable to initialize linux kmap:"
			    " %d", error);
			goto init_fail0;
		}
		error = linux_workqueue_init();
		if (error) {
			aprint_error("drmkms: unable to initialize workqueues:"
			    " %d", error);
			goto init_fail1;
		}
		error = config_init_component(cfdriver_ioconf_drmkms,
		    cfattach_ioconf_drmkms, cfdata_ioconf_drmkms);
		if (error) {
			aprint_error("drmkms: unable to init component: %d\n",
			    error);
			goto init_fail2;
		}
		error = devsw_attach("drm", NULL, &bmajor,
		    &drmkms_cdevsw, &cmajor);
		if (error) {
			aprint_error("drmkms: unable to attach devsw: %d\n",
			    error);
			goto init_fail3;
		}
#endif
		return 0;

#ifdef _MODULE
#if 0
init_fail4:	(void)devsw_detach(NULL, &drmkms_cdevsw);
#endif
init_fail3:	(void)config_fini_component(cfdriver_ioconf_drmkms,
		    cfattach_ioconf_drmkms, cfdata_ioconf_drmkms);
init_fail2:	linux_workqueue_fini();
init_fail1:	linux_kmap_fini();
init_fail0:	linux_mutex_destroy(&drm_global_mutex);
		return error;
#endif	/* _MODULE */

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = devsw_detach(NULL, &drmkms_cdevsw);
		if (error)
			return error;
		error = config_fini_component(cfdriver_ioconf_drmkms,
		    cfattach_ioconf_drmkms, cfdata_ioconf_drmkms);
		if (error)
			/* XXX Now what?  Reattach the devsw?  */
			return error;
		linux_workqueue_fini();
		linux_kmap_fini();
		linux_mutex_destroy(&drm_global_mutex);
#endif
		return 0;

	default:
		return ENOTTY;
	}
}
