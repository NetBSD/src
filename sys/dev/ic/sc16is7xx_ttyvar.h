/*	$NetBSD: sc16is7xx_ttyvar.h,v 1.1 2025/10/24 23:16:11 brad Exp $	*/

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

#ifndef _DEV_IC_SC16IS7XX_TTYVAR_H_
#define _DEV_IC_SC16IS7XX_TTYVAR_H_

#include <sys/tty.h>
#include <sys/bus.h>

#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>	/* for com_regs */

struct sc16is7xx_tty_softc {
	struct com_softc sc_com;

#ifdef __REMOVE
	struct sysctllog *sc_sc16is7xx_tty_log;
#endif
};

#endif
