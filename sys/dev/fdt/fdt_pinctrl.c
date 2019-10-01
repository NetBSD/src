/* $NetBSD: fdt_pinctrl.c,v 1.10 2019/10/01 23:32:52 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jason R. Thorpe
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
 * Copyright (c) 2015 Martin Fouts
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_pinctrl.c,v 1.10 2019/10/01 23:32:52 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/gpio.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_pinctrl_controller {
	device_t pc_dev;
	int pc_phandle;
	const struct fdtbus_pinctrl_controller_func *pc_funcs;

	LIST_ENTRY(fdtbus_pinctrl_controller) pc_next;
};

static LIST_HEAD(, fdtbus_pinctrl_controller) fdtbus_pinctrl_controllers =
    LIST_HEAD_INITIALIZER(fdtbus_pinctrl_controllers);

int
fdtbus_register_pinctrl_config(device_t dev, int phandle,
    const struct fdtbus_pinctrl_controller_func *funcs)
{
	struct fdtbus_pinctrl_controller *pc;

	pc = kmem_alloc(sizeof(*pc), KM_SLEEP);
	pc->pc_dev = dev;
	pc->pc_phandle = phandle;
	pc->pc_funcs = funcs;

	LIST_INSERT_HEAD(&fdtbus_pinctrl_controllers, pc, pc_next);

	return 0;
}

static struct fdtbus_pinctrl_controller *
fdtbus_pinctrl_lookup(int phandle)
{
	struct fdtbus_pinctrl_controller *pc;

	LIST_FOREACH(pc, &fdtbus_pinctrl_controllers, pc_next) {
		if (pc->pc_phandle == phandle)
			return pc;
	}

	return NULL;
}

int
fdtbus_pinctrl_set_config_index(int phandle, u_int index)
{
	struct fdtbus_pinctrl_controller *pc;
	const u_int *pinctrl_data;
	char buf[16];
	u_int xref, pinctrl_cells;
	int len, error;

	snprintf(buf, sizeof(buf), "pinctrl-%u", index);

	pinctrl_data = fdtbus_get_prop(phandle, buf, &len);
	if (pinctrl_data == NULL)
		return ENOENT;

	while (len > 0) {
		xref = fdtbus_get_phandle_from_native(be32toh(pinctrl_data[0]));
		pc = fdtbus_pinctrl_lookup(xref);
		if (pc == NULL)
			return ENXIO;

		if (of_getprop_uint32(OF_parent(xref), "#pinctrl-cells", &pinctrl_cells) != 0)
			pinctrl_cells = 1;

		error = pc->pc_funcs->set_config(pc->pc_dev, pinctrl_data, pinctrl_cells * 4);
		if (error != 0)
			return error;

		pinctrl_data += pinctrl_cells;
		len -= (pinctrl_cells * 4);
	}

	return 0;
}

int
fdtbus_pinctrl_set_config(int phandle, const char *cfgname)
{
	u_int index;
	int err;

	err = fdtbus_get_index(phandle, "pinctrl-names", cfgname, &index);
	if (err != 0)
		return ENOENT;

	return fdtbus_pinctrl_set_config_index(phandle, index);
}

bool
fdtbus_pinctrl_has_config(int phandle, const char *cfgname)
{
	u_int index;

	return fdtbus_get_index(phandle, "pinctrl-names", cfgname, &index) == 0;
}

/*
 * Helper routines for parsing put properties related to pinctrl bindings.
 */

/*
 * Pin mux settings apply to sets of pins specified by one of 3
 * sets of properties:
 *
 *	- "pins" + "function"
 *	- "groups" + "function"
 *	- "pinmux"
 *
 * Eactly one of those 3 combinations must be specified.
 */

const char *
fdtbus_pinctrl_parse_function(int phandle)
{
	return fdtbus_get_string(phandle, "function");
}

const void *
fdtbus_pinctrl_parse_pins(int phandle, int *pins_len)
{
	int len;

	/*
	 * The pinctrl bindings specify that entries in "pins"
	 * may be integers or strings; this is determined by
	 * the hardware-specific binding.
	 */

	len = OF_getproplen(phandle, "pins");
	if (len > 0) {
		return fdtbus_get_prop(phandle, "pins", pins_len);
	}

	return NULL;
}

