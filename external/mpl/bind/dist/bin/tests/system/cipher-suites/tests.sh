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

# shellcheck disable=SC1091
. ../conf.sh

testing="testing zone transfer over TLS (XoT): "

common_dig_options="+noadd +nosea +nostat +noquest +nocmd"

status=0
n=0

dig_with_tls_opts() {
  # shellcheck disable=SC2086
  "$DIG" +tls $common_dig_options -p "${TLSPORT}" "$@"
}

wait_for_tls_xfer() (
  srv_number="$1"
  shift
  zone_name="$1"
  shift
  # Let's bind to .10 to make it possible to easily distinguish dig from NSs in packet traces
  dig_with_tls_opts -b 10.53.0.10 "@10.53.0.$srv_number" "${zone_name}." AXFR >"dig.out.ns$srv_number.${zone_name}.test$n" || return 1
  grep "^;" "dig.out.ns$srv_number.${zone_name}.test$n" >/dev/null && return 1
  return 0
)

tls_xfer_expect_success() {
  test_message="$1"
  shift
  n=$((n + 1))
  echo_i "$test_message - zone \"$2\" at \"ns$1\" ($n)"
  ret=0
  retry_quiet 10 wait_for_tls_xfer "$@" || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
}

tls_xfer_expect_failure() {
  test_message="$1"
  shift
  n=$((n + 1))
  echo_i "$test_message - zone \"$2\" at \"ns$1\", failure expected ($n)"
  ret=0
  retry_quiet 10 wait_for_tls_xfer "$@" && ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
}

tls_xfer_expect_success "$testing" 2 example
tls_xfer_expect_success "$testing" 3 example
tls_xfer_expect_success "$testing" 4 example

tls_xfer_expect_success "$testing" 2 example-aes-128
tls_xfer_expect_success "$testing" 3 example-aes-256
if ! $FEATURETEST --have-fips-mode; then
  tls_xfer_expect_success "$testing" 4 example-chacha-20
fi

tls_xfer_expect_failure "$testing" 2 example-aes-256
if ! $FEATURETEST --have-fips-mode; then
  tls_xfer_expect_failure "$testing" 2 example-chacha-20
fi

tls_xfer_expect_failure "$testing" 3 example-aes-128
if ! $FEATURETEST --have-fips-mode; then
  tls_xfer_expect_failure "$testing" 3 example-chacha-20
fi

tls_xfer_expect_failure "$testing" 4 example-aes-128
tls_xfer_expect_failure "$testing" 4 example-aes-256

# NS5 tries to download the zone over TLSv1.2
tls_xfer_expect_failure "$testing" 5 example
tls_xfer_expect_failure "$testing" 5 example-aes-128
tls_xfer_expect_failure "$testing" 5 example-aes-256
if ! $FEATURETEST --have-fips-mode; then
  tls_xfer_expect_failure "$testing" 5 example-chacha-20
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
