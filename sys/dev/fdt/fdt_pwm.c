/* $NetBSD: fdt_pwm.c,v 1.1.2.3 2018/07/28 04:37:44 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: fdt_pwm.c,v 1.1.2.3 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_pwm_controller {
	device_t pc_dev;
	int pc_phandle;
	const struct fdtbus_pwm_controller_func *pc_funcs;

	LIST_ENTRY(fdtbus_pwm_controller) pc_next;
};

static LIST_HEAD(, fdtbus_pwm_controller) fdtbus_pwm_controllers =
    LIST_HEAD_INITIALIZER(fdtbus_pwm_controllers);

int
fdtbus_register_pwm_controller(device_t dev, int phandle,
    const struct fdtbus_pwm_controller_func *funcs)
{
	struct fdtbus_pwm_controller *pc;

	pc = kmem_alloc(sizeof(*pc), KM_SLEEP);
	pc->pc_dev = dev;
	pc->pc_phandle = phandle;
	pc->pc_funcs = funcs;

	LIST_INSERT_HEAD(&fdtbus_pwm_controllers, pc, pc_next);

	return 0;
}

static struct fdtbus_pwm_controller *
fdtbus_get_pwm_controller(int phandle)
{
	struct fdtbus_pwm_controller *pc;

	LIST_FOREACH(pc, &fdtbus_pwm_controllers, pc_next) {
		if (pc->pc_phandle == phandle)
			return pc;
	}

	return NULL;
}

pwm_tag_t
fdtbus_pwm_acquire(int phandle, const char *prop)
{
	return fdtbus_pwm_acquire_index(phandle, prop, 0);
}

pwm_tag_t
fdtbus_pwm_acquire_index(int phandle, const char *prop, int index)
{
	struct fdtbus_pwm_controller *pc;
	const uint32_t *pwms, *p;
	u_int n, pwm_cells;
	int len, resid;

	pwms = fdtbus_get_prop(phandle, prop, &len);
	if (pwms == NULL)
		return NULL;

	p = pwms;
	for (n = 0, resid = len; resid > 0; n++) {
		const int pc_phandle =
		    fdtbus_get_phandle_from_native(be32toh(p[0]));
		if (of_getprop_uint32(pc_phandle, "#pwm-cells", &pwm_cells))
			break;
		if (n == index) {
			pc = fdtbus_get_pwm_controller(pc_phandle);
			if (pc == NULL)
				return NULL;
			return pc->pc_funcs->get_tag(pc->pc_dev,
			    &p[0], (pwm_cells + 1) * 4);
		}
		resid -= (pwm_cells + 1) * 4;
		p += pwm_cells + 1;
	}

	return NULL;
}
