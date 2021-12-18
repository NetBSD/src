/*	$NetBSD: selftest_rc6.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2019 Intel Corporation
 */

#ifndef SELFTEST_RC6_H
#define SELFTEST_RC6_H

int live_rc6_ctx_wa(void *arg);
int live_rc6_manual(void *arg);

#endif /* SELFTEST_RC6_H */
