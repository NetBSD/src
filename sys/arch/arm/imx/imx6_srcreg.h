/*	$NetBSD: imx6_srcreg.h,v 1.1 2014/09/25 05:05:28 ryo Exp $	*/

/*
 * Copyright (c) 2014 Ryo Shimizu <ryo@nerv.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IMX6_SRCREG_H_
#define _IMX6_SRCREG_H_

#include <sys/cdefs.h>

/* SRC - System Reset Controller */
#define SRC_SCR					0x00000000
#define  SRC_SCR_DBG_RST_MASK_PG		__BIT(25)
#define  SRC_SCR_CORE3_ENABLE			__BIT(24)
#define  SRC_SCR_CORE2_ENABLE			__BIT(23)
#define  SRC_SCR_CORE1_ENABLE			__BIT(22)
#define  SRC_SCR_CORES_DBG_RST			__BIT(21)
#define  SRC_SCR_CORE3_DBG_RST			__BIT(20)
#define  SRC_SCR_CORE2_DBG_RST			__BIT(19)
#define  SRC_SCR_CORE1_DBG_RST			__BIT(18)
#define  SRC_SCR_CORE0_DBG_RST			__BIT(17)
#define  SRC_SCR_CORE3_RST			__BIT(16)
#define  SRC_SCR_CORE2_RST			__BIT(15)
#define  SRC_SCR_CORE1_RST			__BIT(14)
#define  SRC_SCR_CORE0_RST			__BIT(13)
#define  SRC_SCR_SW_IPU2_RST			__BIT(12)
#define  SRC_SCR_EIM_RST			__BIT(11)
#define  SRC_SCR_MASK_WDOG_RST			__BITS(10, 7)
#define  SRC_SCR_WARM_RST_BYPASS_COUNT		__BITS(6, 5)
#define  SRC_SCR_SW_OPEN_VG_RS			__BIT(4)
#define  SRC_SCR_SW_IPU1_RST			__BIT(3)
#define  SRC_SCR_SW_VPU_RST			__BIT(2)
#define  SRC_SCR_SW_GPU_RST			__BIT(1)
#define  SRC_SCR_WARM_RESET_ENABLE		__BIT(0)
#define SRC_SBMR1				0x00000004
#define SRC_SRSR				0x00000008
#define  SRC_SRSR_WARM_BOOT			__BIT(16)
#define  SRC_SRSR_RESERVED7			__BITS(15, 7)
#define  SRC_SRSR_JTAG_SW_RST			__BIT(6)
#define  SRC_SRSR_JTAG_RST_B			__BIT(5)
#define  SRC_SRSR_WDOG_RST_B			__BIT(4)
#define  SRC_SRSR_IPP_USER_RESET_		__BIT(3)
#define  SRC_SRSR_CSU_RESET_B			__BIT(2)
#define  SRC_SRSR_RESERVED1			__BIT(1)
#define  SRC_SRSR_IPP_RESET_B			__BIT(0)
#define SRC_SISR				0x00000014
#define  SRC_SISR_CORE3_WDOG_RST_REQ		__BIT(8)
#define  SRC_SISR_CORE2_WDOG_RST_REQ		__BIT(7)
#define  SRC_SISR_CORE1_WDOG_RST_REQ		__BIT(6)
#define  SRC_SISR_CORE0_WDOG_RST_REQ		__BIT(5)
#define  SRC_SISR_IPU2_PASSED_RESET		__BIT(4)
#define  SRC_SISR_OPEN_VG_PASSED_RESET		__BIT(3)
#define  SRC_SISR_IPU1_PASSED_RESET		__BIT(2)
#define  SRC_SISR_VPU_PASSED_RESET		__BIT(1)
#define  SRC_SISR_GPU_PASSED_RESET		__BIT(0)
#define SRC_SIMR				0x00000018
#define  SRC_SIMR_MASK_IPU2_PASSED_RESET	_BIT(4)
#define  SRC_SIMR_MASK_OPEN_VG_PASSED_RESET	_BIT(3)
#define  SRC_SIMR_MASK_IPU_PASSED_RESET		_BIT(2)
#define  SRC_SIMR_MASK_VPU_PASSED_RESET		_BIT(1)
#define  SRC_SIMR_MASK_GPU_PASSED_RESET		_BIT(0)
#define SRC_SBMR2				0x0000001c
#define SRC_GPR1				0x00000020 /* core0 entry */
#define SRC_GPR2				0x00000024
#define SRC_GPR3				0x00000028 /* core1 entry */
#define SRC_GPR4				0x0000002c
#define SRC_GPR5				0x00000030 /* core2 entry */
#define SRC_GPR6				0x00000034
#define SRC_GPR7				0x00000038 /* core3 entry */
#define SRC_GPR8				0x0000003c
#define SRC_GPR9				0x00000040
#define SRC_GPR10				0x00000044
#define  SRC_GPR10_CORE3_ERROR_STATUS		__BIT(27)
#define  SRC_GPR10_CORE2_ERROR_STATUS		__BIT(26)
#define  SRC_GPR10_CORE1_ERROR_STATUS		__BIT(25)

#endif /* _IMX6_SRCREG_H_ */
