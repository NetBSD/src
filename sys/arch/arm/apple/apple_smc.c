/*	$NetBSD: apple_smc.c,v 1.1 2022/05/10 08:09:57 skrll Exp $	*/
/*	$OpenBSD: apple_smc.c,v 1.11 2022/03/25 15:52:03 kettenis Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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

/*
 * Copyright (c) 2021 Mark Kettenis <kettenis@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
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

#include <sys/param.h>

#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

#include <arm/apple/apple_rtkit.h>

extern void (*cpuresetfn)(void);
extern void (*powerdownfn)(void);

/* SMC mailbox endpoint */
#define SMC_EP			32

/* SMC commands */
#define SMC_READ_KEY		0x10
#define SMC_WRITE_KEY		0x11
#define SMC_GET_KEY_BY_INDEX	0x12
#define SMC_GET_KEY_INFO	0x13
#define SMC_GET_SRAM_ADDR	0x17
#define  SMC_SRAM_SIZE		0x4000

#define SMC_DATA		__BITS(63, 32)
#define SMC_WLEN		__BITS(31, 24)
#define SMC_LENGTH		__BITS(23, 16)
#define SMC_ID			__BITS(15, 12)
#define SMC_CMD			__BITS(7, 0)

/* SMC errors */
#define SMC_ERROR_MASK		__BITS(7, 0)
#define SMC_ERROR(d)		__SHIFTOUT((d), SMC_ERROR_MASK)
#define SMC_OK			0x00
#define SMC_KEYNOTFOUND		0x84

/* SMC keys */
#define SMC_KEY(s)	((s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3])

struct smc_key_info {
	uint8_t		size;
	uint8_t		type[4];
	uint8_t		flags;
};

/* SMC GPIO commands */
#define SMC_GPIO_CMD_OUTPUT	(0x01 << 24)

/* RTC related constants */
#define RTC_OFFSET_LEN		6
#define SMC_CLKM_LEN		6

#define APLSMC_BE		__BIT(0)
#define APLSMC_HIDDEN		__BIT(1)

#define APLSMC_MAX_SENSORS	19

struct apple_smc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_space_handle_t	sc_sram_bsh;

	struct rtkit_state	*sc_rs;
	uint8_t			sc_msgid;
	uint64_t		sc_data;

	kmutex_t                sc_mutex;
	kcondvar_t		sc_cv;
	bool			sc_done;

};

struct apple_smc_gpio_pin {
	u_int		pin_no;
	u_int		pin_flags;
	bool		pin_actlo;
};

struct apple_smc_softc *apple_smc_sc;


void	apple_smc_callback(void *, uint64_t);
int	apple_smc_send_cmd(struct apple_smc_softc *, uint8_t, uint32_t, uint16_t);
int	apple_smc_wait_cmd(struct apple_smc_softc *sc);
int	apple_smc_read_key(struct apple_smc_softc *, uint32_t, void *, size_t);


static const struct device_compatible_entry compat_data[] = {
	{ .compat = "apple,smc" },
	DEVICE_COMPAT_EOL
};

void
apple_smc_callback(void *arg, uint64_t data)
{
	struct apple_smc_softc * const sc = arg;

	mutex_enter(&sc->sc_mutex);
	sc->sc_data = data;
	sc->sc_done = true;
	cv_signal(&sc->sc_cv);
	mutex_exit(&sc->sc_mutex);
}

int
apple_smc_send_cmd(struct apple_smc_softc *sc, uint8_t cmd, uint32_t key,
    uint16_t len)
{
	uint64_t data =
	    __SHIFTIN(key, SMC_DATA) |
	    __SHIFTIN(cmd, SMC_CMD) |
	    __SHIFTIN(len, SMC_LENGTH) |
	    __SHIFTIN((sc->sc_msgid++ & 0xf), SMC_ID)
	    ;

	return rtkit_send_endpoint(sc->sc_rs, SMC_EP, data);
}

int
apple_smc_wait_cmd(struct apple_smc_softc *sc)
{
	int error;
	if (cold) {
		int timo;

		/* Poll for completion. */
		for (timo = 1000; timo > 0; timo--) {
			error = rtkit_poll(sc->sc_rs);
			if (error == 0)
				return 0;
			delay(10);
		}

		return EWOULDBLOCK;
	}

	mutex_enter(&sc->sc_mutex);
	sc->sc_done = false;
	while (!sc->sc_done) {
		error = cv_timedwait(&sc->sc_cv, &sc->sc_mutex, hz / 10);
		if (error)
			break;
	}
	mutex_exit(&sc->sc_mutex);

	return error;
}

