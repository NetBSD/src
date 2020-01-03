/*	$NetBSD: i2c.h,v 1.1.1.2 2020/01/03 14:33:03 skrll Exp $	*/

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This header provides constants for I2C bindings
 *
 * Copyright (C) 2015 by Sang Engineering
 * Copyright (C) 2015 by Renesas Electronics Corporation
 *
 * Wolfram Sang <wsa@sang-engineering.com>
 */

#ifndef _DT_BINDINGS_I2C_I2C_H
#define _DT_BINDINGS_I2C_I2C_H

#define I2C_TEN_BIT_ADDRESS	(1 << 31)
#define I2C_OWN_SLAVE_ADDRESS	(1 << 30)

#endif
