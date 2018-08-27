/*	$NetBSD: firmware.h,v 1.9 2018/08/27 13:40:15 riastradh Exp $	*/

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

#ifndef _LINUX_FIRMWARE_H_
#define _LINUX_FIRMWARE_H_

#include <sys/types.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/systm.h>

#include <dev/firmload.h>

#include <linux/slab.h>
#include <linux/string.h>
#include <linux/workqueue.h>

struct device;

struct firmware {
	char			*data;
	size_t			size;
};

struct firmload_work {
	char		*flw_name;
	void		(*flw_callback)(const struct firmware *, void *);
	void		*flw_cookie;
	struct device	*flw_device;
	struct module	*flw_module;
	struct work_struct flw_work;
};

static inline int
request_firmware(const struct firmware **fwp, const char *image_name,
    struct device *dev)
{
	const char *drvname;
	struct firmware *fw;
	firmware_handle_t handle;
	int ret;

	fw = kmem_alloc(sizeof(*fw), KM_SLEEP);

	/*
	 * If driver xyz(4) asks for xyz/foo/bar.bin, turn that into
	 * just foo/bar.bin.  This leaves open the possibility of name
	 * collisions.  Let's hope upstream is sensible about this.
	 */
	drvname = device_cfdriver(dev)->cd_name;
	if ((strncmp(drvname, image_name, strlen(drvname)) == 0) &&
	    (image_name[strlen(drvname)] == '/'))
		image_name += (strlen(drvname) + 1);

	/* XXX errno NetBSD->Linux */
	ret = -firmware_open(drvname, image_name, &handle);
	if (ret)
		goto fail0;
	fw->size = firmware_get_size(handle);
	fw->data = firmware_malloc(fw->size);

	/* XXX errno NetBSD->Linux */
	ret = -firmware_read(handle, 0, fw->data, fw->size);
	(void)firmware_close(handle);
	if (ret)
		goto fail1;

	/* Success!  */
	*fwp = fw;
	return 0;

fail1:	firmware_free(fw->data, fw->size);
fail0:	KASSERT(ret);
	kmem_free(fw, sizeof(*fw));
	*fwp = NULL;
	return ret;
}

static inline void
request_firmware_work(struct work_struct *wk)
{
	struct firmload_work *work = container_of(wk, struct firmload_work,
	    flw_work);
	const struct firmware *fw;
	int ret;

	/* Reqeust the firmware.  If it failed, set it to NULL.  */
	ret = request_firmware(&fw, work->flw_name, work->flw_device);
	if (ret)
		fw = NULL;

	/* Call the callback. */
	(*work->flw_callback)(fw, work->flw_cookie);

	/*
	 * Release the device and module references now that we're
	 * done.
	 *
	 * XXX Heh. What if the module gets unloaded _during_
	 * module_rele because it went to zero?
	 */
	/* XXX device_release */
	if (work->flw_module)
		module_rele(work->flw_module);
}

static inline int
request_firmware_nowait(struct module *module, bool uevent, const char *name,
    struct device *device, gfp_t gfp, void *cookie,
    void (*callback)(const struct firmware *, void *))
{
	char *namedup;
	struct firmload_work *work;

	/* Allocate memory for it, or fail if we can't.  */
	work = kzalloc(sizeof(*work), gfp);
	if (work == NULL)
		goto fail0;

	/* Copy the name just in case.  */
	namedup = kstrdup(name, gfp);
	if (namedup == NULL)
		goto fail1;

	/* Hold the module and device so they don't go away before callback. */
	if (module)
		module_hold(module);
	/* XXX device_acquire(device) */

	/* Initialize the work.  */
	work->flw_name = namedup;
	work->flw_callback = callback;
	work->flw_cookie = cookie;
	work->flw_device = device;
	work->flw_module = module;
	INIT_WORK(&work->flw_work, request_firmware_work);

	/* Kick it off.  */
	schedule_work(&work->flw_work);

	/* Success!  */
	return 0;

fail1:	kfree(work);
fail0:	return -ENOMEM;
}

static inline void
release_firmware(const struct firmware *fw)
{

	if (fw != NULL) {
		firmware_free(fw->data, fw->size);
		kmem_free(__UNCONST(fw), sizeof(*fw));
	}
}

#endif  /* _LINUX_FIRMWARE_H_ */
