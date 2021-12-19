/*	$NetBSD: intel_llc_types.h,v 1.3 2021/12/19 11:46:38 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2019 Intel Corporation
 */

#ifndef INTEL_LLC_TYPES_H
#define INTEL_LLC_TYPES_H

struct intel_llc {
	int x;	/* XXX only exists because container_of on empty aggregate is an error */
};

#endif /* INTEL_LLC_TYPES_H */
