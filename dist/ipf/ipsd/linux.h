/*	$NetBSD: linux.h,v 1.4 2012/02/15 17:55:05 riz Exp $	*/

/*
 * Copyright (C) 1997-1998 by Darren Reed.
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
