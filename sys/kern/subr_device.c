/*	$NetBSD: subr_device.c,v 1.15 2025/10/03 14:02:10 thorpej Exp $	*/

/*
 * Copyright (c) 2006, 2021, 2025 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: subr_device.c,v 1.15 2025/10/03 14:02:10 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/device_impl.h>
#include <sys/kmem.h>
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
devhandle_impl_subclass(struct devhandle_impl *new_impl,
    const struct devhandle_impl *super,
    device_call_t (*new_lookup)(devhandle_t, const char *, devhandle_t *))
{
	new_impl->type = super->type;
	new_impl->super = super;
	new_impl->lookup_device_call = new_lookup;
}

/*
 * Helper function that provides a short-hand method of the common
 * "subclass a device handle" flow.
 */
devhandle_t
devhandle_subclass(devhandle_t handle,
    struct devhandle_impl *new_impl,
    device_call_t (*new_lookup)(devhandle_t, const char *, devhandle_t *))
{
	devhandle_impl_subclass(new_impl, handle.impl, new_lookup);
	handle.impl = new_impl;

	return handle;
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
device_call_generic(device_t dev, devhandle_t handle,
    const struct device_call_generic *gen)
{
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

/*****************************************************************************
 * Device properties infrastructure.
 *****************************************************************************/

struct device_get_property_args {
	const char *	prop;		/* IN */
	void *		buf;		/* IN */
	size_t		buflen;		/* IN */
	prop_type_t	reqtype;	/* IN */
	int		flags;		/* IN */
	ssize_t		propsize;	/* OUT */
	int		encoding;	/* OUT */
	prop_type_t	type;		/* OUT */
};

static int
device_getprop_dict(device_t dev, struct device_get_property_args *args)
{
	prop_dictionary_t dict = dev->dv_properties;
	prop_object_t propval;
	bool rv;

	/*
	 * Return ENOENT before any other error so that we can rely
	 * on that error to tell us "property does not exist in this
	 * layer, so go check the platform device tree".
	 */
	propval = prop_dictionary_get(dict, args->prop);
	if (propval == NULL) {
		return ENOENT;
	}

	/*
	 * Validate the requested type.  Because it can be convenient
	 * to do so (e.g. properties that constain a strlist, maybe that
	 * property was set as a single string), we allow STRING objects
	 * to be requested as DATA.
	 */
	prop_type_t objtype = prop_object_type(propval);
	switch (args->reqtype) {
	case PROP_TYPE_DATA:
		KASSERT(args->buf != NULL);
		KASSERT(args->buflen != 0);
		if (objtype != PROP_TYPE_DATA && objtype != PROP_TYPE_STRING) {
			return EFTYPE;
		}
		break;

	case PROP_TYPE_UNKNOWN:
		KASSERT(args->buf == NULL);
		KASSERT(args->buflen == 0);
		break;

	default:
		KASSERT(args->buf != NULL);
		KASSERT(args->buflen != 0);
		if (args->reqtype != objtype) {
			return EFTYPE;
		}
	}

	args->encoding = _BYTE_ORDER;	/* these are always native */
	args->type = objtype;

	switch (args->type) {
	case PROP_TYPE_NUMBER:
		/* prop_number_size() returns bits. */
		args->propsize = prop_number_size(propval) >> 3;
		if (args->buf != NULL) {
			KASSERT(args->buflen == sizeof(uint64_t));
			/*
			 * Fetching a -ve value as uint64_t will fail
			 * a range check, so check what we have before
			 * we fetch.  We'll reconcile it based on what
			 * the caller is asking for later.
			 */
			if (prop_number_unsigned(propval)) {
				rv = prop_number_uint64_value(propval,
				    args->buf);
			} else {
				rv = prop_number_int64_value(propval,
				    args->buf);
			}
			if (! rv) {
				return EIO;	/* off the rails */
			}
		}
		break;

	case PROP_TYPE_STRING:
		/* +1 for trailing NUL */
		args->propsize = prop_string_size(propval) + 1;
		if (args->buf != NULL) {
			if (args->buflen < args->propsize) {
				return EFBIG;
			}
			strlcpy(args->buf, prop_string_value(propval),
			    args->buflen);
		}
		break;

	case PROP_TYPE_DATA:
		args->propsize = prop_data_size(propval);
		if (args->buf != NULL) {
			if (args->buflen < args->propsize) {
				return EFBIG;
			}
			memcpy(args->buf, prop_data_value(propval),
			    args->propsize);
		}
		break;

	case PROP_TYPE_BOOL:
		args->propsize = sizeof(bool);
		if (args->buf != NULL) {
			KASSERT(args->buflen == sizeof(bool));
			*(bool *)args->buf = prop_bool_value(propval);
		}
		break;

	default:
		return EFTYPE;
	}

	return 0;
}

static int
device_getprop_internal(device_t dev, struct device_get_property_args *args)
{
	int error;

	/* Normalize arguments. */
	if (args->buf == NULL || args->buflen == 0) {
		args->buf = NULL;
		args->buflen = 0;
	} else if (args->buflen > SSIZE_MAX) {
		/* Sizes must fit in ssize_t. */
		args->buflen = SSIZE_MAX;
	}

	/* Poison args->propsize for sanity check later. */
	args->propsize = -1;

	args->flags = 0;

	error = device_getprop_dict(dev, args);
	if (error != ENOENT) {
		KASSERT(error != 0 ||
			args->encoding == _BYTE_ORDER);
		goto out;
	}

 out:
	/*
	 * Back-end is expected to return EFBIG if the entire property
	 * does not fit into the provided buffer.  In this case, it is
	 * undefined whether or not the back-end put any data in the
	 * buffer at all, but it *is* expected to return the actual
	 * property size in args->propsize if EFBIG is returned.
	 */
	KASSERT(error != EFBIG || args->propsize >= 0);

	return error;
}

static ssize_t
device_getprop_buf_internal(device_t dev, const char *prop, void *buf,
    size_t buflen, prop_type_t type)
{
	struct device_get_property_args args = {
		.prop = prop,
		.buf = buf,
		.buflen = buflen,
		.reqtype = type,
	};
	int error;

	KASSERT(type == PROP_TYPE_DATA || type == PROP_TYPE_STRING);

	/*
	 * Callers are expeced to provide a valid buffer and length.
	 * Ruthlessly Enforced for DIAGNOSTIC.
	 */
	KASSERT(buf != NULL);
	KASSERT(buflen != 0);
	if (buf == NULL || buflen == 0) {
		return -1;
	}

	error = device_getprop_internal(dev, &args);
	if (error) {
		return -1;
	}

	/*
	 * Back-end is expected to return an error if the buffer isn't
	 * large enough for the entire property.  Ruthlessly Enforced
	 * for DIAGNOSTIC.
	 */
	KASSERT(args.buflen <= SSIZE_MAX);
	KASSERT(args.propsize <= (ssize_t)args.buflen);
	if (args.propsize > args.buflen) {
		return -1;
	}

	return args.propsize;
}

static void *
device_getprop_alloc_internal(device_t dev, const char *prop, size_t *retsizep,
    prop_type_t type)
{
	struct device_get_property_args args = {
		.prop = prop,
		.reqtype = type,
	};
	size_t buflen = 0;
	int error;

	KASSERT(type == PROP_TYPE_DATA || type == PROP_TYPE_STRING);

	/* Get the length. */
	error = device_getprop_internal(dev, &args);
	if (error) {
		return NULL;
	}

	for (;;) {
		/* Check for bogus property size. */
		if (args.propsize <= 0) {
			return NULL;
		}

		/* Allocate the result buffer. */
		args.buflen = buflen = args.propsize;
		args.buf = kmem_alloc(buflen, KM_SLEEP);

		/* Get the property. */
		error = device_getprop_internal(dev, &args);
		if ((error == 0 && (ssize_t)args.buflen == args.propsize) ||
		    error != EFBIG) {
			break;
		}

		/*
		 * We want to allocate an exact-sized buffer, so if
		 * it changed in the short window between getting the
		 * size and allocating the buffer, try again.
		 *
		 * (This is extremely unlikely to happen.)
		 */
		kmem_free(args.buf, buflen);
	}

	KASSERT(args.buf != NULL);
	KASSERT(args.buflen != 0);

	if (error) {
		kmem_free(args.buf, args.buflen);
		args.buf = NULL;
	} else if (retsizep != NULL) {
		/* Buffer length should not have been clamped in this case. */
		KASSERT(args.buflen == buflen);
		KASSERT(args.buflen == args.propsize);
		*retsizep = args.buflen;
	}
	return args.buf;
}

/*
 * device_hasprop --
 *	Returns true if the device has the specified property.
 */
bool
device_hasprop(device_t dev, const char *prop)
{
	return device_getproplen(dev, prop) >= 0;
}

/*
 * device_getproplen --
 *	Get the length of the specified property, -1 if the property
 *	does not exist.
 */
ssize_t
device_getproplen(device_t dev, const char *prop)
{
	struct device_get_property_args args = {
		.prop = prop,
		.reqtype = PROP_TYPE_UNKNOWN,
	};
	int error;

	error = device_getprop_internal(dev, &args);
	if (error) {
		return -1;
	}

	return args.propsize;
}

/*
 * device_getpropencoding --
 *	Returns the byte order encoding of the specified property, -1
 *	if the property does not exist.
 *
 *	N.B. The encoding is determined by the property's backing store,
 *	not by the property itself.
 */
int
device_getpropencoding(device_t dev, const char *prop)
{
	struct device_get_property_args args = {
		.prop = prop,
		.reqtype = PROP_TYPE_UNKNOWN,
	};
	int error;

	error = device_getprop_internal(dev, &args);
	if (error) {
		return -1;
	}

	return args.encoding;
}

/*
 * device_getproptype --
 *	Get the data type of the specified property, PROP_TYPE_UNKNOWN
 *	if the property does not exist or if the data type is unspecified.
 */
prop_type_t
device_getproptype(device_t dev, const char *prop)
{
	struct device_get_property_args args = {
		.prop = prop,
		.reqtype = PROP_TYPE_UNKNOWN,
	};
	int error;

	error = device_getprop_internal(dev, &args);
	if (error) {
		return PROP_TYPE_UNKNOWN;
	}

	return args.type;
}


/*
 * device_getprop_data --
 *	Get the property as a binary data object.
 */
ssize_t
device_getprop_data(device_t dev, const char *prop, void *buf, size_t buflen)
{
	return device_getprop_buf_internal(dev, prop, buf, buflen,
	    PROP_TYPE_DATA);
}

/*
 * device_getprop_data_alloc --
 *	Convenience wrapper around device_getprop_data() that takes care
 *	allocating the buffer.
 */
void *
device_getprop_data_alloc(device_t dev, const char *prop, size_t *retsizep)
{
	return device_getprop_alloc_internal(dev, prop, retsizep,
	    PROP_TYPE_DATA);
}

/*
 * device_getprop_string --
 *	Get the property as a C string.
 */
ssize_t
device_getprop_string(device_t dev, const char *prop, char *buf, size_t buflen)
{
	return device_getprop_buf_internal(dev, prop, buf, buflen,
	    PROP_TYPE_STRING);
}

/*
 * device_getprop_string_alloc --
 *	Convenience wrapper around device_getprop_string() that takes care
 *	allocating the buffer.
 */
char *
device_getprop_string_alloc(device_t dev, const char *prop, size_t *retsizep)
{
	return device_getprop_alloc_internal(dev, prop, retsizep,
	    PROP_TYPE_STRING);
}

/*
 * device_getprop_bool --
 *	Get the boolean value of a property.
 */
bool
device_getprop_bool(device_t dev, const char *prop)
{
	bool val;
	struct device_get_property_args args = {
		.prop = prop,
		.buf = &val,
		.buflen = sizeof(val),
		.reqtype = PROP_TYPE_BOOL,
	};
	int error;

	error = device_getprop_internal(dev, &args);
	if (error) {
		/*
		 * If the property exists but is not a boolean type
		 * (EFTYPE), we map this to 'true'; this is the same
		 * behavior that the traditional OpenBoot, OpenFirmware,
		 * and FDT interfaces have.
		 *
		 * If the property does not exist (ENOENT), or there
		 * is some other problem we translate this to 'false'.
		 */
		return error == EFTYPE ? true : false;
	}
	return val;
}

#define	S8_BIT		__BIT(7)
#define	S8_MASK		__BITS(7,63)
#define	S16_BIT		__BIT(15)
#define	S16_MASK	__BITS(15,63)
#define	S32_BIT		__BIT(31)
#define	S32_MASK	__BITS(31,63)

static bool
device_getprop_number_sext(struct device_get_property_args *args,
    uint64_t *valp)
{
	uint64_t bit, mask;

	/*
	 * Sign-extend the two's-complement number that occupies
	 * the least-significant propsize bytes in *valp into the
	 * full 64 bits.
	 */

	switch (args->propsize) {
	case 1:
		bit = S8_BIT;
		mask = S8_MASK;
		break;

	case 2:
		bit = S16_BIT;
		mask = S16_MASK;
		break;

	case 4:
		bit = S32_BIT;
		mask = S32_MASK;
		break;

	case 8:
		return true;

	default:
		return false;
	}

	/*
	 * If the sign bit and only the sign bit is set, then extend
	 * the sign bit.  Otherwise, check to see if the number has
	 * already been sign-extended into the full 64 bits.  If any
	 * of the extended sign bits are not set, then we are off the
	 * rails (propsize doesn't match the value we were provided)
	 * and fail the operation.
	 */

	if ((*valp & mask) == bit) {
		*valp |= mask;
	} else if ((*valp & mask) != mask) {
		/* value doesn't match propsize?? */
		return false;
	}

	return true;
}

#undef S8_BIT
#undef S8_MASK
#undef S16_BIT
#undef S16_MASK
#undef S32_BIT
#undef S32_MASK

static int
device_getprop_int32_internal(device_t dev, const char *prop, int32_t *valp)
{
	int64_t val64;
	struct device_get_property_args args = {
		.prop = prop,
		.buf = &val64,
		.buflen = sizeof(val64),
		.reqtype = PROP_TYPE_NUMBER,
	};
	int error;

	error = device_getprop_internal(dev, &args);
	if (error) {
		return error;
	}

	if (! device_getprop_number_sext(&args, (uint64_t *)&val64)) {
		return ERANGE;
	}

	if (val64 < INT32_MIN || val64 > INT32_MAX) {
		return ERANGE;
	}

	*valp = (int32_t)val64;
	return 0;
}

static int
device_getprop_uint32_internal(device_t dev, const char *prop, uint32_t *valp)
{
	uint64_t val64;
	struct device_get_property_args args = {
		.prop = prop,
		.buf = &val64,
		.buflen = sizeof(val64),
		.reqtype = PROP_TYPE_NUMBER,
	};
	int error;

	error = device_getprop_internal(dev, &args);
	if (error) {
		return error;
	}

	if (val64 > UINT32_MAX) {
		return ERANGE;
	}

	*valp = (uint32_t)val64;
	return 0;
}

static int
device_getprop_int64_internal(device_t dev, const char *prop, int64_t *valp)
{
	int64_t val64;
	struct device_get_property_args args = {
		.prop = prop,
		.buf = &val64,
		.buflen = sizeof(val64),
		.reqtype = PROP_TYPE_NUMBER,
	};
	int error;

	error = device_getprop_internal(dev, &args);
	if (error) {
		return error;
	}

	if (! device_getprop_number_sext(&args, &val64)) {
		return ERANGE;
	}

	*valp = val64;
	return 0;
}

static int
device_getprop_uint64_internal(device_t dev, const char *prop, uint64_t *valp)
{
	struct device_get_property_args args = {
		.prop = prop,
		.buf = valp,
		.buflen = sizeof(*valp),
		.reqtype = PROP_TYPE_NUMBER,
	};

	return device_getprop_internal(dev, &args);
}

#define	TEMPLATE(name)							\
bool									\
device_getprop_ ## name (device_t dev, const char *prop, 		\
    name ## _t *valp)							\
{									\
	return device_getprop_ ## name ## _internal(dev, prop, valp)	\
	    == 0;							\
}									\
									\
name ## _t								\
device_getprop_ ## name ## _default(device_t dev, const char *prop,	\
    name ## _t defval)							\
{									\
	name ## _t val;							\
									\
	return device_getprop_ ## name ## _internal(dev, prop, &val)	\
	    ? defval : val;						\
}

/*
 * device_getprop_int32 --
 *	Get the specified property as a signed 32-bit integer.
 */
TEMPLATE(int32)
__strong_alias(device_getprop_int,device_getprop_int32);


/*
 * device_getprop_uint32 --
 *	Get the specified property as an unsigned 32-bit integer.
 */
TEMPLATE(uint32)
__strong_alias(device_getprop_uint,device_getprop_uint32);

/*
 * device_getprop_int64 --
 *	Get the specified property as a signed 64-bit integer.
 */
TEMPLATE(int64)

/*
 * device_getprop_uint64 --
 *	Get the specified property as an unsigned 64-bit integer.
 */
TEMPLATE(uint64)

#undef TEMPLATE

/*
 * device_setprop_data --
 *	Set the specified binary data property.
 */
bool
device_setprop_data(device_t dev, const char *prop, const void *buf, size_t len)
{
	return prop_dictionary_set_data(dev->dv_properties, prop, buf, len);
}

/*
 * device_setprop_string --
 *	Set the specified C string property.
 */
bool
device_setprop_string(device_t dev, const char *prop, const char *str)
{
	return prop_dictionary_set_string(dev->dv_properties, prop, str);
}

/*
 * device_setprop_bool --
 *	Set the specified boolean property.
 */
bool
device_setprop_bool(device_t dev, const char *prop, bool val)
{
	return prop_dictionary_set_bool(dev->dv_properties, prop, val);
}

/*
 * device_setprop_int32 --
 *	Set the specified 32-bit signed integer property.
 */
bool
device_setprop_int32(device_t dev, const char *prop, int32_t val)
{
	return prop_dictionary_set_int32(dev->dv_properties, prop, val);
}
__strong_alias(device_setprop_int,device_setprop_int32);

/*
 * device_setprop_uint32 --
 *	Set the specified 32-bit unsigned integer property.
 */
bool
device_setprop_uint32(device_t dev, const char *prop, uint32_t val)
{
	return prop_dictionary_set_uint32(dev->dv_properties, prop, val);
}
__strong_alias(device_setprop_uint,device_setprop_uint32);

/*
 * device_setprop_int64 --
 *	Set the specified 64-bit signed integer property.
 */
bool
device_setprop_int64(device_t dev, const char *prop, int64_t val)
{
	return prop_dictionary_set_int64(dev->dv_properties, prop, val);
}

/*
 * device_setprop_uint64 --
 *	Set the specified 64-bit unsigned integer property.
 */
bool
device_setprop_uint64(device_t dev, const char *prop, uint64_t val)
{
	return prop_dictionary_set_uint64(dev->dv_properties, prop, val);
}

/*
 * device_delprop --
 *	Delete the specified property.
 */
void
device_delprop(device_t dev, const char *prop)
{
	prop_dictionary_remove(dev->dv_properties, prop);
}
