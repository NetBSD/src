/* $NetBSD: amlogic_rtcreg.h,v 1.1.20.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_AMLOGIC_RTCREG_H_
#define _ARM_AMLOGIC_RTCREG_H_

#define AO_RTC_REG0			0x00
#define AO_RTC_REG1			0x04
#define AO_RTC_REG2			0x08
#define AO_RTC_REG3			0x0c
#define AO_RTC_REG4			0x10

#define AO_RTC_REG0_STATIC_REG_LSB	__BITS(31,24)
#define AO_RTC_REG0_UNUSED_23		__BIT(23)
#define AO_RTC_REG0_SERIAL_BUSY		__BIT(22)
#define AO_RTC_REG0_UNUSED_21		__BIT(21)
#define AO_RTC_REG0_SCLK_STATIC		__BIT(20)
#define AO_RTC_REG0_UNUSED_19_18	__BITS(19,18)
#define AO_RTC_REG0_SERIAL_START	__BIT(17)
#define AO_RTC_REG0_ONE_SHOT_POLARITY	__BIT(16)
#define AO_RTC_REG0_RESERVED_15_11	__BIT(15,11)
#define AO_RTC_REG0_UNUSED_10_8		__BITS(10,8)
#define AO_RTC_REG0_RESERVED_7_6	__BIT(7,6)
#define AO_RTC_REG0_TEST_MODE		__BIT(5)
#define AO_RTC_REG0_TEST_CLK		__BIT(4)
#define AO_RTC_REG0_TEST_BYPASS		__BIT(3)
#define AO_RTC_REG0_SDI			__BIT(2)
#define AO_RTC_REG0_SEN			__BIT(1)
#define AO_RTC_REG0_SCLK		__BIT(0)

#define AO_RTC_REG1_UNUSED_31_16	__BITS(31,16)
#define AO_RTC_REG1_RESERVED		__BITS(15,12)
#define AO_RTC_REG1_UNUSED_11_4		__BITS(11,4)
#define AO_RTC_REG1_GPO_TO_DIG		__BIT(3)
#define AO_RTC_REG1_GPI_TO_DIG		__BIT(2)
#define AO_RTC_REG1_S_READY		__BIT(1)
#define AO_RTC_REG1_SDO			__BIT(0)

#define AO_RTC_REG2_OSC_CLK_COUNT	__BITS(31,0)

#define AO_RTC_REG3_UNUSED_31_30	__BITS(31,30)
#define AO_RTC_REG3_USE_CLK_TB		__BIT(29)
#define AO_RTC_REG3_USE_NIKE_D_RTC	__BIT(28)
#define AO_RTC_REG3_AUTO_TB_SEL		__BITS(27,26)
#define AO_RTC_REG3_FILTER_SEL		__BITS(25,23)
#define AO_RTC_REG3_FILTER_TB		__BITS(22,21)
#define AO_RTC_REG3_MSR_BUSY		__BIT(20)
#define AO_RTC_REG3_UNUSED_19		__BIT(19)
#define AO_RTC_REG3_FAST_CLK_MODE	__BIT(18)
#define AO_RTC_REG3_COUNT_ALWAYS	__BIT(17)
#define AO_RTC_REG3_MSR_EN		__BIT(16)
#define AO_RTC_REG3_MSR_GATE_TIME	__BIT(15,0)

#define AO_RTC_REG4_UNUSED		__BITS(31,8)
#define AO_RTC_REG4_STATIC_REG_MSB	__BITS(7,0)

/* Define RTC register address mapping */
#define RTC_COUNTER_ADDR		0
#define RTC_GPO_COUNTER_ADDR		1
#define RTC_SEC_ADJUST_ADDR		2
#define RTC_UNUSED_ADDR			3
#define RTC_REGMEM_ADDR0		4
#define RTC_REGMEM_ADDR1		5
#define RTC_REGMEM_ADDR2		6
#define RTC_REGMEM_ADDR3		7

#define RTC_COUNTER_VALUE		__BITS(31,0)
#define RTC_SEC_ADJUST_PENDING		__BIT(25)
#define RTC_SEC_ADJUST_INC		__BIT(24)
#define RTC_SEC_ADJUST_VALID		__BIT(23)
#define RTC_SEC_ADJUST_MONITOR		__BIT(22)
#define RTC_SEC_ADJUST_RESERVED		__BIT(21)
#define RTC_SEC_ADJUST_CTRL		__BITS(20,19)
#define RTC_SEC_ADJUST_COUNTER		__BITS(18,0)

#endif /* _ARM_AMLOGIC_RTCREG_H_ */
