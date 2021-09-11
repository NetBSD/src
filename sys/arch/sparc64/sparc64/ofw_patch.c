/*	$NetBSD: ofw_patch.c,v 1.7.14.3 2021/09/11 01:03:18 thorpej Exp $ */

/*-
 * Copyright (c) 2020, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman and Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: ofw_patch.c,v 1.7.14.3 2021/09/11 01:03:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/kmem.h>

#include <dev/i2c/i2cvar.h>
#include <dev/scsipi/scsipiconf.h>

#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <sparc64/sparc64/ofw_patch.h>
#include <sparc64/sparc64/static_edid.h>

/*****************************************************************************
 * GPIO fixup support
 *****************************************************************************/

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

struct gpio_pin_fixup {
	const char *name;
	int num;
	int act;
	int def;
};

static void
add_gpio_pins(device_t dev, const struct gpio_pin_fixup *addpins, int npins)
{
	prop_dictionary_t dict = device_properties(dev);
	prop_array_t pins = prop_array_create();
	int i;

	for (i = 0; i < npins; i++) {
		add_gpio_pin(pins, addpins[i].name, addpins[i].num,
		    addpins[i].act, addpins[i].def);
	}

	prop_dictionary_set(dict, "pins", pins);
	prop_object_release(pins);
}

/*****************************************************************************
 * I2C device fixup support
 *****************************************************************************/

/*
 * On some systems, there are lots of i2c devices missing from the
 * device tree.
 *
 * The way we deal with this is by subclassing the controller's
 * devhandle_impl and overriding the "i2c-enumerate-devices" device
 * call.  Our implementation will enumerate using the super-class
 * (OpenFirmware enumeration), and then enumerate the missing entries
 * from our own static tables.
 *
 * This devhandle_impl will be wrapped inside of a container structure
 * that will point to the extra devices that need to be added as children
 * of that node.  The "i2c-enumerate-devices" call will first enumerate
 * devices that are present in the device tree, and then enumerate the
 * additions.
 */

struct i2c_fixup_container {
	struct devhandle_impl i2c_devhandle_impl;
	devhandle_t i2c_super_handle;
	const struct i2c_deventry *i2c_additions;
	int i2c_nadditions;
};

static int
i2c_fixup_enumerate_devices(device_t dev, devhandle_t call_handle, void *v)
{
	struct i2c_enumerate_devices_args *args = v;
	const struct devhandle_impl *impl = call_handle.impl;
	const struct i2c_fixup_container *fixup =
	    container_of((struct devhandle_impl *)__UNCONST(impl),
			 struct i2c_fixup_container, i2c_devhandle_impl);
	devhandle_t super_handle = fixup->i2c_super_handle;
	device_call_t super_call;
	int super_error, error;

	/* First, enumerate using whatever is in the device tree. */
	super_call = devhandle_lookup_device_call(super_handle,
	    "i2c-enumerate-devices", &super_handle);
	super_error = super_call != NULL ? super_call(dev, super_handle, args)
					 : 0;

	/* Now enumerate our additions. */
	KASSERT(fixup->i2c_additions != NULL);
	error = i2c_enumerate_deventries(dev, call_handle, args,
	    fixup->i2c_additions, fixup->i2c_nadditions);

	return super_error != 0 ? super_error : error;
}

static device_call_t
i2c_fixup_lookup_device_call(devhandle_t handle, const char *name,
    devhandle_t *call_handlep)
{
	if (strcmp(name, "i2c-enumerate-devices") == 0) {
		return i2c_fixup_enumerate_devices;
	}

	/* Defer everything else to the "super". */
	return NULL;
}

