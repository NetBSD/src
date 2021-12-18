/*	$NetBSD: intel_llc.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2019 Intel Corporation
 */

#ifndef INTEL_LLC_H
#define INTEL_LLC_H

struct intel_llc;

void intel_llc_enable(struct intel_llc *llc);
void intel_llc_disable(struct intel_llc *llc);

#endif /* INTEL_LLC_H */
