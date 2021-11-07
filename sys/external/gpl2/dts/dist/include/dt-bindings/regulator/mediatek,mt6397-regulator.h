/*	$NetBSD: mediatek,mt6397-regulator.h,v 1.1.1.1 2021/11/07 16:49:57 jmcneill Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _DT_BINDINGS_REGULATOR_MEDIATEK_MT6397_H_
#define _DT_BINDINGS_REGULATOR_MEDIATEK_MT6397_H_

/*
 * Buck mode constants which may be used in devicetree properties (eg.
 * regulator-initial-mode, regulator-allowed-modes).
 * See the manufacturer's datasheet for more information on these modes.
 */

#define MT6397_BUCK_MODE_AUTO		0
#define MT6397_BUCK_MODE_FORCE_PWM	1

#endif
