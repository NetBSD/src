/*	$NetBSD: namedconf.h,v 1.6.2.1 2024/02/25 15:47:33 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#pragma once

/*! \file isccfg/namedconf.h
 * \brief
 * This module defines the named.conf, rndc.conf, and rndc.key grammars.
 */

#include <isccfg/cfg.h>

/*
 * Configuration object types.
 */
extern cfg_type_t cfg_type_namedconf;
/*%< A complete named.conf file. */

extern cfg_type_t cfg_type_bindkeys;
/*%< A bind.keys file. */

extern cfg_type_t cfg_type_newzones;
/*%< A new-zones file (for zones added by 'rndc addzone'). */

extern cfg_type_t cfg_type_addzoneconf;
/*%< A single zone passed via the addzone rndc command. */

extern cfg_type_t cfg_type_rndcconf;
/*%< A complete rndc.conf file. */

extern cfg_type_t cfg_type_rndckey;
/*%< A complete rndc.key file. */

extern cfg_type_t cfg_type_sessionkey;
/*%< A complete session.key file. */

extern cfg_type_t cfg_type_keyref;
/*%< A key reference, used as an ACL element */

/*%< Zone options */
extern cfg_type_t cfg_type_zoneopts;

/*%< DNSSEC Key and Signing Policy options */
extern cfg_type_t cfg_type_dnssecpolicyopts;
