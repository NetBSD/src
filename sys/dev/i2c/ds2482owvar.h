/*	$NetBSD: ds2482owvar.h,v 1.1 2024/11/04 20:43:38 brad Exp $	*/

/*
 * Copyright (c) 2024 Brad Spencer <brad@anduin.eldar.org>
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

#ifndef _DEV_I2C_DS2482VAR_H_
#define _DEV_I2C_DS2482VAR_H_

#define DS2482_NUM_INSTANCES	8

struct ds2482ow_sc;

struct ds2482_instance {
	int				sc_i_channel;
	struct onewire_bus		sc_i_ow_bus;
	device_t			sc_i_ow_dev;
	struct ds2482ow_sc		*sc;
};

struct ds2482ow_sc {
	int 		sc_ds2482debug;
	device_t 	sc_dev;
	i2c_tag_t 	sc_tag;
	i2c_addr_t 	sc_addr;
	kmutex_t 	sc_mutex;
	struct sysctllog *sc_ds2482log;
	bool		sc_activepullup;
	bool		sc_strongpullup;
	bool		sc_is_800;
	struct		ds2482_instance	sc_instances[DS2482_NUM_INSTANCES];
};


#endif
