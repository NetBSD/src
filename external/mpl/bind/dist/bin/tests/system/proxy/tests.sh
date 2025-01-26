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
testing="PROXY test"
fail_regex='(^(; EDE: 18 \(Prohibited\))|(; Transfer failed\.$))'

dig_with_opts() {
  # shellcheck disable=SC2086
  "$DIG" +noadd +nosea +nostat +noquest +nocmd +tries=1 "$@"
}

status=0
n=0

run_dig_test() {
  test_message="$1"
  shift
  n=$((n + 1))
  echo_i "$test_message ($n)"
  ret=0
  dig_failed=0
  dig_with_opts "$@" >"$dig_out_basename$n" || dig_failed=1
}

run_dig_test_expect_success() {
  run_dig_test "$@"
  if [ $dig_failed != 0 ]; then
    ret=1
  elif ! [ -s "$dig_out_basename$n" ]; then
    ret=1
  else
    grep -E "$fail_regex" "$dig_out_basename$n" >/dev/null && ret=1
  fi
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
}

run_dig_test_expect_failure() {
  run_dig_test "$@"
  if [ $dig_failed -eq 0 ] && [ -s "$dig_out_basename$n" ]; then
    grep -E "$fail_regex" "$dig_out_basename$n" >/dev/null || ret=1
  fi
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
}

