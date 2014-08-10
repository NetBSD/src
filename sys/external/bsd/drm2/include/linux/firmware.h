/*	$NetBSD: firmware.h,v 1.2.2.1 2014/08/10 06:55:39 tls Exp $	*/

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
#include <sys/systm.h>

#include <dev/firmload.h>

struct device;

struct firmware {
	void			*data;
	size_t			size;
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
release_firmware(const struct firmware *fw)
{

	if (fw != NULL) {
		firmware_free(fw->data, fw->size);
		kmem_free(__UNCONST(fw), sizeof(*fw));
	}
}

#endif  /* _LINUX_FIRMWARE_H_ */
