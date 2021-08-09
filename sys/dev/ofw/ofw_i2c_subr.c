/*	$NetBSD: ofw_i2c_subr.c,v 1.1.16.1 2021/08/09 00:30:09 thorpej Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofw_i2c_subr.c,v 1.1.16.1 2021/08/09 00:30:09 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <dev/ofw/openfirm.h>
#include <dev/i2c/i2cvar.h>

#ifdef __HAVE_OPENFIRMWARE_VARIANT_AAPL
/*
 * Apple OpenFirmware implementations have the i2c device
 * address shifted left 1 bit to account for the r/w bit
 * on the wire.  We also want to look at only the least-
 * significant 8 bits of the address cell.
 */
#define	OFW_I2C_ADDRESS_MASK	__BITS(0,7)
#define	OFW_I2C_ADDRESS_SHIFT	1

/*
 * Some of Apple's older OpenFirmware implementations are rife with
 * nodes lacking "compatible" properties.
 */
#define	OFW_I2C_ALLOW_MISSING_COMPATIBLE_PROPERTY
#endif /* __HAVE_OPENFIRMWARE_VARIANT_AAPL */

#ifdef __HAVE_OPENFIRMWARE_VARIANT_SUNW
/*
 * Sun OpenFirmware implementations use 2 cells for the
 * i2c device "reg" property, the first containing the
 * channel number, the second containing the i2c device
 * address shifted left 1 bit to account for the r/w bit
 * on the wire.
 */
#define	OFW_I2C_REG_NCELLS	2
#define	OFW_I2C_REG_CHANNEL	0
#define	OFW_I2C_REG_ADDRESS	1
#define	OFW_I2C_ADDRESS_SHIFT	1
#endif /* __HAVE_OPENFIRMWARE_VARIANT_SUNW */

#ifndef OFW_I2C_REG_NCELLS
#define	OFW_I2C_REG_NCELLS	1
#endif

#ifndef OFW_I2C_REG_ADDRESS
#define	OFW_I2C_REG_ADDRESS	0
#endif

/* No default for OFW_I2C_REG_CHANNEL. */

#ifndef OFW_I2C_ADDRESS_MASK
#define	OFW_I2C_ADDRESS_MASK	__BITS(0,31)
#endif

#ifndef OFW_I2C_ADDRESS_SHIFT
#define	OFW_I2C_ADDRESS_SHIFT	0
#endif

static bool
of_i2c_get_address(i2c_tag_t tag, int node, uint32_t *addrp)
{
	uint32_t reg[OFW_I2C_REG_NCELLS];
	uint32_t addr;
#ifdef OFW_I2C_REG_CHANNEL
	uint32_t channel;
#endif

	if (OF_getprop(node, "reg", reg, sizeof(reg)) != sizeof(reg)) {
		/*
		 * "reg" property is malformed; reject the device.
		 */
		return false;
	}

	addr = be32toh(reg[OFW_I2C_REG_ADDRESS]);
	addr = (addr & OFW_I2C_ADDRESS_MASK) >> OFW_I2C_ADDRESS_SHIFT;

#ifdef OFW_I2C_REG_CHANNEL
	/*
	 * If the channel in the "reg" property does not match,
	 * reject the device.
	 */
	channel = be32toh(reg[OFW_I2C_REG_CHANNEL]);
	if (channel != tag->ic_channel) {
		return false;
	}
#endif

	*addrp = addr;
	return true;
}

/*
 * This follows the Device Tree bindings for i2c, which for the most part
 * work for the classical OpenFirmware implementations, as well.  There are
 * some quirks do deal with for different OpenFirmware implementations, which
 * are mainly in how the "reg" property is interpreted.
 */
static int
of_i2c_enumerate_devices(device_t dev, devhandle_t call_handle, void *v)
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
		if (!of_i2c_get_address(args->ia->ia_tag, node, &addr)) {
			continue;
		}

		clist_size = OF_getproplen(node, "compatible");
		if (clist_size <= 0) {
#ifndef OFW_I2C_ALLOW_MISSING_COMPATIBLE_PROPERTY
			continue;
#else
			clist_size = 0;
#endif
		}
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
OF_DEVICE_CALL_REGISTER("i2c-enumerate-devices", of_i2c_enumerate_devices);
