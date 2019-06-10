/*	$NetBSD: rk3188-power.h,v 1.1.1.1.2.2 2019/06/10 22:08:57 christos Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __DT_BINDINGS_POWER_RK3188_POWER_H__
#define __DT_BINDINGS_POWER_RK3188_POWER_H__

/* VD_CORE */
#define RK3188_PD_A9_0		0
#define RK3188_PD_A9_1		1
#define RK3188_PD_A9_2		2
#define RK3188_PD_A9_3		3
#define RK3188_PD_DBG		4
#define RK3188_PD_SCU		5

/* VD_LOGIC */
#define RK3188_PD_VIDEO		6
#define RK3188_PD_VIO		7
#define RK3188_PD_GPU		8
#define RK3188_PD_PERI		9
#define RK3188_PD_CPU		10
#define RK3188_PD_ALIVE		11

/* VD_PMU */
#define RK3188_PD_RTC		12

#endif