const char *
fdtbus_pinctrl_parse_groups(int phandle, int *groups_len)
{
	int len;

	len = OF_getproplen(phandle, "groups");
	if (len > 0) {
		*groups_len = len;
		return fdtbus_get_string(phandle, "groups");
	}

	return NULL;
}

const u_int *
fdtbus_pinctrl_parse_pinmux(int phandle, int *pinmux_len)
{
	int len;

	len = OF_getproplen(phandle, "pinmux");
	if (len > 0) {
		return fdtbus_get_prop(phandle, "pinmux", pinmux_len);
	}

	return NULL;
}

int
fdtbus_pinctrl_parse_bias(int phandle, int *pull_strength)
{
	const char *bias_prop = NULL;
	int bias = -1;

	/*
	 * bias-pull-{up,down,pin-default} properties have an optional
	 * argument: the pull strength in Ohms.  (In practice, this is
	 * sometimes a hardware-specific constant.)
	 *
	 * XXXJRT How to represent bias-pull-pin-default?
	 */

	if (of_hasprop(phandle, "bias-disable")) {
		bias = 0;
	} else if (of_hasprop(phandle, "bias-pull-up")) {
		bias_prop = "bias-pull-up";
		bias = GPIO_PIN_PULLUP;
	} else if (of_hasprop(phandle, "bias-pull-down")) {
		bias_prop = "bias-pull-down";
		bias = GPIO_PIN_PULLDOWN;
	}

	if (pull_strength) {
		*pull_strength = -1;
		if (bias_prop) {
			uint32_t val;
			if (of_getprop_uint32(phandle, bias_prop, &val) == 0) {
				*pull_strength = (int)val;
			}
		}
	}

	return bias;
}

int
fdtbus_pinctrl_parse_drive(int phandle)
{
	int drive = -1;

	if (of_hasprop(phandle, "drive-push-pull"))
		drive = GPIO_PIN_PUSHPULL;
	else if (of_hasprop(phandle, "drive-open-drain"))
		drive = GPIO_PIN_OPENDRAIN;
	else if (of_hasprop(phandle, "drive-open-source"))
		drive = 0;

	return drive;
}

int
fdtbus_pinctrl_parse_drive_strength(int phandle)
{
	int val;

	/*
	 * drive-strength has as an argument the target strength
	 * in mA.
	 */

	if (of_getprop_uint32(phandle, "drive-strength", &val) == 0)
		return val;

	return -1;
}
int fdtbus_pinctrl_parse_input_output(int phandle, int *output_value)
{
	int direction = -1;
	int pinval = -1;

	if (of_hasprop(phandle, "input-enable")) {
		direction = GPIO_PIN_INPUT;
	} else if (of_hasprop(phandle, "input-disable")) {
		/*
		 * XXXJRT How to represent this?  This is more than
		 * just "don't set the direction" - it's an active
		 * command that might involve disabling an input
		 * buffer on the pin.
		 */
	}

	if (of_hasprop(phandle, "output-enable")) {
		if (direction == -1)
			direction = 0;
		direction |= GPIO_PIN_OUTPUT;
	} else if (of_hasprop(phandle, "output-disable")) {
		if (direction == -1)
			direction = 0;
		direction |= GPIO_PIN_TRISTATE;
	}

	if (of_hasprop(phandle, "output-low")) {
		if (direction == -1)
			direction = 0;
		direction |= GPIO_PIN_OUTPUT;
		pinval = GPIO_PIN_LOW;
	} else if (of_hasprop(phandle, "output-high")) {
		if (direction == -1)
			direction = 0;
		direction |= GPIO_PIN_OUTPUT;
		pinval = GPIO_PIN_HIGH;
	}

	if (output_value)
		*output_value = pinval;

	/*
	 * XXX input-schmitt-enable
	 * XXX input-schmitt-disable
	 */

	if (direction != -1
	    && (direction & (GPIO_PIN_INPUT | GPIO_PIN_OUTPUT))
			 == (GPIO_PIN_INPUT | GPIO_PIN_OUTPUT)) {
		direction |= GPIO_PIN_INOUT;
	}

	return direction;
}
