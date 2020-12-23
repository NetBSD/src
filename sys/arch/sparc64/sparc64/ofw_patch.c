/*	$NetBSD: ofw_patch.c,v 1.7 2020/12/23 07:01:14 jdc Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ofw_patch.c,v 1.7 2020/12/23 07:01:14 jdc Exp $");

#include <sys/param.h>

#include <dev/i2c/i2cvar.h>
#include <dev/scsipi/scsipiconf.h>

#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <sparc64/sparc64/ofw_patch.h>
#include <sparc64/sparc64/static_edid.h>

static void
add_gpio_pin(prop_array_t pins, const char *name, int num, int act, int def)
{
	prop_dictionary_t pin = prop_dictionary_create();
	prop_dictionary_set_string(pin, "name", name);
	prop_dictionary_set_uint32(pin, "pin", num);
	prop_dictionary_set_bool(pin, "active_high", act);
	if (def != -1)
		prop_dictionary_set_int32(pin, "default_state", def);
	prop_array_add(pins, pin);
	prop_object_release(pin);
}

static prop_array_t
create_i2c_dict(device_t busdev)
{
	prop_dictionary_t props = device_properties(busdev);
	prop_array_t cfg = NULL;

	cfg = prop_dictionary_get(props, "i2c-child-devices");
	if (!cfg) {
		DPRINTF(ACDB_PROBE, ("\nCreating new i2c-child-devices\n"));
		cfg = prop_array_create();
		prop_dictionary_set(props, "i2c-child-devices", cfg);
		prop_dictionary_set_bool(props, "i2c-indirect-config", false);
	}
	return cfg;
}

static void
add_i2c_device(prop_array_t cfg, const char *name, const char *compat,
uint32_t addr, uint64_t node)
{
	prop_dictionary_t dev;

	DPRINTF(ACDB_PROBE, ("\nAdding i2c device: %s (%s) @ 0x%x (%lx)\n",
	    name, compat == NULL ? "NULL" : compat, addr, node & 0xffffffff));
	dev = prop_dictionary_create();
	prop_dictionary_set_string(dev, "name", name);
	if (compat != NULL)
		prop_dictionary_set_data(dev, "compatible", compat,
		    strlen(compat) + 1);
	prop_dictionary_set_uint32(dev, "addr", addr);
	prop_dictionary_set_uint64(dev, "cookie", node);
	prop_array_add(cfg, dev);
	prop_object_release(dev);
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
			add_gpio_pin(pins, "LED indicator", 7, 0, -1);
			add_gpio_pin(pins, "LED fault", 5, 0, 0);
			add_gpio_pin(pins, "LED power", 4, 0, 1);
			prop_dictionary_set(dict, "pins", pins);
			prop_object_release(pins);
			break;
		case 0x23:	/* drive bay O/1 LEDs */
			pins = prop_array_create();
			add_gpio_pin(pins, "LED bay0_fault", 10, 0, 0);
			add_gpio_pin(pins, "LED bay1_fault", 11, 0, 0);
			add_gpio_pin(pins, "LED bay0_remove", 12, 0, 0);
			add_gpio_pin(pins, "LED bay1_remove", 13, 0, 0);
			prop_dictionary_set(dict, "pins", pins);
			prop_object_release(pins);
			break;
		case 0x25:	/* drive bay 2/3 LEDs (v240 only)*/
			pins = prop_array_create();
			add_gpio_pin(pins, "LED bay2_fault", 10, 0, 0);
			add_gpio_pin(pins, "LED bay3_fault", 11, 0, 0);
			add_gpio_pin(pins, "LED bay2_remove", 12, 0, 0);
			add_gpio_pin(pins, "LED bay3_remove", 13, 0, 0);
			prop_dictionary_set(dict, "pins", pins);
			prop_object_release(pins);
			break;
	}
}

