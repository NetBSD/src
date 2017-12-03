/*	$NetBSD: smc.h,v 1.1.10.3 2017/12/03 11:35:56 jdolecek Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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


#ifndef _ARM_SAMSUNG_SMC_H_
#define _ARM_SAMSUNG_SMC_H_

/* provided by Samsung for the Exynos SMC calls */

#define SMC_CMD_INIT		(-1)
#define SMC_CMD_INFO		(-2)
/* For Power Management */
#define SMC_CMD_SLEEP		(-3)
#define SMC_CMD_CPU1BOOT	(-4)
#define SMC_CMD_CPU0AFTR	(-5)
/* For CP15 Access */
#define SMC_CMD_C15RESUME	(-11)
/* For L2 Cache Access */
#define SMC_CMD_L2X0CTRL	(-21)
#define SMC_CMD_L2X0SETUP1	(-22)
#define SMC_CMD_L2X0SETUP2	(-23)
#define SMC_CMD_L2X0INVALL	(-24)
#define SMC_CMD_L2X0DEBUG	(-25)


/* XXX status of the following SMC is not known */
/* For Accessing CP15/SFR (General) */
#define SMC_CMD_REG             (-101)

/* MACRO for SMC_CMD_REG */
#define SMC_REG_CLASS_CP15      (0x0 << 30)
#define SMC_REG_CLASS_SFR_W     (0x1 << 30)
#define SMC_REG_CLASS_SFR_R     (0x3 << 30)
#define SMC_REG_CLASS_MASK      (0x3 << 30)
#define SMC_REG_ID_CP15(CRn, Op1, CRm, Op2) \
           (SMC_REG_CLASS_CP15 | \
            ((CRn) << 10) | ((Op1) << 7) | ((CRm) << 3) | (Op2))
#define SMC_REG_ID_SFR_W(ADDR)  (SMC_REG_CLASS_SFR_W | ((ADDR) >> 2))
#define SMC_REG_ID_SFR_R(ADDR)  (SMC_REG_CLASS_SFR_R | ((ADDR) >> 2))


extern void exynos_smc(uint32_t cmd, uint32_t arg1, uint32_t arg2, uint32_t arg3);

#endif	/* _ARM_SAMSUNG_SMC_H_ */