static void
add_i2c_devices(device_t dev, const struct i2c_deventry *i2c_adds,
    unsigned int nadds)
{
	struct i2c_fixup_container *fixup;

	fixup = kmem_alloc(sizeof(*fixup), KM_SLEEP);

	fixup->i2c_additions = i2c_adds;
	fixup->i2c_nadditions = nadds;

	/* Stash away the super-class handle. */
	devhandle_t devhandle = device_handle(dev);
	fixup->i2c_super_handle = devhandle;

	/* Sub-class the devhandle_impl. */
	devhandle_impl_inherit(&fixup->i2c_devhandle_impl, devhandle.impl);
	fixup->i2c_devhandle_impl.lookup_device_call =
	    i2c_fixup_lookup_device_call;

	/*
	 * ...and slide that on into the device.  This handle will be
	 * passed on to the iic bus instance, and our enumeration method
	 * will get called to enumerate the child devices.
	 */
	devhandle.impl = &fixup->i2c_devhandle_impl;
	device_set_handle(dev, devhandle);
}

/*****************************************************************************
 * System fixup machinery
 *****************************************************************************/

struct system_fixup {
	const struct device_compatible_entry *dtnode_fixups;
	void (*special_fixups)(device_t, void *);
};

static bool
device_handle_matches_phandle(device_t dev, int phandle)
{
	devhandle_t devhandle = device_handle(dev);

	if (devhandle_type(devhandle) != DEVHANDLE_TYPE_OF) {
		return false;
	}
	return devhandle_to_of(devhandle) == phandle;
}

static int onboard_scsi_phandle __read_mostly;

static void
onboard_scsi_fixup(device_t dev, void *aux)
{
	/* We're just going to remember the phandle for now. */
	devhandle_t devhandle = device_handle(dev);
	KASSERT(devhandle_type(devhandle) == DEVHANDLE_TYPE_OF);
	onboard_scsi_phandle = devhandle_to_of(devhandle);
}

static bool
device_is_on_onboard_scsi(device_t dev)
{
	device_t parent = device_parent(dev);

	if (onboard_scsi_phandle == 0) {
		return false;
	}

	if (!device_is_a(parent, "scsibus")) {
		return false;
	}

	device_t grandparent = device_parent(parent);
	KASSERT(grandparent != NULL);

	return device_handle_matches_phandle(grandparent, onboard_scsi_phandle);
}

/*****************************************************************************
 * System fixups for SUNW,Sun-Fire-V210 and SUNW,Sun-Fire-V240
 *****************************************************************************/

static void
v210_drive_bay_0_1_fixup(device_t dev, void *aux)
{
	static const struct gpio_pin_fixup addpins[] = {
		{ .name = "LED bay0_fault",  .num = 10, .act = 0, .def = 0 },
		{ .name = "LED bay1_fault",  .num = 11, .act = 0, .def = 0 },
		{ .name = "LED bay0_remove", .num = 12, .act = 0, .def = 0 },
		{ .name = "LED bay1_remove", .num = 13, .act = 0, .def = 0 },
	};
	add_gpio_pins(dev, addpins, __arraycount(addpins));
}

static void
v240_drive_bay_2_3_fixup(device_t dev, void *aux)
{
	static const struct gpio_pin_fixup addpins[] = {
		{ .name = "LED bay2_fault",  .num = 10, .act = 0, .def = 0 },
		{ .name = "LED bay3_fault",  .num = 11, .act = 0, .def = 0 },
		{ .name = "LED bay2_remove", .num = 12, .act = 0, .def = 0 },
		{ .name = "LED bay3_remove", .num = 13, .act = 0, .def = 0 },
	};
	add_gpio_pins(dev, addpins, __arraycount(addpins));
}

static void
v210_front_panel_fixup(device_t dev, void *aux)
{
	static const struct gpio_pin_fixup addpins[] = {
		{ .name = "LED indicator", .num = 7, .act = 0, .def = -1 },
		{ .name = "LED fault",     .num = 5, .act = 0, .def = 0 },
		{ .name = "LED power",     .num = 4, .act = 0, .def = 1 },
	};
	add_gpio_pins(dev, addpins, __arraycount(addpins));
}

static int v210_env_sensors_i2c_phandle __read_mostly;