void
add_gpio_props_e250(device_t dev, void *aux)
{
	struct i2c_attach_args *ia = aux;
	prop_dictionary_t dict = device_properties(dev);
	prop_array_t pins;

	switch (ia->ia_addr) {
		case 0x38:	/* interrupt status */
			pins = prop_array_create();
			add_gpio_pin(pins, "ALERT high_temp", 1, 0, 30);
			add_gpio_pin(pins, "ALERT disk_event", 2, 0, 30);
			add_gpio_pin(pins, "ALERT fan_fail", 4, 0, 30);
			add_gpio_pin(pins, "ALERT key_event", 5, 0, 30);
			add_gpio_pin(pins, "ALERT psu_event", 6, 0, 30);
			prop_dictionary_set(dict, "pins", pins);
			prop_object_release(pins);
			break;
		case 0x39:	/* PSU status */
			pins = prop_array_create();
			add_gpio_pin(pins, "INDICATOR psu0_present", 0, 0, -1);
			add_gpio_pin(pins, "INDICATOR psu1_present", 1, 0, -1);
			add_gpio_pin(pins, "INDICATOR psu0_fault", 4, 0, -1);
			add_gpio_pin(pins, "INDICATOR psu1_fault", 5, 0, -1);
			prop_dictionary_set(dict, "pins", pins);
			prop_object_release(pins);
			break;
		case 0x3d:	/* disk status */
			pins = prop_array_create();
			add_gpio_pin(pins, "INDICATOR disk0_present",
			    0, 0, -1);
			add_gpio_pin(pins, "INDICATOR disk1_present",
			    1, 0, -1);
			add_gpio_pin(pins, "INDICATOR disk2_present",
			    2, 0, -1);
			add_gpio_pin(pins, "INDICATOR disk3_present",
			    3, 0, -1);
			add_gpio_pin(pins, "INDICATOR disk4_present",
			    4, 0, -1);
			add_gpio_pin(pins, "INDICATOR disk5_present",
			    5, 0, -1);
			prop_dictionary_set(dict, "pins", pins);
			prop_object_release(pins);
			break;
		case 0x3e:	/* front panel LEDs (E250/E450) */
			pins = prop_array_create();
			add_gpio_pin(pins, "LED disk_fault", 0, 0, -1);
			add_gpio_pin(pins, "LED psu_fault", 1, 0, -1);
			add_gpio_pin(pins, "LED overtemp", 2, 0, -1);
			add_gpio_pin(pins, "LED fault", 3, 0, -1);
			add_gpio_pin(pins, "LED activity", 4, 0, -1);
			/* Pin 5 is power LED, but not controllable */
			add_gpio_pin(pins, "INDICATOR key_normal", 6, 0, -1);
			add_gpio_pin(pins, "INDICATOR key_diag", 7, 0, -1);
			/* If not "normal" or "diag", key is "lock" */
			prop_dictionary_set(dict, "pins", pins);
			prop_object_release(pins);
			break;
		case 0x3f:	/* disk fault LEDs */
			pins = prop_array_create();
			add_gpio_pin(pins, "LED disk0_fault", 0, 0, -1);
			add_gpio_pin(pins, "LED disk1_fault", 1, 0, -1);
			add_gpio_pin(pins, "LED disk2_fault", 2, 0, -1);
			add_gpio_pin(pins, "LED disk3_fault", 3, 0, -1);
			add_gpio_pin(pins, "LED disk4_fault", 4, 0, -1);
			add_gpio_pin(pins, "LED disk5_fault", 5, 0, -1);
			prop_dictionary_set(dict, "pins", pins);
			prop_object_release(pins);
			break;
	}
}

