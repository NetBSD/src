/*	$NetBSD: stm32fx-clock.h,v 1.1.1.1.6.2 2017/08/28 17:53:01 skrll Exp $	*/

/*
 * stm32fx-clock.h
 *
 * Copyright (C) 2016 STMicroelectronics
 * Author: Gabriel Fernandez for STMicroelectronics.
 * License terms:  GNU General Public License (GPL), version 2
 */

/*
 * List of clocks wich are not derived from system clock (SYSCLOCK)
 *
 * The index of these clocks is the secondary index of DT bindings
 * (see Documentatoin/devicetree/bindings/clock/st,stm32-rcc.txt)
 *
 * e.g:
	<assigned-clocks = <&rcc 1 CLK_LSE>;
*/

#ifndef _DT_BINDINGS_CLK_STMFX_H
#define _DT_BINDINGS_CLK_STMFX_H

#define SYSTICK			0
#define FCLK			1
#define CLK_LSI			2
#define CLK_LSE			3
#define CLK_HSE_RTC		4
#define CLK_RTC			5
#define PLL_VCO_I2S		6
#define PLL_VCO_SAI		7
#define CLK_LCD			8
#define CLK_I2S			9
#define CLK_SAI1		10
#define CLK_SAI2		11
#define CLK_I2SQ_PDIV		12
#define CLK_SAIQ_PDIV		13

#define END_PRIMARY_CLK		14

#define CLK_HSI			14
#define CLK_SYSCLK		15
#define CLK_HDMI_CEC		16
#define CLK_SPDIF		17
#define CLK_USART1		18
#define CLK_USART2		19
#define CLK_USART3		20
#define CLK_UART4		21
#define CLK_UART5		22
#define CLK_USART6		23
#define CLK_UART7		24
#define CLK_UART8		25
#define CLK_I2C1		26
#define CLK_I2C2		27
#define CLK_I2C3		28
#define CLK_I2C4		29
#define CLK_LPTIMER		30

#define END_PRIMARY_CLK_F7	31

#endif
