/* $NetBSD: jh7110_pinctrl.c,v 1.1 2024/11/11 19:23:18 skrll Exp $ */

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: jh7110_pinctrl.c,v 1.1 2024/11/11 19:23:18 skrll Exp $");

#include <sys/param.h>

#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

struct jh7110_pinctrl_softc;
struct jh7110_pinctrl_data {
	u_int		jpd_npins;
	u_int		jpd_ngpios;

	bus_size_t	jpd_dout;
	uint32_t	jpd_dout_mask;
	bus_size_t	jpd_doen;
	uint32_t	jpd_doen_mask;
	bus_size_t	jpd_gpi;
	uint32_t	jpd_gpi_mask;
	bus_size_t	jpd_gin;
	bus_size_t	jpd_gpioin;
};

struct jh7110_pinctrl_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	kmutex_t		sc_lock;

	const struct jh7110_pinctrl_data *
				sc_jpd;
};

struct jh7110_pinctrl_gpio_pin {
	struct jh7110_pinctrl_softc	*pin_sc;
	u_int				 pin_no;
	bool				 pin_actlo;
};


// https://doc-en.rvspace.org/JH7110/TRM/JH7110_TRM/sys_iomux_cfg.html

/* SYS registers */
#define JH7110_SYS_DOEN			0x0000
#define JH7110_SYS_DOUT			0x0040
#define JH7110_SYS_GPI			0x0080
#define JH7110_SYS_GPIOIN		0x0118

#define JH7110_SYS_NGPIO		64
#define JH7110_SYS_NPIN			96

/* AON registers */
#define JH7110_AON_DOEN			0x0000
#define JH7110_AON_DOUT			0x0004
#define JH7110_AON_GPI			0x0008
#define JH7110_AON_GPIOIN		0x002c

#define JH7110_AON_NGPIO		4
#define JH7110_AON_NPIN			20

// XXXNH rename
#define GPOUT_LOW			0
#define GPOUT_HIGH			1

#define  GPI_NONE			0xff


#define RD4(sc, reg)						       \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)					       \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

/* pad control bits */
#define JH7110_PADCFG_IE	__BIT(0)
#define JH7110_PADCFG_DS_MASK	__BITS(1, 2)
#define JH7110_PADCFG_DS_2MA	__SHIFTIN(0, JH7110_PADCFG_DS_MASK)
#define JH7110_PADCFG_DS_4MA	__SHIFTIN(1, JH7110_PADCFG_DS_MASK)
#define JH7110_PADCFG_DS_8MA	__SHIFTIN(2, JH7110_PADCFG_DS_MASK)
#define JH7110_PADCFG_DS_12MA	__SHIFTIN(3, JH7110_PADCFG_DS_MASK)
#define JH7110_PADCFG_PU	__BIT(3)
#define JH7110_PADCFG_PD	__BIT(4)
#define JH7110_PADCFG_BIAS_MASK	(JH7110_PADCFG_PD | JH7110_PADCFG_PU)
#define JH7110_PADCFG_SLEW	__BIT(5)
#define JH7110_PADCFG_SMT	__BIT(6)
#define JH7110_PADCFG_POS	__BIT(7)

