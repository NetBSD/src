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

if ! ${PERL} -MNet::DNS -e ''; then
  echo_i "perl Net::DNS module is required"
  exit 1
fi

if ! ${PERL} -MNet::DNS::Nameserver -e ''; then
  echo_i "perl Net::DNS::Nameserver module is required"
  exit 1
fi

exit 0
