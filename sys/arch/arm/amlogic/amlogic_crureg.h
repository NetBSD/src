/* $NetBSD: amlogic_crureg.h,v 1.1 2015/02/27 19:57:10 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _ARM_AMLOGIC_CRUREG_H
#define _ARM_AMLOGIC_CRUREG_H

#define CBUS_REG(n)	((n) << 2)

#define HHI_SYS_CPU_CLK_CNTL1_REG	CBUS_REG(0x1057)
#define HHI_SYS_CPU_CLK_CNTL1_SDIV	__BITS(29,20)

#define HHI_SYS_CPU_CLK_CNTL0_REG	CBUS_REG(0x1067)
#define HHI_SYS_CPU_CLK_CNTL0_CLKSEL	__BIT(7)
#define HHI_SYS_CPU_CLK_CNTL0_SOUTSEL	__BITS(3,2)
#define HHI_SYS_CPU_CLK_CNTL0_PLLSEL	__BITS(1,0)

#define HHI_SYS_PLL_CNTL_REG		CBUS_REG(0x10c0)
#define HHI_SYS_PLL_CNTL_MUL		__BITS(8,0)
#define HHI_SYS_PLL_CNTL_DIV		__BITS(14,9)
#define HHI_SYS_PLL_CNTL_OD		__BITS(17,16)

#define PREG_CTLREG0_ADDR_REG		CBUS_REG(0x2000)
#define PREG_CTLREG0_ADDR_CLKRATE	__BITS(9,4)

#endif /* _ARM_AMLOGIC_CRUREG_H */
