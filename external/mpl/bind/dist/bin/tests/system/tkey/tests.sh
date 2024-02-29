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

dig_with_opts() {
  "$DIG" @10.53.0.1 -p "$PORT" "$@"
}

status=0
n=1

echo_i "generating new DH key ($n)"
ret=0
dhkeyname=$($KEYGEN -T KEY -a DH -b 768 -n host client) || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
  echo_i "exit status: $status"
  exit $status
fi
status=$((status + ret))
n=$((n + 1))

for owner in . foo.example.; do
  echo_i "creating new key using owner name \"$owner\" ($n)"
  ret=0
  keyname=$($KEYCREATE 10.53.0.1 "$PORT" "$dhkeyname" $owner) || ret=1
  if [ $ret != 0 ]; then
    echo_i "failed"
    status=$((status + ret))
    echo_i "exit status: $status"
    exit $status
  fi
  status=$((status + ret))
  n=$((n + 1))

  echo_i "checking the new key ($n)"
  ret=0
  dig_with_opts txt txt.example -k "$keyname" >dig.out.test$n || ret=1
  grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
  grep "TSIG.*hmac-md5.*NOERROR" dig.out.test$n >/dev/null || ret=1
  grep "Some TSIG could not be validated" dig.out.test$n >/dev/null && ret=1
  if [ $ret != 0 ]; then
    echo_i "failed"
  fi
  status=$((status + ret))
  n=$((n + 1))

  echo_i "deleting new key ($n)"
  ret=0
  $KEYDELETE 10.53.0.1 "$PORT" "$keyname" || ret=1
  if [ $ret != 0 ]; then
    echo_i "failed"
  fi
  status=$((status + ret))
  n=$((n + 1))

  echo_i "checking that new key has been deleted ($n)"
  ret=0
  dig_with_opts txt txt.example -k "$keyname" >dig.out.test$n || ret=1
  grep "status: NOERROR" dig.out.test$n >/dev/null && ret=1
  grep "TSIG.*hmac-md5.*NOERROR" dig.out.test$n >/dev/null && ret=1
  grep "Some TSIG could not be validated" dig.out.test$n >/dev/null || ret=1
  if [ $ret != 0 ]; then
    echo_i "failed"
  fi
  status=$((status + ret))
  n=$((n + 1))
done

echo_i "creating new key using owner name bar.example. ($n)"
ret=0
keyname=$($KEYCREATE 10.53.0.1 "$PORT" "$dhkeyname" bar.example.) || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
  echo_i "exit status: $status"
  exit $status
fi
status=$((status + ret))
n=$((n + 1))

echo_i "checking the key with 'rndc tsig-list' ($n)"
ret=0
$RNDC -c ../_common/rndc.conf -s 10.53.0.1 -p "$CONTROLPORT" tsig-list >rndc.out.test$n
grep "key \"bar.example.server" rndc.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
fi
status=$((status + ret))
n=$((n + 1))

echo_i "using key in a request ($n)"
ret=0
dig_with_opts -k "$keyname" txt.example txt >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
fi
status=$((status + ret))
n=$((n + 1))

echo_i "deleting the key with 'rndc tsig-delete' ($n)"
ret=0
$RNDC -c ../_common/rndc.conf -s 10.53.0.1 -p "$CONTROLPORT" tsig-delete bar.example.server >/dev/null || ret=1
$RNDC -c ../_common/rndc.conf -s 10.53.0.1 -p "$CONTROLPORT" tsig-list >rndc.out.test$n
grep "key \"bar.example.server" rndc.out.test$n >/dev/null && ret=1
dig_with_opts -k "$keyname" txt.example txt >dig.out.test$n || ret=1
grep "TSIG could not be validated" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
fi
status=$((status + ret))
n=$((n + 1))

echo_i "recreating the bar.example. key ($n)"
ret=0
keyname=$($KEYCREATE 10.53.0.1 "$PORT" "$dhkeyname" bar.example.) || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=$((status + ret))
  echo_i "exit status: $status"
  exit $status
fi
status=$((status + ret))
n=$((n + 1))

echo_i "checking the new key with 'rndc tsig-list' ($n)"
ret=0
$RNDC -c ../_common/rndc.conf -s 10.53.0.1 -p "$CONTROLPORT" tsig-list >rndc.out.test$n
grep "key \"bar.example.server" rndc.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
fi
status=$((status + ret))
n=$((n + 1))

echo_i "using the new key in a request ($n)"
ret=0
dig_with_opts -k "$keyname" txt.example txt >dig.out.test$n || ret=1
grep "status: NOERROR" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
fi
status=$((status + ret))
n=$((n + 1))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
