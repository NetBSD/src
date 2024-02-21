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
testing="testing allow-transfer transport ACL functionality"

dig_with_opts() {
  # shellcheck disable=SC2086
  "$DIG" +noadd +nosea +nostat +noquest +nocmd "$@"
}

status=0
n=0

run_dig_test() {
  test_message="$1"
  shift
  n=$((n + 1))
  echo_i "$test_message ($n)"
  ret=0
  dig_with_opts "$@" >"$dig_out_basename$n" || ret=1
}

run_dig_expect_axfr_success() {
  run_dig_test "$@"
  grep "; Transfer failed" "$dig_out_basename$n" >/dev/null && ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
}

run_dig_expect_axfr_failure() {
  run_dig_test "$@"
  grep "; Transfer failed" "$dig_out_basename$n" >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
}

# generic tests
run_dig_expect_axfr_success "$testing for XoT" -p "${TLSPORT}" +tls -b 10.53.0.10 @10.53.0.1 axfr example0

run_dig_expect_axfr_failure "$testing XFR via TCP (failure expected)" -p "${PORT}" +tcp -b 10.53.0.10 @10.53.0.1 axfr example0

# 1. Test allow-transfer port X, transfer works with TCP and TLS on port X but not port Y.

run_dig_expect_axfr_success "$testing for XFR via TCP" -p "${EXTRAPORT1}" +tcp -b 10.53.0.10 @10.53.0.1 axfr example1

run_dig_expect_axfr_success "$testing for XoT" -p "${EXTRAPORT1}" +tls -b 10.53.0.10 @10.53.0.2 axfr example1

run_dig_expect_axfr_failure "$testing for XFR via TCP (failure expected)" -p "${EXTRAPORT2}" +tcp -b 10.53.0.10 @10.53.0.1 axfr example1

run_dig_expect_axfr_failure "$testing for XoT (failure expected)" -p "${EXTRAPORT2}" +tls -b 10.53.0.10 @10.53.0.2 axfr example1

# 2. Test allow-transfer transport tcp, transfer works with TCP on any port but not TLS.

run_dig_expect_axfr_success "$testing for XFR via TCP" -p "${EXTRAPORT1}" +tcp -b 10.53.0.10 @10.53.0.3 axfr example2

run_dig_expect_axfr_success "$testing for XFR via TCP" -p "${EXTRAPORT2}" +tcp -b 10.53.0.10 @10.53.0.3 axfr example2

run_dig_expect_axfr_failure "$testing for XoT (failure expected)" -p "${EXTRAPORT1}" +tls -b 10.53.0.10 @10.53.0.4 axfr example2

run_dig_expect_axfr_failure "$testing for XoT (failure expected)" -p "${EXTRAPORT2}" +tls -b 10.53.0.10 @10.53.0.4 axfr example2

# 3. Test allow-transfer transport tls, transfer works with TLS on any port but not TCP.
run_dig_expect_axfr_success "$testing for XoT" -p "${EXTRAPORT3}" +tls -b 10.53.0.10 @10.53.0.3 axfr example3

run_dig_expect_axfr_success "$testing for XoT" -p "${EXTRAPORT4}" +tls -b 10.53.0.10 @10.53.0.3 axfr example3

run_dig_expect_axfr_failure "$testing for XFR via TCP (failure expected)" -p "${EXTRAPORT3}" +tcp -b 10.53.0.10 @10.53.0.4 axfr example3

run_dig_expect_axfr_failure "$testing for XFR via TCP (failure expected)" -p "${EXTRAPORT4}" +tcp -b 10.53.0.10 @10.53.0.4 axfr example3

# 4. Test allow-transfer port X transport tcp, transfer works with TCP on port X but not port Y and not with TLS on port X.

run_dig_expect_axfr_success "$testing for XFR via TCP" -p "${EXTRAPORT1}" +tcp -b 10.53.0.10 @10.53.0.5 axfr example4

run_dig_expect_axfr_failure "$testing for XFR via TCP (failure expected)" -p "${EXTRAPORT2}" +tcp -b 10.53.0.10 @10.53.0.5 axfr example4

run_dig_expect_axfr_failure "$testing for XoT (failure expected)" -p "${EXTRAPORT1}" +tls -b 10.53.0.10 @10.53.0.6 axfr example4

# 5. Test allow-transfer port X transport tls, transfer works with TLS on port X but not port Y and not with TCP on port X.

run_dig_expect_axfr_success "$testing for XoT" -p "${EXTRAPORT3}" +tls -b 10.53.0.10 @10.53.0.1 axfr example5

run_dig_expect_axfr_failure "$testing for XoT (failure expected)" -p "${EXTRAPORT4}" +tls -b 10.53.0.10 @10.53.0.1 axfr example5

run_dig_expect_axfr_failure "$testing for XFR via TCP (failure expected)" -p "${EXTRAPORT3}" +tcp -b 10.53.0.10 @10.53.0.2 axfr example5

# 6. Test with multiple allow-transfer available, first ACL is a match.
run_dig_expect_axfr_success "$testing for XFR via TCP" -p "${EXTRAPORT5}" +tcp -b 10.53.0.7 @10.53.0.1 axfr example6

run_dig_expect_axfr_failure "$testing for XFR via TCP (failure expected)" -p "${EXTRAPORT5}" +tcp -b 10.53.0.6 @10.53.0.1 axfr example6

# 7. Test with multiple allow-transfer available, last ACL is a match.
run_dig_expect_axfr_success "$testing for XoT" -p "${EXTRAPORT6}" +tls -b 10.53.0.9 @10.53.0.1 axfr example7

run_dig_expect_axfr_failure "$testing for XoT (failure expected)" -p "${EXTRAPORT6}" +tls -b 10.53.0.6 @10.53.0.1 axfr example7

# 8. Test with multiple allow-transfer available, no ACL is a match.
run_dig_expect_axfr_failure "$testing for XoT (failure expected)" -p "${EXTRAPORT7}" +tls -b 10.53.0.7 @10.53.0.1 axfr example8

# 9. Test with multiple allow-transfer available, negated ACL is used.
run_dig_expect_axfr_success "$testing for XFR via TCP" -p "${EXTRAPORT8}" +tcp -b 10.53.0.7 @10.53.0.1 axfr example9

run_dig_expect_axfr_failure "$testing for XoT (failure expected)" -p "${EXTRAPORT8}" +tcp -b 10.53.0.8 @10.53.0.1 axfr example9

run_dig_expect_axfr_success "$testing for XFR via TCP" -p "${EXTRAPORT8}" +tcp -b 10.53.0.9 @10.53.0.1 axfr example9

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
