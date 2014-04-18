/* $NetBSD */
/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson.
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

#ifndef _ARM_SAMSUNG_EXYNOS_WDT_REG_H_
#define _ARM_SAMSUNG_EXYNOS_WDT_REG_H_

#define EXYNOS_WDT_WTCON		0x0000
#define  WTCON_PRESCALER		__BITS(15,8)
#define  WTCON_ENABLE			__BIT(5)
#define  WTCON_CLOCK_SELECT		__BITS(4,3)
#define  WTCON_CLOCK_SELECT_16		__SHIFTIN(0, WTCON_CLOCK_SELECT)
#define  WTCON_CLOCK_SELECT_32		__SHIFTIN(1, WTCON_CLOCK_SELECT)
#define  WTCON_CLOCK_SELECT_64		__SHIFTIN(2, WTCON_CLOCK_SELECT)
#define  WTCON_CLOCK_SELECT_128		__SHIFTIN(3, WTCON_CLOCK_SELECT)
#define  WTCON_INT_ENABLE		__BIT(2)
#define  WTCON_RESET_ENABLE		__BIT(0)
#define EXYNOS_WDT_WTDAT		0x0004
#define  WTDAT_RELOAD			__BITS(15,0)
#define EXYNOS_WDT_WTCNT		0x0008
#define  WTCNT_COUNT			__BITS(15,0)
#define EXYNOS_WDT_WTCLRINT		0x000C

#endif /* _ARM_SAMSUNG_EXYNOS_WDT_REG_H_ */

