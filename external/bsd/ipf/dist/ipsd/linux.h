/*	$NetBSD: linux.h,v 1.1.1.1 2012/03/23 21:20:06 christos Exp $	*/

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