static void
v210_env_sensors_fixup(device_t dev, void *aux)
{
	static const struct i2c_deventry i2c_adds[] = {
		{ .name = "hardware-monitor",
		  .compat = "i2c-adm1026", .addr = 0x2e },

		{ .name = "temperature-sensor",
		  .compat = "i2c-lm75", .addr = 0x4e },
	};

	/* Squirrel away the phandle for later. */
	KASSERT(v210_env_sensors_i2c_phandle == 0);
	devhandle_t devhandle = device_handle(dev);
	v210_env_sensors_i2c_phandle = devhandle_to_of(devhandle);

	add_i2c_devices(dev, i2c_adds, __arraycount(i2c_adds));
}

static const struct device_compatible_entry dtnode_fixup_table_v210[] = {
	{ .compat = "/pci/isa@7/i2c@0,320/gpio@0,46",
	  .data = v210_drive_bay_0_1_fixup },

	/* V240 only */
	{ .compat = "/pci/isa@7/i2c@0,320/gpio@0,4a",
	  .data = v240_drive_bay_2_3_fixup },

	{ .compat = "/pci/isa@7/i2c@0,320/gpio@0,70",
	  .data = v210_front_panel_fixup },

	{ .compat = "/pci/isa@7/i2c@0,320",
	  .data = v210_env_sensors_fixup },

	{ .compat = "/pci@1c,600000/scsi@2",
	  .data = onboard_scsi_fixup },

	DEVICE_COMPAT_EOL
};

static void
v210_v240_special_fixups_onboard_scsi(device_t dev, void *aux, int nbays)
{
	if (device_is_on_onboard_scsi(dev)) {
		struct scsipibus_attach_args *sa = aux;
		int target = sa->sa_periph->periph_target;
		prop_dictionary_t dict = device_properties(dev);
		char name[16];

		if (target < nbays) {
			snprintf(name, sizeof(name), "bay%d", target);
			prop_dictionary_set_string(dict, "location", name);
		}
	}
}

static void
v210_special_fixups_hardware_monitor(device_t dev, void *aux)
{
	device_t parent = device_parent(dev);

	/* Set some properties on the fan sensor. */

	if (!device_is_a(parent, "iic")) {
		return;
	}

	device_t grandparent = device_parent(parent);
	KASSERT(grandparent != NULL);


	if (! device_handle_matches_phandle(grandparent,
					    v210_env_sensors_i2c_phandle)) {
		return;
	}

	struct i2c_attach_args *ia = aux;
	if (ia->ia_addr == 0x2e) {
		prop_dictionary_t props = device_properties(dev);
		prop_dictionary_set_uint8(props, "fan_div2", 0x55);
		prop_dictionary_set_bool(props, "multi_read", true);
	}
}

static void
v210_special_fixups(device_t dev, void *aux)
{
	v210_v240_special_fixups_onboard_scsi(dev, aux, 2);
	v210_special_fixups_hardware_monitor(dev, aux);
}

static void
v240_special_fixups(device_t dev, void *aux)
{
	v210_v240_special_fixups_onboard_scsi(dev, aux, 4);
	v210_special_fixups_hardware_monitor(dev, aux);
}

static const struct system_fixup system_fixups_v210 = {
	.dtnode_fixups = dtnode_fixup_table_v210,
	.special_fixups = v210_special_fixups,
};

static const struct system_fixup system_fixups_v240 = {
	.dtnode_fixups = dtnode_fixup_table_v210,
	.special_fixups = v240_special_fixups,
};

/*****************************************************************************
 * System fixups for SUNW,Sun-Fire-V440
 *****************************************************************************/

static void
v440_hardware_monitor_fixup(device_t dev, void *aux)
{
	prop_dictionary_t props = device_properties(dev);
	prop_dictionary_set_bool(props, "multi_read", true);
}

static const struct device_compatible_entry dtnode_fixup_table_v440[] = {
	{ .compat = "/pci/isa@7/i2c@0,320/hardware-monitor@0,5c",
	  .data = v440_hardware_monitor_fixup },

	DEVICE_COMPAT_EOL
};

