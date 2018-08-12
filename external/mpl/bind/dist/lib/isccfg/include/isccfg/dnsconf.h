/*	$NetBSD: dnsconf.h,v 1.1.1.1 2018/08/12 12:08:28 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef ISCCFG_DNSCONF_H
#define ISCCFG_DNSCONF_H 1


/*! \file
 * \brief
 * This module defines the named.conf, rndc.conf, and rndc.key grammars.
 */

#include <isccfg/cfg.h>

/*
 * Configuration object types.
 */
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_dnsconf;
/*%< A complete dns.conf file. */

#endif /* ISCCFG_DNSCONF_H */
