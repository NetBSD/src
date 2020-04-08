/*	$NetBSD: ux500_pm_domains.h,v 1.1.1.1.12.1 2020/04/08 14:08:44 martin Exp $	*/

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2014 Linaro Ltd.
 *
 * Author: Ulf Hansson <ulf.hansson@linaro.org>
 */
#ifndef _DT_BINDINGS_ARM_UX500_PM_DOMAINS_H
#define _DT_BINDINGS_ARM_UX500_PM_DOMAINS_H

#define DOMAIN_VAPE		0

/* Number of PM domains. */
#define NR_DOMAINS		(DOMAIN_VAPE + 1)

#endif
