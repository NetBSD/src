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

dig_out_basename="dig.out.test"
testing="testing if the query is successfully completed"

dig_with_opts() {
  # shellcheck disable=SC2086
  "$DIG" -p "${EXTRAPORT1}" +noadd +nosea +nostat +noquest +nocmd "$@" NS example
}

status=0
n=0

run_dig_test() {
  test_message="$1"
  shift
  n=$((n + 1))
  echo_i "$test_message ($n)"
  dig_failed=0
  dig_with_opts "$@" >"$dig_out_basename$n" || dig_failed=1
}

run_dig_test_expect_success() {
  ret=0
  run_dig_test "$@"
  if [ $dig_failed != 0 ]; then
    ret=1
  elif ! [ -s "$dig_out_basename$n" ]; then
    ret=1
  fi
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
}

run_dig_multitest_expect_success() {
  message="$1"
  shift
  run_dig_test_expect_success "$message (IPv4)" -b 10.53.0.10 @10.53.0.1 "$@"
  run_dig_test_expect_success "$message (IPv6)" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 "$@"
}

reconfig_server() {
  message="$1"
  shift
  config_file="$1"
  shift
  echo_i "$message"
  cp "ns1/$config_file" "ns1/named.conf"
  rndc_reconfig ns1 10.53.0.1
}

run_dig_multitest_expect_success "$testing: a UDP query over Do53"
run_dig_multitest_expect_success "$testing: a TCP query over Do53" +tcp

reconfig_server "reconfiguring the server to use PROXYv2" named-proxy.conf
run_dig_multitest_expect_success "$testing: a UDP query for Do53 over PROXYv2" +proxy
run_dig_multitest_expect_success "$testing: a TCP query for Do53 over PROXYv2" +tcp +proxy

reconfig_server "reconfiguring the server to use TLS/DoT" named-tls.conf
run_dig_multitest_expect_success "$testing: a query over TLS/DoT" +tls

reconfig_server "reconfiguring the server to use TLS/DoT over encrypted PROXYv2" named-tls-proxy-encrypted.conf
run_dig_multitest_expect_success "$testing: a query over TLS/DoT over encrypted PROXYv2" +tls +proxy

reconfig_server "reconfiguring the server to use TLS/DoT over plain PROXYv2" named-tls-proxy-plain.conf
run_dig_multitest_expect_success "$testing: a query over TLS/DoT over plain PROXYv2" +tls +proxy +proxy-plain

reconfig_server "reconfiguring the server to use HTTPS/DoH" named-https.conf
run_dig_multitest_expect_success "$testing: a query over HTTPS/DoH" +https

reconfig_server "reconfiguring the server to use HTTPS/DoH over encrypted PROXYv2" named-https-proxy-encrypted.conf
run_dig_multitest_expect_success "$testing: a query over HTTPS/DoH over encrypted PROXYv2" +https +proxy

reconfig_server "reconfiguring the server to use HTTPS/DoH over plain PROXYv2" named-https-proxy-plain.conf
run_dig_multitest_expect_success "$testing: a query over HTTPS/DoH over plain PROXYv2" +https +proxy +proxy-plain

reconfig_server "reconfiguring the server to use plain HTTP/DoH" named-http-plain.conf
run_dig_multitest_expect_success "$testing: a query over plain HTTP/DoH" +http-plain

reconfig_server "reconfiguring the server to use plain HTTP/DoH over PROXYv2" named-http-plain-proxy.conf
run_dig_multitest_expect_success "$testing: a query over plain HTTP/DoH over PROXYv2" +http-plain +proxy

reconfig_server "reconfiguring the server back to use TLS/DoT" named-tls.conf
run_dig_multitest_expect_success "$testing: a query over TLS/DoT" +tls

reconfig_server "reconfiguring the server back to use HTTPS/DoH" named-https.conf
run_dig_multitest_expect_success "$testing: a query over HTTPS/DoH" +https

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
