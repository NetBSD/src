/*	$NetBSD: mediatek,mt6360-regulator.h,v 1.1.1.1 2021/11/07 16:49:57 jmcneill Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __DT_BINDINGS_MEDIATEK_MT6360_REGULATOR_H__
#define __DT_BINDINGS_MEDIATEK_MT6360_REGULATOR_H__

/*
 * BUCK/LDO mode constants which may be used in devicetree properties
 * (eg. regulator-allowed-modes).
 * See the manufacturer's datasheet for more information on these modes.
 */

#define MT6360_OPMODE_LP		2
#define MT6360_OPMODE_ULP		3
#define MT6360_OPMODE_NORMAL		0

#endif
