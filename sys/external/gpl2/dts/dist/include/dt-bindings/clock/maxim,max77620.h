/*	$NetBSD: maxim,max77620.h,v 1.1.1.1 2017/06/15 20:14:23 jmcneill Exp $	*/

/*
 * Copyright (C) 2016 NVIDIA CORPORATION. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Device Tree binding constants clocks for the Maxim 77620 PMIC.
 */

#ifndef _DT_BINDINGS_CLOCK_MAXIM_MAX77620_CLOCK_H
#define _DT_BINDINGS_CLOCK_MAXIM_MAX77620_CLOCK_H

/* Fixed rate clocks. */

#define MAX77620_CLK_32K_OUT0		0

/* Total number of clocks. */
#define MAX77620_CLKS_NUM		(MAX77620_CLK_32K_OUT0 + 1)

#endif /* _DT_BINDINGS_CLOCK_MAXIM_MAX77620_CLOCK_H */
