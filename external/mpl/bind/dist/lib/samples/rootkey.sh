#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# Fetch a copy of a current root signing key; used for testing
# DNSSEC validation in 'sample'.
#
# After running this script, "sample `cat sample.key` <args>" will
# perform a lookup as specified in <args> and validate the result
# using the root key.
#
# (This is NOT a secure method of obtaining the root key; it is
# included here for testing purposes only.)
dig +noall +answer dnskey . | perl -n -e '
local ($dn, $ttl, $class, $type, $flags, $proto, $alg, @rest) = split;
next if ($flags != 257);
local $key = join("", @rest);
print "-a $alg -e -k $dn -K $key\n"
' > sample.key
