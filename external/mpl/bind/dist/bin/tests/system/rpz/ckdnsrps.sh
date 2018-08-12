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

set -e

# Say on stdout whether to test DNSRPS
#	and create dnsrps.conf and dnsrps-slave.conf
# Note that dnsrps.conf and dnsrps-slave.conf are included in named.conf
#	and differ from dnsrpz.conf which is used by dnsrpzd.


SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

DNSRPS_CMD=../rpz/dnsrps

AS_NS=
TEST_DNSRPS=
MCONF=dnsrps.conf
SCONF=dnsrps-slave.conf
USAGE="$0: [-xAD] [-M dnsrps.conf] [-S dnsrps-slave.conf]"
while getopts "xADM:S:" c; do
    case $c in
	x) set -x; DEBUG=-x;;
	A) AS_NS=yes;;
	D) TEST_DNSRPS=yes;;
	M) MCONF="$OPTARG";;
	S) SCONF="$OPTARG";;
	*) echo "$USAGE" 1>&2; exit 1;;
    esac
done
shift `expr $OPTIND - 1 || true`
if [ "$#" -ne 0 ]; then
    echo "$USAGE" 1>&2
    exit 1
fi

# erase any existing conf files
cat /dev/null > $MCONF
cat /dev/null > $SCONF

add_conf () {
    echo "$*" >>$MCONF
    echo "$*" >>$SCONF
}

if ! $FEATURETEST --enable-dnsrps; then
    if [ -n "$TEST_DNSRPS" ]; then
        add_conf "## DNSRPS disabled at compile time"
    fi
    add_conf "#skip"
    exit 0
fi

if [ -z "$TEST_DNSRPS" ]; then
    add_conf "## DNSRPS testing is disabled"
    add_conf '#skip'
    exit 0
fi

if [ ! -x $DNSRPS_CMD ]; then
    add_conf "## make $DNSRPS_CMD to test DNSRPS"
    add_conf '#skip'
    exit 0
fi

if $DNSRPS_CMD -a >/dev/null; then :
else
    add_conf "## DNSRPS provider library is not available"
    add_conf '#skip'
    exit 0
fi

CMN="	dnsrps-options { dnsrpzd-conf ../dnsrpzd.conf
			 dnsrpzd-sock ../dnsrpzd.sock
			 dnsrpzd-rpzf ../dnsrpzd.rpzf
			 dnsrpzd-args '-dddd -L stdout'
			 log-level 3"

MASTER="$CMN"
if [ -n "$AS_NS" ]; then
    MASTER="$MASTER
			qname-as-ns yes
			ip-as-ns yes"
fi

# write dnsrps setttings for master resolver
cat <<EOF >>$MCONF
$MASTER };
EOF

# write dnsrps setttings for resolvers that should not start dnsrpzd
cat <<EOF >>$SCONF
$CMN
			dnsrpzd '' };	# do not start dnsrpzd
EOF


# DNSRPS is available.
# The test should fail if the license is bad.
add_conf "dnsrps-enable yes;"

# Use alt-dnsrpzd-license.conf if it exists
CUR_L=dnsrpzd-license-cur.conf
ALT_L=alt-dnsrpzd-license.conf
# try ../rpz/alt-dnsrpzd-license.conf if alt-dnsrpzd-license.conf does not exist
[ -s $ALT_L ] || ALT_L=../rpz/alt-dnsrpzd-license.conf
if [ -s $ALT_L ]; then
    SRC_L=$ALT_L
    USE_ALT=
else
    SRC_L=../rpz/dnsrpzd-license.conf
    USE_ALT="## consider installing alt-dnsrpzd-license.conf"
fi
cp $SRC_L $CUR_L

# parse $CUR_L for the license zone name, master IP addresses, and optional
#   transfer-source IP addresses
eval `sed -n -e 'y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/'\
    -e 's/.*zone *\([-a-z0-9]*.license.fastrpz.com\).*/NAME=\1/p'	\
    -e 's/.*farsight_fastrpz_license *\([0-9.]*\);.*/IPV4=\1/p'		\
    -e 's/.*farsight_fastrpz_license *\([0-9a-f:]*\);.*/IPV6=\1/p'	\
    -e 's/.*transfer-source *\([0-9.]*\);.*/TS4=-b\1/p'			\
    -e 's/.*transfer-source *\([0-9a-f:]*\);.*/TS6=-b\1/p'		\
    -e 's/.*transfer-source-v6 *\([0-9a-f:]*\);.*/TS6=-b\1/p'		\
	$CUR_L`
if [ -z "$NAME" ]; then
    add_conf "## no DNSRPS tests; no license domain name in $SRC_L"
    add_conf '#fail'
    exit 0
fi
if [ -z "$IPV4" ]; then
    IPV4=license1.fastrpz.com
    TS4=
fi
if [ -z "$IPV6" ]; then
    IPV6=license1.fastrpz.com
    TS6=
fi

# This TSIG key is common and NOT a secret
KEY='hmac-sha256:farsight_fastrpz_license:f405d02b4c8af54855fcebc1'

# Try IPv4 and then IPv6 to deal with IPv6 tunnel and connectivity problems
if `$DIG -4 -t axfr -y$KEY $TS4 $NAME @$IPV4				\
	    | grep -i "^$NAME.*TXT" >/dev/null`; then
    exit 0
fi
if `$DIG -6 -t axfr -y$KEY $TS6 $NAME @$IPV6				\
	    | grep -i "^$NAME.*TXT" >/dev/null`; then
    exit 0
fi

add_conf "## DNSRPS lacks a valid license via $SRC_L"
[ -z "$USE_ALT" ] || add_conf "$USE_ALT"
add_conf '#fail'
