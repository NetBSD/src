/*	$NetBSD: fdt_gpio.h,v 1.1 2025/09/06 20:11:30 thorpej Exp $	*/

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _DEV_FDT_FDT_GPIO_H_
#define	_DEV_FDT_FDT_GPIO_H_

#include <sys/device.h>
#include <sys/gpio.h>

struct fdtbus_gpio_controller;

struct fdtbus_gpio_pin {
	struct fdtbus_gpio_controller *gp_gc;
	void *gp_priv;
};

struct fdtbus_gpio_controller_func {
	void *	(*acquire)(device_t, const void *, size_t, int);
	void	(*release)(device_t, void *);
	int	(*read)(device_t, void *, bool);
	void	(*write)(device_t, void *, int, bool);
};

int		fdtbus_register_gpio_controller(device_t, int,
		    const struct fdtbus_gpio_controller_func *);

int		fdtbus_gpio_count(int, const char *);
struct fdtbus_gpio_pin *
		fdtbus_gpio_acquire(int, const char *, int);
struct fdtbus_gpio_pin *
		fdtbus_gpio_acquire_index(int, const char *, int, int);
void		fdtbus_gpio_release(struct fdtbus_gpio_pin *);
int		fdtbus_gpio_read(struct fdtbus_gpio_pin *);
void		fdtbus_gpio_write(struct fdtbus_gpio_pin *, int);
int		fdtbus_gpio_read_raw(struct fdtbus_gpio_pin *);
void		fdtbus_gpio_write_raw(struct fdtbus_gpio_pin *, int);

#endif /* _DEV_FDT_FDT_GPIO_H_ */
