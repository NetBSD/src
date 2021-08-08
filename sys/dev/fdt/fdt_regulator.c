/* $NetBSD: fdt_regulator.c,v 1.9 2021/08/08 15:23:42 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_regulator.c,v 1.9 2021/08/08 15:23:42 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

#define	REGULATOR_TO_RC(_reg)	\
	container_of((_reg), struct fdtbus_regulator_controller, rc_reg)

struct fdtbus_regulator_controller {
	device_t rc_dev;
	int rc_phandle;
	const struct fdtbus_regulator_controller_func *rc_funcs;

	u_int rc_enable_ramp_delay;

	struct fdtbus_regulator rc_reg;	/* handle returned by acquire() */

	LIST_ENTRY(fdtbus_regulator_controller) rc_next;
};

static LIST_HEAD(, fdtbus_regulator_controller) fdtbus_regulator_controllers =
    LIST_HEAD_INITIALIZER(fdtbus_regulator_controllers);

int
fdtbus_register_regulator_controller(device_t dev, int phandle,
    const struct fdtbus_regulator_controller_func *funcs)
{
	struct fdtbus_regulator_controller *rc;

	rc = kmem_zalloc(sizeof(*rc), KM_SLEEP);
	rc->rc_dev = dev;
	rc->rc_phandle = phandle;
	rc->rc_funcs = funcs;
	rc->rc_reg.reg_rc = rc;

	of_getprop_uint32(phandle, "regulator-enable-ramp-delay", &rc->rc_enable_ramp_delay);

	LIST_INSERT_HEAD(&fdtbus_regulator_controllers, rc, rc_next);

	return 0;
}

static struct fdtbus_regulator_controller *
fdtbus_get_regulator_controller(int phandle)
{
	struct fdtbus_regulator_controller *rc;

	LIST_FOREACH(rc, &fdtbus_regulator_controllers, rc_next) {
		if (rc->rc_phandle == phandle)
			return rc;
	}

	return NULL;
}

struct fdtbus_regulator *
fdtbus_regulator_acquire(int phandle, const char *prop)
{
	struct fdtbus_regulator_controller *rc;
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

	return &rc->rc_reg;
}

void
fdtbus_regulator_release(struct fdtbus_regulator *reg)
{
	struct fdtbus_regulator_controller *rc = REGULATOR_TO_RC(reg);

	rc->rc_funcs->release(rc->rc_dev);
}

int
fdtbus_regulator_enable(struct fdtbus_regulator *reg)
{
	struct fdtbus_regulator_controller *rc = REGULATOR_TO_RC(reg);
	int error;

	error = rc->rc_funcs->enable(rc->rc_dev, true);
	if (error != 0)
		return error;

	if (rc->rc_enable_ramp_delay != 0)
		delay(rc->rc_enable_ramp_delay);

	return 0;
}

int
fdtbus_regulator_disable(struct fdtbus_regulator *reg)
{
	struct fdtbus_regulator_controller *rc = REGULATOR_TO_RC(reg);

	if (of_hasprop(rc->rc_phandle, "regulator-always-on"))
		return EIO;

	return rc->rc_funcs->enable(rc->rc_dev, false);
}

int
fdtbus_regulator_set_voltage(struct fdtbus_regulator *reg, u_int min_uvol,
    u_int max_uvol)
{
	struct fdtbus_regulator_controller *rc = REGULATOR_TO_RC(reg);

	if (rc->rc_funcs->set_voltage == NULL)
		return EINVAL;

	return rc->rc_funcs->set_voltage(rc->rc_dev, min_uvol, max_uvol);
}

int
fdtbus_regulator_get_voltage(struct fdtbus_regulator *reg, u_int *puvol)
{
	struct fdtbus_regulator_controller *rc = REGULATOR_TO_RC(reg);

	if (rc->rc_funcs->get_voltage == NULL)
		return EINVAL;

	return rc->rc_funcs->get_voltage(rc->rc_dev, puvol);
}

int
fdtbus_regulator_supports_voltage(struct fdtbus_regulator *reg, u_int min_uvol,
    u_int max_uvol)
{
	struct fdtbus_regulator_controller *rc = REGULATOR_TO_RC(reg);
	u_int uvol;

	if (rc->rc_funcs->set_voltage == NULL)
		return EINVAL;

	if (of_getprop_uint32(rc->rc_phandle, "regulator-min-microvolt", &uvol) == 0) {
		if (uvol < min_uvol)
			return ERANGE;
	}
	if (of_getprop_uint32(rc->rc_phandle, "regulator-max-microvolt", &uvol) == 0) {
		if (uvol > max_uvol)
			return ERANGE;
	}

	return 0;
}
