/* $NetBSD: tegra_carreg.h,v 1.1 2015/04/28 11:15:55 jmcneill Exp $ */

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

#ifndef _ARM_TEGRA_CARREG_H
#define _ARM_TEGRA_CARREG_H

#define CAR_PLLX_BASE_REG	0xe0
#define CAR_PLLX_BASE_ENABLE		__BIT(31)
#define CAR_PLLX_BASE_LOCK_OVERRIDE	__BIT(30)
#define CAR_PLLX_BASE_REF		__BIT(29)
#define CAR_PLLX_BASE_LOCK		__BIT(27)
#define CAR_PLLX_BASE_DIVP		__BITS(23,20)
#define CAR_PLLX_BASE_DIVN		__BITS(15,8)
#define CAR_PLLX_BASE_DIVM		__BITS(7,0)

#define CAR_PLLX_MISC_REG	0xe8


#endif /* _ARM_TEGRA_CARREG_H */
