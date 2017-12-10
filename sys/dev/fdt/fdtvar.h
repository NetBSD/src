/* $NetBSD: fdtvar.h,v 1.28 2017/12/10 21:38:27 skrll Exp $ */

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
#include <sys/termios.h>

#include <dev/i2c/i2cvar.h>
#include <dev/clk/clk.h>

#include <dev/clock_subr.h>

#include <dev/ofw/openfirm.h>

struct fdt_attach_args {
	const char *faa_name;
	bus_space_tag_t faa_bst;
	bus_space_tag_t faa_a4x_bst;
	bus_dma_tag_t faa_dmat;
	int faa_phandle;
	int faa_quiet;
};

/* flags for fdtbus_intr_establish */
#define FDT_INTR_MPSAFE	__BIT(0)

struct fdtbus_interrupt_controller_func {
	void *	(*establish)(device_t, u_int *, int, int,
			     int (*)(void *), void *);
	void	(*disestablish)(device_t, void *);
	bool	(*intrstr)(device_t, u_int *, char *, size_t);
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
	int	(*read)(device_t, void *, bool);
	void	(*write)(device_t, void *, int, bool);
};

struct fdtbus_pinctrl_controller;

struct fdtbus_pinctrl_pin {
	struct fdtbus_pinctrl_controller *pp_pc;
	void *pp_priv;
};

struct fdtbus_pinctrl_controller_func {
	int (*set_config)(device_t, const void *, size_t);
};

struct fdtbus_regulator_controller;

struct fdtbus_regulator {
	struct fdtbus_regulator_controller *reg_rc;
};

struct fdtbus_regulator_controller_func {
	int	(*acquire)(device_t);
	void	(*release)(device_t);
	int	(*enable)(device_t, bool);
	int	(*set_voltage)(device_t, u_int, u_int);
	int	(*get_voltage)(device_t, u_int *);
};

struct fdtbus_clock_controller_func {
	struct clk *	(*decode)(device_t, const void *, size_t);
};

struct fdtbus_reset_controller;

struct fdtbus_reset {
	struct fdtbus_reset_controller *rst_rc;
	void *rst_priv;
};

struct fdtbus_reset_controller_func {
	void *	(*acquire)(device_t, const void *, size_t);
	void	(*release)(device_t, void *);
	int	(*reset_assert)(device_t, void *);
	int	(*reset_deassert)(device_t, void *);
};

struct fdtbus_dma_controller;

struct fdtbus_dma {
	struct fdtbus_dma_controller *dma_dc;
	void *dma_priv;
};

enum fdtbus_dma_dir {
	FDT_DMA_READ,		/* device -> memory */
	FDT_DMA_WRITE		/* memory -> device */
};

struct fdtbus_dma_opt {
	int opt_bus_width;		/* Bus width */
	int opt_burst_len;		/* Burst length */
	int opt_swap;			/* Enable data swapping */
	int opt_dblbuf;			/* Enable double buffering */
	int opt_wrap_len;		/* Address wrap-around window */
};

struct fdtbus_dma_req {
	bus_dma_segment_t *dreq_segs;	/* Memory */
	int dreq_nsegs;

	bus_addr_t dreq_dev_phys;	/* Device */
	int dreq_sel;			/* Device selector */

	enum fdtbus_dma_dir dreq_dir;	/* Transfer direction */

	int dreq_block_irq;		/* Enable IRQ at end of block */
	int dreq_block_multi;		/* Enable multiple block transfers */
	int dreq_flow;			/* Enable flow control */

	struct fdtbus_dma_opt dreq_mem_opt;	/* Memory options */
	struct fdtbus_dma_opt dreq_dev_opt;	/* Device options */
};

struct fdtbus_dma_controller_func {
	void *	(*acquire)(device_t, const void *, size_t,
			   void (*)(void *), void *);
	void	(*release)(device_t, void *);
	int	(*transfer)(device_t, void *, struct fdtbus_dma_req *);
	void	(*halt)(device_t, void *);
};

struct fdtbus_power_controller;

struct fdtbus_power_controller_func {
	void 	(*reset)(device_t);
	void	(*poweroff)(device_t);
};

struct fdtbus_phy_controller;

struct fdtbus_phy {
	struct fdtbus_phy_controller *phy_pc;
	void *phy_priv;
};

struct fdtbus_phy_controller_func {
	void *	(*acquire)(device_t, const void *, size_t);
	void	(*release)(device_t, void *);
	int	(*enable)(device_t, void *, bool);
};

struct fdtbus_mmc_pwrseq;

struct fdtbus_mmc_pwrseq_func {
	void	(*pre_power_on)(device_t);
	void	(*post_power_on)(device_t);
	void	(*power_off)(device_t);
	void	(*reset)(device_t);
};

struct fdt_console {
	int	(*match)(int);
	void	(*consinit)(struct fdt_attach_args *, u_int);
};

struct fdt_console_info {
	const struct fdt_console *ops;
};

#define	_FDT_CONSOLE_REGISTER(name)	\
	__link_set_add_rodata(fdt_consoles, __CONCAT(name,_consinfo));

#define	FDT_CONSOLE(_name, _ops)					\
static const struct fdt_console_info __CONCAT(_name,_consinfo) = {	\
	.ops = (_ops)							\
};									\
_FDT_CONSOLE_REGISTER(_name)

TAILQ_HEAD(fdt_conslist, fdt_console_info);

int		fdtbus_register_interrupt_controller(device_t, int,
		    const struct fdtbus_interrupt_controller_func *);
int		fdtbus_register_i2c_controller(device_t, int,
		    const struct fdtbus_i2c_controller_func *);
int		fdtbus_register_gpio_controller(device_t, int,
		    const struct fdtbus_gpio_controller_func *);
