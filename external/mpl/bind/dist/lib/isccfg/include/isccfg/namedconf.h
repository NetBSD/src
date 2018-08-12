/*	$NetBSD: namedconf.h,v 1.1.1.1 2018/08/12 12:08:28 christos Exp $	*/

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


#ifndef ISCCFG_NAMEDCONF_H
#define ISCCFG_NAMEDCONF_H 1

/*! \file isccfg/namedconf.h
 * \brief
 * This module defines the named.conf, rndc.conf, and rndc.key grammars.
 */

#include <isccfg/cfg.h>

/*
 * Configuration object types.
 */
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_namedconf;
/*%< A complete named.conf file. */

LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_bindkeys;
/*%< A bind.keys file. */

LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_newzones;
/*%< A new-zones file (for zones added by 'rndc addzone'). */

LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_addzoneconf;
/*%< A single zone passed via the addzone rndc command. */

LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_rndcconf;
/*%< A complete rndc.conf file. */

LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_rndckey;
/*%< A complete rndc.key file. */

LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_sessionkey;
/*%< A complete session.key file. */

LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_keyref;
/*%< A key reference, used as an ACL element */

/*%< An EDNS client subnet address, used as an ACL element */
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_ecsprefix;

/*%< Zone options */
LIBISCCFG_EXTERNAL_DATA extern cfg_type_t cfg_type_zoneopts;

#endif /* ISCCFG_NAMEDCONF_H */
