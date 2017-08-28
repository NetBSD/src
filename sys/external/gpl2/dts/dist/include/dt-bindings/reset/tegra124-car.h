/*	$NetBSD: tegra124-car.h,v 1.1.1.1.6.2 2017/08/28 17:53:04 skrll Exp $	*/

/*
 * This header provides Tegra124-specific constants for binding
 * nvidia,tegra124-car.
 */

#ifndef _DT_BINDINGS_RESET_TEGRA124_CAR_H
#define _DT_BINDINGS_RESET_TEGRA124_CAR_H

#define TEGRA124_RESET(x)		(6 * 32 + (x))
#define TEGRA124_RST_DFLL_DVCO		TEGRA124_RESET(0)

#endif	/* _DT_BINDINGS_RESET_TEGRA124_CAR_H */