/* SYS pins */
#define JH7110_SYS_PAD_GPIO0               0
#define JH7110_SYS_PAD_GPIO1               1
#define JH7110_SYS_PAD_GPIO2               2
#define JH7110_SYS_PAD_GPIO3               3
#define JH7110_SYS_PAD_GPIO4               4
#define JH7110_SYS_PAD_GPIO5               5
#define JH7110_SYS_PAD_GPIO6               6
#define JH7110_SYS_PAD_GPIO7               7
#define JH7110_SYS_PAD_GPIO8               8
#define JH7110_SYS_PAD_GPIO9               9
#define JH7110_SYS_PAD_GPIO10              10
#define JH7110_SYS_PAD_GPIO11              11
#define JH7110_SYS_PAD_GPIO12              12
#define JH7110_SYS_PAD_GPIO13              13
#define JH7110_SYS_PAD_GPIO14              14
#define JH7110_SYS_PAD_GPIO15              15
#define JH7110_SYS_PAD_GPIO16              16
#define JH7110_SYS_PAD_GPIO17              17
#define JH7110_SYS_PAD_GPIO18              18
#define JH7110_SYS_PAD_GPIO19              19
#define JH7110_SYS_PAD_GPIO20              20
#define JH7110_SYS_PAD_GPIO21              21
#define JH7110_SYS_PAD_GPIO22              22
#define JH7110_SYS_PAD_GPIO23              23
#define JH7110_SYS_PAD_GPIO24              24
#define JH7110_SYS_PAD_GPIO25              25
#define JH7110_SYS_PAD_GPIO26              26
#define JH7110_SYS_PAD_GPIO27              27
#define JH7110_SYS_PAD_GPIO28              28
#define JH7110_SYS_PAD_GPIO29              29
#define JH7110_SYS_PAD_GPIO30              30
#define JH7110_SYS_PAD_GPIO31              31
#define JH7110_SYS_PAD_GPIO32              32
#define JH7110_SYS_PAD_GPIO33              33
#define JH7110_SYS_PAD_GPIO34              34
#define JH7110_SYS_PAD_GPIO35              35
#define JH7110_SYS_PAD_GPIO36              36
#define JH7110_SYS_PAD_GPIO37              37
#define JH7110_SYS_PAD_GPIO38              38
#define JH7110_SYS_PAD_GPIO39              39
#define JH7110_SYS_PAD_GPIO40              40
#define JH7110_SYS_PAD_GPIO41              41
#define JH7110_SYS_PAD_GPIO42              42
#define JH7110_SYS_PAD_GPIO43              43
#define JH7110_SYS_PAD_GPIO44              44
#define JH7110_SYS_PAD_GPIO45              45
#define JH7110_SYS_PAD_GPIO46              46
#define JH7110_SYS_PAD_GPIO47              47
#define JH7110_SYS_PAD_GPIO48              48
#define JH7110_SYS_PAD_GPIO49              49
#define JH7110_SYS_PAD_GPIO50              50
#define JH7110_SYS_PAD_GPIO51              51
#define JH7110_SYS_PAD_GPIO52              52
#define JH7110_SYS_PAD_GPIO53              53
#define JH7110_SYS_PAD_GPIO54              54
#define JH7110_SYS_PAD_GPIO55              55
#define JH7110_SYS_PAD_GPIO56              56
#define JH7110_SYS_PAD_GPIO57              57
#define JH7110_SYS_PAD_GPIO58              58
#define JH7110_SYS_PAD_GPIO59              59
#define JH7110_SYS_PAD_GPIO60              60
#define JH7110_SYS_PAD_GPIO61              61
#define JH7110_SYS_PAD_GPIO62              62
#define JH7110_SYS_PAD_GPIO63              63
#define JH7110_SYS_PAD_SD0_CLK             64
#define JH7110_SYS_PAD_SD0_CMD             65
#define JH7110_SYS_PAD_SD0_DATA0           66
#define JH7110_SYS_PAD_SD0_DATA1           67
#define JH7110_SYS_PAD_SD0_DATA2           68
#define JH7110_SYS_PAD_SD0_DATA3           69
#define JH7110_SYS_PAD_SD0_DATA4           70
#define JH7110_SYS_PAD_SD0_DATA5           71
#define JH7110_SYS_PAD_SD0_DATA6           72
#define JH7110_SYS_PAD_SD0_DATA7           73
#define JH7110_SYS_PAD_SD0_STRB            74
#define JH7110_SYS_PAD_GMAC1_MDC           75
#define JH7110_SYS_PAD_GMAC1_MDIO          76
#define JH7110_SYS_PAD_GMAC1_RXD0          77
#define JH7110_SYS_PAD_GMAC1_RXD1          78
#define JH7110_SYS_PAD_GMAC1_RXD2          79
#define JH7110_SYS_PAD_GMAC1_RXD3          80
#define JH7110_SYS_PAD_GMAC1_RXDV          81
#define JH7110_SYS_PAD_GMAC1_RXC           82
#define JH7110_SYS_PAD_GMAC1_TXD0          83
#define JH7110_SYS_PAD_GMAC1_TXD1          84
#define JH7110_SYS_PAD_GMAC1_TXD2          85
#define JH7110_SYS_PAD_GMAC1_TXD3          86
#define JH7110_SYS_PAD_GMAC1_TXEN          87
#define JH7110_SYS_PAD_GMAC1_TXC           88
#define JH7110_SYS_PAD_QSPI_SCLK           89
#define JH7110_SYS_PAD_QSPI_CS0            90
#define JH7110_SYS_PAD_QSPI_DATA0          91
#define JH7110_SYS_PAD_QSPI_DATA1          92
#define JH7110_SYS_PAD_QSPI_DATA2          93
#define JH7110_SYS_PAD_QSPI_DATA3          94

