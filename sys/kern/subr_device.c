/*	$NetBSD: subr_device.c,v 1.12 2022/03/03 06:25:46 riastradh Exp $	*/

/*
 * Copyright (c) 2006, 2021 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: subr_device.c,v 1.12 2022/03/03 06:25:46 riastradh Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <sys/device_calls.h>

/* Root device. */
device_t			root_device;

/*
 * devhandle_t accessors / mutators.
 */

static bool
devhandle_is_valid_internal(const devhandle_t * const handlep)
{
	if (handlep->impl == NULL) {
		return false;
	}
	return handlep->impl->type != DEVHANDLE_TYPE_INVALID;
}

bool
devhandle_is_valid(devhandle_t handle)
{
	return devhandle_is_valid_internal(&handle);
}

devhandle_t
devhandle_invalid(void)
{
	static const devhandle_t invalid_devhandle = {
		.impl = NULL,
		.uintptr = 0,
	};
	return invalid_devhandle;
}

devhandle_type_t
devhandle_type(devhandle_t handle)
{
	if (!devhandle_is_valid_internal(&handle)) {
		return DEVHANDLE_TYPE_INVALID;
	}

	return handle.impl->type;
}

int
devhandle_compare(devhandle_t handle1, devhandle_t handle2)
{
	devhandle_type_t type1 = devhandle_type(handle1);
	devhandle_type_t type2 = devhandle_type(handle2);

	if (type1 == DEVHANDLE_TYPE_INVALID) {
		return -1;
	}
	if (type2 == DEVHANDLE_TYPE_INVALID) {
		return 1;
	}

	if (type1 < type2) {
		return -1;
	}
	if (type1 > type2) {
		return 1;
	}

	/* For private handles, we also compare the impl pointers. */
	if (type1 == DEVHANDLE_TYPE_PRIVATE) {
		intptr_t impl1 = (intptr_t)handle1.impl;
		intptr_t impl2 = (intptr_t)handle2.impl;

		if (impl1 < impl2) {
			return -1;
		}
		if (impl1 > impl2) {
			return 1;
		}
	}

	if (handle1.integer < handle2.integer) {
		return -1;
	}
	if (handle1.integer > handle2.integer) {
		return 1;
	}

	return 0;
}

device_call_t
devhandle_lookup_device_call(devhandle_t handle, const char *name,
    devhandle_t *call_handlep)
{
	const struct devhandle_impl *impl;
	device_call_t call;

	/*
	 * The back-end can override the handle to use for the call,
	 * if needed.
	 */
	*call_handlep = handle;

	for (impl = handle.impl; impl != NULL; impl = impl->super) {
		if (impl->lookup_device_call != NULL) {
			call = impl->lookup_device_call(handle, name,
			    call_handlep);
			if (call != NULL) {
				return call;
			}
		}
	}
	return NULL;
}

void
devhandle_impl_inherit(struct devhandle_impl *impl,
    const struct devhandle_impl *super)
{
	memcpy(impl, super, sizeof(*impl));
	impl->super = super;
}

/*
 * Accessor functions for the device_t type.
 */

devclass_t
device_class(device_t dev)
{

	return dev->dv_class;
}

cfdata_t
device_cfdata(device_t dev)
{

	return dev->dv_cfdata;
}

cfdriver_t
device_cfdriver(device_t dev)
{

	return dev->dv_cfdriver;
}

cfattach_t
device_cfattach(device_t dev)
{

	return dev->dv_cfattach;
}

int
device_unit(device_t dev)
{

	return dev->dv_unit;
}

const char *
device_xname(device_t dev)
{

	return dev->dv_xname;
}

device_t
device_parent(device_t dev)
{

	return dev->dv_parent;
}

