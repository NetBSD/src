/*	$NetBSD: selftest_llc.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2019 Intel Corporation
 */

#ifndef SELFTEST_LLC_H
#define SELFTEST_LLC_H

struct intel_llc;

int st_llc_verify(struct intel_llc *llc);

#endif /* SELFTEST_LLC_H */
