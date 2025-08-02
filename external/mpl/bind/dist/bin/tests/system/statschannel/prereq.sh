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

if ! ${PERL} -MFile::Fetch -e ''; then
  echo_i "perl File::Fetch module is required"
  exit 1
fi

if ! $FEATURETEST --have-libxml2 && ! $FEATURETEST --have-json-c; then
  echo_i "skip: one or both of --with-libxml2 and --with-json-c required"
  exit 255
fi

exit 0
