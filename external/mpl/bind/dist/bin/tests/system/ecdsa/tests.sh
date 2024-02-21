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

set -e

. ../conf.sh

status=0
n=0

dig_with_opts() {
  "$DIG" +tcp +noau +noadd +nosea +nostat +nocmd +dnssec -p "$PORT" "$@"
}

if [ -f ecdsa256-supported.file ]; then
  n=$((n + 1))
  echo_i "checking that ECDSA256 positive validation works ($n)"
  ret=0
  dig_with_opts . @10.53.0.1 soa >dig.out.ns1.test$n || ret=1
  dig_with_opts . @10.53.0.2 soa >dig.out.ns2.test$n || ret=1
  $PERL ../digcomp.pl dig.out.ns1.test$n dig.out.ns2.test$n || ret=1
  grep "flags:.*ad.*QUERY" dig.out.ns2.test$n >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
else
  echo_i "algorithm ECDSA256 not supported, skipping test"
fi

if [ -f ecdsa384-supported.file ]; then
  n=$((n + 1))
  echo_i "checking that ECDSA384 positive validation works ($n)"
  ret=0
  dig_with_opts . @10.53.0.1 soa >dig.out.ns1.test$n || ret=1
  dig_with_opts . @10.53.0.3 soa >dig.out.ns3.test$n || ret=1
  $PERL ../digcomp.pl dig.out.ns1.test$n dig.out.ns3.test$n || ret=1
  grep "flags:.*ad.*QUERY" dig.out.ns3.test$n >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
else
  echo_i "algorithm ECDSA384 not supported, skipping test"
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
