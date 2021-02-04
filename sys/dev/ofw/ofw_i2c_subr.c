/*	$NetBSD: ofw_i2c_subr.c,v 1.1 2021/02/04 20:19:09 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ofw_i2c_subr.c,v 1.1 2021/02/04 20:19:09 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <dev/ofw/openfirm.h>
#include <dev/i2c/i2cvar.h>

/*
 * Iterate over the subtree of a i2c controller node.
 * Add all sub-devices into an array as part of the controller's
 * device properties.
 * This is used by the i2c bus attach code to do direct configuration.
 */
void
of_enter_i2c_devs(prop_dictionary_t props, int ofnode, size_t cell_size,
    int addr_shift)
{
	int node, len;
	char name[32];
	uint64_t reg64;
	uint32_t reg32;
	uint64_t addr;
	prop_array_t array = NULL;
	prop_dictionary_t dev;

	for (node = OF_child(ofnode); node; node = OF_peer(node)) {
		if (OF_getprop(node, "name", name, sizeof(name)) <= 0)
			continue;
		len = OF_getproplen(node, "reg");
		addr = 0;
		if (cell_size == 8 && len >= sizeof(reg64)) {
			if (OF_getprop(node, "reg", &reg64, sizeof(reg64))
			    < sizeof(reg64))
				continue;
			addr = be64toh(reg64);
			/*
			 * The i2c bus number (0 or 1) is encoded in bit 33
			 * of the register, but we encode it in bit 8 of
			 * i2c_addr_t.
			 */
			if (addr & 0x100000000)
				addr = (addr & 0xff) | 0x100;
		} else if (cell_size == 4 && len >= sizeof(reg32)) {
			if (OF_getprop(node, "reg", &reg32, sizeof(reg32))
			    < sizeof(reg32))
				continue;
			addr = be32toh(reg32);
		} else {
			continue;
		}
		addr >>= addr_shift;
		if (addr == 0) continue;

		if (array == NULL)
			array = prop_array_create();

		dev = prop_dictionary_create();
		prop_dictionary_set_string(dev, "name", name);
		prop_dictionary_set_uint32(dev, "addr", addr);
		prop_dictionary_set_uint64(dev, "cookie", node);
		prop_dictionary_set_uint32(dev, "cookietype", I2C_COOKIE_OF);
		of_to_dataprop(dev, node, "compatible", "compatible");
		prop_array_add(array, dev);
		prop_object_release(dev);
	}

	if (array != NULL) {
		prop_dictionary_set(props, "i2c-child-devices", array);
		prop_object_release(array);
	}
}
