/* $NetBSD: fdt_rtc.c,v 1.2 2025/09/08 00:12:21 thorpej Exp $ */

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: fdt_rtc.c,v 1.2 2025/09/08 00:12:21 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device_calls.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>
#include <dev/ofw/openfirm.h>

int
fdtbus_todr_attach(device_t dev, int phandle, todr_chip_handle_t tch)
{
	const char *prop;

	/*
	 * The kernel will only use the first device to register with
	 * todr_attach. If we have an "rtc0" alias, ensure that it matches
	 * this phandle and ignore all other RTC devices.
	 */
	prop = fdt_get_alias(fdtbus_get_data(), "rtc0");
	if (prop != NULL && OF_finddevice(prop) != phandle) {
		device_printf(dev, "disabled\n");
		return EINVAL;
	}

	todr_attach(tch);

	return 0;
}

static int
fdtbus_device_is_system_todr(device_t dev, devhandle_t call_handle, void *v)
{
	struct device_is_system_todr_args *args = v;
	int phandle = devhandle_to_of(call_handle);
	const char *prop;

	/*
	 * The kernel will only use the first device to register with
	 * todr_attach. If we have an "rtc0" alias, ensure that it matches
	 * this phandle and ignore all other RTC devices.
	 */
	prop = fdt_get_alias(fdtbus_get_data(), "rtc0");
	if (prop == NULL) {
		/* No "rtc0" alias.  System gets default policy. */
		return ESRCH;
	}
	args->result = OF_finddevice(prop) == phandle;
	return 0;
}

OF_DEVICE_CALL_REGISTER(DEVICE_IS_SYSTEM_TODR_STR,
			fdtbus_device_is_system_todr)
