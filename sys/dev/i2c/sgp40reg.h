/*	$NetBSD: sgp40reg.h,v 1.1 2021/10/14 13:54:46 brad Exp $	*/

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

#ifndef _DEV_I2C_SGP40REG_H_
#define _DEV_I2C_SGP40REG_H_

#define SGP40_TYPICAL_ADDR	0x59

/* There are three documented commands for this chip, plus one that is called a
   soft reset.  But really the soft reset is a general reset of all devices on
   the bus, hence it is not as usable as one would hope
 */

#define SGP40_MEASURE_RAW 0x260f
#define SGP40_MEASURE_TEST 0x280e
#define SGP40_HEATER_OFF 0x3615
/* The get serial number command is documented in version 1.1 of the
   datasheet.
*/
#define SGP40_GET_SERIAL_NUMBER 0x3682
/* The get featureset command is not documented in any datasheet that could
   be found.  However, it is present and used in a lot of example code.
   It has no additional arguments aside from the command itself, and returns
   a uint16_t for the data and a uint8_t for the crc.
*/
#define SGP40_GET_FEATURESET 0x202f

/* The results of a self test are documented as being either everything is ok, or
   something failed.
 */

#define SGP40_TEST_RESULTS_ALL_PASSED 0xd400
#define SGP40_TEST_RESULTS_SOME_FAILED 0x4b00

#endif
