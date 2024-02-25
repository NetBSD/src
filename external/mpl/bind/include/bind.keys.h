#pragma once
#define TRUST_ANCHORS "\
# Copyright (C) Internet Systems Consortium, Inc. (\"ISC\")\n\
#\n\
# SPDX-License-Identifier: MPL-2.0\n\
#\n\
# This Source Code Form is subject to the terms of the Mozilla Public\n\
# License, v. 2.0. If a copy of the MPL was not distributed with this\n\
# file, you can obtain one at https://mozilla.org/MPL/2.0/.\n\
#\n\
# See the COPYRIGHT file distributed with this work for additional\n\
# information regarding copyright ownership.\n\
\n\
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
