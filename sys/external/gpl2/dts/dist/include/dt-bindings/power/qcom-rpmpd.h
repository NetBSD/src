/*	$NetBSD: qcom-rpmpd.h,v 1.1.1.1 2019/05/25 11:29:13 jmcneill Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018, The Linux Foundation. All rights reserved. */

#ifndef _DT_BINDINGS_POWER_QCOM_RPMPD_H
#define _DT_BINDINGS_POWER_QCOM_RPMPD_H

/* SDM845 Power Domain Indexes */
#define SDM845_EBI	0
#define SDM845_MX	1
#define SDM845_MX_AO	2
#define SDM845_CX	3
#define SDM845_CX_AO	4
#define SDM845_LMX	5
#define SDM845_LCX	6
#define SDM845_GFX	7
#define SDM845_MSS	8

/* SDM845 Power Domain performance levels */
#define RPMH_REGULATOR_LEVEL_RETENTION	16
#define RPMH_REGULATOR_LEVEL_MIN_SVS	48
#define RPMH_REGULATOR_LEVEL_LOW_SVS	64
#define RPMH_REGULATOR_LEVEL_SVS	128
#define RPMH_REGULATOR_LEVEL_SVS_L1	192
#define RPMH_REGULATOR_LEVEL_NOM	256
#define RPMH_REGULATOR_LEVEL_NOM_L1	320
#define RPMH_REGULATOR_LEVEL_NOM_L2	336
#define RPMH_REGULATOR_LEVEL_TURBO	384
#define RPMH_REGULATOR_LEVEL_TURBO_L1	416

/* MSM8996 Power Domain Indexes */
#define MSM8996_VDDCX		0
#define MSM8996_VDDCX_AO	1
#define MSM8996_VDDCX_VFC	2
#define MSM8996_VDDMX		3
#define MSM8996_VDDMX_AO	4
#define MSM8996_VDDSSCX		5
#define MSM8996_VDDSSCX_VFC	6

#endif
