/*	$NetBSD: debugfs_gt_pm.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef DEBUGFS_GT_PM_H
#define DEBUGFS_GT_PM_H

struct intel_gt;
struct dentry;

void debugfs_gt_pm_register(struct intel_gt *gt, struct dentry *root);

#endif /* DEBUGFS_GT_PM_H */
