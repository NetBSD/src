/*	$NetBSD: ofw_patch.c,v 1.1 2020/10/16 07:35:16 jdc Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
__KERNEL_RCSID(0, "$NetBSD: ofw_patch.c,v 1.1 2020/10/16 07:35:16 jdc Exp $");

#include <sys/param.h>

#include <dev/i2c/i2cvar.h>
#include <dev/scsipi/scsipiconf.h>

#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <sparc64/sparc64/ofw_patch.h>
#include <sparc64/sparc64/static_edid.h>

static void
add_gpio_LED(prop_array_t pins, const char *name, int num, int act, int def)
{
	prop_dictionary_t pin = prop_dictionary_create();
	prop_dictionary_set_string(pin, "name", name);
	prop_dictionary_set_uint32(pin, "type", 0);	/* 0 for LED, for now */
	prop_dictionary_set_uint32(pin, "pin", num);
	prop_dictionary_set_bool(pin, "active_high", act);
	if (def != -1)
		prop_dictionary_set_int32(pin, "default_state", def);
	prop_array_add(pins, pin);
	prop_object_release(pin);
}
	
void
add_gpio_props_v210(device_t dev, void *aux)
{
	struct i2c_attach_args *ia = aux;
	prop_dictionary_t dict = device_properties(dev);
	prop_array_t pins;

	switch (ia->ia_addr) {
		case 0x38:	/* front panel LEDs */
			pins = prop_array_create();
			add_gpio_LED(pins, "indicator", 7, 0, -1);
			add_gpio_LED(pins, "fault", 5, 0, 0);
			add_gpio_LED(pins, "power", 4, 0, 1);
			prop_dictionary_set(dict, "pins", pins);
			prop_object_release(pins);
			break;
		case 0x23:	/* drive bay LEDs */
			pins = prop_array_create();
			add_gpio_LED(pins, "bay0_fault", 10, 0, 0);
			add_gpio_LED(pins, "bay1_fault", 11, 0, 0);
			add_gpio_LED(pins, "bay0_remove", 12, 0, 0);
			add_gpio_LED(pins, "bay1_remove", 13, 0, 0);
			prop_dictionary_set(dict, "pins", pins);
			prop_object_release(pins);
			break;
	}
}

void
add_drivebay_props_v210(device_t dev, int ofnode, void *aux)
{
	struct scsipibus_attach_args *sa = aux;
	int target = sa->sa_periph->periph_target;
	char path[256]= "";

	OF_package_to_path(ofnode, path, sizeof(path));

	/* see if we're on the onboard controller's 1st channel */
	if (strcmp(path, "/pci@1c,600000/scsi@2") != 0)
		return;
	/* yes, yes we are */
	if ( target < 2) {
		prop_dictionary_t dict = device_properties(dev);
		char name[16];

		snprintf(name, sizeof(name), "bay%d", target);		
		prop_dictionary_set_string(dict, "location", name);
	}
}

/*
 * Add SPARCle spdmem devices (0x50 and 0x51) that are not in the OFW tree
 */
void
add_spdmem_props_sparcle(device_t busdev)
{
	prop_dictionary_t props = device_properties(busdev);
	prop_array_t cfg = prop_array_create();
	int i;

	DPRINTF(ACDB_PROBE, ("\nAdding spdmem for SPARCle "));
	for (i = 0x50; i <= 0x51; i++) {
		prop_dictionary_t spd = prop_dictionary_create();
		prop_dictionary_set_string(spd, "name", "dimm-spd");
		prop_dictionary_set_uint32(spd, "addr", i);
		prop_dictionary_set_uint64(spd, "cookie", 0);
		prop_array_add(cfg, spd);
		prop_object_release(spd);
	}
	prop_dictionary_set(props, "i2c-child-devices", cfg);
	prop_object_release(cfg);
}

/*
 * Add V210/V240 environmental sensors that are not in the OFW tree.
 */
void
add_env_sensors_v210(device_t busdev)
{
	prop_dictionary_t props = device_properties(busdev);
	prop_array_t cfg = NULL;
	prop_dictionary_t sens;
	prop_data_t data;
	const char name_lm[] = "i2c-lm75";
	const char name_adm[] = "i2c-adm1026";

	DPRINTF(ACDB_PROBE, ("\nAdding sensors for %s ", machine_model));
	cfg = prop_dictionary_get(props, "i2c-child-devices");
 	if (!cfg) {
		cfg = prop_array_create();
		prop_dictionary_set(props, "i2c-child-devices", cfg);
		prop_dictionary_set_bool(props, "i2c-indirect-config", false);
	}

	/* ADM1026 at 0x2e */
	sens = prop_dictionary_create();
	prop_dictionary_set_uint32(sens, "addr", 0x2e);
	prop_dictionary_set_uint64(sens, "cookie", 0);
	prop_dictionary_set_string(sens, "name", "hardware-monitor");
	data = prop_data_create_copy(&name_adm[0], sizeof(name_adm));
	prop_dictionary_set(sens, "compatible", data);
	prop_object_release(data);
	prop_array_add(cfg, sens);
	prop_object_release(sens);

	/* LM75 at 0x4e */
	sens = prop_dictionary_create();
	prop_dictionary_set_uint32(sens, "addr", 0x4e);
	prop_dictionary_set_uint64(sens, "cookie", 0);
	prop_dictionary_set_string(sens, "name", "temperature-sensor");
	data = prop_data_create_copy(&name_lm[0], sizeof(name_lm));
	prop_dictionary_set(sens, "compatible", data);
	prop_object_release(data);
	prop_array_add(cfg, sens);
	prop_object_release(sens);
}

/* Hardware specific device properties */
void
set_hw_props(device_t dev)
{
	device_t busdev = device_parent(dev);

	if ((!strcmp(machine_model, "SUNW,Sun-Fire-V240") ||
	    !strcmp(machine_model, "SUNW,Sun-Fire-V210"))) {
		device_t busparent = device_parent(busdev);
		prop_dictionary_t props = device_properties(dev);

		if (busparent != NULL && device_is_a(busparent, "pcfiic") &&
		    device_is_a(dev, "adm1026hm") && props != NULL) {
			prop_dictionary_set_uint8(props, "fan_div2", 0x55);
			prop_dictionary_set_bool(props, "multi_read", true);
		}
	}

	if (!strcmp(machine_model, "SUNW,Sun-Fire-V440")) {
		device_t busparent = device_parent(busdev);
		prop_dictionary_t props = device_properties(dev);
		if (busparent != NULL && device_is_a(busparent, "pcfiic") &&
		    device_is_a(dev, "adm1026hm") && props != NULL) {
			prop_dictionary_set_bool(props, "multi_read", true);
		}
	}
}

/* Static EDID definitions */
void
set_static_edid(prop_dictionary_t dict)
{
	if (!strcmp(machine_model, "NATE,Meso-999")) {
		prop_data_t edid;

		DPRINTF(ACDB_PROBE, ("\nAdding EDID for Meso-999 "));
		edid = prop_data_create_copy(edid_meso999,
		    sizeof(edid_meso999));
		prop_dictionary_set(dict, "EDID:1", edid);
		prop_object_release(edid);
	}
}
