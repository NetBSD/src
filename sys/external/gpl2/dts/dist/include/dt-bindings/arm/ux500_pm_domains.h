/*	$NetBSD: ux500_pm_domains.h,v 1.1.1.1.6.2 2017/08/28 17:53:00 skrll Exp $	*/

/*
 * Copyright (C) 2014 Linaro Ltd.
 *
 * Author: Ulf Hansson <ulf.hansson@linaro.org>
 * License terms: GNU General Public License (GPL) version 2
 */
#ifndef _DT_BINDINGS_ARM_UX500_PM_DOMAINS_H
#define _DT_BINDINGS_ARM_UX500_PM_DOMAINS_H

#define DOMAIN_VAPE		0

/* Number of PM domains. */
#define NR_DOMAINS		(DOMAIN_VAPE + 1)

#endif
