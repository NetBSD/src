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

# touch dnsrps-off to not test with DNSRPS

set -e

. ../conf.sh

USAGE="$0: [-DNx]"
DEBUG=
while getopts "DNx" c; do
  case $c in
    x)
      set -x
      DEBUG=-x
      ;;
    D) TEST_DNSRPS="-D" ;;
    N) NOCLEAN=set ;;
    *)
      echo "$USAGE" 1>&2
      exit 1
      ;;
  esac
done
shift $((OPTIND - 1))
if test "$#" -ne 0; then
  echo "$USAGE" 1>&2
  exit 1
fi

[ ${NOCLEAN:-unset} = unset ] && $SHELL clean.sh $DEBUG

$PERL testgen.pl

copy_setports ns1/named.conf.in ns1/named.conf

copy_setports ns2/named.conf.header.in ns2/named.conf.header
copy_setports ns2/named.default.conf ns2/named.conf

copy_setports ns3/named1.conf.in ns3/named.conf

copy_setports ns4/named.conf.in ns4/named.conf

# setup policy zones for a 64-zone test
i=1
while test $i -le 64; do
  echo "\$TTL 60" >ns2/db.max$i.local
  echo "@ IN SOA root.ns ns 1996072700 3600 1800 86400 60" >>ns2/db.max$i.local
  echo "     NS ns" >>ns2/db.max$i.local
  echo "ns   A 127.0.0.1" >>ns2/db.max$i.local

  j=1
  while test $j -le $i; do
    echo "name$j A 10.53.0.$i" >>ns2/db.max$i.local
    j=$((j + 1))
  done
  i=$((i + 1))
done

# decide whether to test DNSRPS
$SHELL ../ckdnsrps.sh $TEST_DNSRPS $DEBUG
test -z "$(grep 'dnsrps-enable yes' dnsrps.conf)" && TEST_DNSRPS=

CWD=$(pwd)
cat <<EOF >dnsrpzd.conf
PID-FILE $CWD/dnsrpzd.pid;

include $CWD/dnsrpzd-license-cur.conf

zone "policy" { type primary; file "$(pwd)/ns3/policy.db"; };
EOF
sed -n -e 's/^ *//' -e "/zone.*.*primary/s@file \"@&$CWD/ns2/@p" ns2/*.conf \
  >>dnsrpzd.conf

# Run dnsrpzd to get the license and prime the static policy zones
if test -n "$TEST_DNSRPS"; then
  DNSRPZD="$(../rpz/dnsrps -p)"
  "$DNSRPZD" -D./dnsrpzd.rpzf -S./dnsrpzd.sock -C./dnsrpzd.conf \
    -w 0 -dddd -L stdout >./dnsrpzd.run 2>&1
fi
