/*	$NetBSD: intel_dp_link_training.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef __INTEL_DP_LINK_TRAINING_H__
#define __INTEL_DP_LINK_TRAINING_H__

struct intel_dp;

void intel_dp_start_link_train(struct intel_dp *intel_dp);
void intel_dp_stop_link_train(struct intel_dp *intel_dp);

#endif /* __INTEL_DP_LINK_TRAINING_H__ */
