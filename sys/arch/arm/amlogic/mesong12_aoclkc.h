/* $NetBSD: mesong12_aoclkc.h,v 1.1 2021/01/01 07:21:58 ryo Exp $ */

/*
 * Copyright (c) 2021 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MESONG12_AOCLKC_H
#define _MESONG12_AOCLKC_H

/*
 * RESET IDs.
 *  The values are matched to those in dt-bindings/reset/g12a-aoclkc.h
 */
#define	MESONG12_RESET_AO_IR_IN		0
#define	MESONG12_RESET_AO_UART		1
#define	MESONG12_RESET_AO_I2C_M		2
#define	MESONG12_RESET_AO_I2C_S		3
#define	MESONG12_RESET_AO_SAR_ADC	4
#define	MESONG12_RESET_AO_UART2		5
#define	MESONG12_RESET_AO_IR_OUT	6

/*
 * CLOCK IDs.
 *  The values are matched to those in dt-bindings/clock/g12a-clkc.h
 */
#define	MESONG12_CLOCK_AO_AHB		0
#define	MESONG12_CLOCK_AO_IR_IN		1
#define	MESONG12_CLOCK_AO_I2C_M0	2
#define	MESONG12_CLOCK_AO_I2C_S0	3
#define	MESONG12_CLOCK_AO_UART		4
#define	MESONG12_CLOCK_AO_PROD_I2C	5
#define	MESONG12_CLOCK_AO_UART2		6
#define	MESONG12_CLOCK_AO_IR_OUT	7
#define	MESONG12_CLOCK_AO_SAR_ADC	8
#define	MESONG12_CLOCK_AO_MAILBOX	9
#define	MESONG12_CLOCK_AO_M3		10
#define	MESONG12_CLOCK_AO_AHB_SRAM	11
#define	MESONG12_CLOCK_AO_RTI		12
#define	MESONG12_CLOCK_AO_M4_FCLK	13
#define	MESONG12_CLOCK_AO_M4_HCLK	14
#define	MESONG12_CLOCK_AO_CLK81		15
#define	MESONG12_CLOCK_AO_SAR_ADC_SEL	16
#define	MESONG12_CLOCK_AO_SAR_ADC_CLK	18
#define	MESONG12_CLOCK_AO_CTS_OSCIN	19
#define	MESONG12_CLOCK_AO_32K		23
#define	MESONG12_CLOCK_AO_CEC		27
#define	MESONG12_CLOCK_AO_CTS_RTC_OSCIN	28

#endif /* _MESONG12_AOCLKC_H */
