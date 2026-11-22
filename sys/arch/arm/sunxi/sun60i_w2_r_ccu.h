/* $NetBSD: sun60i_w2_r_ccu.h,v 1.1 2026/11/22 11:47:24 jmcneill Exp $ */

/*-
 * Copyright (c) 2026 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _SUN60I_W2_R_CCU_H
#define _SUN60I_W2_R_CCU_H

#define	W2_R_RST_TIMER			0
#define	W2_R_RST_PWM			1
#define	W2_R_RST_SPI			2
#define	W2_R_RST_MBOX			3
#define	W2_R_RST_UART1			4
#define	W2_R_RST_UART0			5
#define	W2_R_RST_TWI2			6
#define	W2_R_RST_TWI1			7
#define	W2_R_RST_TWI0			8
#define	W2_R_RST_IRRX			9
#define	W2_R_RST_RTC			10
#define	W2_R_RST_RISCV_CFG		11
#define	W2_R_RST_CPUCFG			12
#define	W2_R_RST_MODULE			13

#define	W2_R_CLK_AHB			0
#define	W2_R_CLK_APBS0			1
#define	W2_R_CLK_APBS1			2
#define	W2_R_CLK_TIMER0			3
#define	W2_R_CLK_TIMER1			4
#define	W2_R_CLK_TIMER2			5
#define	W2_R_CLK_TIMER3			6
#define	W2_R_CLK_TIMER			7
#define	W2_R_CLK_TWD			8
#define	W2_R_CLK_BUS_PWM		9
#define	W2_R_CLK_PWM			10
#define	W2_R_CLK_SPI			11
#define	W2_R_CLK_BUS_SPI		12
#define	W2_R_CLK_MBOX			13
#define	W2_R_CLK_UART1			14
#define	W2_R_CLK_UART0			15
#define	W2_R_CLK_TWI2			16
#define	W2_R_CLK_TWI1			17
#define	W2_R_CLK_TWI0			18
#define	W2_R_CLK_PPU			19
#define	W2_R_CLK_TZMA			20
#define	W2_R_CLK_CPUS_BIST		21
#define	W2_R_CLK_IRRX			22
#define	W2_R_CLK_BUS_IRRX		23
#define	W2_R_CLK_RTC			24
#define	W2_R_CLK_RISCV_24M		25
#define	W2_R_CLK_RISCV_CFG		26
#define	W2_R_CLK_RISCV			27
#define	W2_R_CLK_CPUCFG			28
#define	W2_R_CLK_VDD_USB2CPUS		29
#define	W2_R_CLK_VDD_SYS2USB		30
#define	W2_R_CLK_VDD_SYS2CPUS		31
#define	W2_R_CLK_VDD_DDR		32
#define	W2_R_CLK_TT_AUTO		33
#define	W2_R_CLK_CPU_ICACHE_AUTO	34
#define	W2_R_CLK_AHBS_AUTO_CLK		35

#endif /* !_SUN60I_W2_R_CCU_H */
