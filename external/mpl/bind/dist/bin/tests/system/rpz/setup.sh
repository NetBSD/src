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

for dir in ns*; do
  touch $dir/named.run
  nextpart $dir/named.run >/dev/null
done

touch dnsrps.conf
touch dnsrps.cache

# set up test policy zones.
#   bl is the main test zone
#   bl-2 is used to check competing zones.
#   bl-{given,disabled,passthru,no-data,nxdomain,cname,wildcard,garden,
#	    drop,tcp-only} are used to check policy overrides in named.conf.
#   NO-OP is an obsolete synonym for PASSHTRU
for NM in '' -2 -given -disabled -passthru -no-op -nodata -nxdomain -cname -wildcname -garden -drop -tcp-only; do
  sed -e "/SOA/s/blx/bl$NM/g" ns3/base.db >ns3/bl$NM.db
done
#  bl zones are dynamically updated.  Add one zone that is updated manually.
cp ns3/manual-update-rpz.db.in ns3/manual-update-rpz.db
cp ns8/manual-update-rpz.db.in ns8/manual-update-rpz.db

cp ns3/evil-cname.db.in ns3/evil-cname.db
cp ns3/wild-cname.db.in ns3/wild-cname.db

cp ns3/mixed-case-rpz-1.db.in ns3/mixed-case-rpz.db

cp ns3/include-rpz.db.in ns3/include-rpz.db
cp ns3/include-rpz.inc-1.in ns3/include-rpz.inc

# a "big" zone (tested with '-T rpzslow' enabled to slow down loading)
cp ns3/slow-rpz.db.in ns3/slow-rpz.db

# a zone that expires quickly and then can't be refreshed
cp ns5/fast-expire.db.in ns5/fast-expire.db
cp ns5/expire.conf.in ns5/expire.conf

# $1=directory
# $2=domain name
# $3=input zone file
# $4=output file
signzone() {
  KEYNAME=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -K $1 $2)
  cat $1/$3 $1/$KEYNAME.key >$1/tmp
  $SIGNER -P -K $1 -o $2 -f $1/$4 $1/tmp >/dev/null
  sed -n -e 's/\(.*\) IN DNSKEY \([0-9]\{1,\} [0-9]\{1,\} [0-9]\{1,\}\) \(.*\)/trust-anchors {"\1" static-key \2 "\3";};/p' $1/$KEYNAME.key >>trusted.conf
  DSFILENAME=dsset-${2}.
  rm $DSFILENAME $1/tmp
}
signzone ns2 tld2s base-tld2s.db tld2s.db

# Performance and a few other checks.
cat <<EOF >ns5/rpz-switch
response-policy {
	zone "bl0"; zone "bl1"; zone "bl2"; zone "bl3"; zone "bl4";
	zone "bl5"; zone "bl6"; zone "bl7"; zone "bl8"; zone "bl9";
	zone "bl10"; zone "bl11"; zone "bl12"; zone "bl13"; zone "bl14";
	zone "bl15"; zone "bl16"; zone "bl17"; zone "bl18"; zone "bl19";
    } recursive-only no
    qname-wait-recurse no
    nsip-enable yes
    nsdname-enable yes
    max-policy-ttl 90
    break-dnssec yes
    ;
EOF

cat <<EOF >ns5/example.db
\$TTL	300
@	SOA	.  hostmaster.ns.example.tld5. ( 1 3600 1200 604800 60 )
	NS	ns
	NS	ns1
ns	A	10.53.0.5
ns1	A	10.53.0.5
EOF

cat <<EOF >ns5/bl.db
\$TTL	300
@		SOA	.  hostmaster.ns.blperf. ( 1 3600 1200 604800 60 )
		NS	ns.tld5.

; for "qname-wait-recurse no" in #35 test1
x.servfail	A	35.35.35.35
; for "recursive-only no" in #8 test5
a3-5.tld2	CNAME	.
; for "break-dnssec" in #9 & #10 test5
a3-5.tld2s	CNAME	.
; for "max-policy-ttl 90" in #17 test5
a3-17.tld2	500 A	17.17.17.17

; dummy NSDNAME policy to trigger lookups
ns1.x.rpz-nsdname	CNAME	.
EOF

cp ns2/bl.tld2.db.in ns2/bl.tld2.db
cp ns5/empty.db.in ns5/empty.db
cp ns5/empty.db.in ns5/policy2.db
cp ns6/bl.tld2s.db.in ns6/bl.tld2s.db
