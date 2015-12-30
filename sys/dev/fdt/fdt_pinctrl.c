/* $NetBSD: fdt_pinctrl.c,v 1.1 2015/12/30 04:23:39 marty Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_pinctrl.c,v 1.1 2015/12/30 04:23:39 marty Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_pinctrl_controller {
	device_t pc_dev;
	int pc_phandle;
	const struct fdtbus_pinctrl_controller_func *pc_funcs;

	struct fdtbus_pinctrl_controller *pc_next;
};

static struct fdtbus_pinctrl_controller *fdtbus_pc = NULL;

int
fdtbus_register_pinctrl_controller(device_t dev, int phandle,
    const struct fdtbus_pinctrl_controller_func *funcs)
{
	struct fdtbus_pinctrl_controller *pc;

	pc = kmem_alloc(sizeof(*pc), KM_SLEEP);
	pc->pc_dev = dev;
	pc->pc_phandle = phandle;
	pc->pc_funcs = funcs;

	pc->pc_next = fdtbus_pc;
	fdtbus_pc = pc;

	return 0;
}

struct fdtbus_pinctrl_pin *
fdtbus_pinctrl_acquire(int phandle, const char *prop)
{
	struct fdtbus_pinctrl_controller *pc;
	struct fdtbus_pinctrl_pin *gp;

	gp = kmem_alloc(sizeof(*gp), KM_SLEEP);
	for (pc = fdtbus_pc; pc; pc = pc->pc_next) {
		gp->pp_pc = pc;
		gp->pp_priv = pc->pc_funcs->acquire(pc->pc_dev, prop);
		if (gp->pp_priv != NULL)
			break;
	}

	if (gp->pp_priv == NULL) {
		kmem_free(gp, sizeof(*gp));
		return NULL;
	}

	return gp;
}

void
fdtbus_pinctrl_release(struct fdtbus_pinctrl_pin *gp)
{
	struct fdtbus_pinctrl_controller *pc = gp->pp_pc;

	pc->pc_funcs->release(pc->pc_dev, gp->pp_priv);
	kmem_free(gp, sizeof(*gp));
}

void
fdtbus_pinctrl_get_cfg(struct fdtbus_pinctrl_pin *gp, void *cookie)
{
	struct fdtbus_pinctrl_controller *pc = gp->pp_pc;

	pc->pc_funcs->get(gp, cookie);
}

void
fdtbus_pinctrl_set_cfg(struct fdtbus_pinctrl_pin *gp, void *cookie)
{
	struct fdtbus_pinctrl_controller *pc = gp->pp_pc;

	pc->pc_funcs->set(gp, cookie);
}
