/*	$NetBSD: ds28e17iicvar.h,v 1.1 2025/01/23 19:02:42 brad Exp $	*/

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

#ifndef _DEV_I2C_DS28E17IICVAR_H_
#define _DEV_I2C_DS28E17IICVAR_H_

#include <sys/sysctl.h>
#include <sys/types.h>

#include <dev/i2c/i2cvar.h>


struct ds28e17iic_softc {
	device_t			sc_dv;
	void				*sc_onewire;
	u_int64_t			sc_rom;
	int				sc_dying;
	int				sc_ds28e17iicdebug;

	struct i2c_controller		sc_i2c_tag;
	device_t			sc_i2c_dev;

	struct sysctllog		*sc_ds28e17iiclog;
	bool				sc_reportreadnostop;
	bool				sc_reportzerolen;

	int				sc_readycount;
	int				sc_readydelay;
};

#endif