int
apple_smc_read_key(struct apple_smc_softc *sc, uint32_t key, void *data, size_t len)
{
	int error;

	apple_smc_send_cmd(sc, SMC_READ_KEY, key, len);
	error = apple_smc_wait_cmd(sc);
	if (error)
		return error;
	switch (SMC_ERROR(sc->sc_data)) {
	case SMC_OK:
		break;
	case SMC_KEYNOTFOUND:
		return EINVAL;
		break;
	default:
		return EIO;
		break;
	}

	len = MIN(len, (sc->sc_data >> 16) & 0xffff);
	if (len > sizeof(uint32_t)) {
		bus_space_read_region_1(sc->sc_bst, sc->sc_sram_bsh, 0,
		    data, len);
	} else {
		uint32_t tmp = (sc->sc_data >> 32);
		memcpy(data, &tmp, len);
	}

	return 0;
}

static int
apple_smc_write_key(struct apple_smc_softc *sc, uint32_t key, void *data, size_t len)
{
	bus_space_write_region_1(sc->sc_bst, sc->sc_sram_bsh, 0, data, len);
	bus_space_barrier(sc->sc_bst, sc->sc_sram_bsh, 0, len,
	    BUS_SPACE_BARRIER_WRITE);
	apple_smc_send_cmd(sc, SMC_WRITE_KEY, key, len);

	return apple_smc_wait_cmd(sc);
}

static void *
apple_smc_gpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct apple_smc_gpio_pin *pin;
	const u_int *gpio = data;

	if (len != 12)
		return NULL;

	const u_int pinno = be32toh(gpio[1]);
	const bool actlo = be32toh(gpio[2]) & 1;

	if (pinno >= 256)
		return NULL;

	pin = kmem_alloc(sizeof(*pin), KM_SLEEP);
	pin->pin_no = pinno;
	pin->pin_flags = flags;
	pin->pin_actlo = actlo;

	return pin;
}


static void
apple_smc_gpio_release(device_t dev, void *priv)
{
	struct apple_smc_gpio_pin *pin = priv;

	kmem_free(pin, sizeof(*pin));
}

static void
apple_smc_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct apple_smc_softc * const sc = device_private(dev);
	struct apple_smc_gpio_pin *pin = priv;
	const u_int pn = pin->pin_no;
	static const char *digits = "0123456789abcdef";
	uint32_t key = SMC_KEY("gP\0\0");
	uint32_t data;

	key |= __SHIFTIN(digits[__SHIFTOUT(pn, __BITS(3, 0))], __BITS(7, 0));
	key |= __SHIFTIN(digits[__SHIFTOUT(pn, __BITS(7, 4))], __BITS(15, 8));

	if (pin->pin_actlo)
		val = !val;
	data = SMC_GPIO_CMD_OUTPUT | !!val;

	apple_smc_write_key(sc, key, &data, sizeof(data));
}

static int
apple_smc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}


static struct fdtbus_gpio_controller_func apple_smc_gpio_funcs = {
	.acquire = apple_smc_gpio_acquire,
	.release = apple_smc_gpio_release,
//	.read = apple_smc_gpio_read,
	.write = apple_smc_gpio_write
};


static void
apple_smc_attach(device_t parent, device_t self, void *aux)
{
	struct apple_smc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_cv, "applesmc");

	sc->sc_rs = rtkit_init(phandle, NULL);
	if (sc->sc_rs == NULL) {
		aprint_error(": can't map mailbox channel\n");
		return;
	}

	error = rtkit_boot(sc->sc_rs);
	if (error) {
		aprint_error(": can't boot firmware\n");
		return;
	}

	error = rtkit_start_endpoint(sc->sc_rs, SMC_EP, apple_smc_callback, sc);
	if (error) {
		aprint_error(": can't start SMC endpoint\n");
		return;
	}

	apple_smc_send_cmd(sc, SMC_GET_SRAM_ADDR, 0, 0);
	error = apple_smc_wait_cmd(sc);
	if (error) {
		aprint_error(": can't get SRAM address\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, sc->sc_data, SMC_SRAM_SIZE, 0,
	    &sc->sc_sram_bsh)) {
		aprint_error(": can't map SRAM\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Apple SMC\n");

	apple_smc_sc = sc;

	const int gpio = of_find_firstchild_byname(phandle, "gpio");
	if (gpio > 0) {
		fdtbus_register_gpio_controller(self, gpio,
		    &apple_smc_gpio_funcs);
	}
}


CFATTACH_DECL_NEW(apple_rtkitsmc, sizeof(struct apple_smc_softc),
    apple_smc_match, apple_smc_attach, NULL, NULL);