static const struct system_fixup system_fixups_v440 = {
	.dtnode_fixups = dtnode_fixup_table_v440,
};

/*****************************************************************************
 * System fixups for SUNW,Ultra-250
 *****************************************************************************/

static int e250_envctrltwo_phandle __read_mostly;

static void
e250_envctrltwo_fixup(device_t dev, void *aux)
{
	static const struct i2c_deventry i2c_adds[] = {
		/* PSU temperature / CPU fan */
		{ .name = "PSU", .compat = "ecadc", .addr = 0x4a },

		/* CPU and system board temperature */
		{ .name = "CPU", .compat = "ecadc", .addr = 0x4f },

		/* GPIOs */
		{ .name = "gpio", .compat = "i2c-pcf8574", .addr = 0x38 },
		{ .name = "gpio", .compat = "i2c-pcf8574", .addr = 0x39 },
		{ .name = "gpio", .compat = "i2c-pcf8574", .addr = 0x3d },
		{ .name = "gpio", .compat = "i2c-pcf8574", .addr = 0x3e },
		{ .name = "gpio", .compat = "i2c-pcf8574", .addr = 0x3f },

		/* NVRAM */
		{ .name = "nvram", .compat = "i2c-at24c02", .addr = 0x52 },

		/* RSC clock */
		{ .name = "rscrtc", .compat = "i2c-ds1307", .addr = 0x68 },
	};
	devhandle_t devhandle = device_handle(dev);
	KASSERT(devhandle_type(devhandle) == DEVHANDLE_TYPE_OF);

	/* Squirrel away the phandle for later. */
	KASSERT(e250_envctrltwo_phandle == 0);
	e250_envctrltwo_phandle = devhandle_to_of(devhandle);

	add_i2c_devices(dev, i2c_adds, __arraycount(i2c_adds));
}

static const struct device_compatible_entry dtnode_fixup_table_e250[] = {
	{ .compat = "/pci@1f,4000/ebus@1/SUNW,envctrltwo@14,600000",
	  .data = e250_envctrltwo_fixup },

	{ .compat = "/pci@1f,4000/scsi@3",
	  .data = onboard_scsi_fixup, },

	DEVICE_COMPAT_EOL
};

