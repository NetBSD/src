/*	$NetBSD: i2c_enum.h,v 1.2 2025/09/21 17:54:16 thorpej Exp $	*/

/*-             
 * Copyright (c) 2021, 2025 The NetBSD Foundation, Inc.
 * All rights reserved. 
 *       
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _DEV_I2C_I2C_ENUM_H_
#define	_DEV_I2C_I2C_ENUM_H_

#include <dev/i2c/i2cvar.h>

/*
 * Helpers for enumerating known i2c devices, that can be used from
 * the i2c-enumerate-devices device call in cases where platform
 * device tree information is unavailable or incomplete.
 */
struct i2c_deventry {
	const char *name;
	const char *compat;
	i2c_addr_t addr;
	union {
		const void *data;
		uintptr_t value;
	};
};

#define	I2C_DEVENTRY_EOL	{ 0 }

static inline bool __unused
i2c_enumerate_device(device_t dev, struct i2c_enumerate_devices_args *args,
    const char *name, const char *clist, size_t clist_size, i2c_addr_t addr,
    devhandle_t child_devhandle)
{
	args->ia->ia_addr = addr;
	args->ia->ia_name = name;
	args->ia->ia_clist = clist;
	if (clist == NULL) {
		clist_size = 0;
	} else if (clist_size == 0) {
		clist_size = strlen(clist) + 1;
	}
	args->ia->ia_clist_size = clist_size;
	args->ia->ia_devhandle = child_devhandle;

	return args->callback(dev, args);
}

static inline int __unused
i2c_enumerate_deventries(device_t dev, devhandle_t call_handle,
    struct i2c_enumerate_devices_args *args,
    const struct i2c_deventry *entry,
    devhandle_t (*get_devhandle)(device_t, devhandle_t,
    				 const struct i2c_deventry *))
{
	devhandle_t child_devhandle;
	bool cbrv;

	for (; entry->name != NULL; entry++) {
		if (get_devhandle != NULL) {
			child_devhandle =
			    (*get_devhandle)(dev, call_handle, entry);
		} else {
			child_devhandle = devhandle_invalid();
		}

		cbrv = i2c_enumerate_device(dev, args, entry->name,
		    entry->compat, 0, entry->addr,
		    child_devhandle);

		if (!cbrv) {
			break;
		}
	}

	return 0;
}

#endif /* _DEV_I2C_I2C_ENUM_H_ */
