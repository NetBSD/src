/*	$NetBSD: linux.h,v 1.1.1.4 2004/03/28 08:55:47 martti Exp $	*/

/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * I hate legaleese, don't you ?
 * Id: linux.h,v 2.3 2001/06/09 17:09:22 darrenr Exp
 */

#include <linux/config.h>
#ifdef MODULE
#include <linux/module.h>
#include <linux/version.h>
#endif /* MODULE */

#include "ip_compat.h"
