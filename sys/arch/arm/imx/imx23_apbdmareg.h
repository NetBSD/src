/* $Id: imx23_apbdmareg.h,v 1.2.14.1 2015/04/06 15:17:52 skrll Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_IMX_IMX23_APBDMAREG_H_
#define _ARM_IMX_IMX23_APBDMAREG_H_

#include <sys/cdefs.h>

/*
 * Common register definitions for both APBH and APBX.
 */

/*
 * AHB to APB{H,X} Bridge Control and Status Register 0.
 */
#define HW_APB_CTRL0		0x000
#define HW_APB_CTRL0_SET	0x004
#define HW_APB_CTRL0_CLR	0x008
#define HW_APB_CTRL0_TOG	0x00C

#define HW_APB_CTRL0_SFTRST	__BIT(31)
#define HW_APB_CTRL0_CLKGATE	__BIT(30)
#define HW_APB_CTRL0_RSVD0	__BITS(29, 0)

/*
 * AHB to APB{H,X} Bridge Control Register 1.
 */
#define HW_APB_CTRL1		0x010
#define HW_APB_CTRL1_SET	0x014
#define HW_APB_CTRL1_CLR	0x018
#define HW_APB_CTRL1_TOG	0x01C

/*
 * AHB to APB{H,X} Bridge Control and Status Register 2.
 */
#define HW_APB_CTRL2		0x020
#define HW_APB_CTRL2_SET	0x024
#define HW_APB_CTRL2_CLR	0x028
#define HW_APB_CTRL2_TOG	0x02C

#define HW_APBX_CHANNEL_CTRL 0x30
#define HW_APBX_CHANNEL_CTRL_SET 0x34
#endif /* !_ARM_IMX_IMX23_APBDMAREG_H_ */
