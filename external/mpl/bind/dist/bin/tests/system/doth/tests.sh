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

common_dig_options="+noadd +nosea +nostat +noquest +nocmd"
msg_xfrs_not_allowed=";; zone transfers over the established TLS connection are not allowed"
msg_peer_verification_failed=";; TLS peer certificate verification"

ca_file="./CA/CA.pem"

OPENSSL_VERSION=$("$PYTHON" "$TOP_SRCDIR/bin/tests/system/doth/get_openssl_version.py")
OPENSSL_VERSION_MAJOR=$(echo "$OPENSSL_VERSION" | cut -d ' ' -f 1)
OPENSSL_VERSION_MINOR=$(echo "$OPENSSL_VERSION" | cut -d ' ' -f 2)

# According to the RFC 8310, Section 8.1, Subject field MUST
# NOT be inspected when verifying a hostname when using
# DoT. Only SubjectAltName must be checked instead. That is
# not the case for HTTPS, though.

# Unfortunately, some quite old versions of OpenSSL (< 1.1.1)
# might lack the functionality to implement that. It should
# have very little real-world consequences, as most of the
# production-ready certificates issued by real CAs will have
# SubjectAltName set. In such a case, the Subject field is
# ignored.
#
# On the platforms with too old TLS versions, e.g. RedHat 7, we should
# ignore the tests checking the correct handling of absence of
# SubjectAltName.
if [ $OPENSSL_VERSION_MAJOR -gt 1 ]; then
  run_san_tests=1
elif [ $OPENSSL_VERSION_MAJOR -eq 1 ] && [ $OPENSSL_VERSION_MINOR -ge 1 ]; then
  run_san_tests=1
fi

dig_with_tls_opts() {
  # shellcheck disable=SC2086
  "$DIG" +tls $common_dig_options -p "${TLSPORT}" "$@"
}

dig_with_https_opts() {
  # shellcheck disable=SC2086
  "$DIG" +https $common_dig_options -p "${HTTPSPORT}" "$@"
}

dig_with_http_opts() {
  # shellcheck disable=SC2086
  "$DIG" +http-plain $common_dig_options -p "${HTTPPORT}" "$@"
}

