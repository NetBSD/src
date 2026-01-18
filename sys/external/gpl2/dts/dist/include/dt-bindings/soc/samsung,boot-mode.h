/*	$NetBSD: samsung,boot-mode.h,v 1.1.1.1 2026/01/18 05:21:56 skrll Exp $	*/

/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause) */
/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd.
 * Author: Chanho Park <chanho61.park@samsung.com>
 *
 * Device Tree bindings for Samsung Boot Mode.
 */

#ifndef __DT_BINDINGS_SAMSUNG_BOOT_MODE_H
#define __DT_BINDINGS_SAMSUNG_BOOT_MODE_H

/* Boot mode definitions for Exynos Auto v9 SoC */

#define EXYNOSAUTOV9_BOOT_FASTBOOT	0xfa
#define EXYNOSAUTOV9_BOOT_BOOTLOADER	0xfc
#define EXYNOSAUTOV9_BOOT_RECOVERY	0xff

#endif /* __DT_BINDINGS_SAMSUNG_BOOT_MODE_H */
