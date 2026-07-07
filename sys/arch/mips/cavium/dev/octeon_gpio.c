/*	$NetBSD: octeon_gpio.c,v 1.1 2026/07/07 08:14:15 kbowling Exp $	*/
/*	$OpenBSD: octgpio.c,v 1.2 2019/09/29 04:28:52 visa Exp $	*/

/*
 * Copyright (c) 2019 Visa Hankala
 * Copyright (c) 2026 Kevin Bowling <kevin.bowling@kev009.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Driver for OCTEON GPIO controller. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_gpio.c,v 1.1 2026/07/07 08:14:15 kbowling Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/gpio.h>
#include <sys/kmem.h>
#include <sys/mutex.h>

#include <dev/fdt/fdtvar.h>

#include <mips/cpuregs.h>
#include <mips/locore.h>

#include <mips/cavium/dev/octeon_gpioreg.h>

struct octgpio_softc {
	device_t		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
	kmutex_t		 sc_lock;
	uint32_t		 sc_npins;
	uint32_t		 sc_xbit;
	bool			 sc_out_sel;	/* BIT_CFG has OUTPUT_SEL */
};

struct octgpio_pin {
	struct octgpio_softc	*pin_sc;
	uint32_t		 pin_no;
	bool			 pin_actlo;
};

#define GPIO_RD_8(sc, reg) \
	bus_space_read_8((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define GPIO_WR_8(sc, reg, val) \
	bus_space_write_8((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

static int	octgpio_match(device_t, cfdata_t, void *);
static void	octgpio_attach(device_t, device_t, void *);

static void	*octgpio_acquire(device_t, const void *, size_t, int);
static void	 octgpio_release(device_t, void *);
static int	 octgpio_read(device_t, void *, bool);
static void	 octgpio_write(device_t, void *, int, bool);

static void	 octgpio_pin_ctl(struct octgpio_softc *, uint32_t, int);

CFATTACH_DECL_NEW(octgpio, sizeof(struct octgpio_softc),
    octgpio_match, octgpio_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "cavium,octeon-3860-gpio" },
	{ .compat = "cavium,octeon-7890-gpio" },
	DEVICE_COMPAT_EOL
};

static const struct fdtbus_gpio_controller_func octgpio_funcs = {
	.acquire = octgpio_acquire,
	.release = octgpio_release,
	.read = octgpio_read,
	.write = octgpio_write,
};

static int
octgpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
octgpio_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	struct octgpio_softc *sc = device_private(self);
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": could not get registers\n");
		return;
	}
	/*
	 * Device trees give a 0x100 window but the extended pin
	 * configuration registers (GPIO_XBIT_CFG) live at +0x100.
	 */
	if (size < GPIO_SIZE)
		size = GPIO_SIZE;
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error(": could not map registers\n");
		return;
	}

	/*
	 * Set the pin count and the register layout per the SDK CSR
	 * definitions (cvmx-gpio-defs.h).  Pins below sc_xbit are
	 * configured through GPIO_BIT_CFG, the rest through
	 * GPIO_XBIT_CFG.  On CN73XX and CN78XX all pins use the
	 * GPIO_XBIT_CFG range.  The BIT_CFG OUTPUT_SEL output mux
	 * exists on CN70XX and newer only; on CN61XX-class chips the
	 * same bits belong to SYNCE_SEL and must not be touched.
	 */
	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_CN30XX:
	case MIPS_CN31XX:
	case MIPS_CN50XX:
		sc->sc_npins = 24;
		sc->sc_xbit = 16;
		break;
	case MIPS_CN38XX:
	case MIPS_CN52XX:
	case MIPS_CN56XX:
	case MIPS_CN58XX:
	case MIPS_CN63XX:
	case MIPS_CN68XX:
		sc->sc_npins = 16;
		sc->sc_xbit = 16;
		break;
	case MIPS_CN61XX:
	case MIPS_CN66XX:
	case MIPS_CNF71XX:
		sc->sc_npins = 20;
		sc->sc_xbit = 16;
		break;
	case MIPS_CN70XX:
		sc->sc_npins = 20;
		sc->sc_xbit = 16;
		sc->sc_out_sel = true;
		break;
	case MIPS_CN73XX:
	case MIPS_CNF75XX:
		sc->sc_npins = 32;
		sc->sc_xbit = 0;
		sc->sc_out_sel = true;
		break;
	case MIPS_CN78XX:
		sc->sc_npins = 20;
		sc->sc_xbit = 0;
		sc->sc_out_sel = true;
		break;
	default:
		/* Be conservative about unknown models. */
		sc->sc_npins = 16;
		sc->sc_xbit = 16;
		break;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	aprint_naive("\n");
	aprint_normal(": GPIO controller, %u pins\n", sc->sc_npins);

	fdtbus_register_gpio_controller(self, phandle, &octgpio_funcs);
}

