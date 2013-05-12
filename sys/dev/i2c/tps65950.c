/* $NetBSD: tps65950.c,v 1.3.10.3 2013/05/12 00:42:50 khorben Exp $ */

/*-
 * Copyright (c) 2012 Jared D. McNeill <jmcneill@invisible.ca>
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

/*
 * TI TPS65950 OMAP Power Management and System Companion Device
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tps65950.c,v 1.3.10.3 2013/05/12 00:42:50 khorben Exp $");

#define _INTR_PRIVATE

#include "gpio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/workqueue.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>
#include <sys/wdog.h>

#include <dev/i2c/i2cvar.h>

#if NGPIO > 0
#include <arm/pic/picvar.h>
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>
#endif /* NGPIO > 0 */

#include <dev/clock_subr.h>
#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/tps65950reg.h>

/* Default watchdog period, in seconds */
#ifndef TPS65950_WDOG_DEFAULT_PERIOD
#define TPS65950_WDOG_DEFAULT_PERIOD	30
#endif

struct tps65950_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	struct sysctllog *sc_sysctllog;

	/* PIH */
	void			*sc_intr;
	struct workqueue	*sc_workq;
	struct work		sc_work;
	bool			sc_queued;

#if NGPIO > 0
	/* GPIO */
	struct gpio_chipset_tag	sc_gpio;
	gpio_pin_t		sc_gpio_pins[18];
	struct pic_softc	sc_gpio_pic;

#define PIC_TO_SOFTC(pic) \
	((struct tps65950_softc *)((char *)(pic) - \
		offsetof(struct tps65950_softc, sc_gpio_pic)))
#endif /* NGPIO > 0 */

	/* WATCHDOG */
	struct sysmon_wdog sc_smw;
	struct todr_chip_handle sc_todr;
};


/* XXX global workqueue to re-enable interrupts once handled */
static struct workqueue		*tps65950_pih_workqueue;
static struct work		tps65950_pih_workqueue_work;
static bool			tps65950_pih_workqueue_available;

static int	tps65950_match(device_t, cfdata_t, void *);
static void	tps65950_attach(device_t, device_t, void *);

static int	tps65950_read_1(struct tps65950_softc *, uint8_t, uint8_t *);
static int	tps65950_write_1(struct tps65950_softc *, uint8_t, uint8_t);

static void	tps65950_sysctl_attach(struct tps65950_softc *);

static int	tps65950_intr(void *);
static void	tps65950_intr_work(struct work *, void *);

static void	tps65950_pih_attach(struct tps65950_softc *, int);
static void	tps65950_pih_intr_work(struct work *, void *);

#if NGPIO > 0
static void	tps65950_gpio_attach(struct tps65950_softc *, int);

static void	tps65950_gpio_intr(struct tps65950_softc *);

static int	tps65950_gpio_pin_read(void *, int);
static void	tps65950_gpio_pin_write(void *, int, int);
static void	tps65950_gpio_pin_ctl(void *, int, int);

static void	tps65950_gpio_pic_block_irqs(struct pic_softc *, size_t,
		uint32_t);
static void	tps65950_gpio_pic_unblock_irqs(struct pic_softc *, size_t,
		uint32_t);
static int	tps65950_gpio_pic_find_pending_irqs(struct pic_softc *);
static void	tps65950_gpio_pic_establish_irq(struct pic_softc *,
		struct intrsource *);

const struct pic_ops tps65950_gpio_pic_ops = {
	.pic_block_irqs = tps65950_gpio_pic_block_irqs,
	.pic_unblock_irqs = tps65950_gpio_pic_unblock_irqs,
	.pic_find_pending_irqs = tps65950_gpio_pic_find_pending_irqs,
	.pic_establish_irq = tps65950_gpio_pic_establish_irq
};
#endif /* NGPIO > 0 */

static void	tps65950_rtc_attach(struct tps65950_softc *);
static int	tps65950_rtc_enable(struct tps65950_softc *, bool);
static int	tps65950_rtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int	tps65950_rtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

