#!/bin/sh -e
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

. ../conf.sh

[ "prereq/var/tmp/etc/openssl-provider.cnf" = "prereq${OPENSSL_CONF}" ] || {
  echo_i "skip: pkcs11-provider not enabled"
  exit 255
}

[ -n "${SOFTHSM2_CONF}" ] || {
  echo_i "skip: softhsm2 configuration not available"
  exit 255
}

[ -f "$SOFTHSM2_MODULE" ] || {
  echo_i "skip: softhsm2 module not available"
  exit 1
}

for _bin in softhsm2-util pkcs11-tool; do
  command -v "$_bin" >/dev/null || {
    echo_i "skip: $_bin not available"
    exit 1
  }
done
