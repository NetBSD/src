/*	$NetBSD: sc16is7xxvar.h,v 1.1 2025/10/24 23:16:11 brad Exp $	*/

/*
 * Copyright (c) 2025 Brad Spencer <brad@anduin.eldar.org>
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

#ifndef _DEV_IC_SC16IS7XXVAR_H_
#define _DEV_IC_SC16IS7XXVAR_H_

#include <sys/tty.h>
#include <sys/bus.h>
#include <sys/pool.h>

#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>	/* for com_regs */

#ifndef SC16IS7XX_DEFAULT_FREQUENCY
#define SC16IS7XX_DEFAULT_FREQUENCY 14745600
#endif

#ifndef SC16IS7XX_DEFAULT_POLL
#define SC16IS7XX_DEFAULT_POLL 50
#endif

#define SC16IS7XX_NPINS 8

#define SC16IS7XX_TYPE_STRINGLEN 40

struct sc16is7xx_sc;

struct sc16is7xx_accessfuncs {
	int (*read_reg)(struct sc16is7xx_sc *, uint8_t, int, uint8_t *,
	    size_t);
	int (*write_reg)(struct sc16is7xx_sc *, uint8_t, int, uint8_t *,
	    size_t);
	void (*copy_handles)(struct sc16is7xx_sc *, struct com_regs *);
	     uint8_t(*com_read_1)(struct com_regs *, u_int);
	void (*com_write_1)(struct com_regs *, u_int, uint8_t);
	void (*com_write_multi_1)(struct com_regs *, u_int, const uint8_t *, bus_size_t);
};

struct sc16is7xx_sc {
	device_t sc_dev;
	struct sysctllog *sc_sc16is7xx_log;

	int sc_num_channels;

	const struct sc16is7xx_accessfuncs *sc_funcs;
	const struct sc16is7xx_accessfuncs *sc_com_funcs;

	device_t sc_ttydevchannel[2];
	device_t sc_gpio_dev;
	struct gpio_chipset_tag sc_gpio_gc;
	gpio_pin_t sc_gpio_pins[SC16IS7XX_NPINS];

	int sc_frequency;

	bool sc_thread_run;
	struct lwp *sc_thread;
	int sc_poll;

	int sc_phandle;
	struct workqueue *sc_wq;
	pool_cache_t sc_wk_pool;
	void *sc_ih;
	void *sc_sih;
};

extern const struct device_compatible_entry sc16is7xx_compat_data[];

struct sc16is7xx_tty_attach_args {
	int aa_channel;
};

void sc16is7xx_attach(struct sc16is7xx_sc *);
int sc16is7xx_detach(struct sc16is7xx_sc *, int);

#endif