static bus_size_t
octgpio_cfg_reg(struct octgpio_softc *sc, uint32_t pin)
{

	if (pin >= sc->sc_xbit)
		return GPIO_XBIT_CFG16_OFFSET + (pin - sc->sc_xbit) * 8;
	return GPIO_BIT_CFG0_OFFSET + pin * 8;
}

static void
octgpio_pin_ctl(struct octgpio_softc *sc, uint32_t pin, int flags)
{
	uint64_t value;
	bus_size_t reg;

	KASSERT(pin < sc->sc_npins);
	reg = octgpio_cfg_reg(sc, pin);

	mutex_enter(&sc->sc_lock);
	value = GPIO_RD_8(sc, reg);
	if (ISSET(flags, GPIO_PIN_OUTPUT)) {
		value |= GPIO_BIT_CFG_TX_OE;
		/* Select normal GPIO output where the mux exists. */
		if (sc->sc_out_sel)
			value &= ~GPIO_BIT_CFG_OUT_SEL;
	} else {
		value &= ~(GPIO_BIT_CFG_TX_OE | GPIO_BIT_CFG_RX_XOR);
	}
	/*
	 * Interrupts are not supported; keep INT_EN clear (on models
	 * whose XBIT_CFG lacks INT_EN the bit reads as zero anyway).
	 */
	value &= ~GPIO_BIT_CFG_INT_EN;
	GPIO_WR_8(sc, reg, value);
	mutex_exit(&sc->sc_lock);
}

static void *
octgpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct octgpio_softc *sc = device_private(dev);
	const u_int *gpio = data;
	struct octgpio_pin *pin;
	u_int pinno;
	bool actlo;

	/* #gpio-cells = <2>: controller phandle, pin, flags. */
	if (len != 12)
		return NULL;

	pinno = be32toh(gpio[1]);
	actlo = (be32toh(gpio[2]) & 1) != 0;
	if (pinno >= sc->sc_npins)
		return NULL;

	octgpio_pin_ctl(sc, pinno, flags);

	pin = kmem_zalloc(sizeof(*pin), KM_SLEEP);
	pin->pin_sc = sc;
	pin->pin_no = pinno;
	pin->pin_actlo = actlo;

	return pin;
}

static void
octgpio_release(device_t dev, void *priv)
{
	struct octgpio_softc *sc = device_private(dev);
	struct octgpio_pin *pin = priv;

	octgpio_pin_ctl(sc, pin->pin_no, GPIO_PIN_INPUT);
	kmem_free(pin, sizeof(*pin));
}

static int
octgpio_read(device_t dev, void *priv, bool raw)
{
	struct octgpio_pin *pin = priv;
	struct octgpio_softc *sc = pin->pin_sc;
	int val;

	val = (GPIO_RD_8(sc, GPIO_RX_DAT_OFFSET) >> pin->pin_no) & 1;
	if (!raw && pin->pin_actlo)
		val = !val;

	return val;
}

static void
octgpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct octgpio_pin *pin = priv;
	struct octgpio_softc *sc = pin->pin_sc;

	if (!raw && pin->pin_actlo)
		val = !val;

	if (val)
		GPIO_WR_8(sc, GPIO_TX_SET_OFFSET, 1ull << pin->pin_no);
	else
		GPIO_WR_8(sc, GPIO_TX_CLR_OFFSET, 1ull << pin->pin_no);
}
