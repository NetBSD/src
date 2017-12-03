/*	$NetBSD: ux500_pm_domains.h,v 1.1.1.1.8.2 2017/12/03 11:38:34 jdolecek Exp $	*/

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
