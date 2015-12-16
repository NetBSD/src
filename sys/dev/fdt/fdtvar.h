/* $NetBSD: fdtvar.h,v 1.2 2015/12/16 12:17:45 jmcneill Exp $ */

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

#ifndef _DEV_FDT_FDTVAR_H
#define _DEV_FDT_FDTVAR_H

#include <sys/types.h>
#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>

#include <dev/ofw/openfirm.h>

struct fdt_attach_args {
	const char *faa_name;
	bus_space_tag_t faa_bst;
	bus_space_tag_t faa_a4x_bst;
	bus_dma_tag_t faa_dmat;
	int faa_phandle;

	const char **faa_init;
	int faa_ninit;
};

/* flags for fdtbus_intr_establish */
#define FDT_INTR_MPSAFE	__BIT(0)

struct fdtbus_interrupt_controller_func {
	void *	(*establish)(device_t, int, u_int, int, int,
			     int (*)(void *), void *);
	void	(*disestablish)(device_t, void *);
	bool	(*intrstr)(device_t, int, u_int, char *, size_t);
};

struct fdtbus_i2c_controller_func {
	i2c_tag_t (*get_tag)(device_t);
};

struct fdtbus_gpio_controller;

struct fdtbus_gpio_pin {
	struct fdtbus_gpio_controller *gp_gc;
	void *gp_priv;
};

struct fdtbus_gpio_controller_func {
	void *	(*acquire)(device_t, const void *, size_t, int);
	void	(*release)(device_t, void *);
	int	(*read)(device_t, void *);
	void	(*write)(device_t, void *, int);
};

struct fdtbus_regulator_controller;

struct fdtbus_regulator {
	struct fdtbus_regulator_controller *reg_rc;
};

struct fdtbus_regulator_controller_func {
	int	(*acquire)(device_t);
	void	(*release)(device_t);
	int	(*enable)(device_t, bool);
};

int		fdtbus_register_interrupt_controller(device_t, int,
		    const struct fdtbus_interrupt_controller_func *);
int		fdtbus_register_i2c_controller(device_t, int,
		    const struct fdtbus_i2c_controller_func *);
int		fdtbus_register_gpio_controller(device_t, int,
		    const struct fdtbus_gpio_controller_func *);
int		fdtbus_register_regulator_controller(device_t, int,
		    const struct fdtbus_regulator_controller_func *);

int		fdtbus_get_reg(int, u_int, bus_addr_t *, bus_size_t *);
int		fdtbus_get_phandle(int, const char *);
i2c_tag_t	fdtbus_get_i2c_tag(int);
void *		fdtbus_intr_establish(int, u_int, int, int,
		    int (*func)(void *), void *arg);
void		fdtbus_intr_disestablish(int, void *);
bool		fdtbus_intr_str(int, u_int, char *, size_t);
struct fdtbus_gpio_pin *fdtbus_gpio_acquire(int, const char *, int);
void		fdtbus_gpio_release(struct fdtbus_gpio_pin *);
int		fdtbus_gpio_read(struct fdtbus_gpio_pin *);
void		fdtbus_gpio_write(struct fdtbus_gpio_pin *, int);
struct fdtbus_regulator *fdtbus_regulator_acquire(int, const char *);
void		fdtbus_regulator_release(struct fdtbus_regulator *);
int		fdtbus_regulator_enable(struct fdtbus_regulator *);
int		fdtbus_regulator_disable(struct fdtbus_regulator *);

bool		fdtbus_set_data(const void *);
const void *	fdtbus_get_data(void);
int		fdtbus_phandle2offset(int);
int		fdtbus_offset2phandle(int);

#endif /* _DEV_FDT_FDTVAR_H */
