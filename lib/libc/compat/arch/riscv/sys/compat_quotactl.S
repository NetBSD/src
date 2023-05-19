/*	$NetBSD: compat_quotactl.S,v 1.1 2023/05/19 06:50:07 skrll Exp $	*/

/*
 * This file placed in the public domain.
 * Nick Hudson, 19th May, 2023.
 */

#include "SYS.h"

WARN_REFERENCES(quotactl, \
    "warning: reference to compatibility quotactl(); include <sys/quota.h> for correct reference")

PSEUDO(quotactl,compat_50_quotactl)
