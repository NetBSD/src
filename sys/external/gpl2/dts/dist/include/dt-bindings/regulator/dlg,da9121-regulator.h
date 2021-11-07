/*	$NetBSD: dlg,da9121-regulator.h,v 1.1.1.1 2021/11/07 16:49:57 jmcneill Exp $	*/

/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _DT_BINDINGS_REGULATOR_DLG_DA9121_H
#define _DT_BINDINGS_REGULATOR_DLG_DA9121_H

/*
 * These buck mode constants may be used to specify values in device tree
 * properties (e.g. regulator-initial-mode).
 * A description of the following modes is in the manufacturers datasheet.
 */

#define DA9121_BUCK_MODE_FORCE_PFM		0
#define DA9121_BUCK_MODE_FORCE_PWM		1
#define DA9121_BUCK_MODE_FORCE_PWM_SHEDDING	2
#define DA9121_BUCK_MODE_AUTO			3

#define DA9121_BUCK_RIPPLE_CANCEL_NONE		0
#define DA9121_BUCK_RIPPLE_CANCEL_SMALL		1
#define DA9121_BUCK_RIPPLE_CANCEL_MID		2
#define DA9121_BUCK_RIPPLE_CANCEL_LARGE		3

#endif
