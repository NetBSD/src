/* $NetBSD: fdt_regulator.c,v 1.4.2.2 2017/12/03 11:37:01 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_regulator.c,v 1.4.2.2 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_regulator_controller {
	device_t rc_dev;
	int rc_phandle;
	const struct fdtbus_regulator_controller_func *rc_funcs;

	struct fdtbus_regulator_controller *rc_next;
};

static struct fdtbus_regulator_controller *fdtbus_rc = NULL;

int
fdtbus_register_regulator_controller(device_t dev, int phandle,
    const struct fdtbus_regulator_controller_func *funcs)
{
	struct fdtbus_regulator_controller *rc;

	rc = kmem_alloc(sizeof(*rc), KM_SLEEP);
	rc->rc_dev = dev;
	rc->rc_phandle = phandle;
	rc->rc_funcs = funcs;

	rc->rc_next = fdtbus_rc;
	fdtbus_rc = rc;

	return 0;
}

static struct fdtbus_regulator_controller *
fdtbus_get_regulator_controller(int phandle)
{
	struct fdtbus_regulator_controller *rc;

	for (rc = fdtbus_rc; rc; rc = rc->rc_next) {
		if (rc->rc_phandle == phandle) {
			return rc;
		}
	}

	return NULL;
}

struct fdtbus_regulator *
fdtbus_regulator_acquire(int phandle, const char *prop)
{
	struct fdtbus_regulator_controller *rc;
	struct fdtbus_regulator *reg;
	int regulator_phandle;
	int error;

	regulator_phandle = fdtbus_get_phandle(phandle, prop);
	if (regulator_phandle == -1) {
		return NULL;
	}

	rc = fdtbus_get_regulator_controller(regulator_phandle);
	if (rc == NULL) {
		return NULL;
	}

	error = rc->rc_funcs->acquire(rc->rc_dev);
	if (error) {
		aprint_error_dev(rc->rc_dev, "failed to acquire regulator: %d\n", error);
		return NULL;
	}

	reg = kmem_alloc(sizeof(*reg), KM_SLEEP);
	reg->reg_rc = rc;

	return reg;
}

void
fdtbus_regulator_release(struct fdtbus_regulator *reg)
{
	struct fdtbus_regulator_controller *rc = reg->reg_rc;

	rc->rc_funcs->release(rc->rc_dev);

	kmem_free(reg, sizeof(*reg));
}

int
fdtbus_regulator_enable(struct fdtbus_regulator *reg)
{
	struct fdtbus_regulator_controller *rc = reg->reg_rc;

	return rc->rc_funcs->enable(rc->rc_dev, true);
}

int
fdtbus_regulator_disable(struct fdtbus_regulator *reg)
{
	struct fdtbus_regulator_controller *rc = reg->reg_rc;

	return rc->rc_funcs->enable(rc->rc_dev, false);
}

int
fdtbus_regulator_set_voltage(struct fdtbus_regulator *reg, u_int min_uvol,
    u_int max_uvol)
{
	struct fdtbus_regulator_controller *rc = reg->reg_rc;

	if (rc->rc_funcs->set_voltage == NULL)
		return EINVAL;

	return rc->rc_funcs->set_voltage(rc->rc_dev, min_uvol, max_uvol);
}

int
fdtbus_regulator_get_voltage(struct fdtbus_regulator *reg, u_int *puvol)
{
	struct fdtbus_regulator_controller *rc = reg->reg_rc;

	if (rc->rc_funcs->set_voltage == NULL)
		return EINVAL;

	return rc->rc_funcs->get_voltage(rc->rc_dev, puvol);
}