static void	tps65950_wdog_attach(struct tps65950_softc *);
static int	tps65950_wdog_setmode(struct sysmon_wdog *);
static int	tps65950_wdog_tickle(struct sysmon_wdog *);

CFATTACH_DECL_NEW(tps65950pm, sizeof(struct tps65950_softc),
    tps65950_match, tps65950_attach, NULL, NULL);

static int
tps65950_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	switch (ia->ia_addr) {
	case TPS65950_ADDR_ID1:
	case TPS65950_ADDR_ID2:
	case TPS65950_ADDR_ID3:
	case TPS65950_ADDR_ID4:
	case TPS65950_ADDR_ID5:
		return 1;
	default:
		return 0;
	}
}

static void
tps65950_attach(device_t parent, device_t self, void *aux)
{
	struct tps65950_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	uint8_t buf[4];
	uint32_t idcode;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_naive("\n");

	switch (sc->sc_addr) {
	case TPS65950_ADDR_ID2:
		memset(buf, 0, sizeof(buf));
		iic_acquire_bus(sc->sc_i2c, 0);
		tps65950_write_1(sc, TPS65950_ID2_UNLOCK_TEST_REG,
		    TPS65950_ID2_UNLOCK_TEST_REG_MAGIC);
		tps65950_read_1(sc, TPS65950_ID2_IDCODE_7_0, &buf[0]);
		tps65950_read_1(sc, TPS65950_ID2_IDCODE_15_8, &buf[1]);
		tps65950_read_1(sc, TPS65950_ID2_IDCODE_23_16, &buf[2]);
		tps65950_read_1(sc, TPS65950_ID2_IDCODE_31_24, &buf[3]);
		iic_release_bus(sc->sc_i2c, 0);
		idcode = (buf[0] << 0) | (buf[1] << 8) |
			 (buf[2] << 16) | (buf[3] << 24);
		aprint_normal(": IDCODE %08X", idcode);

		aprint_normal(": PIH");
		tps65950_pih_attach(sc, ia->ia_intr);

#if NGPIO > 0
		aprint_normal(", GPIO");
		tps65950_gpio_attach(sc, ia->ia_intrbase);
#else
		aprint_normal("\n");
#endif /* NGPIO > 0 */
		break;
	case TPS65950_ADDR_ID3:
		aprint_normal(": LED\n");
		tps65950_sysctl_attach(sc);
		break;
	case TPS65950_ADDR_ID4:
		aprint_normal(": RTC, WATCHDOG\n");
		tps65950_rtc_attach(sc);
		tps65950_wdog_attach(sc);
		break;
	default:
		aprint_normal("\n");
		break;
	}
}

static int
tps65950_read_1(struct tps65950_softc *sc, uint8_t reg, uint8_t *val)
{
	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &reg, sizeof(reg), val, sizeof(*val), 0);
}

static int
tps65950_write_1(struct tps65950_softc *sc, uint8_t reg, uint8_t val)
{
	uint8_t data[2] = { reg, val };
	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    NULL, 0, data, sizeof(data), 0);
}

static int
tps65950_sysctl_leda(SYSCTLFN_ARGS)
{
	struct tps65950_softc *sc;
	struct sysctlnode node;
	uint8_t val;
	u_int leda;
	int error;

	node = *rnode;
	sc = node.sysctl_data;
	iic_acquire_bus(sc->sc_i2c, 0);
	error = tps65950_read_1(sc, TPS65950_ID3_REG_LED, &val);
	iic_release_bus(sc->sc_i2c, 0);
	if (error)
		return error;
	leda = (val & TPS65950_ID3_REG_LED_LEDAON) ? 1 : 0;
	node.sysctl_data = &leda;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (leda)
		val |= (TPS65950_ID3_REG_LED_LEDAON|TPS65950_ID3_REG_LED_LEDAPWM);
	else
		val &= ~(TPS65950_ID3_REG_LED_LEDAON|TPS65950_ID3_REG_LED_LEDAPWM);

	
	iic_acquire_bus(sc->sc_i2c, 0);
	error = tps65950_write_1(sc, TPS65950_ID3_REG_LED, val);
	iic_release_bus(sc->sc_i2c, 0);

	return error;
}