run_dig_multitest_expect_success() {
  message="$1"
  shift
  proxy_addrs="$1"
  shift
  run_dig_test_expect_success "$message (UDP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.1 +notcp "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (TCP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.1 +tcp "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (TLS/PROXY encrypted)" -p "${TLSPORT}" -b 10.53.0.10 @10.53.0.1 +tls "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (TLS/PROXY plain)" -p "${EXTRAPORT2}" -b 10.53.0.10 @10.53.0.1 +tls "+proxy-plain=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (HTTPS/PROXY encrypted)" -p "${HTTPSPORT}" -b 10.53.0.10 @10.53.0.1 +https "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (HTTPS/PROXY plain)" -p "${EXTRAPORT3}" -b 10.53.0.10 @10.53.0.1 +https "+proxy-plain=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (HTTP)" -p "${HTTPPORT}" -b 10.53.0.10 @10.53.0.1 +http-plain "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (UDP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +notcp "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (TCP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tcp "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (TLS/PROXY encrypted, IPv6)" -p "${TLSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (TLS/PROXY plain, IPv6)" -p "${EXTRAPORT2}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls "+proxy-plain=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (HTTPS/PROXY encrypted, IPv6)" -p "${HTTPSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +https "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (HTTPS/PROXY plain, IPv6)" -p "${EXTRAPORT3}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +https "+proxy-plain=$proxy_addrs" "$@"
  run_dig_test_expect_success "$message (HTTP, IPv6)" -p "${HTTPPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +http-plain "+proxy=$proxy_addrs" "$@"
}

run_dig_multitest_expect_failure() {
  message="$1"
  shift
  proxy_addrs="$1"
  shift
  run_dig_test_expect_failure "$message (UDP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.1 +notcp "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (TCP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.1 +tcp "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (TLS/PROXY encrypted)" -p "${TLSPORT}" -b 10.53.0.10 @10.53.0.1 +tls "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (TLS/PROXY plain)" -p "${EXTRAPORT2}" -b 10.53.0.10 @10.53.0.1 +tls "+proxy-plain=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (HTTPS/PROXY encrypted)" -p "${HTTPSPORT}" -b 10.53.0.10 @10.53.0.1 +https "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (HTTPS/PROXY plain)" -p "${EXTRAPORT3}" -b 10.53.0.10 @10.53.0.1 +https "+proxy-plain=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (HTTP)" -p "${HTTPPORT}" -b 10.53.0.10 @10.53.0.1 +http-plain "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (UDP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +notcp "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (TCP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tcp "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (TLS/PROXY encrypted, IPv6)" -p "${TLSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (TLS/PROXY plain, IPv6)" -p "${EXTRAPORT2}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls "+proxy-plain=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (HTTPS/PROXY encrypted, IPv6)" -p "${HTTPSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +https "+proxy=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (HTTPS/PROXY plain, IPv6)" -p "${EXTRAPORT3}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +https "+proxy-plain=$proxy_addrs" "$@"
  run_dig_test_expect_failure "$message (HTTP, IPv6)" -p "${HTTPPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +http-plain "+proxy=$proxy_addrs" "$@"
}

# generic tests

# Bind to the IP address that is allowed to send PROXYv2
run_dig_test_expect_success "$testing: allow-proxy expect success (UDP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.1 +notcp +proxy NS example0
run_dig_test_expect_success "$testing: allow-proxy expect success (TCP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.1 +tcp +proxy NS example0
run_dig_test_expect_success "$testing: allow-proxy expect success (TLS/PROXY encrypted)" -p "${TLSPORT}" -b 10.53.0.10 @10.53.0.1 +tls +proxy NS example0
run_dig_test_expect_success "$testing: allow-proxy expect success (TLS/PROXY plain)" -p "${EXTRAPORT2}" -b 10.53.0.10 @10.53.0.1 +tls +proxy-plain NS example0
run_dig_test_expect_success "$testing: allow-proxy expect success (HTTPS/PROXY encrypted)" -p "${HTTPSPORT}" -b 10.53.0.10 @10.53.0.1 +https +proxy NS example0
run_dig_test_expect_success "$testing: allow-proxy expect success (HTTPS/PROXY plain)" -p "${EXTRAPORT3}" -b 10.53.0.10 @10.53.0.1 +https +proxy-plain NS example0
run_dig_test_expect_success "$testing: allow-proxy expect success (HTTP)" -p "${HTTPPORT}" -b 10.53.0.10 @10.53.0.1 +http-plain +proxy NS example0

run_dig_test_expect_success "$testing: allow-proxy expect success (UDP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +notcp +proxy NS example0
run_dig_test_expect_success "$testing: allow-proxy expect success (TCP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tcp +proxy NS example0
run_dig_test_expect_success "$testing: allow-proxy expect success (TLS/PROXY encrypted, IPv6)" -p "${TLSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy NS example0
run_dig_test_expect_success "$testing: allow-proxy expect success (TLS/PROXY plain, IPv6)" -p "${EXTRAPORT2}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy-plain NS example0
run_dig_test_expect_success "$testing: allow-proxy expect success (HTTPS/PROXY encrypted, IPv6)" -p "${HTTPSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +https +proxy NS example0
run_dig_test_expect_success "$testing: allow-proxy expect success (HTTPS/PROXY plain, IPv6)" -p "${EXTRAPORT3}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +https +proxy-plain NS example0
run_dig_test_expect_success "$testing: allow-proxy expect success (HTTP, IPv6)" -p "${HTTPPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +http-plain +proxy NS example0

# Bind to the IP address that is not allowed to send PROXYv2
run_dig_test_expect_failure "$testing: allow-proxy expect failure (UDP)" -p "${EXTRAPORT1}" -b 10.53.0.11 @10.53.0.1 +notcp +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy expect failure (TCP)" -p "${EXTRAPORT1}" -b 10.53.0.11 @10.53.0.1 +tcp +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy expect failure (TLS/PROXY encrypted)" -p "${TLSPORT}" -b 10.53.0.11 @10.53.0.1 +tls +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy expect failure (TLS/PROXY plain)" -p "${EXTRAPORT2}" -b 10.53.0.11 @10.53.0.1 +tls +proxy-plain NS example0
run_dig_test_expect_failure "$testing: allow-proxy expect failure (HTTPS/PROXY encrypted)" -p "${HTTPSPORT}" -b 10.53.0.11 @10.53.0.1 +https +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy expect failure (HTTPS/PROXY plain)" -p "${EXTRAPORT3}" -b 10.53.0.11 @10.53.0.1 +https +proxy-plain NS example0
run_dig_test_expect_failure "$testing: allow-proxy expect failure (HTTP)" -p "${HTTPPORT}" -b 10.53.0.11 @10.53.0.1 +http-plain +proxy NS example0

run_dig_test_expect_failure "$testing: allow-proxy expect failure (UDP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::11 @fd92:7065:b8e:ffff::1 +notcp +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy expect failure (TCP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::11 @fd92:7065:b8e:ffff::1 +tcp +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy expect failure (TLS/PROXY encrypted, IPv6)" -p "${TLSPORT}" -b fd92:7065:b8e:ffff::11 @fd92:7065:b8e:ffff::1 +tls +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy expect failure (TLS/PROXY plain, IPv6)" -p "${EXTRAPORT2}" -b fd92:7065:b8e:ffff::11 @fd92:7065:b8e:ffff::1 +tls +proxy-plain NS example0
run_dig_test_expect_failure "$testing: allow-proxy expect failure (HTTPS/PROXY encrypted, IPv6)" -p "${HTTPSPORT}" -b fd92:7065:b8e:ffff::11 @fd92:7065:b8e:ffff::1 +https +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy expect failure (HTTPS/PROXY plain, IPv6)" -p "${EXTRAPORT3}" -b fd92:7065:b8e:ffff::11 @fd92:7065:b8e:ffff::1 +https +proxy-plain NS example0
run_dig_test_expect_failure "$testing: allow-proxy expect failure (HTTP, IPv6)" -p "${HTTPPORT}" -b fd92:7065:b8e:ffff::11 @fd92:7065:b8e:ffff::1 +http-plain +proxy NS example0

# Send a query to the interface that is not allowed to accept PROXYv2
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (UDP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.2 +notcp +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (TCP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.2 +tcp +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (TLS/PROXY encrypted)" -p "${TLSPORT}" -b 10.53.0.10 @10.53.0.2 +tls +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (TLS/PROXY plain)" -p "${EXTRAPORT2}" -b 10.53.0.10 @10.53.0.2 +tls +proxy-plain NS example0
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (HTTPS/PROXY encrypted)" -p "${HTTPSPORT}" -b 10.53.0.10 @10.53.0.2 +https +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (HTTPS/PROXY plain)" -p "${EXTRAPORT3}" -b 10.53.0.10 @10.53.0.2 +https +proxy-plain NS example0
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (HTTP)" -p "${HTTPPORT}" -b 10.53.0.10 @10.53.0.2 +http-plain +proxy NS example0

run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (UDP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::2 +notcp +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (TCP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::2 +tcp +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (TLS/PROXY encrypted, IPv6)" -p "${TLSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::2 +tls +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (TLS/PROXY plain, IPv6)" -p "${EXTRAPORT2}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::2 +tls +proxy-plain NS example0
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (HTTPS/PROXY encrypted, IPv6)" -p "${HTTPSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::2 +https +proxy NS example0
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (HTTPS/PROXY plain, IPv6)" -p "${EXTRAPORT3}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::2 +https +proxy-plain NS example0
run_dig_test_expect_failure "$testing: allow-proxy-on expect failure (HTTP, IPv6)" -p "${HTTPPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::2 +http-plain +proxy NS example0

## Now let's check if the addresses passed via PROXYv2 are getting used by BIND9.
run_dig_multitest_expect_success "check if IPv4 addresses are getting passed and accepted" "1.2.3.4-4.3.2.1" NS example1
run_dig_multitest_expect_success "check if IPv6 addresses are getting passed and accepted" "fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321" NS example1

run_dig_multitest_expect_failure "check if IPv4 addresses are getting passed and rejected" "4.4.4.4-5.5.5.5" NS example1
run_dig_multitest_expect_failure "check if IPv6 addresses are getting passed and rejected" "fd0f:99d3:98a7::4444-fd0f:99d3:98a7::5555" NS example1

run_dig_multitest_expect_failure "check if IPv4 addresses are getting passed and an unexpected source address is getting rejected" "4.4.4.4-4.3.2.1" NS example1
run_dig_multitest_expect_failure "check if IPv6 addresses are getting passed and an unexpected source address is getting rejected" "fd0f:99d3:98a7::4444-fd0f:99d3:98a7::4321" NS example1

run_dig_multitest_expect_failure "check if IPv4 addresses are getting passed and an unexpected destination address is getting rejected" "1.2.3.4-5.5.5.5" NS example1
run_dig_multitest_expect_failure "check if IPv6 addresses are getting passed and an unexpected destination address is getting rejected" "fd0f:99d3:98a7::1234-fd0f:99d3:98a7::5.5.5.5" NS example1

## Let's check if the real addresses are used by BIND9 for LOCAL requests
run_dig_multitest_expect_success "check if LOCAL PROXYv2 headers are accepted and real connection addresses are used" "" NS example2

## Let's check if BIND9 does not like suspicious PROXY headers with zeroed addresses or destination ports
run_dig_multitest_expect_failure "check if port 0 is not accepted when used in the destination IPv4 address" "1.2.3.4-4.3.2.1#0" NS example1
run_dig_multitest_expect_failure "check if port 0 is not accepted when used in the destination IPv6 address" "fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#0" NS example1

run_dig_multitest_expect_failure "check if \"0.0.0.0\" is not accepted as a source address" "0.0.0.0-4.3.2.1" NS example1
run_dig_multitest_expect_failure "check if \"::\" is not accepted as a source address" "::-fd0f:99d3:98a7::4321" NS example1

run_dig_multitest_expect_failure "check if \"0.0.0.0\" is not accepted as a destination address" "1.2.3.4-0.0.0.0" NS example1
run_dig_multitest_expect_failure "check if \"::\" is not accepted as a destination address" "fd0f:99d3:98a7::1234-::" NS example1

# Let's verify that ports information from PROXY headers is being used by BIND
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND (TCP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.1 +tcp +proxy="1.2.3.4-4.3.2.1#53" AXFR example-proxy-do53
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND (TLS/PROXY encrypted)" -p "${TLSPORT}" -b 10.53.0.10 @10.53.0.1 +tls +proxy="1.2.3.4-4.3.2.1#853" AXFR example-proxy-encrypted-dot
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND (TLS/PROXY plain)" -p "${EXTRAPORT2}" -b 10.53.0.10 @10.53.0.1 +tls +proxy-plain="1.2.3.4-4.3.2.1#853" AXFR example-proxy-plain-dot

run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND (TCP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tcp +proxy="1.2.3.4-4.3.2.1#53" AXFR example-proxy-do53
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND (TLS/PROXY encrypted, IPv6)" -p "${TLSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy="1.2.3.4-4.3.2.1#853" AXFR example-proxy-encrypted-dot
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND (TLS/PROXY plain, IPv6)" -p "${EXTRAPORT2}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy-plain="1.2.3.4-4.3.2.1#853" AXFR example-proxy-plain-dot

run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND (TCP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.1 +tcp +proxy="fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#53" AXFR example-proxy-do53
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND (TLS/PROXY encrypted)" -p "${TLSPORT}" -b 10.53.0.10 @10.53.0.1 +tls +proxy="fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#853" AXFR example-proxy-encrypted-dot
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND (TLS/PROXY plain)" -p "${EXTRAPORT2}" -b 10.53.0.10 @10.53.0.1 +tls +proxy-plain="fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#853" AXFR example-proxy-plain-dot

run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND (TCP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tcp +proxy="fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#53" AXFR example-proxy-do53
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND (TLS/PROXY encrypted, IPv6)" -p "${TLSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy="fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#853" AXFR example-proxy-encrypted-dot
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND (TLS/PROXY plain, IPv6)" -p "${EXTRAPORT2}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy-plain="fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#853" AXFR example-proxy-plain-dot

# Let's use a wrong ports to see if BIND will reject the queries (as it should)
run_dig_test_expect_failure "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND for rejection (TCP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.1 +tcp +proxy="1.2.3.4-4.3.2.1#1234" AXFR example-proxy-do53
run_dig_test_expect_failure "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND for rejection (TLS/PROXY encrypted)" -p "${TLSPORT}" -b 10.53.0.10 @10.53.0.1 +tls +proxy="1.2.3.4-4.3.2.1#1234" AXFR example-proxy-encrypted-dot
run_dig_test_expect_failure "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND for rejection (TLS/PROXY plain)" -p "${EXTRAPORT2}" -b 10.53.0.10 @10.53.0.1 +tls +proxy-plain="1.2.3.4-4.3.2.1#1234" AXFR example-proxy-plain-dot

run_dig_test_expect_failure "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND for rejection (TCP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tcp +proxy="1.2.3.4-4.3.2.1#1234" AXFR example-proxy-do53
run_dig_test_expect_failure "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND for rejection (TLS/PROXY encrypted, IPv6)" -p "${TLSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy="1.2.3.4-4.3.2.1#1234" AXFR example-proxy-encrypted-dot
run_dig_test_expect_failure "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND for rejection (TLS/PROXY plain, IPv6)" -p "${EXTRAPORT2}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy-plain="1.2.3.4-4.3.2.1#1234" AXFR example-proxy-plain-dot

run_dig_test_expect_failure "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND for rejection (TCP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.1 +tcp +proxy="fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#1234" AXFR example-proxy-do53
run_dig_test_expect_failure "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND for rejection (TLS/PROXY encrypted)" -p "${TLSPORT}" -b 10.53.0.10 @10.53.0.1 +tls +proxy="fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#1234" AXFR example-proxy-encrypted-dot
run_dig_test_expect_failure "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND for rejection (TLS/PROXY plain)" -p "${EXTRAPORT2}" -b 10.53.0.10 @10.53.0.1 +tls +proxy-plain="fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#1234" AXFR example-proxy-plain-dot

run_dig_test_expect_failure "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND for rejection (TCP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tcp +proxy="fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#1234" AXFR example-proxy-do53
run_dig_test_expect_failure "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND for rejection (TLS/PROXY encrypted, IPv6)" -p "${TLSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy="fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#1234" AXFR example-proxy-encrypted-dot
run_dig_test_expect_failure "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND for rejection (TLS/PROXY plain, IPv6)" -p "${EXTRAPORT2}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy-plain="fd0f:99d3:98a7::1234-fd0f:99d3:98a7::4321#1234" AXFR example-proxy-plain-dot

# Now let's make something similar, but for LOCAL PROXY requests
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND (TCP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.1 +tcp +proxy AXFR example-proxy-local-do53
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND (TLS/PROXY encrypted)" -p "${TLSPORT}" -b 10.53.0.10 @10.53.0.1 +tls +proxy AXFR example-proxy-local-encrypted-dot
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND (TLS/PROXY plain)" -p "${EXTRAPORT2}" -b 10.53.0.10 @10.53.0.1 +tls +proxy-plain AXFR example-proxy-local-plain-dot

run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND (TCP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tcp +proxy AXFR example-proxy-local-do53
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND (TLS/PROXY encrypted, IPv6)" -p "${TLSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy AXFR example-proxy-local-encrypted-dot
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv4 addresses is being used by BIND (TLS/PROXY plain, IPv6)" -p "${EXTRAPORT2}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy-plain AXFR example-proxy-local-plain-dot

run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND (TCP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.1 +tcp +proxy AXFR example-proxy-local-do53
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND (TLS/PROXY encrypted)" -p "${TLSPORT}" -b 10.53.0.10 @10.53.0.1 +tls +proxy AXFR example-proxy-local-encrypted-dot
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND (TLS/PROXY plain)" -p "${EXTRAPORT2}" -b 10.53.0.10 @10.53.0.1 +tls +proxy-plain AXFR example-proxy-local-plain-dot

run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND (TCP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tcp +proxy AXFR example-proxy-local-do53
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND (TLS/PROXY encrypted, IPv6)" -p "${TLSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy AXFR example-proxy-local-encrypted-dot
run_dig_test_expect_success "$testing: check if ports information from a PROXYv2 header with IPv6 addresses is being used by BIND (TLS/PROXY plain, IPv6)" -p "${EXTRAPORT2}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::1 +tls +proxy-plain AXFR example-proxy-local-plain-dot

# verify that by default PROXYv2 is not accepted
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (UDP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.3 +proxy NS example
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (TCP)" -p "${EXTRAPORT1}" -b 10.53.0.10 @10.53.0.3 +tcp +proxy NS example
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (TLS/PROXY encrypted)" -p "${TLSPORT}" -b 10.53.0.10 @10.53.0.3 +tls +proxy NS example
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (TLS/PROXY plain)" -p "${EXTRAPORT2}" -b 10.53.0.10 @10.53.0.3 +tls +proxy NS example
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (HTTPS/PROXY encrypted)" -p "${HTTPSPORT}" -b 10.53.0.10 @10.53.0.3 +https +proxy NS example
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (HTTPS/PROXY plain)" -p "${EXTRAPORT3}" -b 10.53.0.10 @10.53.0.3 +https +proxy-plain NS example
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (HTTP)" -p "${HTTPPORT}" -b 10.53.0.10 @10.53.0.3 +https +proxy NS example

run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (UDP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::3 +proxy NS example
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (TCP, IPv6)" -p "${EXTRAPORT1}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::3 +tcp +proxy NS example
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (TLS/PROXY encrypted, IPv6)" -p "${TLSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::3 +tls +proxy NS example
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (TLS/PROXY plain, IPv6)" -p "${EXTRAPORT2}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::3 +tls +proxy NS example
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (HTTPS/PROXY encrypted, IPv6)" -p "${HTTPSPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::3 +https +proxy NS example
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (HTTPS/PROXY plain, IPv6)" -p "${EXTRAPORT3}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::3 +https +proxy-plain NS example
run_dig_test_expect_failure "$testing: check if BIND does not accept PROXYv2 by default (HTTP, IPv6)" -p "${HTTPPORT}" -b fd92:7065:b8e:ffff::10 @fd92:7065:b8e:ffff::3 +https +proxy NS example

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
