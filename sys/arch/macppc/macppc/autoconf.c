/*	$NetBSD: autoconf.c,v 1.63 2025/09/22 12:48:46 thorpej Exp $	*/

/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.63 2025/09/22 12:48:46 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ofw/openfirm.h>

#include <powerpc/ofw_machdep.h>

static const char *
sensor_prop_name(char *buf, size_t buflen, int num)
{
	snprintf(buf, buflen, "s%02x", num);
	return buf;
}

static void
macppc_assign_sensor_names_old(device_t gparent, int phandle,
    prop_dictionary_t props)
{
	uint32_t ids[4];
	char buf[256];
	char num[8];

	/* Only if the grandparent is ki2c. */
	if (! device_is_a(gparent, "ki2c")) {
		return;
	}

	int len = OF_getprop(phandle, "hwsensor-id", ids, sizeof(ids));

	if (len <= 0) {
		/*
		 * No info, fill in what we know based on the model.
		 */
		int root = OF_finddevice("/");
		if (OF_getprop(phandle, "name", buf, sizeof(buf)) <= 0) {
			return;
		}
		if (strcmp(buf, "temp-monitor") == 0) {
			if (OF_getprop(root, "model", buf, sizeof(buf)) <= 0) {
				return;
			}
			if (strcmp(buf, "RackMac1,2") == 0) {
				prop_dictionary_set_string(props,
				    "s00", "CASE");
				return;
			}
		}
		return;
	}
	int sllen = OF_getprop(phandle, "hwsensor-location", buf,
			       sizeof(buf));
	if (sllen <= 0) {
		/* Well that sucks! */
		return;
	}
	len >>= 2;
	for (int i = 0; i < len; i++) {
		const char *descr = strlist_string(buf, sllen, i);
		if (descr == NULL) {
			/* Ran out of descriptions. */
			return;
		}
		/*
		 * Yes, array index instead if ids[i].  The old
		 * OpenFirmware encoding was apparently ... odd.
		 * But macallan@ assures me this is correct.
		 */
		prop_dictionary_set_string(props,
		    sensor_prop_name(num, sizeof(num), i),
		    descr);
	}
}

static void
macppc_assign_sensor_names(device_t gparent, device_t dev)
{
	int phandle = devhandle_to_of(device_handle(dev));
	prop_dictionary_t props = device_properties(dev);
	int snode;

	snode = OF_child(phandle);
	if (snode == 0) {
		macppc_assign_sensor_names_old(gparent, phandle, props);
		return;
	}

	for (; snode != 0; snode = OF_peer(snode)) {
		char descr[64], num[8];
		int reg;

		if (OF_getprop(snode, "reg", &reg, sizeof(reg)) < sizeof(reg)) {
			continue;
		}
		if (OF_getprop(snode, "location", descr, sizeof(descr)) <= 0) {
			continue;
		}
		prop_dictionary_set_string(props,
		    sensor_prop_name(num, sizeof(num), reg),
		    descr);
	}
}

void
device_register(device_t dev, void *aux)
{
	ofw_device_register(dev, aux);

	/*
	 * Look for sensors hung off an I2C controller and pass
	 * along the sensor name properties.
	 */
	device_t parent = device_parent(dev);
	if (device_is_a(parent, "iic") &&
	    devhandle_type(device_handle(dev)) == DEVHANDLE_TYPE_OF) {
		macppc_assign_sensor_names(device_parent(parent), dev);
	}
}
