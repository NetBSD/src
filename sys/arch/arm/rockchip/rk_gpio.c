/* $NetBSD: rk_gpio.c,v 1.8 2025/06/03 18:26:38 rjs Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: rk_gpio.c,v 1.8 2025/06/03 18:26:38 rjs Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/gpio.h>
#include <sys/bitops.h>
#include <sys/lwp.h>

#include <dev/fdt/fdtvar.h>
#include <dev/gpio/gpiovar.h>

#define	GPIO_SWPORTA_DR_REG		0x0000
#define	GPIO_SWPORTA_DDR_REG		0x0004
#define	GPIO_INTEN_REG			0x0030
#define	GPIO_INTMASK_REG		0x0034
#define	GPIO_INTTYPE_LEVEL_REG		0x0038
#define	GPIO_INT_POLARITY_REG		0x003c
#define	GPIO_INT_STATUS_REG		0x0040
#define	GPIO_INT_RAWSTATUS_REG		0x0044
#define	GPIO_DEBOUNCE_REG		0x0048
#define	GPIO_PORTA_EOI_REG		0x004c
#define	GPIO_EXT_PORTA_REG		0x0050
#define	GPIO_LS_SYNC_REG		0x0060
#define	GPIO_VER_ID_REG			0x0078
#define	GPIO_VER_ID_GPIOV2		0x0101157c

/*
 * In "version 2" GPIO controllers, half of each register is used by the
 * write_enable mask, so the 32 pins are spread over two registers.
 *
 * pins  0 - 15 go into the GPIO_SWPORT_*_L register
 * pins 16 - 31 go into the GPIO_SWPORT_*_H register
 */
#define GPIOV2_SWPORT_DR_BASE		0x0000
#define GPIOV2_SWPORT_DR_REG(pin)	\
	(GPIOV2_SWPORT_DR_BASE + GPIOV2_REG_OFFSET(pin))
#define	GPIOV2_SWPORT_DDR_BASE		0x0008
#define	GPIOV2_SWPORT_DDR_REG(pin)	\
	(GPIOV2_SWPORT_DDR_BASE + GPIOV2_REG_OFFSET(pin))
#define	GPIOV2_EXT_PORT_REG		0x0070
#define	GPIOV2_REG_OFFSET(pin)		(((pin) >> 4) << 2)
#define	GPIOV2_DATA_MASK(pin)		(__BIT((pin) & 0xF))
#define	GPIOV2_WRITE_MASK(pin)		(__BIT(((pin) & 0xF) | 0x10))

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,gpio-bank" },
	DEVICE_COMPAT_EOL
};

struct rk_gpio_eint {
	int (*eint_func)(void *);
	void *eint_arg;
	bool eint_mpsafe;
	int eint_num;
};

struct rk_gpio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_lock;

	struct gpio_chipset_tag sc_gp;
	gpio_pin_t sc_pins[32];
	device_t sc_gpiodev;

	void *sc_ih;
	struct rk_gpio_eint sc_eint[32];
};

struct rk_gpio_pin {
	struct rk_gpio_softc *pin_sc;
	u_int pin_nr;
	int pin_flags;
	bool pin_actlo;
};

#define RD4(sc, reg) 		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val) 	\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	rk_gpio_match(device_t, cfdata_t, void *);
static void	rk_gpio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rk_gpio, sizeof(struct rk_gpio_softc),
	rk_gpio_match, rk_gpio_attach, NULL, NULL);

static void *
rk_gpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct rk_gpio_softc * const sc = device_private(dev);
	struct rk_gpio_pin *gpin;
	const u_int *gpio = data;

	if (len != 12)
		return NULL;

	const uint8_t pin = be32toh(gpio[1]) & 0xff;
	const bool actlo = be32toh(gpio[2]) & 1;

	if (pin >= __arraycount(sc->sc_pins))
		return NULL;

	sc->sc_gp.gp_pin_ctl(sc, pin, flags);

	gpin = kmem_zalloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_sc = sc;
	gpin->pin_nr = pin;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	return gpin;
}