static int
tps65950_sysctl_ledb(SYSCTLFN_ARGS)
{
	struct tps65950_softc *sc;
	struct sysctlnode node;
	uint8_t val;
	u_int ledb;
	int error;

	node = *rnode;
	sc = node.sysctl_data;
	iic_acquire_bus(sc->sc_i2c, 0);
	error = tps65950_read_1(sc, TPS65950_ID3_REG_LED, &val);
	iic_release_bus(sc->sc_i2c, 0);
	if (error)
		return error;
	ledb = (val & TPS65950_ID3_REG_LED_LEDBON) ? 1 : 0;
	node.sysctl_data = &ledb;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (ledb)
		val |= (TPS65950_ID3_REG_LED_LEDBON|TPS65950_ID3_REG_LED_LEDBPWM);
	else
		val &= ~(TPS65950_ID3_REG_LED_LEDBON|TPS65950_ID3_REG_LED_LEDBPWM);

	iic_acquire_bus(sc->sc_i2c, 0);
	error = tps65950_write_1(sc, TPS65950_ID3_REG_LED, val);
	iic_release_bus(sc->sc_i2c, 0);

	return error;
}

static void
tps65950_sysctl_attach(struct tps65950_softc *sc)
{
	struct sysctllog **log = &sc->sc_sysctllog;
	const struct sysctlnode *rnode, *cnode;
	int error;

	error = sysctl_createv(log, 0, NULL, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "hw", NULL, NULL, 0, NULL, 0, CTL_HW, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "tps65950", SYSCTL_DESCR("tps65950 control"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "leda",
	    SYSCTL_DESCR("LEDA enable"), tps65950_sysctl_leda, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "ledb",
	    SYSCTL_DESCR("LEDB enable"), tps65950_sysctl_ledb, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;
}

static int
tps65950_intr(void *v)
{
	struct tps65950_softc *sc = v;

	if (sc->sc_queued == false) {
		workqueue_enqueue(sc->sc_workq, &sc->sc_work, NULL);
		sc->sc_queued = true;
	}

	/* disable the interrupt until it's properly handled */
	return 0;
}

static void
tps65950_intr_work(struct work *work, void *v)
{
	struct tps65950_softc *sc = v;
	uint8_t u8;

	iic_acquire_bus(sc->sc_i2c, 0);

	/* acknowledge the interrupt */
	tps65950_read_1(sc, TPS65950_PIH_REG_ISR_P1, &u8);
	tps65950_write_1(sc, TPS65950_PIH_REG_ISR_P1, u8);

	/* dispatch the interrupt */
#if NGPIO > 0
	if (u8 & TPS65950_PIH_REG_ISR_P1_ISR0)
		tps65950_gpio_intr(sc);
#endif /* NGPIO > 0 */

	iic_release_bus(sc->sc_i2c, 0);

	/* allow the workqueue to be entered again */
	sc->sc_queued = false;

	/* restore the main interrupt handler */
	if (tps65950_pih_workqueue_available) {
		tps65950_pih_workqueue_available = false;
		workqueue_enqueue(tps65950_pih_workqueue,
				&tps65950_pih_workqueue_work, NULL);
	}
}

static void
tps65950_pih_attach(struct tps65950_softc *sc, int intr)
{
	int error;

	/* create the workqueues */
	error = workqueue_create(&sc->sc_workq, device_xname(sc->sc_dev),
			tps65950_intr_work, sc, PRIO_MAX, IPL_VM, 0);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't create workqueue\n");
		return;
	}
	sc->sc_queued = false;

	error = workqueue_create(&tps65950_pih_workqueue,
			device_xname(sc->sc_dev), tps65950_pih_intr_work, sc,
			PRIO_MAX, IPL_HIGH, 0);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't create workqueue\n");
		return;
	}
	tps65950_pih_workqueue_available = true;

	/* establish the interrupt handler */
	sc->sc_intr = intr_establish(intr, IPL_VM, IST_LEVEL, tps65950_intr,
			sc);
	if (sc->sc_intr == NULL) {
		aprint_error_dev(sc->sc_dev, "couldn't establish interrupt\n");
	}
}

