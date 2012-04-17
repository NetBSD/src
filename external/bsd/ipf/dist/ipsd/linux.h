/*	$NetBSD: linux.h,v 1.1.1.1.2.2 2012/04/17 00:03:14 yamt Exp $	*/

/*
 * Copyright (C) 2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * @(#)linux.h	1.1 8/19/95
 */

#include <linux/config.h>
#ifdef MODULE
#include <linux/module.h>
#include <linux/version.h>
#endif /* MODULE */

#include "ip_compat.h"
