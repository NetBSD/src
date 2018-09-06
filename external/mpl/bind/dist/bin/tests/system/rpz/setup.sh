#! /bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# touch dnsrps-off to not test with DNSRPS

set -e

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

QPERF=`$SHELL qperf.sh`

USAGE="$0: [-Dx]"
DEBUG=
while getopts "Dx" c; do
    case $c in
	x) set -x; DEBUG=-x;;
        D) TEST_DNSRPS="-D";;
	N) NOCLEAN=set;;
	*) echo "$USAGE" 1>&2; exit 1;;
    esac
done
shift `expr $OPTIND - 1 || true`
if test "$#" -ne 0; then
    echo "$USAGE" 1>&2
    exit 1
fi

[ ${NOCLEAN:-unset} = unset ] && $SHELL clean.sh $DEBUG

copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns2/named.conf.in ns2/named.conf
copy_setports ns3/named.conf.in ns3/named.conf
copy_setports ns4/named.conf.in ns4/named.conf
copy_setports ns5/named.conf.in ns5/named.conf
copy_setports ns6/named.conf.in ns6/named.conf
copy_setports ns7/named.conf.in ns7/named.conf

copy_setports dnsrpzd.conf.in dnsrpzd.conf

# decide whether to test DNSRPS
# Note that dnsrps.conf and dnsrps-slave.conf are included in named.conf
# and differ from dnsrpz.conf which is used by dnsrpzd.
$SHELL ../rpz/ckdnsrps.sh -A $TEST_DNSRPS $DEBUG
test -z "`grep 'dnsrps-enable yes' dnsrps.conf`" && TEST_DNSRPS=

# set up test policy zones.
#   bl is the main test zone
#   bl-2 is used to check competing zones.
#   bl-{given,disabled,passthru,no-data,nxdomain,cname,wildcard,garden,
#	    drop,tcp-only} are used to check policy overrides in named.conf.
#   NO-OP is an obsolete synonym for PASSHTRU
for NM in '' -2 -given -disabled -passthru -no-op -nodata -nxdomain -cname -wildcname -garden -drop -tcp-only; do
    sed -e "/SOA/s/blx/bl$NM/g" ns3/base.db >ns3/bl$NM.db
done

# sign the root and a zone in ns2
test -r $RANDFILE || $GENRANDOM 800 $RANDFILE

# $1=directory, $2=domain name, $3=input zone file, $4=output file
signzone () {
    KEYNAME=`$KEYGEN -q -a rsasha256 -r $RANDFILE -K $1 $2`
    cat $1/$3 $1/$KEYNAME.key > $1/tmp
    $SIGNER -Pp -K $1 -o $2 -f $1/$4 $1/tmp >/dev/null
    sed -n -e 's/\(.*\) IN DNSKEY \([0-9]\{1,\} [0-9]\{1,\} [0-9]\{1,\}\) \(.*\)/trusted-keys {"\1" \2 "\3";};/p' $1/$KEYNAME.key >>trusted.conf
    DSFILENAME=dsset-`echo $2 |sed -e "s/\.$//g"`$TP
    rm $DSFILENAME $1/tmp
}
signzone ns2 tld2s. base-tld2s.db tld2s.db


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

if test -n "$QPERF"; then
    # Do not build the full zones if we will not use them.
    $PERL -e 'for ($val = 1; $val <= 65535; ++$val) {
	printf("host-%05d\tA    192.168.%d.%d\n", $val, $val/256, $val%256);
	}' >>ns5/example.db

    echo >>ns5/bl.db
    echo "; rewrite some names" >>ns5/bl.db
    $PERL -e 'for ($val = 2; $val <= 65535; $val += 69) {
	printf("host-%05d.example.tld5\tCNAME\t.\n", $val);
	}' >>ns5/bl.db

    echo >>ns5/bl.db
    echo "; rewrite with some not entirely trivial patricia trees" >>ns5/bl.db
    $PERL -e 'for ($val = 3; $val <= 65535; $val += 69) {
	printf("32.%d.%d.168.192.rpz-ip  \tCNAME\t.\n",
		$val%256, $val/256);
	}' >>ns5/bl.db
fi

# some psuedo-random queryperf requests
$PERL -e 'for ($cnt = $val = 1; $cnt <= 3000; ++$cnt) {
	printf("host-%05d.example.tld5 A\n", $val);
	$val = ($val * 9 + 32771) % 65536;
	}' >ns5/requests

cp ns2/bl.tld2.db.in ns2/bl.tld2.db
cp ns5/empty.db.in ns5/empty.db
cp ns5/empty.db.in ns5/policy2.db
rm -f ns2/bl.tld2.db.jnl
rm -f ns5/empty.db.jnl
rm -f cp ns5/policy2.db.jnl

# Run dnsrpzd to get the license and prime the static policy zones
if test -n "$TEST_DNSRPS"; then
   DNSRPZD="`../rpz/dnsrps -p`"
   cd ns3
   "$DNSRPZ" -D../dnsrpzd.rpzf -S../dnsrpzd.sock -C../dnsrpzd.conf \
             -w 0 -dddd -L stdout >./dnsrpzd.run 2>&1
fi
