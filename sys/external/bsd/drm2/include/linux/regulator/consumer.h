/*	$NetBSD: consumer.h,v 1.4 2018/08/27 15:29:54 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef	_LINUX_REGULATOR_CONSUMER_H_
#define	_LINUX_REGULATOR_CONSUMER_H_

#ifdef _KERNEL_OPT
#include "opt_fdt.h"
#endif

#ifdef FDT

#include <dev/fdt/fdtvar.h>

struct regulator {
	struct fdtbus_regulator	regulator;
};

static inline int
regulator_get_voltage(struct regulator *reg)
{
	unsigned uvolt;
	int error;

	error = fdtbus_regulator_get_voltage(&reg->regulator, &uvolt);
	if (error) {
		/* XXX errno NetBSD->Linux */
		KASSERTMSG(error > 0, "negative error: %d", error);
		return -error;
	}

	KASSERTMSG(uvolt <= INT_MAX, "high voltage: %u uV", uvolt);
	return (int)uvolt;
}

static inline int
regulator_set_voltage(struct regulator *reg, int min_uvolt, int max_uvolt)
{

	if (min_uvolt < 0 || max_uvolt < 0)
		return -EINVAL;

	/* XXX errno NetBSD->Linux */
	return -fdtbus_regulator_set_voltage(&reg->regulator, min_uvolt,
	    max_uvolt);
}

#else

struct regulator;

static inline int
regulator_get_voltage(struct regulator *reg)
{
	panic("no voltage regulators here");
}

static inline int
regulator_set_voltage(struct regulator *reg, int min_uvolt, int max_uvolt)
{
	panic("no voltage regulators here");
}


#endif

#endif	/* _LINUX_REGULATOR_CONSUMER_H_ */
