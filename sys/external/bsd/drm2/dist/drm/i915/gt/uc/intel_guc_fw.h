/*	$NetBSD: intel_guc_fw.h,v 1.1.1.1 2021/12/18 20:15:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2017-2019 Intel Corporation
 */

#ifndef _INTEL_GUC_FW_H_
#define _INTEL_GUC_FW_H_

struct intel_guc;

void intel_guc_fw_init_early(struct intel_guc *guc);
int intel_guc_fw_upload(struct intel_guc *guc);

#endif