int		fdtbus_register_pinctrl_config(device_t, int,
		    const struct fdtbus_pinctrl_controller_func *);
int		fdtbus_register_regulator_controller(device_t, int,
		    const struct fdtbus_regulator_controller_func *);
int		fdtbus_register_clock_controller(device_t, int,
		    const struct fdtbus_clock_controller_func *);
int		fdtbus_register_reset_controller(device_t, int,
		    const struct fdtbus_reset_controller_func *);
int		fdtbus_register_dma_controller(device_t, int,
		    const struct fdtbus_dma_controller_func *);
int		fdtbus_register_power_controller(device_t, int,
		    const struct fdtbus_power_controller_func *);
int		fdtbus_register_phy_controller(device_t, int,
		    const struct fdtbus_phy_controller_func *);
int		fdtbus_register_mmc_pwrseq(device_t, int,
		    const struct fdtbus_mmc_pwrseq_func *);

void		fdtbus_set_decoderegprop(bool);

int		fdtbus_get_reg(int, u_int, bus_addr_t *, bus_size_t *);
int		fdtbus_get_reg_byname(int, const char *, bus_addr_t *,
		    bus_size_t *);
int		fdtbus_get_reg64(int, u_int, uint64_t *, uint64_t *);
int		fdtbus_get_phandle(int, const char *);
int		fdtbus_get_phandle_from_native(int);
i2c_tag_t	fdtbus_get_i2c_tag(int);
void *		fdtbus_intr_establish(int, u_int, int, int,
		    int (*func)(void *), void *arg);
void		fdtbus_intr_disestablish(int, void *);
bool		fdtbus_intr_str(int, u_int, char *, size_t);
struct fdtbus_gpio_pin *fdtbus_gpio_acquire(int, const char *, int);
struct fdtbus_gpio_pin *fdtbus_gpio_acquire_index(int, const char *, int, int);
void		fdtbus_gpio_release(struct fdtbus_gpio_pin *);
int		fdtbus_gpio_read(struct fdtbus_gpio_pin *);
void		fdtbus_gpio_write(struct fdtbus_gpio_pin *, int);
int		fdtbus_gpio_read_raw(struct fdtbus_gpio_pin *);
void		fdtbus_gpio_write_raw(struct fdtbus_gpio_pin *, int);
void		fdtbus_pinctrl_configure(void);
int		fdtbus_pinctrl_set_config_index(int, u_int);
int		fdtbus_pinctrl_set_config(int, const char *);
struct fdtbus_regulator *fdtbus_regulator_acquire(int, const char *);
void		fdtbus_regulator_release(struct fdtbus_regulator *);
int		fdtbus_regulator_enable(struct fdtbus_regulator *);
int		fdtbus_regulator_disable(struct fdtbus_regulator *);
int		fdtbus_regulator_set_voltage(struct fdtbus_regulator *,
		    u_int, u_int);
int		fdtbus_regulator_get_voltage(struct fdtbus_regulator *,
		    u_int *);

struct fdtbus_dma *fdtbus_dma_get(int, const char *, void (*)(void *), void *);
struct fdtbus_dma *fdtbus_dma_get_index(int, u_int, void (*)(void *),
		    void *);
void		fdtbus_dma_put(struct fdtbus_dma *);
int		fdtbus_dma_transfer(struct fdtbus_dma *,
		    struct fdtbus_dma_req *);
void		fdtbus_dma_halt(struct fdtbus_dma *);

struct clk *	fdtbus_clock_get(int, const char *);
struct clk *	fdtbus_clock_get_index(int, u_int);

struct fdtbus_reset *fdtbus_reset_get(int, const char *);
struct fdtbus_reset *fdtbus_reset_get_index(int, u_int);
void		fdtbus_reset_put(struct fdtbus_reset *);
int		fdtbus_reset_assert(struct fdtbus_reset *);
int		fdtbus_reset_deassert(struct fdtbus_reset *);

struct fdtbus_phy *fdtbus_phy_get(int, const char *);
struct fdtbus_phy *fdtbus_phy_get_index(int, u_int);
void		fdtbus_phy_put(struct fdtbus_phy *);
int		fdtbus_phy_enable(struct fdtbus_phy *, bool);

struct fdtbus_mmc_pwrseq *fdtbus_mmc_pwrseq_get(int);
void		fdtbus_mmc_pwrseq_pre_power_on(struct fdtbus_mmc_pwrseq *);
void		fdtbus_mmc_pwrseq_post_power_on(struct fdtbus_mmc_pwrseq *);
void		fdtbus_mmc_pwrseq_power_off(struct fdtbus_mmc_pwrseq *);
void		fdtbus_mmc_pwrseq_reset(struct fdtbus_mmc_pwrseq *);

int		fdtbus_todr_attach(device_t, int, todr_chip_handle_t);

void		fdtbus_power_reset(void);
void		fdtbus_power_poweroff(void);

bool		fdtbus_set_data(const void *);
const void *	fdtbus_get_data(void);
int		fdtbus_phandle2offset(int);
int		fdtbus_offset2phandle(int);
bool		fdtbus_get_path(int, char *, size_t);

const struct fdt_console *fdtbus_get_console(void);

const char *	fdtbus_get_stdout_path(void);
int		fdtbus_get_stdout_phandle(void);
int		fdtbus_get_stdout_speed(void);
tcflag_t	fdtbus_get_stdout_flags(void);

bool		fdtbus_status_okay(int);

const void *	fdtbus_get_prop(int, const char *, int *);
const char *	fdtbus_get_string(int, const char *);
const char *	fdtbus_get_string_index(int, const char *, u_int);

int		fdtbus_print(void *, const char *);

#endif /* _DEV_FDT_FDTVAR_H */
