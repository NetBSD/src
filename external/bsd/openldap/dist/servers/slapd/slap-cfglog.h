/*	$NetBSD: slap-cfglog.h,v 1.1.1.1 2025/09/05 21:09:46 christos Exp $	*/

/* slap-cfglog.h - logging configuration */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2024 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef CFGLOG_H
#define CFGLOG_H

enum {
	CFG_LOGLEVEL = 1,
	CFG_LOGFILE,
	CFG_LOGFILE_ROTATE,
	CFG_LOGFILE_ONLY,
	CFG_LOGFILE_FORMAT
};

extern ConfigDriver config_logging;

#endif /* CFGLOG_H */