void
add_drivebay_props(device_t dev, int ofnode, void *aux)
{
	struct scsipibus_attach_args *sa = aux;
	int target = sa->sa_periph->periph_target;
	prop_dictionary_t dict = device_properties(dev);
	char path[256]= "";
	char name[16];
	int nbays;

	if ((strcmp(machine_model, "SUNW,Sun-Fire-V210") == 0) ||
	    (strcmp(machine_model, "SUNW,Sun-Fire-V240") == 0)) {
		OF_package_to_path(ofnode, path, sizeof(path));

		/* see if we're on the onboard controller's 1st channel */
		if (strcmp(path, "/pci@1c,600000/scsi@2") != 0)
			return;

		/* yes, yes we are */
		if (strcmp(machine_model, "SUNW,Sun-Fire-V240") == 0)
			nbays = 4;
		else
			nbays = 2;
		if ( target < nbays) {
			snprintf(name, sizeof(name), "bay%d", target);
			prop_dictionary_set_string(dict, "location", name);
		}
	}

	if (!strcmp(machine_model, "SUNW,Ultra-250")) {
		OF_package_to_path(ofnode, path, sizeof(path));

		/* see if we're on the onboard controller's 1st channel */
		if (strcmp(path, "/pci@1f,4000/scsi@3") != 0)
			return;

		/* disk 0 is target 0 */
		if (!target) {
			strncpy(name, "bay0", sizeof(name));
			prop_dictionary_set_string(dict, "location", name);
		/* disks 1 - 5 are targets 8 - 12 */
		} else if ( target < 13) {
			snprintf(name, sizeof(name), "bay%d", target - 7);
			prop_dictionary_set_string(dict, "location", name);
		}
	}
}

/*
 * Add SPARCle spdmem devices (0x50 and 0x51) that are not in the OFW tree
 */
void
add_spdmem_props_sparcle(device_t busdev)
{
	prop_array_t cfg;
	int i;

	DPRINTF(ACDB_PROBE, ("\nAdding spdmem for SPARCle "));

	cfg = create_i2c_dict(busdev);
	for (i = 0x50; i <= 0x51; i++)
		add_i2c_device(cfg, "dimm-spd", NULL, i, 0);
	prop_object_release(cfg);
}

/*
 * Add V210/V240 environmental sensors that are not in the OFW tree.
 */
void
add_env_sensors_v210(device_t busdev)
{
	prop_array_t cfg;

	DPRINTF(ACDB_PROBE, ("\nAdding sensors for %s ", machine_model));
	cfg = create_i2c_dict(busdev);

	/* ADM1026 at 0x2e */
	add_i2c_device(cfg, "hardware-monitor", "i2c-adm1026", 0x2e, 0);
	/* LM75 at 0x4e */
	add_i2c_device(cfg, "temperature-sensor", "i2c-lm75", 0x4e, 0);
}

/* Sensors and GPIO's for E450 and E250 */
void
add_i2c_props_e450(device_t busdev, uint64_t node)
{
	prop_array_t cfg;

	DPRINTF(ACDB_PROBE, ("\nAdding sensors for %s ", machine_model));
	cfg = create_i2c_dict(busdev);

	/* Power supply 1 temperature. */
	add_i2c_device(cfg, "PSU-1", "ecadc", 0x48, node);

	/* Power supply 2 temperature. */
	add_i2c_device(cfg, "PSU-2", "ecadc", 0x49, node);

	/* Power supply 3 temperature. */
	add_i2c_device(cfg, "PSU-3", "ecadc", 0x4a, node);

	/* Ambient temperature. */
	add_i2c_device(cfg, "ambient", "i2c-lm75", 0x4d, node);

	/* CPU temperatures. */
	add_i2c_device(cfg, "CPU", "ecadc", 0x4f, node);

	prop_object_release(cfg);
}

void
add_i2c_props_e250(device_t busdev, uint64_t node)
{
	prop_array_t cfg;
	int i;

	DPRINTF(ACDB_PROBE, ("\nAdding sensors for %s ", machine_model));
	cfg = create_i2c_dict(busdev);

	/* PSU temperature / CPU fan */
	add_i2c_device(cfg, "PSU", "ecadc", 0x4a, node);

	/* CPU & system board temperature */
	add_i2c_device(cfg, "CPU", "ecadc", 0x4f, node);

	/* GPIO's */
	for (i = 0x38; i <= 0x39; i++)
		add_i2c_device(cfg, "gpio", "i2c-pcf8574", i, node);
	for (i = 0x3d; i <= 0x3f; i++)
		add_i2c_device(cfg, "gpio", "i2c-pcf8574", i, node);

	/* NVRAM */
	add_i2c_device(cfg, "nvram", "i2c-at24c02", 0x52, node);

	/* RSC clock */
	add_i2c_device(cfg, "rscrtc", "i2c-ds1307", 0x68, node);

	prop_object_release(cfg);
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
