/*	$NetBSD: ofw_i2c_machdep.c,v 1.1 2025/09/21 17:58:56 thorpej Exp $	*/

/*
 * Copyright (c) 2021, 2025 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: ofw_i2c_machdep.c,v 1.1 2025/09/21 17:58:56 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <dev/ofw/openfirm.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_calls.h>
#include <dev/i2c/i2c_enum.h>

/*
 * Apple OpenFirmware implementations have the i2c device
 * address shifted left 1 bit to account for the r/w bit
 * on the wire.  Some implementations also encode the channel
 * number (for multi-channel controllers) in bit 8 of the
 * address.
 */
#define	OFW_I2C_ADDRESS_MASK	__BITS(1,7)
#define	OFW_I2C_ADDRESS_CHMASK	__BIT(8)

static bool
of_i2c_get_address(device_t dev, int i2c_node, i2c_tag_t tag, int node,
    uint32_t *addrp)
{
	uint32_t reg;
	uint32_t addr;
	int channel;

	/*
	 * dev is the iic bus instance.  We need the controller parent
	 * so we can compare their OFW nodes.
	 */
	int ctlr_node = devhandle_to_of(device_handle(device_parent(dev)));

	if (OF_getprop(node, "reg", &reg, sizeof(reg)) != sizeof(reg)) {
		/*
		 * Some Apple OpenFirmware implementations use "i2c-address"
		 * instead of "reg".
		 */
		if (OF_getprop(node, "i2c-address", &reg,
			       sizeof(reg)) != sizeof(reg)) {
			/*
			 * No address property; reject the device.
			 */
			return false;
		}
	}

	addr = __SHIFTOUT(reg, OFW_I2C_ADDRESS_MASK);
	channel = __SHIFTOUT(reg, OFW_I2C_ADDRESS_CHMASK);

	/*
	 * If the controller supports multiple channels and the controller
	 * node and the i2c bus node are the same, then the devices for
	 * multiple channels are all mixed together in the device tree.
	 * We need to filter them by channel in this case.
	 */
	if (tag->ic_channel != I2C_CHANNEL_DEFAULT && i2c_node == ctlr_node &&
	    tag->ic_channel != channel) {
		return false;
	}

	*addrp = addr;
	return true;
}

static int
of_i2c_enumerate_devices(device_t dev, devhandle_t call_handle, void *v)
{
	struct i2c_enumerate_devices_args *args = v;
	int i2c_node, node;
	char name[32], compat_buf[32];
	uint32_t addr;
	char *clist;
	int clist_size;
	bool cbrv;

	i2c_node = devhandle_to_of(call_handle);

	for (node = OF_child(i2c_node); node != 0; node = OF_peer(node)) {
		if (OF_getprop(node, "name", name, sizeof(name)) <= 0) {
			continue;
		}
		if (!of_i2c_get_address(dev, i2c_node, args->ia->ia_tag,
					node, &addr)) {
			continue;
		}

		/*
		 * Some of Apple's older OpenFirmware implementations are
		 * rife with nodes lacking "compatible" properties.
		 */     
		clist_size = OF_getproplen(node, "compatible");
		if (clist_size <= 0) {
			clist = name;
			clist_size = 0;
		} else {
			clist = kmem_tmpbuf_alloc(clist_size,
			    compat_buf, sizeof(compat_buf), KM_SLEEP);
			if (OF_getprop(node, "compatible", clist, clist_size)
				       < clist_size) {
				kmem_tmpbuf_free(clist, clist_size, compat_buf);
				continue;
			}
		}

		cbrv = i2c_enumerate_device(dev, args, name,
		    clist, clist_size, addr,
		    devhandle_from_of(call_handle, node));

		if (clist != name) {
			kmem_tmpbuf_free(clist, clist_size, compat_buf);
		}

		if (!cbrv) {
			break;
		}
	}

	return 0;
}
OF_DEVICE_CALL_REGISTER(I2C_ENUMERATE_DEVICES_STR,
			of_i2c_enumerate_devices);
