/*	$NetBSD: fdt_pinctrl.h,v 1.1 2025/09/06 20:11:30 thorpej Exp $	*/

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

#ifndef _DEV_FDT_FDT_PINCTRL_H_
#define	_DEV_FDT_FDT_PINCTRL_H_

#include <sys/device.h>

struct fdtbus_pinctrl_controller;

struct fdtbus_pinctrl_pin {
	struct fdtbus_pinctrl_controller *pp_pc;
	void *pp_priv;
};

struct fdtbus_pinctrl_controller_func {
	int (*set_config)(device_t, const void *, size_t);
};

int		fdtbus_register_pinctrl_config(device_t, int,
		    const struct fdtbus_pinctrl_controller_func *);

int		fdtbus_pinctrl_set_config_index(int, u_int);
int		fdtbus_pinctrl_set_config(int, const char *);
bool		fdtbus_pinctrl_has_config(int, const char *);
const char *	fdtbus_pinctrl_parse_function(int);
const void *	fdtbus_pinctrl_parse_pins(int, int *);
const char *	fdtbus_pinctrl_parse_groups(int, int *);
const u_int *	fdtbus_pinctrl_parse_pinmux(int, int *);
int		fdtbus_pinctrl_parse_bias(int, int *);
int		fdtbus_pinctrl_parse_drive(int);
int		fdtbus_pinctrl_parse_drive_strength(int);
int		fdtbus_pinctrl_parse_input_output(int, int *);

#endif /* _DEV_FDT_FDT_PINCTRL_H_ */
