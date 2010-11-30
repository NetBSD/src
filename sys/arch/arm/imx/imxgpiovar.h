/*	$NetBSD: imxgpiovar.h,v 1.1 2010/11/30 13:05:27 bsh Exp $ */
/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas
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
#ifndef _ARM_IMX_IMXGRPIOVAR_H
#define _ARM_IMX_IMXGRPIOVAR_H

#include <sys/bus.h>
#include <sys/device.h>
#include <arm/imx/imxgpioreg.h>	/* for GPIO_NPINS */

void imxgpio_attach_common(device_t, bus_space_tag_t, bus_space_handle_t,
    int, int, int);
/* defined imx[35]1_gpio.c */
extern const int imxgpio_ngroups;
int imxgpio_match(device_t, cfdata_t, void *);
void imxgpio_attach(device_t, device_t, void *);


#define	GPIO_NO(group, pin)	(((group) - 1) * GPIO_NPINS + (pin))
#define	GPIO_MODULE(pin)	((pin) / GPIO_NPINS)

/* in-kernel GPIO access utility functions */
enum GPIO_DIRECTION {GPIO_DIR_IN, GPIO_DIR_OUT};
void gpio_set_direction(u_int, enum GPIO_DIRECTION);
void gpio_data_write(u_int, u_int);
bool gpio_data_read(u_int);
#endif	/* _ARM_IMX_IMXGRPIOVAR_H */