static void
tps65950_pih_intr_work(struct work *work, void *v)
{
	struct tps65950_softc *sc = v;

	tps65950_pih_workqueue_available = true;
	intr_enable(sc->sc_intr);
}

#if NGPIO > 0
static void
tps65950_gpio_attach(struct tps65950_softc *sc, int intrbase)
{
	struct gpio_chipset_tag * const gp = &sc->sc_gpio;
	struct gpiobus_attach_args gba;
	gpio_pin_t *pins;
	uint32_t mask;
	int pin;

	/* disable interrupts */
	iic_acquire_bus(sc->sc_i2c, 0);
	tps65950_write_1(sc, TPS65950_GPIO_GPIO_IMR1A, 0);
	tps65950_write_1(sc, TPS65950_GPIO_GPIO_IMR2A, 0);
	tps65950_write_1(sc, TPS65950_GPIO_GPIO_IMR3A, 0);
	iic_release_bus(sc->sc_i2c, 0);

	/* map interrupts */
	if (sc->sc_intr == NULL || intrbase < 0) {
		aprint_normal("\n");
		aprint_error_dev(sc->sc_dev, "couldn't map GPIO interrupts\n");
		return;
	} else {
		sc->sc_gpio_pic.pic_ops = &tps65950_gpio_pic_ops;
		strlcpy(sc->sc_gpio_pic.pic_name, device_xname(sc->sc_dev),
				sizeof(sc->sc_gpio_pic.pic_name));
		sc->sc_gpio_pic.pic_maxsources = 18;
		pic_add(&sc->sc_gpio_pic, intrbase);
		aprint_normal(": interrupts %d..%d\n", intrbase, intrbase + 17);
	}

	gp->gp_cookie = sc;
	gp->gp_pin_read = tps65950_gpio_pin_read;
	gp->gp_pin_write = tps65950_gpio_pin_write;
	gp->gp_pin_ctl = tps65950_gpio_pin_ctl;

	gba.gba_gc = gp;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = __arraycount(sc->sc_gpio_pins);

	for (pin = 0, mask = 1, pins = sc->sc_gpio_pins;
			pin < 18; pin++, mask <<= 1, pins++) {
		pins->pin_num = pin;
		pins->pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
		pins->pin_flags = GPIO_PIN_INPUT;
		pins->pin_state = GPIO_PIN_HIGH;
	}

	config_found_ia(sc->sc_dev, "gpiobus", &gba, gpiobus_print);
}

static void
tps65950_gpio_intr(struct tps65950_softc *sc)
{
	pic_handle_intr(&sc->sc_gpio_pic);
}

static int
tps65950_gpio_pin_read(void *v, int pin)
{
	struct tps65950_softc *sc = v;
	uint8_t reg;
	uint8_t bit;
	uint8_t val;

	if (pin < 0)
		return ENODEV;
	else if (pin < 8)
	{
		reg = TPS65950_GPIO_GPIODATAIN1;
		bit = pin;
	}
	else if (pin < 16)
	{
		reg = TPS65950_GPIO_GPIODATAIN2;
		bit = pin - 8;
	}
	else if (pin < 18)
	{
		reg = TPS65950_GPIO_GPIODATAIN3;
		bit = pin - 16;
	}
	else
		return ENODEV;

	iic_acquire_bus(sc->sc_i2c, 0);
	tps65950_read_1(sc, reg, &val);
	iic_release_bus(sc->sc_i2c, 0);

	return val & (1 << bit);
}