static void
e250_special_fixups_envctrltwo(device_t dev, void *aux)
{
	/* interrupt status */
	static const struct gpio_pin_fixup gpio_0x38_addpins[] = {
		{ .name = "ALERT high_temp",  .num = 1, .act = 0, .def = 30 },
		{ .name = "ALERT disk_event", .num = 2, .act = 0, .def = 30 },
		{ .name = "ALERT fan_fail",   .num = 4, .act = 0, .def = 30 },
		{ .name = "ALERT key_event",  .num = 5, .act = 0, .def = 30 },
		{ .name = "ALERT psu_event",  .num = 6, .act = 0, .def = 30 },
	};

	/* PSU status */
	static const struct gpio_pin_fixup gpio_0x39_addpins[] = {
		{ .name = "INDICATOR psu0_present",
		  .num = 0, .act = 0, .def = -1 },
		{ .name = "INDICATOR psu1_present",
		  .num = 1, .act = 0, .def = -1 },
		{ .name = "INDICATOR psu0_fault",
		  .num = 4, .act = 0, .def = -1 },
		{ .name = "INDICATOR psu1_fault",
		  .num = 5, .act = 0, .def = -1 },
	};

	/* disk status */
	static const struct gpio_pin_fixup gpio_0x3d_addpins[] = {
		{ .name = "INDICATOR disk0_present",
		  .num = 0, .act = 0, .def = -1 },
		{ .name = "INDICATOR disk1_present",
		  .num = 1, .act = 0, .def = -1 },
		{ .name = "INDICATOR disk2_present",
		  .num = 2, .act = 0, .def = -1 },
		{ .name = "INDICATOR disk3_present",
		  .num = 3, .act = 0, .def = -1 },
		{ .name = "INDICATOR disk4_present",
		  .num = 4, .act = 0, .def = -1 },
		{ .name = "INDICATOR disk5_present",
		  .num = 5, .act = 0, .def = -1 },
	};

	/* front panel */
	static const struct gpio_pin_fixup gpio_0x3e_addpins[] = {
		{ .name = "LED disk_fault",
		  .num = 0, .act = 0, .def = -1 },
		{ .name = "LED psu_fault",
		  .num = 1, .act = 0, .def = -1 },
		{ .name = "LED overtemp",
		  .num = 2, .act = 0, .def = -1 },
		{ .name = "LED fault",
		  .num = 3, .act = 0, .def = -1 },
		{ .name = "LED activity",
		  .num = 4, .act = 0, .def = -1 },

		/* Pin 5 is power LED, but is not controllable. */

		{ .name = "INDICATOR key_normal",
		  .num = 6, .act = 0, .def = -1 },
		{ .name = "INDICATOR key_diag",
		  .num = 7, .act = 0, .def = -1 },
		/* If not "normal" or "diag", key is "lock". */
	};

	/* disk fault LEDs */
	static const struct gpio_pin_fixup gpio_0x3f_addpins[] = {
		{ .name = "LED disk0_fault", .num = 0, .act = 0, .def = -1 },
		{ .name = "LED disk1_fault", .num = 1, .act = 0, .def = -1 },
		{ .name = "LED disk2_fault", .num = 2, .act = 0, .def = -1 },
		{ .name = "LED disk3_fault", .num = 3, .act = 0, .def = -1 },
		{ .name = "LED disk4_fault", .num = 4, .act = 0, .def = -1 },
		{ .name = "LED disk5_fault", .num = 5, .act = 0, .def = -1 },
	};

	/*
	 * We need to fix up GPIO pin properties for GPIO controllers
	 * that don't appear in the device tree.
	 *
	 * We need to check if our parent is "iic" and grandparent
	 * is the envctrltwo node.
	 */
	device_t parent = device_parent(dev);
	if (!device_is_a(parent, "iic")) {
		return;
	}

	device_t grandparent = device_parent(dev);
	KASSERT(grandparent != NULL);

	if (! device_handle_matches_phandle(grandparent,
					    e250_envctrltwo_phandle)) {
		return;
	}

	struct i2c_attach_args *ia = aux;
	const struct gpio_pin_fixup *addpins;
	int naddpins;

	switch (ia->ia_addr) {
	case 0x38:
		addpins = gpio_0x38_addpins;
		naddpins = __arraycount(gpio_0x38_addpins);
		break;

	case 0x39:
		addpins = gpio_0x39_addpins;
		naddpins = __arraycount(gpio_0x39_addpins);
		break;

	case 0x3d:
		addpins = gpio_0x3d_addpins;
		naddpins = __arraycount(gpio_0x3d_addpins);
		break;

	case 0x3e:
		addpins = gpio_0x3e_addpins;
		naddpins = __arraycount(gpio_0x3e_addpins);
		break;

	case 0x3f:
		addpins = gpio_0x3f_addpins;
		naddpins = __arraycount(gpio_0x3f_addpins);
		break;

	default:
		/* No fixups. */
		return;
	}
	add_gpio_pins(dev, addpins, naddpins);
}

static void
e250_special_fixups_onboard_scsi(device_t dev, void *aux)
{
	if (device_is_on_onboard_scsi(dev)) {
		struct scsipibus_attach_args *sa = aux;
		int target = sa->sa_periph->periph_target;
		prop_dictionary_t dict = device_properties(dev);
		char name[16];

		/*
		 * disk 0 is target 0.
		 * disks 1 - 5 are targets 8 - 12.
		 */
		if (target == 0) {
			prop_dictionary_set_string(dict, "location", "bay0");
		} else if (target >= 8 && target <= 12) {
			snprintf(name, sizeof(name), "bay%d", target - 7);
			prop_dictionary_set_string(dict, "location", name);
		}
	}
}