static void
rk_gpio_release(device_t dev, void *priv)
{
	struct rk_gpio_softc * const sc = device_private(dev);
	struct rk_gpio_pin *pin = priv;

	KASSERT(sc == pin->pin_sc);

	sc->sc_gp.gp_pin_ctl(sc, pin->pin_nr, GPIO_PIN_INPUT);

	kmem_free(pin, sizeof(*pin));
}

static int
rk_gpio_read(device_t dev, void *priv, bool raw)
{
	struct rk_gpio_softc * const sc = device_private(dev);
	struct rk_gpio_pin *pin = priv;
	int val;

	KASSERT(sc == pin->pin_sc);

	val = sc->sc_gp.gp_pin_read(sc, pin->pin_nr);
	if (!raw && pin->pin_actlo)
		val = !val;

	return val;
}

static void
rk_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct rk_gpio_softc * const sc = device_private(dev);
	struct rk_gpio_pin *pin = priv;

	KASSERT(sc == pin->pin_sc);

	if (!raw && pin->pin_actlo)
		val = !val;

	sc->sc_gp.gp_pin_write(sc, pin->pin_nr, val);
}

static struct fdtbus_gpio_controller_func rk_gpio_funcs = {
	.acquire = rk_gpio_acquire,
	.release = rk_gpio_release,
	.read = rk_gpio_read,
	.write = rk_gpio_write,
};

static int
rk_gpio_pin_read(void *priv, int pin)
{
	struct rk_gpio_softc * const sc = priv;
	uint32_t data;
	int val;

	KASSERT(pin < __arraycount(sc->sc_pins));

	const uint32_t data_mask = __BIT(pin);

	/* No lock required for reads */
	data = RD4(sc, GPIO_EXT_PORTA_REG);
	val = __SHIFTOUT(data, data_mask);

	return val;
}

static void
rk_gpio_pin_write(void *priv, int pin, int val)
{
	struct rk_gpio_softc * const sc = priv;
	uint32_t data;

	KASSERT(pin < __arraycount(sc->sc_pins));

	const uint32_t data_mask = __BIT(pin);

	mutex_enter(&sc->sc_lock);
	data = RD4(sc, GPIO_SWPORTA_DR_REG);
	if (val)
		data |= data_mask;
	else
		data &= ~data_mask;
	WR4(sc, GPIO_SWPORTA_DR_REG, data);
	mutex_exit(&sc->sc_lock);
}

static void
rk_gpio_pin_ctl(void *priv, int pin, int flags)
{
	struct rk_gpio_softc * const sc = priv;
	uint32_t ddr;

	KASSERT(pin < __arraycount(sc->sc_pins));

	mutex_enter(&sc->sc_lock);
	ddr = RD4(sc, GPIO_SWPORTA_DDR_REG);
	if (flags & GPIO_PIN_INPUT)
		ddr &= ~__BIT(pin);
	else if (flags & GPIO_PIN_OUTPUT)
		ddr |= __BIT(pin);
	WR4(sc, GPIO_SWPORTA_DDR_REG, ddr);
	mutex_exit(&sc->sc_lock);
}

static int
rk_gpio_v2_pin_read(void *priv, int pin)
{
	struct rk_gpio_softc * const sc = priv;
	uint32_t data;
	int val;

	KASSERT(pin < __arraycount(sc->sc_pins));

	const uint32_t data_mask = __BIT(pin);

	/* No lock required for reads */
	data = RD4(sc, GPIOV2_EXT_PORT_REG);
	val = __SHIFTOUT(data, data_mask);

	return val;
}

static void
rk_gpio_v2_pin_write(void *priv, int pin, int val)
{
	struct rk_gpio_softc * const sc = priv;
	uint32_t data;

	KASSERT(pin < __arraycount(sc->sc_pins));

	const uint32_t write_mask = GPIOV2_WRITE_MASK(pin);

	/* No lock required for writes on v2 controllers  */
	data = val ? GPIOV2_DATA_MASK(pin) : 0;
	WR4(sc, GPIOV2_SWPORT_DR_REG(pin), write_mask | data);
}

