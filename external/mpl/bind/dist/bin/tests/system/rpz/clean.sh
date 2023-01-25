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

# Clean up after rpz tests.

USAGE="$0: [-Px]"
DEBUG=
while getopts "Px" c; do
    case $c in
	x) set -x ;;
	P) PARTIAL=set ;;
	*) echo "$USAGE" 1>&2; exit 1;;
    esac
done
shift `expr $OPTIND - 1 || true`
if test "$#" -ne 0; then
    echo "$USAGE" 1>&2
    exit 1
fi

# this might be called from setup.sh to partially clean up the files
# from the first test pass so the second pass can be set up correctly.
# remove those files first, then decide whether to remove the others.
rm -f ns*/*.key ns*/*.private
rm -f ns2/tld2s.db */bl.tld2.db */bl.tld2s.db
rm -f ns3/bl*.db ns3/fast-expire.db ns*/empty.db
rm -f ns3/manual-update-rpz.db
rm -f ns3/mixed-case-rpz.db
rm -f ns5/example.db ns5/bl.db ns5/fast-expire.db ns5/expire.conf
rm -f ns8/manual-update-rpz.db
rm -f */policy2.db
rm -f */*.jnl

if [ ${PARTIAL:-unset} = unset ]; then
    rm -f proto.* dsset-* trusted.conf dig.out* nsupdate.tmp ns*/*tmp
    rm -f ns5/requests ns5/*.perf
    rm -f */named.memstats */*.run */*.run.prev */named.stats */session.key
    rm -f */*.log */*core */*.pid
    rm -f ns*/named.lock
    rm -f ns*/named.conf
    rm -f ns*/*switch
    rm -f dnsrps*.conf
    rm -f dnsrpzd.conf
    rm -f dnsrpzd-license-cur.conf dnsrpzd.rpzf dnsrpzd.sock dnsrpzd.pid
    rm -f ns*/managed-keys.bind*
    rm -f tmp
fi
