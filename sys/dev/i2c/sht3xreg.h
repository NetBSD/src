/*	$NetBSD: sht3xreg.h,v 1.2 2022/04/27 23:11:25 brad Exp $	*/

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

#ifndef _DEV_I2C_SHT3XREG_H_
#define _DEV_I2C_SHT3XREG_H_

#define SHT3X_TYPICAL_ADDR_1	0x44
#define SHT3X_TYPICAL_ADDR_2	0x45


/* Measurement repeatability and clock steatching
 * for single shot measurement command
 */
#define SHT3X_MEASURE_REPEATABILITY_CS_HIGH	0x2C06
#define SHT3X_MEASURE_REPEATABILITY_CS_MEDIUM	0x2C0D
#define SHT3X_MEASURE_REPEATABILITY_CS_LOW	0x2C10
#define SHT3X_MEASURE_REPEATABILITY_NOCS_HIGH	0x2400
#define SHT3X_MEASURE_REPEATABILITY_NOCS_MEDIUM	0x240B
#define SHT3X_MEASURE_REPEATABILITY_NOCS_LOW	0x2416

/* Periodic measurements ... .5 mps, 1 mps, 2 mps, 4 mps
 * and 10 mps at various repeatability.  One sets up the
 * desired mps and repeatability and then calls fetch data
 * to get the data back at the specified period rate
 */
#define SHT3X_HALF_MPS_HIGH		0x2032
#define SHT3X_HALF_MPS_MEDIUM		0x2024
#define SHT3X_HALF_MPS_LOW		0x202F
#define SHT3X_ONE_MPS_HIGH		0x2130
#define SHT3X_ONE_MPS_MEDIUM		0x2126
#define SHT3X_ONE_MPS_LOW		0x212D
#define SHT3X_TWO_MPS_HIGH		0x2236
#define SHT3X_TWO_MPS_MEDIUM		0x2220
#define SHT3X_TWO_MPS_LOW		0x222B
#define SHT3X_FOUR_MPS_HIGH		0x2334
#define SHT3X_FOUR_MPS_MEDIUM		0x2322
#define SHT3X_FOUR_MPS_LOW		0x2329
#define SHT3X_TEN_MPS_HIGH		0x2737
#define SHT3X_TEN_MPS_MEDIUM		0x2721
#define SHT3X_TEN_MPS_LOW		0x272A
#define SHT3X_PERIODIC_FETCH_DATA	0xE000

/* ART, accelerated response time.  A method of getting periodic
 * measurements at 4Hz
 */
#define SHT3X_ART_ENABLE	0x2B32

/* Break command or stop periodic measurement */
#define SHT3X_BREAK	0x3093

/* The heater */
#define SHT3X_HEATER_ENABLE	0x306D
#define SHT3X_HEATER_DISABLE	0x3066

/* status register */
#define SHT3X_GET_STATUS_REGISTER	0xF32D
#define SHT3X_CLEAR_STATUS_REGISTER	0x3041
/* the bits */
#define SHT3X_ALERT_PENDING		0x8000
#define SHT3X_HEATER_STATUS		0x2000
#define SHT3X_RH_TRACKING_ALERT		0x0800
#define SHT3X_TEMP_TRACKING_ALERT	0x0400
#define SHT3X_RESET_DETECTED		0x0010
#define SHT3X_LAST_COMMAND_STATUS	0x0002
#define SHT3X_WRITE_DATA_CHECKSUM	0x0001

/* Alert mode */
/* This is not supported by the sht3xtemp driver as
   the information in the datasheet was not enough to
   get it working.  A read of the registers appears to
   funtion just fine, but writes do not do anything, and
   the chip does not indicate any errors occured.
*/
#define SHT3X_READ_HIGH_ALERT_SET	0xE11F
#define SHT3X_READ_HIGH_ALERT_CLEAR	0xE114
#define SHT3X_READ_LOW_ALERT_SET	0xE102
#define SHT3X_READ_LOW_ALERT_CLEAR	0xE109
#define SHT3X_WRITE_HIGH_ALERT_SET	0x611D
#define SHT3X_WRITE_HIGH_ALERT_CLEAR	0x6116
#define SHT3X_WRITE_LOW_ALERT_SET	0x6100
#define SHT3X_WRITE_LOW_ALERT_CLEAR	0x610B

/* Other commands */
#define SHT3X_SOFT_RESET		0x30A2
/* this is not documented in the datasheet, but is present in a
 * lot of example code
 */
#define SHT3X_READ_SERIAL_NUMBER	0x3780
/* this is also not defined in the datasheet, but is present in some
 * example code.  There are, however, no examples of its use.
 */
#define SHT3X_NO_SLEEP			0x303E

#endif