static void
rk_gpio_v2_pin_ctl(void *priv, int pin, int flags)
{
	struct rk_gpio_softc * const sc = priv;
	uint32_t ddr;

	KASSERT(pin < __arraycount(sc->sc_pins));

	/* No lock required for writes on v2 controllers  */
	ddr = (flags & GPIO_PIN_OUTPUT) ? GPIOV2_DATA_MASK(pin) : 0;
	WR4(sc, GPIOV2_SWPORT_DDR_REG(pin), GPIOV2_WRITE_MASK(pin) | ddr);
}

static int
rk_gpio_intr(void *priv)
{
	struct rk_gpio_softc * const sc = priv;
	struct rk_gpio_eint *eint;
	uint32_t status, bit;
	int ret = 0;

	status = RD4(sc, GPIO_INT_STATUS_REG);
	if (status == 0)
		return ret;

	WR4(sc, GPIO_PORTA_EOI_REG, status);

	while ((bit = ffs32(status)) != 0) {
		status &= ~__BIT(bit - 1);
		eint = &sc->sc_eint[bit - 1];
		if (eint == NULL || eint->eint_func == NULL)
			continue;
		if (!eint->eint_mpsafe)
			KERNEL_LOCK(1, curlwp);
		ret |= eint->eint_func(eint->eint_arg);
		if (!eint->eint_mpsafe)
			KERNEL_UNLOCK_ONE(curlwp);
	}

	return ret;
}

static void *
rk_intr_enable(struct rk_gpio_softc *sc, u_int pin, uint32_t level,
    uint32_t polarity, bool mpsafe, int (*func)(void *), void *arg)
{
	uint32_t val;
	struct rk_gpio_eint *eint;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_eint[pin].eint_func != NULL) {
		mutex_exit(&sc->sc_lock);
		return NULL;	/* in use */
	}

	eint = &sc->sc_eint[pin];

	eint->eint_func = func;
	eint->eint_arg = arg;
	eint->eint_mpsafe = mpsafe;
	eint->eint_num = pin;

	val = RD4(sc, GPIO_INTTYPE_LEVEL_REG);
	if (level)
		val |= 1 << pin;
	else
		val &= ~(1 << pin);
	WR4(sc, GPIO_INTTYPE_LEVEL_REG, val);

	val = RD4(sc, GPIO_INT_POLARITY_REG);
	if (polarity)
		val |= 1 << pin;
	else
		val &= ~(1 << pin);
	WR4(sc, GPIO_INT_POLARITY_REG, val);

	val = RD4(sc, GPIO_INTEN_REG);
	val |= 1 << pin;
	WR4(sc, GPIO_INTEN_REG, val);
#if 0
	/* Configure eint mode */
	val = R4(sc, SUNXI_GPIO_INT_CFG, pin);
	val &= ~SUNXI_GPIO_INT_MODEMASK(eint->eint_num);
	val |= __SHIFTIN(mode, SUNXI_GPIO_INT_MODEMASK(eint->eint_num));
	GPIO_WRITE(sc, SUNXI_GPIO_INT_CFG(eint->eint_bank, eint->eint_num), val);

	val = SUNXI_GPIO_INT_DEBOUNCE_CLK_SEL;
	GPIO_WRITE(sc, SUNXI_GPIO_INT_DEBOUNCE(eint->eint_bank), val);

	/* Enable eint */
	val = GPIO_READ(sc, SUNXI_GPIO_INT_CTL(eint->eint_bank));
	val |= __BIT(eint->eint_num);
	GPIO_WRITE(sc, SUNXI_GPIO_INT_CTL(eint->eint_bank), val);
#endif
	mutex_exit(&sc->sc_lock);

	return eint;
}