struct jh7110_func_sel {
    uint16_t	jfs_funcreg;
    uint16_t	jfs_max;
    uint32_t	jfs_mask;
};

#define JH7110_FS(_reg, _mask, _max)					\
    {									\
	.jfs_funcreg = (_reg),						\
	.jfs_max = (_max),						\
	.jfs_mask = (_mask),						\
    }

// https://doc-en.rvspace.org/JH7110/TRM/JH7110_TRM/sys_iomux_cfg.html#sys_iomux_cfg__section_fw2_v3b_xsb
static const struct jh7110_func_sel jh7110_sys_func_sel[] = {
	[JH7110_SYS_PAD_GMAC1_RXC] = JH7110_FS(0x29c, __BITS( 1, 0), 1),
	[JH7110_SYS_PAD_GPIO10]    = JH7110_FS(0x29c, __BITS( 4, 2), 3),
	[JH7110_SYS_PAD_GPIO11]    = JH7110_FS(0x29c, __BITS( 7, 5), 3),
	[JH7110_SYS_PAD_GPIO12]    = JH7110_FS(0x29c, __BITS(10, 8), 3),
	[JH7110_SYS_PAD_GPIO13]    = JH7110_FS(0x29c, __BITS(13,11), 3),
	[JH7110_SYS_PAD_GPIO14]    = JH7110_FS(0x29c, __BITS(16,14), 3),
	[JH7110_SYS_PAD_GPIO15]    = JH7110_FS(0x29c, __BITS(19,17), 3),
	[JH7110_SYS_PAD_GPIO16]    = JH7110_FS(0x29c, __BITS(22,20), 3),
	[JH7110_SYS_PAD_GPIO17]    = JH7110_FS(0x29c, __BITS(25,23), 3),
	[JH7110_SYS_PAD_GPIO18]    = JH7110_FS(0x29c, __BITS(28,26), 3),
	[JH7110_SYS_PAD_GPIO19]    = JH7110_FS(0x29c, __BITS(31,29), 3),

	[JH7110_SYS_PAD_GPIO20]    = JH7110_FS(0x2a0, __BITS( 2, 0), 3),
	[JH7110_SYS_PAD_GPIO21]    = JH7110_FS(0x2a0, __BITS( 5, 3), 3),
	[JH7110_SYS_PAD_GPIO22]    = JH7110_FS(0x2a0, __BITS( 8, 6), 3),
	[JH7110_SYS_PAD_GPIO23]    = JH7110_FS(0x2a0, __BITS(11, 9), 3),
	[JH7110_SYS_PAD_GPIO24]    = JH7110_FS(0x2a0, __BITS(14,12), 3),
	[JH7110_SYS_PAD_GPIO25]    = JH7110_FS(0x2a0, __BITS(17,15), 3),
	[JH7110_SYS_PAD_GPIO26]    = JH7110_FS(0x2a0, __BITS(20,18), 3),
	[JH7110_SYS_PAD_GPIO27]    = JH7110_FS(0x2a0, __BITS(23,21), 3),
	[JH7110_SYS_PAD_GPIO28]    = JH7110_FS(0x2a0, __BITS(26,24), 3),
	[JH7110_SYS_PAD_GPIO29]    = JH7110_FS(0x2a0, __BITS(29,27), 3),

	[JH7110_SYS_PAD_GPIO30]    = JH7110_FS(0x2a4, __BITS( 2, 0), 3),
	[JH7110_SYS_PAD_GPIO31]    = JH7110_FS(0x2a4, __BITS( 5, 3), 3),
	[JH7110_SYS_PAD_GPIO32]    = JH7110_FS(0x2a4, __BITS( 8, 6), 3),
	[JH7110_SYS_PAD_GPIO33]    = JH7110_FS(0x2a4, __BITS(11, 9), 3),
	[JH7110_SYS_PAD_GPIO34]    = JH7110_FS(0x2a4, __BITS(14,12), 3),
	[JH7110_SYS_PAD_GPIO35]    = JH7110_FS(0x2a4, __BITS(17,15), 3),
	[JH7110_SYS_PAD_GPIO36]    = JH7110_FS(0x2a4, __BITS(19,17), 3),
	[JH7110_SYS_PAD_GPIO37]    = JH7110_FS(0x2a4, __BITS(23,20), 3),
	[JH7110_SYS_PAD_GPIO38]    = JH7110_FS(0x2a4, __BITS(26,23), 3),
	[JH7110_SYS_PAD_GPIO39]    = JH7110_FS(0x2a4, __BITS(28,26), 3),
	[JH7110_SYS_PAD_GPIO40]    = JH7110_FS(0x2a4, __BITS(31,29), 3),

	[JH7110_SYS_PAD_GPIO41]    = JH7110_FS(0x2a8, __BITS( 2, 0), 3),
	[JH7110_SYS_PAD_GPIO42]    = JH7110_FS(0x2a8, __BITS( 5, 3), 3),
	[JH7110_SYS_PAD_GPIO43]    = JH7110_FS(0x2a8, __BITS( 8, 6), 3),
	[JH7110_SYS_PAD_GPIO44]    = JH7110_FS(0x2a8, __BITS(11, 9), 3),
	[JH7110_SYS_PAD_GPIO45]    = JH7110_FS(0x2a8, __BITS(14,12), 3),
	[JH7110_SYS_PAD_GPIO46]    = JH7110_FS(0x2a8, __BITS(17,15), 3),
	[JH7110_SYS_PAD_GPIO47]    = JH7110_FS(0x2a8, __BITS(20,18), 3),
	[JH7110_SYS_PAD_GPIO48]    = JH7110_FS(0x2a8, __BITS(23,21), 3),
	[JH7110_SYS_PAD_GPIO49]    = JH7110_FS(0x2a8, __BITS(26,24), 3),
	[JH7110_SYS_PAD_GPIO50]    = JH7110_FS(0x2a8, __BITS(29,27), 3),
	[JH7110_SYS_PAD_GPIO51]    = JH7110_FS(0x2a8, __BITS(31,30), 3),

	[JH7110_SYS_PAD_GPIO52]    = JH7110_FS(0x2ac, __BITS( 1, 0), 3),
	[JH7110_SYS_PAD_GPIO53]    = JH7110_FS(0x2ac, __BITS( 3, 2), 3),
	[JH7110_SYS_PAD_GPIO54]    = JH7110_FS(0x2ac, __BITS( 5, 4), 3),
	[JH7110_SYS_PAD_GPIO55]    = JH7110_FS(0x2ac, __BITS( 8, 6), 3),
	[JH7110_SYS_PAD_GPIO56]    = JH7110_FS(0x2ac, __BITS(11, 9), 3),
	[JH7110_SYS_PAD_GPIO57]    = JH7110_FS(0x2ac, __BITS(14,12), 3),
	[JH7110_SYS_PAD_GPIO58]    = JH7110_FS(0x2ac, __BITS(17,15), 3),
	[JH7110_SYS_PAD_GPIO59]    = JH7110_FS(0x2ac, __BITS(20,18), 3),
	[JH7110_SYS_PAD_GPIO60]    = JH7110_FS(0x2ac, __BITS(23,21), 3),
	[JH7110_SYS_PAD_GPIO61]    = JH7110_FS(0x2ac, __BITS(26,24), 3),
	[JH7110_SYS_PAD_GPIO62]    = JH7110_FS(0x2ac, __BITS(29,27), 3),
	[JH7110_SYS_PAD_GPIO63]    = JH7110_FS(0x2ac, __BITS(31,30), 3),

	[JH7110_SYS_PAD_GPIO6]     = JH7110_FS(0x2b0, __BITS( 1, 0), 3),
	[JH7110_SYS_PAD_GPIO7]     = JH7110_FS(0x2b0, __BITS( 4, 2), 3),
	[JH7110_SYS_PAD_GPIO8]     = JH7110_FS(0x2b0, __BITS( 7, 5), 3),
	[JH7110_SYS_PAD_GPIO9]     = JH7110_FS(0x2b0, __BITS(10, 8), 3),
};