dig_with_opts() {
  # shellcheck disable=SC2086
  "$DIG" $common_dig_options -p "${PORT}" "$@"
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

status=0
n=0

n=$((n + 1))
echo_i "testing XoT server functionality (using dig) ($n)"
ret=0
dig_with_tls_opts example. -b 10.53.0.10 @10.53.0.1 axfr >dig.out.ns1.test$n || ret=1
grep "^;" dig.out.ns1.test$n | cat_i
digcomp example.axfr.good dig.out.ns1.test$n || ret=1
if test $ret != 0; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing incoming XoT functionality (from the first secondary) ($n)"
ret=0
if retry_quiet 10 wait_for_tls_xfer 2 example; then
  digcomp example.axfr.good "dig.out.ns2.example.test$n" || ret=1
else
  echo_i "timed out waiting for zone transfer"
  grep "^;" "dig.out.ns2.example.test$n" | cat_i
  ret=1
fi
if test $ret != 0; then echo_i "failed"; fi
status=$((status + ret))

if [ -n "$run_san_tests" ]; then
  n=$((n + 1))
  echo_i "testing incoming XoT functionality (from the first secondary, no SubjectAltName, failure expected) ($n)"
  ret=0
  if retry_quiet 10 wait_for_tls_xfer 2 example3; then
    ret=1
  else
    echo_i "timed out waiting for zone transfer"
  fi
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

n=$((n + 1))
echo_i "testing incoming XoT functionality (from the first secondary, StrictTLS via implicit IP) ($n)"
ret=0
if retry_quiet 10 wait_for_tls_xfer 2 example4; then
  retry_quiet 5 test -f "ns2/example4.db" || ret=1
else
  echo_i "timed out waiting for zone transfer"
  grep "^;" "dig.out.ns2.example4.test$n" | cat_i
  ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing incoming XoT functionality (from the first secondary, StrictTLS via specified IPv4) ($n)"
ret=0
if retry_quiet 10 wait_for_tls_xfer 2 example5; then
  retry_quiet 5 test -f "ns2/example5.db" || ret=1
else
  echo_i "timed out waiting for zone transfer"
  grep "^;" "dig.out.ns2.example5.test$n" | cat_i
  ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing incoming XoT functionality (from the first secondary, StrictTLS via specified IPv6) ($n)"
ret=0
if retry_quiet 10 wait_for_tls_xfer 2 example6; then
  retry_quiet 5 test -f "ns2/example6.db" || ret=1
else
  echo_i "timed out waiting for zone transfer"
  grep "^;" "dig.out.ns2.example6.test$n" | cat_i
  ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing incoming XoT functionality (from the first secondary, wrong hostname, failure expected) ($n)"
ret=0
if retry_quiet 10 wait_for_tls_xfer 2 example7; then
  ret=1
else
  echo_i "timed out waiting for zone transfer"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing incoming XoT functionality (from the first secondary, expired certificate, failure expected) ($n)"
ret=0
if retry_quiet 10 wait_for_tls_xfer 2 example8; then
  ret=1
else
  echo_i "timed out waiting for zone transfer"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing incoming XoT functionality (from the first secondary, MutualTLS) ($n)"
ret=0
if retry_quiet 10 wait_for_tls_xfer 2 example9; then
  retry_quiet 5 test -f "ns2/example9.db" || ret=1
else
  echo_i "timed out waiting for zone transfer"
  grep "^;" "dig.out.ns2.example9.test$n" | cat_i
  ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing incoming XoT functionality (from the first secondary, MutualTLS, no client cert, failure expected) ($n)"
ret=0
if retry_quiet 10 wait_for_tls_xfer 2 example10; then
  ret=1
else
  echo_i "timed out waiting for zone transfer"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing incoming XoT functionality (from the first secondary, MutualTLS, expired client cert, failure expected) ($n)"
ret=0
if retry_quiet 10 wait_for_tls_xfer 2 example11; then
  ret=1
else
  echo_i "timed out waiting for zone transfer"
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing incoming XoT functionality (from the second secondary) ($n)"
ret=0
if retry_quiet 10 wait_for_tls_xfer 3 example; then
  digcomp example.axfr.good "dig.out.ns3.example.test$n" || ret=1
else
  echo_i "timed out waiting for zone transfer"
  grep "^;" "dig.out.ns3.example.test$n" | cat_i
  ret=1
fi
if test $ret != 0; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing incoming XoT functionality (from the second secondary, mismatching ciphers, failure expected) ($n)"
ret=0
if retry_quiet 10 wait_for_tls_xfer 3 example2; then
  ret=1
else
  echo_i "timed out waiting for zone transfer"
fi
if test $ret != 0; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing incoming XoT functionality (from the third secondary) ($n)"
ret=0
if retry_quiet 10 wait_for_tls_xfer 4 example; then
  digcomp example.axfr.good "dig.out.ns4.example.test$n" || ret=1
else
  echo_i "timed out waiting for zone transfer"
  grep "^;" "dig.out.ns4.example.test$n" | cat_i
  ret=1
fi
if test $ret != 0; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoT query (ephemeral key) ($n)"
ret=0
dig_with_tls_opts @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoT query via IPv6 (ephemeral key) ($n)"
ret=0
dig_with_tls_opts -6 @fd92:7065:b8e:ffff::1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoT query (static key) ($n)"
ret=0
dig_with_tls_opts @10.53.0.2 example SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoT query via IPv6 (static key) ($n)"
ret=0
dig_with_tls_opts -6 @fd92:7065:b8e:ffff::2 example SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoT XFR ($n)"
ret=0
dig_with_tls_opts +comm @10.53.0.1 . AXFR >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# zone transfers are allowed only via TLS
n=$((n + 1))
echo_i "testing zone transfer over Do53 server functionality (using dig, failure expected) ($n)"
ret=0
dig_with_opts example. -b 10.53.0.10 @10.53.0.1 axfr >dig.out.ns1.test$n || ret=1
grep "; Transfer failed." dig.out.ns1.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# querying zones is still allowed via UDP/TCP
n=$((n + 1))
echo_i "checking Do53 query ($n)"
ret=0
dig_with_opts @10.53.0.1 example SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# In this test we are trying to establish a DoT connection over the
# DoH port. That is intentional, as dig should fail right after
# handshake has happened and before sending any queries, as XFRs, per
# the RFC, could happen only over a connection where "dot" ALPN token
# was negotiated.  over DoH it cannot happen, as only "h2" token could
# be selected for a DoH connection.
n=$((n + 1))
echo_i "checking DoT XFR with wrong ALPN token (h2, failure expected) ($n)"
ret=0
# shellcheck disable=SC2086
"$DIG" +tls $common_dig_options -p "${HTTPSPORT}" +comm @10.53.0.1 . AXFR >dig.out.test$n
grep "$msg_xfrs_not_allowed" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Let's try to issue an HTTP/2 query over TLS port to check if dig
# will detect ALPN token negotiation problem.
n=$((n + 1))
echo_i "checking DoH query when ALPN is expected to fail (dot, failure expected) ($n)"
ret=0
# shellcheck disable=SC2086
"$DIG" +https $common_dig_options -p "${TLSPORT}" "$@" @10.53.0.1 . SOA >dig.out.test$n && ret=1
grep "ALPN for HTTP/2 failed." dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (POST) ($n)"
ret=0
dig_with_https_opts +stat @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep -F "(HTTPS)" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query via IPv6 (POST) ($n)"
ret=0
dig_with_https_opts +stat -6 @fd92:7065:b8e:ffff::1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep -F "(HTTPS)" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (POST, static key) ($n)"
ret=0
dig_with_https_opts @10.53.0.2 example SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query via IPv6 (POST, static key) ($n)"
ret=0
dig_with_https_opts -6 @fd92:7065:b8e:ffff::2 example SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (POST, nonstandard endpoint) ($n)"
ret=0
dig_with_https_opts +https=/alter @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query via IPv6 (POST, nonstandard endpoint) ($n)"
ret=0
dig_with_https_opts -6 +https=/alter @fd92:7065:b8e:ffff::1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (POST, undefined endpoint, failure expected) ($n)"
ret=0
dig_with_https_opts +tries=1 +time=1 +https=/fake @10.53.0.1 . SOA >dig.out.test$n && ret=1
grep "communications error" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query via IPv6 (POST, undefined endpoint, failure expected) ($n)"
ret=0
dig_with_https_opts -6 +tries=1 +time=1 +https=/fake @fd92:7065:b8e:ffff::1 . SOA >dig.out.test$n && ret=1
grep "communications error" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH XFR (POST) (failure expected) ($n)"
ret=0
dig_with_https_opts +comm @10.53.0.1 . AXFR >dig.out.test$n || ret=1
grep "; Transfer failed." dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (GET) ($n)"
ret=0
dig_with_https_opts +stat +https-get @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep -F "(HTTPS-GET)" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query via IPv6 (GET) ($n)"
ret=0
dig_with_https_opts -6 +stat +https-get @fd92:7065:b8e:ffff::1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep -F "(HTTPS-GET)" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (GET, static key) ($n)"
ret=0
dig_with_https_opts +https-get @10.53.0.2 example SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query via IPv6 (GET, static key) ($n)"
ret=0
dig_with_https_opts -6 +https-get @fd92:7065:b8e:ffff::2 example SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (GET, nonstandard endpoint) ($n)"
ret=0
dig_with_https_opts +https-get=/alter @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query via IPv6 (GET, nonstandard endpoint) ($n)"
ret=0
dig_with_https_opts -6 +https-get=/alter @fd92:7065:b8e:ffff::1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (GET, undefined endpoint, failure expected) ($n)"
ret=0
dig_with_https_opts +tries=1 +time=1 +https-get=/fake @10.53.0.1 . SOA >dig.out.test$n && ret=1
grep "communications error" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query via IPv6 (GET, undefined endpoint, failure expected) ($n)"
ret=0
dig_with_https_opts -6 +tries=1 +time=1 +https-get=/fake @fd92:7065:b8e:ffff::1 . SOA >dig.out.test$n && ret=1
grep "communications error" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH XFR (GET) (failure expected) ($n)"
ret=0
dig_with_https_opts +https-get +comm @10.53.0.1 . AXFR >dig.out.test$n || ret=1
grep "; Transfer failed." dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking unencrypted DoH query (POST) ($n)"
ret=0
dig_with_http_opts +stat @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep -F "(HTTP)" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking unencrypted DoH query via IPv6 (POST) ($n)"
ret=0
dig_with_http_opts -6 +stat @fd92:7065:b8e:ffff::1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep -F "(HTTP)" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking unencrypted DoH query (GET) ($n)"
ret=0
dig_with_http_opts +stat +http-plain-get @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep -F "(HTTP-GET)" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking unencrypted DoH query via IPv6 (GET) ($n)"
ret=0
dig_with_http_opts -6 +stat +http-plain-get @fd92:7065:b8e:ffff::1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep -F "(HTTP-GET)" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking unencrypted DoH XFR (failure expected) ($n)"
ret=0
dig_with_http_opts +comm @10.53.0.1 . AXFR >dig.out.test$n || ret=1
grep "; Transfer failed." dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query for a large answer (POST) ($n)"
ret=0
dig_with_https_opts @10.53.0.1 biganswer.example A >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep "ANSWER: 2500" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query via IPv6 for a large answer (POST) ($n)"
ret=0
dig_with_https_opts -6 @fd92:7065:b8e:ffff::1 biganswer.example A >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep "ANSWER: 2500" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query for a large answer (GET) ($n)"
ret=0
dig_with_https_opts +https-get @10.53.0.1 biganswer.example A >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep "ANSWER: 2500" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query via IPv6 for a large answer (GET) ($n)"
ret=0
dig_with_https_opts -6 +https-get @fd92:7065:b8e:ffff::1 biganswer.example A >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep "ANSWER: 2500" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking unencrypted DoH query for a large answer (POST) ($n)"
ret=0
dig_with_http_opts @10.53.0.1 biganswer.example A >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep "ANSWER: 2500" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking unencrypted DoH query via IPv6 for a large answer (POST) ($n)"
ret=0
dig_with_http_opts -6 @fd92:7065:b8e:ffff::1 biganswer.example A >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep "ANSWER: 2500" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking unencrypted DoH query for a large answer (GET) ($n)"
ret=0
dig_with_http_opts +http-plain-get @10.53.0.1 biganswer.example A >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep "ANSWER: 2500" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking unencrypted DoH query via IPv6 for a large answer (GET) ($n)"
ret=0
dig_with_http_opts -6 +http-plain-get @fd92:7065:b8e:ffff::1 biganswer.example A >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
grep "ANSWER: 2500" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

wait_for_tlsctx_update_ns4() {
  grep "updating TLS context on 10.53.0.4#${HTTPSPORT}" ns4/named.run >/dev/null || return 1
  grep "updating TLS context on 10.53.0.4#${TLSPORT}" ns4/named.run >/dev/null || return 1
  return 0
}

n=$((n + 1))
echo_i "doing rndc reconfig to see that queries keep being served after that ($n)"
ret=0
rndc_reconfig ns4 10.53.0.4 60
retry_quiet 15 wait_for_tlsctx_update_ns4 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoT query after a reconfiguration ($n)"
ret=0
dig_with_tls_opts @10.53.0.4 example SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (POST) after a reconfiguration ($n)"
ret=0
dig_with_https_opts @10.53.0.4 example SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "doing rndc reconfig to see if HTTP endpoints have gotten reconfigured ($n)"
ret=0
# 'sed -i ...' is not portable. Sigh...
sed 's/\/dns-query/\/dns-query-test/g' "ns4/named.conf" >"ns4/named.conf.sed"
mv -f "ns4/named.conf.sed" "ns4/named.conf"
rndc_reconfig ns4 10.53.0.4 60
retry_quiet 15 wait_for_tlsctx_update_ns4 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (POST) to verify HTTP endpoint reconfiguration ($n)"
ret=0
dig_with_https_opts +https='/dns-query-test' @10.53.0.4 example SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoT query (with TLS verification enabled) ($n)"
ret=0
dig_with_tls_opts +tls-ca="$ca_file" +tls-hostname="srv01.crt01.example.com" @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (with TLS verification enabled, self-signed cert, failure expected) ($n)"
ret=0
dig_with_https_opts +tls-ca="$ca_file" +tls-hostname="srv01.crt01.example.com" @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "$msg_peer_verification_failed" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoT query (with TLS verification using the system's CA store, failure expected) ($n)"
ret=0
dig_with_tls_opts +tls-ca +tls-hostname="srv01.crt01.example.com" @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "$msg_peer_verification_failed" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (with TLS verification using the system's CA store, failure expected) ($n)"
ret=0
dig_with_https_opts +tls-ca +tls-hostname="srv01.crt01.example.com" @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "$msg_peer_verification_failed" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# the primary server's certificate contains the IP address in the
# SubjectAltName section
n=$((n + 1))
echo_i "checking DoT query (with TLS verification, hostname is not specified, IP address is used instead) ($n)"
ret=0
dig_with_tls_opts +tls-ca="$ca_file" @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "$msg_peer_verification_failed" dig.out.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

if [ -n "$run_san_tests" ]; then
  # SubjectAltName is required for DoT as according to RFC 8310, Subject
  # field MUST NOT be inspected when verifying hostname for DoT.
  n=$((n + 1))
  echo_i "checking DoT query (with TLS verification enabled when SubjectAltName is not set, failure expected) ($n)"
  ret=0
  dig_with_tls_opts +tls-ca="$ca_file" +tls-hostname="srv01.crt02-no-san.example.com" @10.53.0.1 . SOA >dig.out.test$n || ret=1
  grep "$msg_peer_verification_failed" dig.out.test$n >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking DoT XFR over a TLS port where SubjectAltName is not set (failure expected) ($n)"
  ret=0
  # shellcheck disable=SC2086
  dig_with_tls_opts +tls-ca="$ca_file" +tls-hostname="srv01.crt02-no-san.example.com" -p "${EXTRAPORT2}" +comm @10.53.0.1 . AXFR >dig.out.test$n || ret=1
  grep "$msg_peer_verification_failed" dig.out.test$n >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

# SubjectAltName is not required for HTTPS. Having a properly set
# Common Name in the Subject field is enough.
n=$((n + 1))
echo_i "checking DoH query (when SubjectAltName is not set) ($n)"
ret=0
dig_with_https_opts +tls-ca="$ca_file" +tls-hostname="srv01.crt02-no-san.example.com" -p "${EXTRAPORT3}" +comm @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoT query (expired certificate, Opportunistic TLS) ($n)"
ret=0
dig_with_tls_opts +tls -p "${EXTRAPORT4}" +comm @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoT query (expired certificate, Strict TLS, failure expected) ($n)"
ret=0
dig_with_tls_opts +tls-ca="$ca_file" -p "${EXTRAPORT4}" +comm @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "$msg_peer_verification_failed" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing XoT server functionality (using dig, client certificate required, failure expected) ($n)"
ret=0
dig_with_tls_opts +tls-ca="$ca_file" -p "${EXTRAPORT5}" example8. -b 10.53.0.10 @10.53.0.1 axfr >dig.out.ns1.test$n || ret=1
grep "; Transfer failed." dig.out.ns1.test$n >/dev/null || ret=1
if test $ret != 0; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "testing XoT server functionality (using dig, client certificate used) ($n)"
ret=0
dig_with_tls_opts +tls-ca="$ca_file" +tls-certfile="./CA/certs/srv01.client01.example.com.pem" +tls-keyfile="./CA/certs/srv01.client01.example.com.key" -p "${EXTRAPORT5}" example8. -b 10.53.0.10 @10.53.0.1 axfr >dig.out.ns1.test$n || ret=1
digcomp dig.out.ns1.test$n example8.axfr.good >/dev/null || ret=1
if test $ret != 0; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (client certificate required, failure expected) ($n)"
ret=0
dig_with_https_opts +tls-ca="$ca_file" -p "${EXTRAPORT6}" +comm @10.53.0.1 . SOA >dig.out.test$n && ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DoH query (client certificate used) ($n)"
ret=0
# shellcheck disable=SC2086
dig_with_https_opts +https +tls-ca="$ca_file" +tls-certfile="./CA/certs/srv01.client01.example.com.pem" +tls-keyfile="./CA/certs/srv01.client01.example.com.key" -p "${EXTRAPORT6}" +comm @10.53.0.1 . SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# send two requests one after another so that session resumption will happen
n=$((n + 1))
echo_i "checking DoH query (client certificate used - session resumption when using Mutual TLS) ($n)"
ret=0
# shellcheck disable=SC2086
dig_with_https_opts +https +tls-ca="$ca_file" +tls-certfile="./CA/certs/srv01.client01.example.com.pem" +tls-keyfile="./CA/certs/srv01.client01.example.com.key" -p "${EXTRAPORT6}" +comm @10.53.0.1 . SOA . SOA >dig.out.test$n || ret=1
grep "TLS error" dig.out.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

test_opcodes() {
  EXPECT_STATUS="$1"
  shift
  for op in "$@"; do
    n=$((n + 1))
    echo_i "checking unexpected opcode query over DoH for opcode $op ($n)"
    ret=0
    dig_with_https_opts +https @10.53.0.1 +opcode="$op" >dig.out.test$n || ret=1
    grep "status: $EXPECT_STATUS" dig.out.test$n >/dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "checking unexpected opcode query over DoH via IPv6 for opcode $op ($n)"
    ret=0
    dig_with_https_opts -6 +https @fd92:7065:b8e:ffff::1 +opcode="$op" >dig.out.test$n || ret=1
    grep "status: $EXPECT_STATUS" dig.out.test$n >/dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "checking unexpected opcode query over DoH without encryption for opcode $op ($n)"
    ret=0
    dig_with_http_opts +http-plain @10.53.0.1 +opcode="$op" >dig.out.test$n || ret=1
    grep "status: $EXPECT_STATUS" dig.out.test$n >/dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "checking unexpected opcode query over DoH via IPv6 without encryption for opcode $op ($n)"
    ret=0
    dig_with_http_opts -6 +http-plain @fd92:7065:b8e:ffff::1 +opcode="$op" >dig.out.test$n || ret=1
    grep "status: $EXPECT_STATUS" dig.out.test$n >/dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "checking unexpected opcode query over DoT for opcode $op ($n)"
    ret=0
    dig_with_tls_opts +tls @10.53.0.1 +opcode="$op" >dig.out.test$n || ret=1
    grep "status: $EXPECT_STATUS" dig.out.test$n >/dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))

    n=$((n + 1))
    echo_i "checking unexpected opcode query over DoT via IPv6 for opcode $op ($n)"
    ret=0
    dig_with_tls_opts -6 +tls @fd92:7065:b8e:ffff::1 +opcode="$op" >dig.out.test$n || ret=1
    grep "status: $EXPECT_STATUS" dig.out.test$n >/dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status + ret))
  done
}

