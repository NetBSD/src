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

if [ -f ed25519-supported.file ]; then
  # Check the example. domain
  n=$((n + 1))
  echo_i "checking that Ed25519 positive validation works ($n)"
  ret=0
  dig_with_opts . @10.53.0.1 soa >dig.out.ns1.test$n || ret=1
  dig_with_opts . @10.53.0.2 soa >dig.out.ns2.test$n || ret=1
  $PERL ../digcomp.pl dig.out.ns1.test$n dig.out.ns2.test$n || ret=1
  grep "flags:.*ad.*QUERY" dig.out.ns2.test$n >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  # Check test vectors (RFC 8080 + errata)
  n=$((n + 1))
  echo_i "checking that Ed25519 test vectors match ($n)"
  ret=0
  grep 'oL9krJun7xfBOIWcGHi7mag5/hdZrKWw15jP' ns2/example.com.db.signed >/dev/null || ret=1
  grep 'VrbpMngwcrqNAg==' ns2/example.com.db.signed >/dev/null || ret=1
  grep 'zXQ0bkYgQTEFyfLyi9QoiY6D8ZdYo4wyUhVi' ns2/example.com.db.signed >/dev/null || ret=1
  grep 'R0O7KuI5k2pcBg==' ns2/example.com.db.signed >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
else
  echo_i "algorithm Ed25519 not supported, skipping vectors match test"
fi

n=$((n + 1))
ret=0
if [ -f ed448-supported.file ]; then
  # Check the example. domain
  n=$((n + 1))
  echo_i "checking that Ed448 positive validation works ($n)"
  ret=0
  dig_with_opts . @10.53.0.1 soa >dig.out.ns1.test$n || ret=1
  dig_with_opts . @10.53.0.3 soa >dig.out.ns3.test$n || ret=1
  $PERL ../digcomp.pl dig.out.ns1.test$n dig.out.ns3.test$n || ret=1
  grep "flags:.*ad.*QUERY" dig.out.ns3.test$n >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  # Check test vectors (RFC 8080 + errata)
  n=$((n + 1))
  echo_i "checking that Ed448 test vectors match ($n)"
  ret=0
  grep '3cPAHkmlnxcDHMyg7vFC34l0blBhuG1qpwLm' ns3/example.com.db.signed >/dev/null || ret=1
  grep 'jInI8w1CMB29FkEAIJUA0amxWndkmnBZ6SKi' ns3/example.com.db.signed >/dev/null || ret=1
  grep 'wZSAxGILn/NBtOXft0+Gj7FSvOKxE/07+4RQ' ns3/example.com.db.signed >/dev/null || ret=1
  grep 'vE581N3Aj/JtIyaiYVdnYtyMWbSNyGEY2213' ns3/example.com.db.signed >/dev/null || ret=1
  grep 'WKsJlwEA' ns3/example.com.db.signed >/dev/null || ret=1

  grep 'E1/oLjSGIbmLny/4fcgM1z4oL6aqo+izT3ur' ns3/example.com.db.signed >/dev/null || ret=1
  grep 'CyHyvEp4Sp8Syg1eI+lJ57CSnZqjJP41O/9l' ns3/example.com.db.signed >/dev/null || ret=1
  grep '4m0AsQ4f7qI1gVnML8vWWiyW2KXhT9kuAICU' ns3/example.com.db.signed >/dev/null || ret=1
  grep 'Sxv5OWbf81Rq7Yu60npabODB0QFPb/rkW3kU' ns3/example.com.db.signed >/dev/null || ret=1
  grep 'ZmQ0YQUA' ns3/example.com.db.signed >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
else
  echo_i "algorithm Ed448 not supported, skipping vectors match test"
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