static void
rk_intr_disable(struct rk_gpio_softc *sc, struct rk_gpio_eint *eint)
{
	uint32_t val;

	KASSERT(eint != NULL && eint->eint_func != NULL);

	mutex_enter(&sc->sc_lock);

	/* Disable eint */
	val = RD4(sc, GPIO_INTEN_REG);
	val &= ~__BIT(eint->eint_num);
	WR4(sc, GPIO_INTEN_REG, val);
	WR4(sc, GPIO_INT_STATUS_REG, __BIT(eint->eint_num));

	sc->sc_eint[eint->eint_num].eint_func = NULL;
	
	mutex_exit(&sc->sc_lock);
}

static void *
rk_fdt_intr_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct rk_gpio_softc * const sc = device_private(dev);
	bool mpsafe = (flags & GPIO_INTR_MPSAFE) != 0;
	uint32_t level, polarity;

	const uint32_t pin = be32toh(specifier[0]);
	const uint32_t type = be32toh(specifier[1]) & 0xf;

	switch (type) {
	case FDT_INTR_TYPE_POS_EDGE:
		level = 1;
		polarity = 1;
		break;
	case FDT_INTR_TYPE_NEG_EDGE:
		level = 1;
		polarity = 0;
		break;
	case FDT_INTR_TYPE_HIGH_LEVEL:
		level = 0;
		polarity = 1;
		break;
	case FDT_INTR_TYPE_LOW_LEVEL:
		level = 0;
		polarity = 0;
		break;
	default:
		aprint_error_dev(dev, "%s: unsupported irq type 0x%x\n",
		    __func__, type);
		return NULL;
	}

	return rk_intr_enable(sc, pin, level, polarity, mpsafe, func, arg);
}

static void
rk_fdt_intr_disestablish(device_t dev, void *ih)
{
	struct rk_gpio_softc * const sc = device_private(dev);
	struct rk_gpio_eint * const eint = ih;

	rk_intr_disable(sc, eint);
}

static bool
rk_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{

	if (!specifier)
		return false;
	const u_int pin = be32toh(specifier[0]);

	if (pin < 0 || pin >= 32)
		return false;

	snprintf(buf, buflen, "GPIO %d", pin);

	return true;
}

static void
rk_fdt_intr_mask(device_t dev, void *ih)
{
	struct rk_gpio_softc * const sc = device_private(dev);
	void * const gpio = device_private(sc->sc_gpiodev);

	gpio_intr_mask(gpio, ih);
}

static void
rk_fdt_intr_unmask(device_t dev, void *ih)
{
	struct rk_gpio_softc * const sc = device_private(dev);
	void * const gpio = device_private(sc->sc_gpiodev);

	gpio_intr_unmask(gpio, ih);
}


static struct fdtbus_interrupt_controller_func rk_gpio_intrfuncs = {
	.establish = rk_fdt_intr_establish,
	.disestablish = rk_fdt_intr_disestablish,
	.intrstr = rk_fdt_intrstr,
	.mask = rk_fdt_intr_mask,
	.unmask = rk_fdt_intr_unmask
};

static void *
rk_gpio_intr_establish(void *vsc, int pin, int ipl, int irqmode,
    int (*func)(void *), void *arg)
{
	struct rk_gpio_softc * const sc = vsc;
	bool mpsafe = (irqmode & GPIO_INTR_MPSAFE) != 0;
	int type = irqmode & GPIO_INTR_MODE_MASK;
	uint32_t level, polarity;

	switch (type) {
	case GPIO_INTR_POS_EDGE:
		level = 1;
		polarity = 1;
		break;
	case GPIO_INTR_NEG_EDGE:
		level = 1;
		polarity = 0;
		break;
	case GPIO_INTR_HIGH_LEVEL:
		level = 0;
		polarity = 1;
		break;
	case GPIO_INTR_LOW_LEVEL:
		level = 0;
		polarity = 0;
		break;
	default:
		aprint_error_dev(sc->sc_dev, "%s: unsupported irq type 0x%x\n",
				 __func__, type);
		return NULL;
	}

	return rk_intr_enable(sc, pin, level, polarity, mpsafe, func, arg);
}