test_opcodes NOERROR 0
test_opcodes NOTIMP 1 2 3 6 7 8 9 10 11 12 13 14 15
test_opcodes FORMERR 4 5

n=$((n + 1))
echo_i "checking server quotas for both encrypted and unencrypted HTTP ($n)"
ret=0
BINDHOST="10.53.0.1" "$PYTHON" "$TOP_SRCDIR/bin/tests/system/doth/stress_http_quota.py" || ret=$?
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# check whether we can use curl for sending test queries.
if [ -x "${CURL}" ]; then
  CURL_HTTP2="$(${CURL} --version | grep -E '^Features:.* HTTP2( |$)' || true)"

  if [ -n "$CURL_HTTP2" ]; then
    testcurl=1
  else
    echo_i "The available version of CURL does not have HTTP/2 support"
  fi
fi

# Note: see README.curl for information on how to generate curl
# queries.
if [ -n "$testcurl" ]; then
  n=$((n + 1))
  echo_i "checking max-age for positive answer ($n)"
  ret=0
  # use curl to query for 'example/SOA'
  $CURL -kD headers.$n "https://10.53.0.1:${HTTPSPORT}/dns-query?dns=AAEAAAABAAAAAAAAB2V4YW1wbGUAAAYAAQ" >/dev/null 2>&1 || ret=1
  grep "cache-control: max-age=86400" headers.$n >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))

  n=$((n + 1))
  echo_i "checking max-age for negative answer ($n)"
  ret=0
  # use curl to query for 'fake.example/TXT'
  $CURL -kD headers.$n "https://10.53.0.1:${HTTPSPORT}/dns-query?dns=AAEAAAABAAAAAAAABGZha2UHZXhhbXBsZQAAEAAB" >/dev/null 2>&1 || ret=1
  grep "cache-control: max-age=3600" headers.$n >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

