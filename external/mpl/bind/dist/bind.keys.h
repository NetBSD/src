/*	$NetBSD: bind.keys.h,v 1.1.1.5 2022/09/23 12:09:06 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef BIND_KEYS_H
#define BIND_KEYS_H 1
#define TRUST_ANCHORS \
	"\
# The bind.keys file is used to override the built-in DNSSEC trust anchors\n\
# which are included as part of BIND 9.  The only trust anchors it contains\n\
# are for the DNS root zone (\".\").  Trust anchors for any other zones MUST\n\
# be configured elsewhere; if they are configured here, they will not be\n\
# recognized or used by named.\n\
#\n\
# To use the built-in root key, set \"dnssec-validation auto;\" in the\n\
# named.conf options, or else leave \"dnssec-validation\" unset.  If\n\
# \"dnssec-validation\" is set to \"yes\", then the keys in this file are\n\
# ignored; keys will need to be explicitly configured in named.conf for\n\
# validation to work.  \"auto\" is the default setting, unless named is\n\
# built with \"configure --disable-auto-validation\", in which case the\n\
# default is \"yes\".\n\
#\n\
# This file is NOT expected to be user-configured.\n\
#\n\
# Servers being set up for the first time can use the contents of this file\n\
# as initializing keys; thereafter, the keys in the managed key database\n\
# will be trusted and maintained automatically.\n\
#\n\
# These keys are current as of Mar 2019.  If any key fails to initialize\n\
# correctly, it may have expired.  In that event you should replace this\n\
# file with a current version.  The latest version of bind.keys can always\n\
# be obtained from ISC at https://www.isc.org/bind-keys.\n\
#\n\
# See https://data.iana.org/root-anchors/root-anchors.xml for current trust\n\
# anchor information for the root zone.\n\
\n\
trust-anchors {\n\
        # This key (20326) was published in the root zone in 2017.\n\
        . initial-key 257 3 8 \"AwEAAaz/tAm8yTn4Mfeh5eyI96WSVexTBAvkMgJzkKTOiW1vkIbzxeF3\n\
                +/4RgWOq7HrxRixHlFlExOLAJr5emLvN7SWXgnLh4+B5xQlNVz8Og8kv\n\
                ArMtNROxVQuCaSnIDdD5LKyWbRd2n9WGe2R8PzgCmr3EgVLrjyBxWezF\n\
                0jLHwVN8efS3rCj/EWgvIWgb9tarpVUDK/b58Da+sqqls3eNbuv7pr+e\n\
                oZG+SrDK6nWeL3c6H5Apxz7LjVc1uTIdsIXxuOLYA4/ilBmSVIzuDWfd\n\
                RUfhHdY6+cn8HFRm+2hM8AnXGXws9555KrUB5qihylGa8subX2Nn6UwN\n\
                R1AkUTV74bU=\";\n\
};\n\
"
#endif /* BIND_KEYS_H */
