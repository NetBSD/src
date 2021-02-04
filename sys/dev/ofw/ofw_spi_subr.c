/*	$NetBSD: ofw_spi_subr.c,v 1.1 2021/02/04 20:19:09 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ofw_spi_subr.c,v 1.1 2021/02/04 20:19:09 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <dev/ofw/openfirm.h>

void
of_enter_spi_devs(prop_dictionary_t props, int ofnode, size_t cell_size)
{
	int node, len;
	char name[32];
	uint64_t reg64;
	uint32_t reg32;
	uint32_t slave;
	u_int32_t maxfreq;
	prop_array_t array = NULL;
	prop_dictionary_t dev;
	int mode;

	for (node = OF_child(ofnode); node; node = OF_peer(node)) {
		if (OF_getprop(node, "name", name, sizeof(name)) <= 0)
			continue;
		len = OF_getproplen(node, "reg");
		slave = 0;
		if (cell_size == 8 && len >= sizeof(reg64)) {
			if (OF_getprop(node, "reg", &reg64, sizeof(reg64))
			    < sizeof(reg64))
				continue;
			slave = be64toh(reg64);
		} else if (cell_size == 4 && len >= sizeof(reg32)) {
			if (OF_getprop(node, "reg", &reg32, sizeof(reg32))
			    < sizeof(reg32))
				continue;
			slave = be32toh(reg32);
		} else {
			continue;
		}
		if (of_getprop_uint32(node, "spi-max-frequency", &maxfreq)) {
			maxfreq = 0;
		}
		mode = ((int)of_hasprop(node, "cpol") << 1) | (int)of_hasprop(node, "cpha");

		if (array == NULL)
			array = prop_array_create();

		dev = prop_dictionary_create();
		prop_dictionary_set_string(dev, "name", name);
		prop_dictionary_set_uint32(dev, "slave", slave);
		prop_dictionary_set_uint32(dev, "mode", mode);
		if (maxfreq > 0)
			prop_dictionary_set_uint32(dev, "spi-max-frequency", maxfreq);
		prop_dictionary_set_uint64(dev, "cookie", node);
		of_to_dataprop(dev, node, "compatible", "compatible");
		prop_array_add(array, dev);
		prop_object_release(dev);
	}

	if (array != NULL) {
		prop_dictionary_set(props, "spi-child-devices", array);
		prop_object_release(array);
	}
}