static void
e250_special_fixups(device_t dev, void *aux)
{
	e250_special_fixups_envctrltwo(dev, aux);
	e250_special_fixups_onboard_scsi(dev, aux);
}

static const struct system_fixup system_fixups_e250 = {
	.dtnode_fixups = dtnode_fixup_table_e250,
	.special_fixups = e250_special_fixups,
};

/*****************************************************************************
 * System fixups for SUNW,Ultra-4
 *****************************************************************************/

static void
e450_envctrl_fixup(device_t dev, void *aux)
{
	static const struct i2c_deventry i2c_adds[] = {
		/* Power supply 1 temperature. */
		{ .name = "PSU-1", .compat = "ecadc", .addr = 0x48 },

		/* Power supply 2 temperature. */
		{ .name = "PSU-2", .compat = "ecadc", .addr = 0x49 },

		/* Power supply 3 temperature. */
		{ .name = "PSU-3", .compat = "ecadc", .addr = 0x4a },

		/* Ambient temperature. */
		{ .name = "ambient", .compat = "i2c-lm75", .addr = 0x4d },

		/* CPU temperatures. */
		{ .name = "CPU", .compat = "ecadc", .addr = 0x4f },
	};
	devhandle_t devhandle = device_handle(dev);
	KASSERT(devhandle_type(devhandle) == DEVHANDLE_TYPE_OF);
	add_i2c_devices(dev, i2c_adds, __arraycount(i2c_adds));
}

static const struct device_compatible_entry dtnode_fixup_table_e450[] = {
	{ .compat = "/pci/ebus@1/SUNW,envctrl@14,600000",
	  .data = e450_envctrl_fixup },

	DEVICE_COMPAT_EOL
};

static const struct system_fixup system_fixups_e450 = {
	.dtnode_fixups = dtnode_fixup_table_e450,
};

/*****************************************************************************
 * System fixups for TAD,SPARCLE
 *****************************************************************************/

static void
sparcle_smbus_fixup(device_t dev, void *aux)
{
	static const struct i2c_deventry i2c_adds[] = {
		{ .name = "dimm-spd",	.addr = 0x50 },
		{ .name = "dimm-spd",	.addr = 0x51 },
	};
	devhandle_t devhandle = device_handle(dev);
	KASSERT(devhandle_type(devhandle) == DEVHANDLE_TYPE_OF);
	add_i2c_devices(dev, i2c_adds, __arraycount(i2c_adds));
};

static const struct device_compatible_entry dtnode_fixup_table_sparcle[] = {
	{ .compat = "/pci/ebut@11",
	  .data = sparcle_smbus_fixup },

	DEVICE_COMPAT_EOL
};

static const struct system_fixup system_fixups_sparcle = {
	.dtnode_fixups = dtnode_fixup_table_sparcle,
};

/*****************************************************************************
 * System fixups for NATE,Meso-999
 *****************************************************************************/

static void
meso999_edid_fixup(device_t dev, void *aux)
{
	prop_dictionary_t props = device_properties(dev);
	prop_dictionary_set_data(props, "EDID:1",
	    edid_meso999, sizeof(edid_meso999));
}

static const struct device_compatible_entry dtnode_fixup_table_meso999[] = {
	{ .compat = "/pci/SUNW,XVR-100@2",
	  .data = meso999_edid_fixup },

	DEVICE_COMPAT_EOL
};

static const struct system_fixup system_fixups_meso999 = {
	.dtnode_fixups = dtnode_fixup_table_meso999,
};

/*****************************************************************************
 * Universal fixups
 *****************************************************************************/

