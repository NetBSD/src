/*	$NetBSD: imxpwmreg.h,v 1.2 2020/05/20 05:10:42 hkenken Exp $	*/

/*
 * Copyright (c) 2014  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_ARM_IMX_IMXPWMREG_H_
#define	_ARM_IMX_IMXPWMREG_H_

#define PWM_CR			0x00	/* PWM Control Register */
#define  PWM_CR_FWM		__BITS(27, 26)
#define  PWM_CR_STOPEN		__BIT(25)
#define  PWM_CR_DOZEN		__BIT(24)
#define  PWM_CR_WAITEN		__BIT(23)
#define  PWM_CR_DBGEN		__BIT(22)
#define  PWM_CR_BCTR		__BIT(21)
#define  PWM_CR_HCTR		__BIT(20)
#define  PWM_CR_POUTC		__BITS(19, 18)
#define  PWM_CR_CLKSRC		__BITS(17, 16)
#define   CLKSRC_IPG_CLK		1
#define   CLKSRC_IPG_CLK_HIGHFREQ	2
#define   CLKSRC_IPG_CLK_32K		3
#define  PWM_CR_PRESCALER	__BITS(15, 4)
#define  PWM_CR_SWR		__BIT(3)
#define  PWM_CR_REPEAT		__BITS(2, 1)
#define  PWM_CR_EN		__BIT(0)
#define PWM_SR			0x04	/* PWM Status Register */
#define  PWM_SR_FWE		__BIT(6)
#define  PWM_SR_CMP		__BIT(5)
#define  PWM_SR_ROV		__BIT(4)
#define  PWM_SR_FE		__BIT(3)
#define  PWM_SR_FIFOAV		__BITS(2, 0)
#define PWM_IR			0x08	/* PWM Interrupt Register */
#define  PWM_IR_CIE		__BIT(2)
#define  PWM_IR_RIE		__BIT(1)
#define  PWM_IR_FIE		__BIT(0)
#define PWM_SAR			0x0C	/* PWM Sample Register */
#define  PWM_SAR_SAMPLE		__BITS(15, 0)
#define PWM_PR			0x10	/* PWM Period Register */
#define  PWM_PR_PERIOD		__BITS(15, 0)
#define PWM_CNR			0x14	/* PWM Counter Register */
#define  PWM_CNR_COUNT		__BITS(15, 0)

#endif	/* _ARM_IMX_IMXPWMREG_H_ */
