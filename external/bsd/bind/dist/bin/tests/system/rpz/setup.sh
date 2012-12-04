#! /bin/sh
#
# Copyright (C) 2011, 2012  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

# Id: setup.sh,v 1.6 2012/01/07 23:46:53 tbox Exp 

set -e

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

QPERF=`sh qperf.sh`

sh clean.sh

# set up test policy zones.  bl-2 is used to check competing zones.
#	bl-{given,disabled,passthru,no-data,nxdomain,cname,wildcard,garden}
#	are used to check policy overrides in named.conf.
#	NO-OP is an obsolete synonym for PASSHTRU
for NM in '' -2 -given -disabled -passthru -no-op -nodata -nxdomain -cname -wildcname -garden; do
    sed -e "/SOA/s/blx/bl$NM/g" ns3/base.db >ns3/bl$NM.db
done

# sign the root and a zone in ns2
../../../tools/genrandom 400 random.data

# $1=directory, $2=domain name, $3=input zone file, $4=output file
signzone () {
    KEYNAME=`$KEYGEN -q -r random.data -b 512 -K $1 $2`
    cat $1/$3 $1/$KEYNAME.key > $1/tmp
    $SIGNER -Pp -K $1 -o $2 -f $1/$4 $1/tmp >/dev/null
    sed -n -e 's/\(.*\) IN DNSKEY \([0-9]\{1,\} [0-9]\{1,\} [0-9]\{1,\}\) \(.*\)/trusted-keys {"\1" \2 "\3";};/p' $1/$KEYNAME.key >>trusted.conf
    rm dsset-$2 $1/tmp
}
signzone ns2 tld2s. base-tld2s.db tld2s.db


# Performance checks.
# First with rpz off.
cat <<EOF >ns5/rpz-switch
response-policy {zone "bl";}
    recursive-only no
    max-policy-ttl 90
    break-dnssec yes;
EOF

cat <<EOF >ns5/example.db
\$TTL	120
@	SOA	.  hostmaster.ns.example. ( 1 3600 1200 604800 60 )
	NS	ns
ns	A	10.53.0.5
EOF

cat <<EOF >ns5/bl.db
\$TTL	120
@		SOA	.  hostmaster.ns.blperf. ( 1 3600 1200 604800 60 )
		NS	ns
ns		A	10.53.0.5

; used only in failure for "recursive-only no" in #8 test5
a3-5.tld2	CNAME	*.
; for "break-dnssec" in #9 test5
a3-5.tld2s	CNAME	*.
; for "max-policy-ttl 90" in test5
a3-17.tld2	500 A	17.17.17.17

; dummy NSDNAME policies to trigger lookups
ns-1.example.com.rpz-nsdname	CNAME	.
ns-2.example.com.rpz-nsdname	CNAME	.
ns-3.example.com.rpz-nsdname	CNAME	.
ns-4.example.com.rpz-nsdname	CNAME	.
ns-5.example.com.rpz-nsdname	CNAME	.
EOF

if test -n "$QPERF"; then
    # do not build the full zones if we will not use them to avoid the long
    # time otherwise required to shut down the server
    $PERL -e 'for ($val = 1; $val <= 65535; ++$val) {
	printf("host-%d-%d\tA    192.168.%d.%d\n",
		$val/256, $val%256, $val/256, $val%256);
	}' >>ns5/example.db

    echo >>ns5/bl.db
    echo "; rewrite some names" >>ns5/bl.db
    $PERL -e 'for ($val = 2; $val <= 65535; $val += 69) {
	printf("host-%d.sub%d.example.com\tCNAME\t.\n", $val/256, $val%256);
	}' >>ns5/bl.db

    echo >>ns5/bl.db
    echo "; rewrite with some not entirely trivial patricia trees" >>ns5/bl.db
    $PERL -e 'for ($val = 3; $val <= 65535; $val += 69) {
	printf("32.%d.%d.168.192.rpz-ip  \tCNAME\t.\n",
		$val%256, $val/256);
	printf("32.%d.%d.168.192.rpz-nsip\tCNAME\t.\n",
		($val+1)%256, ($val+1)/256);
	}' >>ns5/bl.db
fi

# some psuedo-random queryperf requests
$PERL -e 'for ($cnt = $val = 1; $cnt <= 2000; ++$cnt) {
	printf("host-%d.sub%d.example.com A\n", $val%256, $val/256);
		$val = ($val * 9 + 32771) % 65536;
	}' >ns5/requests