static void
jh7110_set_function(struct jh7110_pinctrl_softc *sc, u_int pin_no,
    u_int func)
{
	if (pin_no >= __arraycount(jh7110_sys_func_sel))
		return;

	const struct jh7110_func_sel * const jfs =
	    &jh7110_sys_func_sel[pin_no];

	if (func > jfs->jfs_max)
		return;

	if (jfs->jfs_funcreg == 0)
		return;

	uint32_t funcold, funcval;
	mutex_enter(&sc->sc_lock);
	funcold = RD4(sc, jfs->jfs_funcreg);

	funcval = funcold & ~jfs->jfs_mask;
	funcval |= __SHIFTIN(func, jfs->jfs_mask);

	WR4(sc, jfs->jfs_funcreg, funcval);
	mutex_exit(&sc->sc_lock);
}


static void
jh7110_set_gpiomux(struct jh7110_pinctrl_softc * const sc, u_int pin_no,
    u_int din, u_int dout, u_int doen)
{
	const struct jh7110_pinctrl_data * const jpd = sc->sc_jpd;
	const u_int offset = 4 * (pin_no / 4);
	const u_int shift = 8 * (pin_no % 4);
	const uint32_t dout_mask = jpd->jpd_dout_mask << shift;
	const uint32_t doen_mask = jpd->jpd_doen_mask << shift;
	const bus_size_t dout_reg = jpd->jpd_dout + offset;
	const bus_size_t doen_reg = jpd->jpd_doen + offset;
	uint32_t doutval, doutold;
	uint32_t doenval, doenold;
	uint32_t dinval, dinold;

	mutex_enter(&sc->sc_lock);
	doutold = RD4(sc, dout_reg);
	doutval = doutold & ~dout_mask;
	doutval |= __SHIFTIN(dout, dout_mask);

	doenold = RD4(sc, doen_reg);
	doenval = doenold & ~doen_mask;
	doenval |= __SHIFTIN(doen, doen_mask);

	WR4(sc, dout_reg, doutval);
	WR4(sc, doen_reg, doenval);
	if (din != GPI_NONE) {
		const u_int din_offset = 4 * (din / 4);
		const u_int din_shift = 8 * (din % 4);
		const uint32_t din_mask = jpd->jpd_gpi_mask << din_shift;
		const bus_size_t din_reg = jpd->jpd_gpi + din_offset;

		dinold = RD4(sc, din_reg);
		dinval = dinold & ~din_mask;
		/*
		 * The register value indicates the selected GPIO number + 2
		 * (GPIO2 - GPIO63, GPIO0 and GPIO1 are not available) for the
		 * input signal.
		 */
		dinval |= __SHIFTIN(pin_no + 2, din_mask);
		WR4(sc, din_reg, dinval);
	}
	mutex_exit(&sc->sc_lock);
}


