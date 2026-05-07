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

$PERL testgen.pl

touch dnsrps.conf
touch dnsrps.cache

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
