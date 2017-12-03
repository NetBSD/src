/*	$NetBSD: tegra124-car.h,v 1.1.1.2.2.2 2017/12/03 11:38:40 jdolecek Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This header provides Tegra124-specific constants for binding
 * nvidia,tegra124-car.
 */

#ifndef _DT_BINDINGS_RESET_TEGRA124_CAR_H
#define _DT_BINDINGS_RESET_TEGRA124_CAR_H

#define TEGRA124_RESET(x)		(6 * 32 + (x))
#define TEGRA124_RST_DFLL_DVCO		TEGRA124_RESET(0)

#endif	/* _DT_BINDINGS_RESET_TEGRA124_CAR_H */