static const struct jh7110_pinctrl_data jh7110_aon_pinctrl_data = {
	.jpd_npins	= JH7110_AON_NPIN,
	.jpd_ngpios	= JH7110_AON_NGPIO,
	.jpd_doen	= JH7110_AON_DOEN,
	.jpd_doen_mask	= __BITS(2, 0),
	.jpd_dout	= JH7110_AON_DOUT,
	.jpd_dout_mask	= __BITS(3, 0),
	.jpd_gpi	= JH7110_AON_GPI,
	.jpd_gpi_mask	= __BITS(3, 0),
	.jpd_gpioin	= JH7110_AON_GPIOIN,
};

static const struct jh7110_pinctrl_data jh7110_sys_pinctrl_data = {
	.jpd_npins	= JH7110_SYS_NPIN,
	.jpd_ngpios	= JH7110_SYS_NGPIO,
	.jpd_doen	= JH7110_SYS_DOEN,
	.jpd_doen_mask	= __BITS(5, 0),
	.jpd_dout	= JH7110_SYS_DOUT,
	.jpd_dout_mask	= __BITS(6, 0),
	.jpd_gpi	= JH7110_SYS_GPI,
	.jpd_gpi_mask	= __BITS(6, 0),
	.jpd_gpioin	= JH7110_SYS_GPIOIN,
};


