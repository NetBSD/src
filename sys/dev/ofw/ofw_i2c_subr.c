/*	$NetBSD: ofw_i2c_subr.c,v 1.1.6.2 2021/05/08 15:51:30 thorpej Exp $	*/

/*
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

/*
 * Copyright 1998
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofw_i2c_subr.c,v 1.1.6.2 2021/05/08 15:51:30 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <dev/ofw/openfirm.h>
#include <dev/i2c/i2cvar.h>

/*
 * Standard routine for fetching an i2c device address, according
 * to the standard OpenFirmware / Device Tree bindings.
 */
static bool
of_i2c_get_address(int node, uint32_t *addrp)
{
	uint32_t reg;

	if (of_getprop_uint32(node, "reg", &reg) == -1) {
		return false;
	}

	*addrp = reg;
	return true;
}

static int
of_i2c_enumerate_devices(device_t dev, devhandle_t call_handle, void *v)
{
	return of_i2c_enumerate_devices_ext(dev, call_handle, v,
	    of_i2c_get_address);
}
OF_DEVICE_CALL_REGISTER("i2c-enumerate-devices", of_i2c_enumerate_devices);

int
of_i2c_enumerate_devices_ext(device_t dev, devhandle_t call_handle, void *v,
    bool (*get_address)(int, uint32_t *))
{
	struct i2c_enumerate_devices_args *args = v;
	int i2c_node, node;
	char name[32], compat_buf[32];
	prop_dictionary_t props;
	uint32_t addr;
	char *clist;
	int clist_size;
	bool cbrv;

	i2c_node = devhandle_to_of(call_handle);

	for (node = OF_child(i2c_node); node != 0; node = OF_peer(node)) {
		if (OF_getprop(node, "name", name, sizeof(name)) <= 0) {
			continue;
		}
		if (!get_address(node, &addr)) {
			continue;
		}

		clist_size = OF_getproplen(node, "compatible");
		clist = kmem_tmpbuf_alloc(clist_size,
		    compat_buf, sizeof(compat_buf), KM_SLEEP);
		if (OF_getprop(node, "compatible", clist, clist_size) <
		    clist_size) {
			kmem_tmpbuf_free(clist, clist_size, compat_buf);
			continue;
		}
		props = prop_dictionary_create();

		args->ia->ia_addr = (i2c_addr_t)addr;
		args->ia->ia_name = name;
		args->ia->ia_clist = clist;
		args->ia->ia_clist_size = clist_size;
		args->ia->ia_prop = props;
		args->ia->ia_devhandle = devhandle_from_of(node);

		cbrv = args->callback(dev, args);

		prop_object_release(props);
		kmem_tmpbuf_free(clist, clist_size, compat_buf);

		if (!cbrv) {
			break;
		}
	}

	return 0;
}
