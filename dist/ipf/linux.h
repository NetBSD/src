/*	$NetBSD: linux.h,v 1.1.1.2 2000/05/03 10:55:51 veego Exp $	*/

/*
 * Copyright (C) 1993-2000 by Darren Reed.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and due credit is given
 * to the original author and the contributors.  The author accepts no
 * responsibility and is not changed in any way.
 *
 * I hate legaleese, don't you ?
 * Id: linux.h,v 2.2 2000/03/13 22:10:25 darrenr Exp
 */

#include <linux/config.h>
#ifdef MODULE
#include <linux/module.h>
#include <linux/version.h>
#endif /* MODULE */

#include "ip_compat.h"