static int
jh7110_set_pinmux(struct jh7110_pinctrl_softc *sc, u_int pin,
    u_int din, u_int dout, u_int doen, u_int func)
{
	const struct jh7110_pinctrl_data * const jpd = sc->sc_jpd;

	if (pin < jpd->jpd_ngpios && func == 0)
		jh7110_set_gpiomux(sc, pin, din, dout, doen);
	return 0;

	if (sc->sc_jpd == &jh7110_aon_pinctrl_data)
		return 0;

	if (pin < jpd->jpd_npins)
		jh7110_set_function(sc, pin, func);

	return 0;
}


/* Device Tree encoding */
#define DT_PINMUX_DIN_MASK	__BITS(31, 24)
#define DT_PINMUX_DOUT_MASK	__BITS(23, 16)
#define DT_PINMUX_DOEN_MASK	__BITS(15, 10)
#define DT_PINMUX_FUNC_MASK	__BITS( 9,  8)
#define DT_PINMUX_PIN_MASK	__BITS( 7,  0)

static int
jh7110_parse_slew_rate(int phandle)
{
	int slew_rate;

	if (of_getprop_uint32(phandle, "slew-rate", &slew_rate) == 0)
                return slew_rate;

	return -1;
}

static void
jh7110_pinctrl_pin_properties(struct jh7110_pinctrl_softc *sc, int phandle,
    uint16_t *val, uint16_t *mask)
{
	*mask = 0;
	*val = 0;

	const int bias = fdtbus_pinctrl_parse_bias(phandle, NULL);
	const int drive_strength = fdtbus_pinctrl_parse_drive_strength(phandle);
	const int slew_rate = jh7110_parse_slew_rate(phandle);

#define JH7110_PADCFG_POS	__BIT(7)
#define JH7110_PADCFG_SMT	__BIT(6)
#define JH7110_PADCFG_SLEW	__BIT(5)

	switch (bias) {
	case 0:
		*mask |= JH7110_PADCFG_BIAS_MASK;
		break;
	case GPIO_PIN_PULLUP:
		*mask |= JH7110_PADCFG_BIAS_MASK;
		*val  |= JH7110_PADCFG_PU;
		break;
	case GPIO_PIN_PULLDOWN:
		*mask |= JH7110_PADCFG_BIAS_MASK;
		*val  |= JH7110_PADCFG_PD;
		break;
	case -1:
	default:
		break;
	}

	switch (drive_strength) {
	case 2:
		*mask |=  JH7110_PADCFG_DS_MASK;
		*val  |=  JH7110_PADCFG_DS_2MA;
		break;
	case 4:
		*mask |=  JH7110_PADCFG_DS_MASK;
		*val  |=  JH7110_PADCFG_DS_4MA;
		break;
	case 8:
		*mask |=  JH7110_PADCFG_DS_MASK;
		*val  |=  JH7110_PADCFG_DS_8MA;
		break;
	case 12:
		*mask |=  JH7110_PADCFG_DS_MASK;
		*val  |=  JH7110_PADCFG_DS_12MA;
		break;
	case -1:
		break;
	default:
		aprint_error_dev(sc->sc_dev, "phandle %d invalid drive "
		"strength %d\n", phandle, drive_strength);
	}

	if (of_hasprop(phandle, "input-enable")) {
		*mask |= JH7110_PADCFG_IE;
		*val  |= JH7110_PADCFG_IE;
	}
	if (of_hasprop(phandle, "input-disable")) {
		*mask |=  JH7110_PADCFG_IE;
		*val  &= ~JH7110_PADCFG_IE;
	}
	if (of_hasprop(phandle, "input-schmitt-enable")) {
		*mask |=  JH7110_PADCFG_SMT;
		*val  |=  JH7110_PADCFG_SMT;
	}
	if (of_hasprop(phandle, "input-schmitt-disable")) {
		*mask |=  JH7110_PADCFG_SMT;
		*val  &= ~JH7110_PADCFG_SMT;
	}

	switch (slew_rate) {
	case 0:
		*mask |=  JH7110_PADCFG_SLEW;
		*val  &= ~JH7110_PADCFG_SLEW;
		break;
	case 1:
		*mask |=  JH7110_PADCFG_SLEW;
		*val  |=  JH7110_PADCFG_SLEW;
		break;
	case -1:
		break;
	default:
		aprint_error_dev(sc->sc_dev, "invalid slew rate");
	}
}