static void
universal_fixups(device_t dev, void *aux)
{
	/*
	 * If the parent of an "iic" instance is named "pmu" in the
	 * device tree, then we need to fix up the devhandle for the
	 * bus because the device tree topology is:
	 *
	 *	pmu -> i2c -> [devices...]
	 */
	if (device_is_a(dev, "iic")) {
		device_t parent = device_parent(dev);
		devhandle_t parent_devhandle = device_handle(parent);
		char name[32];

		if (devhandle_type(parent_devhandle) != DEVHANDLE_TYPE_OF) {
			return;
		}
		int parent_phandle = devhandle_to_of(parent_devhandle);

		if (OF_getprop(parent_phandle, "name", name,
			       sizeof(name)) <= 0) {
			return;
		}
		name[sizeof(name) - 1] = '\0';	/* sanity */
		if (strcmp(name, "pmu") != 0) {
			return;
		}

		int node;
		for (node = OF_child(parent_phandle); node != 0;
		     node = OF_peer(node)) {
			if (OF_getprop(node, "name", name,
				       sizeof(name)) <= 0) {
				continue;
			}
			if (strcmp(name, "i2c") == 0) {
				device_set_handle(dev,
				    devhandle_from_of(node));
				break;
			}
		}
	}
}

/*****************************************************************************
 * End of system-specific data
 *****************************************************************************/

/*
 * Some systems are missing some important information in the
 * OpenFirmware device tree, or the tree has some quirks we need
 * to deal with.
 */
static const struct device_compatible_entry system_fixup_table[] = {
	{ .compat = "SUNW,Sun-Fire-V210", .data = &system_fixups_v210 },
	{ .compat = "SUNW,Sun-Fire-V240", .data = &system_fixups_v240 },

	{ .compat = "SUNW,Sun-Fire-V440", .data = &system_fixups_v440 },

	{ .compat = "SUNW,Ultra-250", .data = &system_fixups_e250 },

	{ .compat = "SUNW,Ultra-4", .data = &system_fixups_e450 },

	{ .compat = "TAD,SPARCLE", .data = &system_fixups_sparcle },

	{ .compat = "NATE,Meso-999", .data = &system_fixups_meso999 },

	DEVICE_COMPAT_EOL
};

#define	MAX_PACKAGE_PATH	512

void
sparc64_device_tree_fixup(device_t dev, void *aux)
{
	static const struct system_fixup *system_fixup_entry;
	static bool system_fixup_entry_initialized;
	const struct device_compatible_entry *dce;
	void (*fn)(device_t, void *);
	devhandle_t devhandle;

	/* First, deal with some universal fixups. */
	universal_fixups(dev, aux);

	devhandle = device_handle(dev);

	if (! system_fixup_entry_initialized) {
		dce = device_compatible_lookup((const char **)&machine_model, 1,
		    system_fixup_table);
		if (dce != NULL) {
			system_fixup_entry = dce->data;
		}
		system_fixup_entry_initialized = true;
	}
	if (system_fixup_entry == NULL) {
		/* No fixups for this machine. */
		return;
	}

	/*
	 * First apply any applicable fixups to this device based
	 * on its node in the device tree.
	 */
	if (system_fixup_entry->dtnode_fixups != NULL &&
	    devhandle_type(devhandle) == DEVHANDLE_TYPE_OF) {
		int phandle = devhandle_to_of(devhandle);
		char *package_path = kmem_zalloc(MAX_PACKAGE_PATH, KM_SLEEP);
		int path_size;

		path_size = OF_package_to_path(phandle, package_path,
		    MAX_PACKAGE_PATH);
		package_path[MAX_PACKAGE_PATH - 1] = '\0'; /* sanity */
		if (path_size > 0) {
			const char *ccp = package_path;
			dce = device_compatible_lookup(&ccp, 1,
			    system_fixup_entry->dtnode_fixups);
			if (dce != NULL && (fn = dce->data) != NULL) {
				(*fn)(dev, aux);
			}
		}
		kmem_free(package_path, MAX_PACKAGE_PATH);
	}

	/*
	 * Now apply any special fixups (this is mainly applicable to
	 * deivces that do not have nodes in the device tree.
	 */
	if (system_fixup_entry->special_fixups != NULL) {
		(*system_fixup_entry->special_fixups)(dev, aux);
	}
}