bool
device_activation(device_t dev, devact_level_t level)
{
	int active_flags;

	active_flags = DVF_ACTIVE;
	switch (level) {
	case DEVACT_LEVEL_FULL:
		active_flags |= DVF_CLASS_SUSPENDED;
		/*FALLTHROUGH*/
	case DEVACT_LEVEL_DRIVER:
		active_flags |= DVF_DRIVER_SUSPENDED;
		/*FALLTHROUGH*/
	case DEVACT_LEVEL_BUS:
		active_flags |= DVF_BUS_SUSPENDED;
		break;
	}

	return (dev->dv_flags & active_flags) == DVF_ACTIVE;
}

bool
device_is_active(device_t dev)
{
	int active_flags;

	active_flags = DVF_ACTIVE;
	active_flags |= DVF_CLASS_SUSPENDED;
	active_flags |= DVF_DRIVER_SUSPENDED;
	active_flags |= DVF_BUS_SUSPENDED;

	return (dev->dv_flags & active_flags) == DVF_ACTIVE;
}

bool
device_is_enabled(device_t dev)
{
	return (dev->dv_flags & DVF_ACTIVE) == DVF_ACTIVE;
}

bool
device_has_power(device_t dev)
{
	int active_flags;

	active_flags = DVF_ACTIVE | DVF_BUS_SUSPENDED;

	return (dev->dv_flags & active_flags) == DVF_ACTIVE;
}

int
device_locator(device_t dev, u_int locnum)
{

	KASSERT(dev->dv_locators != NULL);
	return dev->dv_locators[locnum];
}

void *
device_private(device_t dev)
{

	/*
	 * The reason why device_private(NULL) is allowed is to simplify the
	 * work of a lot of userspace request handlers (i.e., c/bdev
	 * handlers) which grab cfdriver_t->cd_units[n].
	 * It avoids having them test for it to be NULL and only then calling
	 * device_private.
	 */
	return dev == NULL ? NULL : dev->dv_private;
}

void
device_set_private(device_t dev, void *private)
{

	KASSERTMSG(dev->dv_private == NULL, "device_set_private(%p, %p):"
	    " device %s already has private set to %p",
	    dev, private, device_xname(dev), device_private(dev));
	KASSERT(private != NULL);
	dev->dv_private = private;
}

prop_dictionary_t
device_properties(device_t dev)
{

	return dev->dv_properties;
}

/*
 * device_is_a:
 *
 *	Returns true if the device is an instance of the specified
 *	driver.
 */
bool
device_is_a(device_t dev, const char *dname)
{
	if (dev == NULL || dev->dv_cfdriver == NULL) {
		return false;
	}

	return strcmp(dev->dv_cfdriver->cd_name, dname) == 0;
}

/*
 * device_attached_to_iattr:
 *
 *	Returns true if the device attached to the specified interface
 *	attribute.
 */
bool
device_attached_to_iattr(device_t dev, const char *iattr)
{
	cfdata_t cfdata = device_cfdata(dev);
	const struct cfparent *pspec;

	if (cfdata == NULL || (pspec = cfdata->cf_pspec) == NULL) {
		return false;
	}

	return strcmp(pspec->cfp_iattr, iattr) == 0;
}

void
device_set_handle(device_t dev, devhandle_t handle)
{
	dev->dv_handle = handle;
}

devhandle_t
device_handle(device_t dev)
{
	return dev->dv_handle;
}

int
device_call_generic(device_t dev, const struct device_call_generic *gen)
{
	devhandle_t handle = device_handle(dev);
	device_call_t call;
	devhandle_t call_handle;

	call = devhandle_lookup_device_call(handle, gen->name, &call_handle);
	if (call == NULL) {
		return ENOTSUP;
	}
	return call(dev, call_handle, gen->args);
}

int
device_enumerate_children(device_t dev,
    bool (*callback)(device_t, devhandle_t, void *),
    void *callback_arg)
{
	struct device_enumerate_children_args args = {
		.callback = callback,
		.callback_arg = callback_arg,
	};

	return device_call(dev, DEVICE_ENUMERATE_CHILDREN(&args));
}