static void
jh7110_pinctrl_set_config_group(struct jh7110_pinctrl_softc *sc, int group)
{
	int pinmux_len;
	const u_int *pinmux = fdtbus_get_prop(group, "pinmux", &pinmux_len);
	size_t plen;
	const u_int *parray;

	aprint_debug_dev(sc->sc_dev, "set_config: group   %d\n", group);
	if (pinmux == NULL) {
		aprint_debug_dev(sc->sc_dev, "group %d neither 'pins' nor "
		    "'pinmux' exist\n", group);
		return;
	}
	if (pinmux != NULL) {
		plen = pinmux_len;
		parray = pinmux;
	}
	const size_t npins = plen / sizeof(uint32_t);

	aprint_debug_dev(sc->sc_dev, "set_config: group   %d, len %zu\n",
	    group, plen);

	uint16_t val, mask;
	jh7110_pinctrl_pin_properties(sc, group, &val, &mask);

	for (size_t i = 0; i < npins; i++) {
		uint32_t p = be32dec(&parray[i]);
		u_int pin_no;

#if 0
		if (pins != NULL) {
			pin_no = p;
			aprint_debug_dev(sc->sc_dev, "set_config: group   %d"
			    ", gpio %d doen %#x\n", group, pin_no,
			    RD4(sc, GPO_DOEN_CFG(pin_no)));
			WR4(sc, GPO_DOEN_CFG(pin_no), GPO_DISABLE);
			jh7110_padctl_rmw(sc, pin_no,
			    val, mask);
		}
#endif
		if (pinmux != NULL) {
			u_int din = __SHIFTOUT(p, DT_PINMUX_DIN_MASK);
			u_int dout = __SHIFTOUT(p, DT_PINMUX_DOUT_MASK);
			u_int doen = __SHIFTOUT(p, DT_PINMUX_DOEN_MASK);
			u_int func = __SHIFTOUT(p, DT_PINMUX_FUNC_MASK);
			pin_no = __SHIFTOUT(p, DT_PINMUX_PIN_MASK);
			jh7110_set_pinmux(sc, pin_no, din, dout,
			    doen, func);
		}
	}
}

static int
jh7110_pinctrl_set_config(device_t dev, const void *data, size_t len)
{
	struct jh7110_pinctrl_softc * const sc = device_private(dev);

	if (len != sizeof(uint32_t))
		return -1;

	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));
	aprint_debug_dev(sc->sc_dev, "set_config: phandle %d\n", phandle);

	for (int child = OF_child(phandle); child; child = OF_peer(child)) {
		jh7110_pinctrl_set_config_group(sc, child);
	}

	return 0;
}

static struct fdtbus_pinctrl_controller_func jh7110_pinctrl_funcs = {
	.set_config = jh7110_pinctrl_set_config,
};


static void *
jh7110_pinctrl_gpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct jh7110_pinctrl_softc * const sc = device_private(dev);

	if (len != 3 * sizeof(uint32_t))
		return NULL;

	const u_int *gpio = data;
	const u_int pin_no = be32toh(gpio[1]);
	const bool actlo = be32toh(gpio[2]) & 1;

	// XXXNH twiddle something??
	struct jh7110_pinctrl_gpio_pin *pin =
	    kmem_zalloc(sizeof(*pin), KM_SLEEP);
	pin->pin_sc = sc;
	pin->pin_no = pin_no;
	pin->pin_actlo = actlo;

	return pin;
}