static void
tps65950_gpio_pin_write(void *v, int pin, int value)
{
	struct tps65950_softc *sc = v;
	uint8_t reg;
	uint8_t bit;
	uint8_t val;
	uint8_t new;

	if (pin < 0)
		return;
	else if (pin < 8)
	{
		reg = TPS65950_GPIO_GPIODATAOUT1;
		bit = pin;
	}
	else if (pin < 16)
	{
		reg = TPS65950_GPIO_GPIODATAOUT2;
		bit = pin - 8;
	}
	else if (pin < 18)
	{
		reg = TPS65950_GPIO_GPIODATAOUT3;
		bit = pin - 16;
	}
	else
		return;

	iic_acquire_bus(sc->sc_i2c, 0);
	tps65950_read_1(sc, reg, &val);
	new = val;
	if (value)
		new |= (1 << bit);
	else
		new &= ~(1 << bit);
	if (new != val)
		tps65950_write_1(sc, reg, new);
	iic_release_bus(sc->sc_i2c, 0);
}

static void
tps65950_gpio_pin_ctl(void *v, int pin, int flags)
{
	struct tps65950_softc *sc = v;
	uint8_t reg;
	uint8_t bit;
	uint8_t val;
	uint8_t new;

	if (pin < 0)
		return;
	else if (pin < 8)
	{
		reg = TPS65950_GPIO_GPIODATADIR1;
		bit = pin;
	}
	else if (pin < 16)
	{
		reg = TPS65950_GPIO_GPIODATADIR2;
		bit = pin - 8;
	}
	else if (pin < 18)
	{
		reg = TPS65950_GPIO_GPIODATADIR3;
		bit = pin - 16;
	}
	else
		return;

	iic_acquire_bus(sc->sc_i2c, 0);
	tps65950_read_1(sc, reg, &val);
	new = val;
	switch (flags & (GPIO_PIN_INPUT | GPIO_PIN_OUTPUT)) {
		case GPIO_PIN_INPUT:	new &= ~(1 << bit); break;
		case GPIO_PIN_OUTPUT:	new |= (1 << bit); break;
		default:		break;
	}
	if (new != val)
		tps65950_write_1(sc, reg, new);
	iic_release_bus(sc->sc_i2c, 0);
}

static void
tps65950_gpio_pic_block_irqs(struct pic_softc *pic, size_t irq_base,
		uint32_t irq_mask)
{
	struct tps65950_softc *sc = PIC_TO_SOFTC(pic);
	int i;
	const uint32_t reg[3] = { TPS65950_GPIO_GPIO_IMR1A,
		TPS65950_GPIO_GPIO_IMR2A, TPS65950_GPIO_GPIO_IMR3A };
	uint8_t u8;

	iic_acquire_bus(sc->sc_i2c, 0);
	for (i = 0; i < 3; i++) {
		tps65950_read_1(sc, reg[i], &u8);
		u8 &= ~((irq_mask >> (i * 8)) & 0xff);
		tps65950_write_1(sc, reg[i], u8);
	}
	iic_release_bus(sc->sc_i2c, 0);
}

static void
tps65950_gpio_pic_unblock_irqs(struct pic_softc *pic, size_t irq_base,
		uint32_t irq_mask)
{
	struct tps65950_softc *sc = PIC_TO_SOFTC(pic);
	int i;
	const uint32_t reg[3] = { TPS65950_GPIO_GPIO_IMR1A,
		TPS65950_GPIO_GPIO_IMR2A, TPS65950_GPIO_GPIO_IMR3A };
	uint8_t u8;

	iic_acquire_bus(sc->sc_i2c, 0);
	for (i = 0; i < 3; i++) {
		tps65950_read_1(sc, reg[i], &u8);
		u8 |= (irq_mask >> (i * 8)) & 0xff;
		tps65950_write_1(sc, reg[i], u8);
	}
	iic_release_bus(sc->sc_i2c, 0);
}

static int
tps65950_gpio_pic_find_pending_irqs(struct pic_softc *pic)
{
	struct tps65950_softc *sc = PIC_TO_SOFTC(pic);
	uint32_t pending;
	uint8_t u8;

	iic_acquire_bus(sc->sc_i2c, 0);
	tps65950_read_1(sc, TPS65950_GPIO_GPIO_ISR1A, &u8);
	pending = u8;
	tps65950_read_1(sc, TPS65950_GPIO_GPIO_ISR2A, &u8);
	pending |= (u8 << 8);
	tps65950_read_1(sc, TPS65950_GPIO_GPIO_ISR3A, &u8);
	pending |= ((u8 & 0x3) << 16);
	iic_release_bus(sc->sc_i2c, 0);
	aprint_normal_dev(sc->sc_dev, "%s() 0x%08x\n", __func__, pending);
	return pending;
}

