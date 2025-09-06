/* $NetBSD: fdtvar.h,v 1.84 2025/09/06 21:24:05 thorpej Exp $ */

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

#ifndef _DEV_FDT_FDTVAR_H_
#define _DEV_FDT_FDTVAR_H_

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/termios.h>

#include <dev/clock_subr.h>

#include <dev/ofw/openfirm.h>

#include <dev/fdt/fdt_clock.h>
#include <dev/fdt/fdt_dai.h>
#include <dev/fdt/fdt_dma.h>
#include <dev/fdt/fdt_gpio.h>
#include <dev/fdt/fdt_i2c.h>
#include <dev/fdt/fdt_intr.h>
#include <dev/fdt/fdt_iommu.h>
#include <dev/fdt/fdt_mbox.h>
#include <dev/fdt/fdt_mmc_pwrseq.h>
#include <dev/fdt/fdt_phy.h>
#include <dev/fdt/fdt_pinctrl.h>
#include <dev/fdt/fdt_power.h>
#include <dev/fdt/fdt_powerdomain.h>
#include <dev/fdt/fdt_pwm.h>
#include <dev/fdt/fdt_regulator.h>
#include <dev/fdt/fdt_reset.h>
#include <dev/fdt/fdt_spi.h>
#include <dev/fdt/fdt_syscon.h>

struct fdt_attach_args {
	const char *faa_name;
	bus_space_tag_t faa_bst;
	bus_dma_tag_t faa_dmat;
	int faa_phandle;
	int faa_quiet;
};

struct fdt_console {
	int	(*match)(int);
	void	(*consinit)(struct fdt_attach_args *, u_int);
};

struct fdt_console_info {
	const struct fdt_console *ops;
};

struct fdt_phandle_data {
	int phandle;
	int count;
	const u_int *values;
};

#define	_FDT_CONSOLE_REGISTER(name)	\
	__link_set_add_rodata(fdt_consoles, __CONCAT(name,_consinfo));

#define	FDT_CONSOLE(_name, _ops)					\
static const struct fdt_console_info __CONCAT(_name,_consinfo) = {	\
	.ops = (_ops)							\
};									\
_FDT_CONSOLE_REGISTER(_name)

struct fdt_dma_range {
	paddr_t		dr_sysbase;
	bus_addr_t	dr_busbase;
	bus_size_t	dr_len;
};

#define	FDT_BUS_SPACE_FLAG_NONPOSTED_MMIO	__BIT(0)

void		fdtbus_set_decoderegprop(bool);

int		fdtbus_get_reg(int, u_int, bus_addr_t *, bus_size_t *);
int		fdtbus_get_reg_byname(int, const char *, bus_addr_t *,
		    bus_size_t *);
int		fdtbus_get_reg64(int, u_int, uint64_t *, uint64_t *);
int		fdtbus_get_addr_cells(int);
int		fdtbus_get_size_cells(int);
uint64_t	fdtbus_get_cells(const uint8_t *, int);
int		fdtbus_get_phandle(int, const char *);
int		fdtbus_get_phandle_with_data(int, const char *, const char *,
		    int, struct fdt_phandle_data *);
int		fdtbus_get_phandle_from_native(int);

int		fdtbus_todr_attach(device_t, int, todr_chip_handle_t);

bool		fdtbus_init(const void *);
const void *	fdtbus_get_data(void);
int		fdtbus_phandle2offset(int);
int		fdtbus_offset2phandle(int);
bool		fdtbus_get_path(int, char *, size_t);

const struct fdt_console *
		fdtbus_get_console(void);

const char *	fdtbus_get_stdout_path(void);
int		fdtbus_get_stdout_phandle(void);
int		fdtbus_get_stdout_speed(void);
tcflag_t	fdtbus_get_stdout_flags(void);

bool		fdtbus_status_okay(int);

const void *	fdtbus_get_prop(int, const char *, int *);
const char *	fdtbus_get_string(int, const char *);
const char *	fdtbus_get_string_index(int, const char *, u_int);
int		fdtbus_get_index(int, const char *, const char *, u_int *);

void		fdtbus_cpus_md_attach(device_t, device_t, void *);

void		fdt_add_bus(device_t, int, struct fdt_attach_args *);
void		fdt_add_bus_match(device_t, int, struct fdt_attach_args *,
		    bool (*)(void *, int), void *);
void		fdt_add_child(device_t, int, struct fdt_attach_args *, u_int);

void		fdt_remove_byhandle(int);
void		fdt_remove_bycompat(const char *[]);
int		fdt_find_with_property(const char *, int *);

int		fdtbus_print(void *, const char *);

bus_dma_tag_t	fdtbus_dma_tag_create(int, const struct fdt_dma_range *,
		    u_int);
bus_space_tag_t	fdtbus_bus_tag_create(int, uint32_t);


#endif /* _DEV_FDT_FDTVAR_H_ */