static void
jh7110_pinctrl_gpio_release(device_t dev, void *priv)
{
	struct jh7110_pinctrl_softc * const sc = device_private(dev);
	struct jh7110_pinctrl_gpio_pin *pin = priv;

	KASSERT(sc == pin->pin_sc);
	// XXXNH untwiddle something?
	kmem_free(pin, sizeof(*pin));
}


static int
jh7110_pinctrl_gpio_read(device_t dev, void *priv, bool raw)
{
	struct jh7110_pinctrl_softc * const sc = device_private(dev);
	struct jh7110_pinctrl_gpio_pin *pin = priv;

	const u_int pin_no = pin ->pin_no;
	const u_int pins_per_bank = 32;
	const size_t banksz = sizeof(uint32_t);
	const bus_size_t offset = ((pin_no) / pins_per_bank) * banksz;
	const uint32_t mask = __BIT(pin_no % pins_per_bank);
	const uint32_t bank = RD4(sc, sc->sc_jpd->jpd_gpioin + offset);

	int val = __SHIFTOUT(bank, mask);
	if (!raw && pin->pin_actlo)
		val = !val;

	return val;
}


static void
jh7110_pinctrl_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct jh7110_pinctrl_softc * const sc = device_private(dev);
	struct jh7110_pinctrl_gpio_pin *pin = priv;

	const u_int pin_no = pin ->pin_no;
	const u_int pins_per_bank = 4;
	const size_t banksz = sizeof(uint32_t);
	const u_int bits_per_bank = banksz * NBBY;
	const u_int bits_per_pin = bits_per_bank / pins_per_bank;
	const bus_size_t offset = ((pin_no) / pins_per_bank) * banksz;
	const u_int shift = bits_per_pin * (pin_no % pins_per_bank);
	const uint32_t mask = sc->sc_jpd->jpd_dout_mask << shift;

	if (!raw && pin->pin_actlo)
		val = !val;

	mutex_enter(&sc->sc_lock);
	uint32_t bank = RD4(sc, sc->sc_jpd->jpd_dout + offset);
	bank &= ~mask;
	bank |= __SHIFTIN(val != 0 ? GPOUT_HIGH : GPOUT_LOW, mask);
	WR4(sc, sc->sc_jpd->jpd_dout + offset, bank);
	mutex_exit(&sc->sc_lock);
}


static struct fdtbus_gpio_controller_func jh7110_pinctrl_gpio_funcs = {
	.acquire = jh7110_pinctrl_gpio_acquire,
	.release = jh7110_pinctrl_gpio_release,
	.read = jh7110_pinctrl_gpio_read,
	.write = jh7110_pinctrl_gpio_write,
};


static const struct device_compatible_entry compat_data[] = {
	{ .compat = "starfive,jh7110-sys-pinctrl", .data = &jh7110_sys_pinctrl_data },
	{ .compat = "starfive,jh7110-aon-pinctrl", .data = &jh7110_aon_pinctrl_data },
	DEVICE_COMPAT_EOL
};


static int
jh7110_pinctrl_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
jh7110_pinctrl_attach(device_t parent, device_t self, void *aux)
{
	struct jh7110_pinctrl_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d", addr,
		    error);
		return;
	}

	sc->sc_jpd = of_compatible_lookup(phandle, compat_data)->data;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	aprint_naive("\n");
	aprint_normal(": Pin Controller\n");

	fdtbus_register_gpio_controller(sc->sc_dev, sc->sc_phandle,
	    &jh7110_pinctrl_gpio_funcs);

	for (int child = OF_child(phandle); child; child = OF_peer(child)) {
		fdtbus_register_pinctrl_config(self, child,
		    &jh7110_pinctrl_funcs);
        }
}

CFATTACH_DECL_NEW(jh7110_pinctrl, sizeof(struct jh7110_pinctrl_softc),
	jh7110_pinctrl_match, jh7110_pinctrl_attach, NULL, NULL);