static void
rk_gpio_intr_disestablish(void *vsc, void *ih)
{
	struct rk_gpio_softc * const sc = vsc;
	struct rk_gpio_eint * const eint = ih;

	rk_intr_disable(sc, eint);
}

static bool
rk_gpio_intrstr(void *vsc, int pin, int irqmode, char *buf, size_t buflen)
{

	if (pin < 0 || pin >= 32)
		return false;

	snprintf(buf, buflen, "GPIO %d", pin);

	return true;
}

static void
rk_gpio_intr_mask(void *priv, void *ih)
{
	struct rk_gpio_softc * const sc = priv;
	struct rk_gpio_eint * const eint = ih;
	uint32_t val;

	val = RD4(sc, GPIO_INTMASK_REG);
	val |= 1 << eint->eint_num;
	WR4(sc, GPIO_INTEN_REG, val);
}

static void
rk_gpio_intr_unmask(void *priv, void *ih)
{
	struct rk_gpio_softc * const sc = priv;
	struct rk_gpio_eint * const eint = ih;
	uint32_t val;

	val = RD4(sc, GPIO_INTMASK_REG);
	val &= ~(1 << eint->eint_num);
	WR4(sc, GPIO_INTEN_REG, val);
}

static void
rk_gpio_attach_ports(struct rk_gpio_softc *sc)
{
	struct gpiobus_attach_args gba;
	u_int pin;

	for (pin = 0; pin < __arraycount(sc->sc_pins); pin++) {
		sc->sc_pins[pin].pin_num = pin;
		sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
		sc->sc_pins[pin].pin_state = rk_gpio_pin_read(sc, pin);
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = &sc->sc_gp;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = __arraycount(sc->sc_pins);
	sc->sc_gpiodev = config_found(sc->sc_dev, &gba, NULL, CFARGS_NONE);
}

static int
rk_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct rk_gpio_softc * const sc = device_private(self);
	struct gpio_chipset_tag * const gp = &sc->sc_gp;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	uint32_t ver_id;
	int ver;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if ((clk = fdtbus_clock_get_index(phandle, 0)) == NULL || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable clock\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	gp->gp_cookie = sc;
	ver_id = RD4(sc, GPIO_VER_ID_REG);
	switch (ver_id) {
	case 0: /* VER_ID not implemented in v1 but reads back as 0 */
		ver = 1;
		gp->gp_pin_read = rk_gpio_pin_read;
		gp->gp_pin_write = rk_gpio_pin_write;
		gp->gp_pin_ctl = rk_gpio_pin_ctl;
		gp->gp_intr_establish = rk_gpio_intr_establish;
		gp->gp_intr_disestablish = rk_gpio_intr_disestablish;
		gp->gp_intr_str = rk_gpio_intrstr;
		gp->gp_intr_mask = rk_gpio_intr_mask;
		gp->gp_intr_unmask = rk_gpio_intr_unmask;
		break;
	case GPIO_VER_ID_GPIOV2:
		ver = 2;
		gp->gp_pin_read = rk_gpio_v2_pin_read;
		gp->gp_pin_write = rk_gpio_v2_pin_write;
		gp->gp_pin_ctl = rk_gpio_v2_pin_ctl;
		/* XXX */
		break;
	default:
		aprint_error(": unknown version 0x%08" PRIx32 "\n", ver_id);
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	aprint_naive("\n");
	aprint_normal(": GPIO v%d (%s)\n", ver, fdtbus_get_string(phandle, "name"));

	fdtbus_register_gpio_controller(self, phandle, &rk_gpio_funcs);

	rk_gpio_attach_ports(sc);

	WR4(sc, GPIO_INTEN_REG, 0);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}
	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, rk_gpio_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
	fdtbus_register_interrupt_controller(self, phandle,
	    &rk_gpio_intrfuncs);
}
