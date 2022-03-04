/* $NetBSD: fdt_mbox.c,v 1.1 2022/03/04 08:19:06 skrll Exp $ */

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: fdt_mbox.c,v 1.1 2022/03/04 08:19:06 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>


struct fdtbus_mbox_controller {
	device_t mc_dev;
	int mc_phandle;

	int mc_cells;

	const struct fdtbus_mbox_controller_func *mc_funcs;

	LIST_ENTRY(fdtbus_mbox_controller) mc_next;
};

static LIST_HEAD(, fdtbus_mbox_controller) fdtbus_mbox_controllers =
    LIST_HEAD_INITIALIZER(fdtbus_mbox_controllers);

int
fdtbus_register_mbox_controller(device_t dev, int phandle,
    const struct fdtbus_mbox_controller_func *funcs)
{
	struct fdtbus_mbox_controller *mc;

	uint32_t cells;
	if (of_getprop_uint32(phandle, "#mbox-cells", &cells) != 0) {
		aprint_debug_dev(dev, "missing #mbox-cells");
		return EINVAL;
	}

	mc = kmem_alloc(sizeof(*mc), KM_SLEEP);
	mc->mc_dev = dev;
	mc->mc_phandle = phandle;
	mc->mc_funcs = funcs;
	mc->mc_cells = cells;

	LIST_INSERT_HEAD(&fdtbus_mbox_controllers, mc, mc_next);

	return 0;
}

static struct fdtbus_mbox_controller *
fdtbus_mbox_lookup(int phandle)
{
	struct fdtbus_mbox_controller *mc;

	LIST_FOREACH(mc, &fdtbus_mbox_controllers, mc_next) {
		if (mc->mc_phandle == phandle)
			return mc;
	}

	return NULL;
}

struct fdtbus_mbox_channel *
fdtbus_mbox_get_index(int phandle, u_int index, void (*cb)(void *), void *cbarg)
{
	struct fdtbus_mbox_controller *mc;
	struct fdtbus_mbox_channel *mbox = NULL;
	void *mbox_priv = NULL;
	uint32_t *mboxes = NULL;
	uint32_t *p;
	u_int n;
	int len, resid;

	len = OF_getproplen(phandle, "mboxes");
	if (len <= 0)
		return NULL;

	mboxes = kmem_alloc(len, KM_SLEEP);
	if (OF_getprop(phandle, "mboxes", mboxes, len) != len) {
		kmem_free(mboxes, len);
		return NULL;
	}

	p = mboxes;
	for (n = 0, resid = len; resid > 0; n++) {
		const int mc_phandle =
		    fdtbus_get_phandle_from_native(be32toh(p[0]));

		mc = fdtbus_mbox_lookup(mc_phandle);
		if (mc == NULL)
			break;

		u_int mbox_cells = mc->mc_cells;
		if (n == index) {
			mbox_priv = mc->mc_funcs->mc_acquire(mc->mc_dev,
			    mbox_cells > 0 ? &p[1] : NULL, mbox_cells * 4,
			    cb, cbarg);
			if (mbox_priv) {
				mbox = kmem_alloc(sizeof(*mbox), KM_SLEEP);
				mbox->mb_ctlr = mc;
				mbox->mb_priv = mbox_priv;
			}
			break;
		}
		resid -= (mbox_cells + 1) * 4;
		p += mbox_cells + 1;
	}

	if (mboxes)
		kmem_free(mboxes, len);

	return mbox;
}

struct fdtbus_mbox_channel *
fdtbus_mbox_get(int phandle, const char *name, void (*cb)(void *), void *cbarg)
{
	u_int index;
	int err;

	err = fdtbus_get_index(phandle, "mbox-names", name, &index);
	if (err != 0)
		return NULL;

	return fdtbus_mbox_get_index(phandle, index, cb, cbarg);
}

void
fdtbus_mbox_put(struct fdtbus_mbox_channel *mbox)
{
	struct fdtbus_mbox_controller *mc = mbox->mb_ctlr;

	mc->mc_funcs->mc_release(mc->mc_dev, mbox->mb_priv);
	kmem_free(mbox, sizeof(*mbox));
}

int
fdtbus_mbox_send(struct fdtbus_mbox_channel *mbox, const void *data, size_t len)
{
	struct fdtbus_mbox_controller * const mc = mbox->mb_ctlr;

	return mc->mc_funcs->mc_send(mc->mc_dev, mbox->mb_priv, data, len);
}

int
fdtbus_mbox_recv(struct fdtbus_mbox_channel *mbox, void *data, size_t len)
{
	struct fdtbus_mbox_controller * const mc = mbox->mb_ctlr;

	return mc->mc_funcs->mc_recv(mc->mc_dev, mbox->mb_priv, data, len);
}
