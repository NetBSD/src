#!/bin/sh

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

. ../conf.sh

# macOS ships with Net::DNS 0.74 which does not work with
# HMAC-SHA256, despite the workarounds in ans.pl

if ${PERL} -MNet::DNS -e 'exit ($Net::DNS::VERSION >= 1.0)'; then
  version=$(${PERL} -MNet::DNS -e 'print $Net::DNS::VERSION')
  echo_i "perl Net::DNS $version is too old - skipping xfer test"
  exit 1
fi

if ! ${PERL} -MDigest::HMAC -e ''; then
  echo_i "perl Digest::HMAC module is required"
  exit 1
fi

exit 0