n=$((n + 1))
echo_i "checking Do53 query to NS5 for zone \"example12\" (verifying successful client TLS context reuse by the NS5 server instance during XoT) ($n)"
ret=0
dig_with_opts +comm @10.53.0.5 example12 SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking Do53 query to NS5 for zone \"example13\" (verifying successful client TLS context reuse by the NS5 server instance during XoT) ($n)"
ret=0
dig_with_opts +comm @10.53.0.5 example13 SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking Do53 query to NS5 for zone \"example14\" (verifying successful client TLS context reuse by the NS5 server instance during XoT) ($n)"
ret=0
dig_with_opts +comm @10.53.0.5 example14 SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking Do53 query to NS5 for zone \"example15\" (verifying successful client TLS context reuse by the NS5 server instance during XoT) ($n)"
ret=0
dig_with_opts +comm @10.53.0.5 example15 SOA >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# see GL #4572
n=$((n + 1))
echo_i "testing that zone forwarding fails when using a wrong TLS configuration on the server without aborting it (a condition for bug #4572, failure expected) ($n)"
ret=0
dig_with_opts test.example.com. -b 10.53.0.10 @10.53.0.2 >dig.out.test$n || ret=1
grep "status: SERVFAIL" dig.out.test$n >/dev/null || ret=1
if test $ret != 0; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
