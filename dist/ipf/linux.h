/*	$NetBSD: linux.h,v 1.1.1.3 2002/01/24 08:18:30 martti Exp $	*/

/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: linux.h,v 2.2.2.1 2001/06/26 10:43:19 darrenr Exp
 */

#include <linux/config.h>
#ifdef MODULE
#include <linux/module.h>
#include <linux/version.h>
#endif /* MODULE */

#include "ip_compat.h"
