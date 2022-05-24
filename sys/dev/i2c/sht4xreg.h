/*	$NetBSD: sht4xreg.h,v 1.2 2022/05/24 06:28:01 andvar Exp $	*/

/*
 * Copyright (c) 2021 Brad Spencer <brad@anduin.eldar.org>
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

#ifndef _DEV_I2C_SHT4XREG_H_
#define _DEV_I2C_SHT4XREG_H_

#define SHT4X_TYPICAL_ADDR	0x44

#define SHT4X_READ_SERIAL 0x89
#define SHT4X_SOFT_RESET 0x94

/* If you do not use the heater, you can take measurements at a couple
   of different precisions */
#define SHT4X_MEASURE_HIGH_PRECISION 0xFD
#define SHT4X_MEASURE_MEDIUM_PRECISION 0xF6
#define SHT4X_MEASURE_LOW_PRECISION 0xE0

/* The SHT4X chip only support the heater when reading with the
   highest precision and then only when the measurement is happening.
   You can have the heater on for 1 second or 1 tenth of a second.
   After the measurement the heater will switch itself off */
#define SHT4X_MEASURE_HIGH_PRECISION_HIGH_HEAT_1_S 0x39
#define SHT4X_MEASURE_HIGH_PRECISION_HIGH_HEAT_TENTH_S 0x32
#define SHT4X_MEASURE_HIGH_PRECISION_MEDIUM_HEAT_1_S 0x2F
#define SHT4X_MEASURE_HIGH_PRECISION_MEDIUM_HEAT_TENTH_S 0x24
#define SHT4X_MEASURE_HIGH_PRECISION_LOW_HEAT_1_S 0x1E
#define SHT4X_MEASURE_HIGH_PRECISION_LOW_HEAT_TENTH_S 0x15

#endif
