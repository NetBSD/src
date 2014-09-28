/*	$NetBSD: exynos_io.h,v 1.6 2014/09/28 18:59:43 reinoud Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#ifndef _ARM_SAMSUMG_EXYNOS_IO_H_
#define _ARM_SAMSUNG_EXYNOS_IO_H_

#include "opt_exynos.h"
#include "locators.h"

#include <arm/samsung/exynos_var.h>


struct exyo_locinfo {
	const struct exyo_locators *locators;
	size_t nlocators;
};

extern const struct exyo_locinfo exynos4_locinfo;
extern const struct exyo_locinfo exynos4_i2c_locinfo;
extern const struct exyo_locinfo exynos5_locinfo;
extern const struct exyo_locinfo exynos5_i2c_locinfo;

/* XXXNH needed? */
#define	NOPORT	EXYOCF_PORT_DEFAULT
#define	NOINTR	EXYOCF_INTR_DEFAULT
#define	EANY	EXYO_ALL
#define	REQ	EXYO_REQUIRED

#endif /* _ARM_SAMSUNG_EXYNOS_IO_H_ */