static void
tps65950_gpio_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	struct tps65950_softc *sc = PIC_TO_SOFTC(pic);
	int index;
	int shift;
	const uint32_t edge[5] = { TPS65950_GPIO_GPIO_EDR1,
		TPS65950_GPIO_GPIO_EDR2, TPS65950_GPIO_GPIO_EDR3,
		TPS65950_GPIO_GPIO_EDR4, TPS65950_GPIO_GPIO_EDR5 };
	uint32_t reg;
	uint8_t bits;
	uint8_t u8;

	index = is->is_irq / 4;
	shift = (is->is_irq % 4) * 2;
	switch (is->is_type) {
		case IST_LEVEL_LOW:
		case IST_LEVEL_HIGH:
			/* FIXME implement */
			return;
		case IST_EDGE_FALLING:
			reg = edge[index];
			bits = 0x1;
			break;
		case IST_EDGE_RISING:
			reg = edge[index];
			bits = 0x2;
			break;
		case IST_EDGE_BOTH:
			reg = edge[index];
			bits = 0x3;
			break;
		default:
			return;
	}
	iic_acquire_bus(sc->sc_i2c, 0);
	tps65950_read_1(sc, reg, &u8);
	u8 |= bits << shift;
	tps65950_write_1(sc, reg, u8);
	iic_release_bus(sc->sc_i2c, 0);
}
#endif /* NGPIO > 0 */

static void
tps65950_rtc_attach(struct tps65950_softc *sc)
{
	sc->sc_todr.todr_gettime_ymdhms = tps65950_rtc_gettime;
	sc->sc_todr.todr_settime_ymdhms = tps65950_rtc_settime;
	sc->sc_todr.cookie = sc;
	todr_attach(&sc->sc_todr);

	iic_acquire_bus(sc->sc_i2c, 0);
	tps65950_rtc_enable(sc, true);
	iic_release_bus(sc->sc_i2c, 0);
}

static int
tps65950_rtc_enable(struct tps65950_softc *sc, bool enable)
{
	uint8_t rtc_ctrl;
	int error;

	error = tps65950_read_1(sc, TPS65950_ID4_REG_RTC_CTRL_REG, &rtc_ctrl);
	if (error)
		return error;

	if (enable) {
		rtc_ctrl |= TPS65950_ID4_REG_RTC_CTRL_REG_STOP_RTC;
	} else {
		rtc_ctrl &= ~TPS65950_ID4_REG_RTC_CTRL_REG_STOP_RTC;
	}

	return tps65950_write_1(sc, TPS65950_ID4_REG_RTC_CTRL_REG, rtc_ctrl);
}

#define RTC_READ(reg, var)						 \
	do {								 \
		tps65950_write_1(sc, TPS65950_ID4_REG_RTC_CTRL_REG,	 \
		    TPS65950_ID4_REG_RTC_CTRL_REG_GET_TIME);		 \
		if ((error = tps65950_read_1(sc, (reg), &(var))) != 0) { \
			iic_release_bus(sc->sc_i2c, 0);			 \
			return error;					 \
		}							 \
	} while (0)

#define RTC_WRITE(reg, val)						 \
	do {								 \
		if ((error = tps65950_write_1(sc, (reg), (val))) != 0) { \
			iic_release_bus(sc->sc_i2c, 0);			 \
			return error;					 \
		}							 \
	} while (0)

