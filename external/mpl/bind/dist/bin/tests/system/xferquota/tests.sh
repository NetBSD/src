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

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"
RNDCCMD="$RNDC -c ../_common/rndc.conf -p ${CONTROLPORT} -s"

#
# Perform tests
#

count=0
ticks=0
while [ $count != 300 ]; do
  if [ $ticks = 1 ]; then
    echo_i "Changing test zone..."
    cp -f ns1/changing2.db ns1/changing.db
    kill -HUP $(cat ns1/named.pid)
  fi
  sleep 1
  ticks=$((ticks + 1))
  seconds=$((ticks * 1))
  if [ $ticks = 360 ]; then
    echo_i "Took too long to load zones"
    exit 1
  fi
  count=$(cat ns2/zone*.bk | grep xyzzy | wc -l)
  echo_i "Have $count zones up in $seconds seconds"
done

status=0

$DIG $DIGOPTS zone000099.example. @10.53.0.1 axfr >dig.out.ns1 || status=1

$DIG $DIGOPTS zone000099.example. @10.53.0.2 axfr >dig.out.ns2 || status=1

digcomp dig.out.ns1 dig.out.ns2 || status=1

sleep 15

$DIG $DIGOPTS a.changing. @10.53.0.1 a >dig.out.ns1 || status=1

$DIG $DIGOPTS a.changing. @10.53.0.2 a >dig.out.ns2 || status=1

digcomp dig.out.ns1 dig.out.ns2 || status=1

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
