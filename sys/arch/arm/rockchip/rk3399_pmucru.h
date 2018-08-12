/* $NetBSD: rk3399_pmucru.h,v 1.1 2018/08/12 16:48:05 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _RK3399_PMUCRU_H
#define	_RK3399_PMUCRU_H

/*
 * Clocks
 */

#define	RK3399_PLL_PPLL			1
#define	RK3399_SCLK_32K_SUSPEND_PMU	2
#define	RK3399_SCLK_SPI3_PMU		3
#define	RK3399_SCLK_TIMER12_PMU		4
#define	RK3399_SCLK_TIMER13_PMU		5
#define	RK3399_SCLK_UART4_PMU		6
#define	RK3399_SCLK_PVTM_PMU		7
#define	RK3399_SCLK_WIFI_PMU		8
#define	RK3399_SCLK_I2C0_PMU		9
#define	RK3399_SCLK_I2C4_PMU		10
#define	RK3399_SCLK_I2C8_PMU		11
#define	RK3399_PCLK_SRC_PMU		19
#define	RK3399_PCLK_PMU			20
#define	RK3399_PCLK_PMUGRF_PMU		21
#define	RK3399_PCLK_INTMEM1_PMU		22
#define	RK3399_PCLK_GPIO0_PMU		23
#define	RK3399_PCLK_GPIO1_PMU		24
#define	RK3399_PCLK_SGRF_PMU		25
#define	RK3399_PCLK_NOC_PMU		26
#define	RK3399_PCLK_I2C0_PMU		27
#define	RK3399_PCLK_I2C4_PMU		28
#define	RK3399_PCLK_I2C8_PMU		29
#define	RK3399_PCLK_RKPWM_PMU		30
#define	RK3399_PCLK_SPI3_PMU		31
#define	RK3399_PCLK_TIMER_PMU		32
#define	RK3399_PCLK_MAILBOX_PMU		33
#define	RK3399_PCLK_UART4_PMU		34
#define	RK3399_PCLK_WDT_M0_PMU		35
#define	RK3399_FCLK_CM0S_SRC_PMU	44
#define	RK3399_FCLK_CM0S_PMU		45
#define	RK3399_SCLK_CM0S_PMU		46
#define	RK3399_HCLK_CM0S_PMU		47
#define	RK3399_DCLK_CM0S_PMU		48
#define	RK3399_PCLK_INTR_ARB_PMU	49
#define	RK3399_HCLK_NOC_PMU		50

#endif /* !_RK3399_PMUCRU_H */