static int
tps65950_rtc_gettime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct tps65950_softc *sc = tch->cookie;
	uint8_t seconds_reg, minutes_reg, hours_reg,
		days_reg, months_reg, years_reg, weeks_reg;
	int error = 0;

	iic_acquire_bus(sc->sc_i2c, 0);
	RTC_READ(TPS65950_ID4_REG_SECONDS_REG, seconds_reg);
	RTC_READ(TPS65950_ID4_REG_MINUTES_REG, minutes_reg);
	RTC_READ(TPS65950_ID4_REG_HOURS_REG, hours_reg);
	RTC_READ(TPS65950_ID4_REG_DAYS_REG, days_reg);
	RTC_READ(TPS65950_ID4_REG_MONTHS_REG, months_reg);
	RTC_READ(TPS65950_ID4_REG_YEARS_REG, years_reg);
	RTC_READ(TPS65950_ID4_REG_WEEKS_REG, weeks_reg);
	iic_release_bus(sc->sc_i2c, 0);

	dt->dt_sec = FROMBCD(seconds_reg);
	dt->dt_min = FROMBCD(minutes_reg);
	dt->dt_hour = FROMBCD(hours_reg);
	dt->dt_day = FROMBCD(days_reg);
	dt->dt_mon = FROMBCD(months_reg);
	dt->dt_year = FROMBCD(years_reg) + 2000;
	dt->dt_wday = FROMBCD(weeks_reg);

	return 0;
}

static int
tps65950_rtc_settime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct tps65950_softc *sc = tch->cookie;
	int error = 0;

	iic_acquire_bus(sc->sc_i2c, 0);
	tps65950_rtc_enable(sc, false);
	RTC_WRITE(TPS65950_ID4_REG_SECONDS_REG, TOBCD(dt->dt_sec));
	RTC_WRITE(TPS65950_ID4_REG_MINUTES_REG, TOBCD(dt->dt_min));
	RTC_WRITE(TPS65950_ID4_REG_HOURS_REG, TOBCD(dt->dt_hour));
	RTC_WRITE(TPS65950_ID4_REG_DAYS_REG, TOBCD(dt->dt_day));
	RTC_WRITE(TPS65950_ID4_REG_MONTHS_REG, TOBCD(dt->dt_mon));
	RTC_WRITE(TPS65950_ID4_REG_YEARS_REG, TOBCD(dt->dt_year % 100));
	RTC_WRITE(TPS65950_ID4_REG_WEEKS_REG, TOBCD(dt->dt_wday));
	tps65950_rtc_enable(sc, true);
	iic_release_bus(sc->sc_i2c, 0);

	return error;
}

static void
tps65950_wdog_attach(struct tps65950_softc *sc)
{
	sc->sc_smw.smw_name = device_xname(sc->sc_dev);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = tps65950_wdog_setmode;
	sc->sc_smw.smw_tickle = tps65950_wdog_tickle;
	sc->sc_smw.smw_period = TPS65950_WDOG_DEFAULT_PERIOD;

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error_dev(sc->sc_dev, "couldn't register watchdog\n");

	iic_acquire_bus(sc->sc_i2c, 0);
	tps65950_write_1(sc, TPS65950_ID4_REG_WATCHDOG_CFG, 0);
	iic_release_bus(sc->sc_i2c, 0);
}

static int
tps65950_wdog_setmode(struct sysmon_wdog *smw)
{
	struct tps65950_softc *sc = smw->smw_cookie;
	int error;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		iic_acquire_bus(sc->sc_i2c, 0);
		error = tps65950_write_1(sc, TPS65950_ID4_REG_WATCHDOG_CFG, 0);
		iic_release_bus(sc->sc_i2c, 0);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
			smw->smw_period = TPS65950_WDOG_DEFAULT_PERIOD;
		}
		if (smw->smw_period > 30) {
			error = EINVAL;
		} else {
			error = tps65950_wdog_tickle(smw);
		}
	}
	return error;
}

static int
tps65950_wdog_tickle(struct sysmon_wdog *smw)
{
	struct tps65950_softc *sc = smw->smw_cookie;
	int error;

	iic_acquire_bus(sc->sc_i2c, 0);
	tps65950_write_1(sc, TPS65950_ID4_REG_WATCHDOG_CFG, 0);
	error = tps65950_write_1(sc, TPS65950_ID4_REG_WATCHDOG_CFG,
	    smw->smw_period + 1);
	iic_release_bus(sc->sc_i2c, 0);

	return error;
}
