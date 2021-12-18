/*	$NetBSD: gen8_ppgtt.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2020 Intel Corporation
 */

#ifndef __GEN8_PPGTT_H__
#define __GEN8_PPGTT_H__

struct intel_gt;

struct i915_ppgtt *gen8_ppgtt_create(struct intel_gt *gt);

#endif
