/*	$NetBSD: i2c_enum.h,v 1.1.2.1 2021/09/13 14:47:28 thorpej Exp $	*/

/*-             
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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
 * the i2c-enumerate-devices device call.
 */
struct i2c_deventry {
	const char *name;
	const char *compat;
	i2c_addr_t addr;
};

static inline int __unused
i2c_enumerate_deventries(device_t dev, devhandle_t call_handle,
    struct i2c_enumerate_devices_args *args,
    const struct i2c_deventry *entries, unsigned int nentries)
{
	unsigned int i;
	bool cbrv;

	for (i = 0; i < nentries; i++) {
		args->ia->ia_addr = entries[i].addr;
		args->ia->ia_name = entries[i].name;
		args->ia->ia_clist = entries[i].compat;
		args->ia->ia_clist_size =
		    entries[i].compat != NULL ? strlen(entries[i].compat) + 1
					      : 0;

		/* no devhandle for child devices. */
		devhandle_invalidate(&args->ia->ia_devhandle);

		cbrv = args->callback(dev, args);

		if (!cbrv) {
			break;
		}
	}

	return 0;
}

#endif /* _DEV_I2C_I2C_ENUM_H_ */
