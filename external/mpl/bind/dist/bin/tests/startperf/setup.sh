#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

usage () {
    echo "Usage: $0 [-s] <number of zones> [<records per zone>]"
    echo "       -s: use the same zone file all zones"
    exit 1
}

if [ "$#" -lt 1 -o "$#" -gt 3 ]; then
    usage
fi

single_file=""
if [ $1 = "-s" ]; then
    single_file=yes
    shift
fi

nzones=$1
shift

nrecords=5
[ "$#" -eq 1 ] && nrecords=$1

. ../system/conf.sh

cat << EOF
options {
        directory "`pwd`";
        listen-on { localhost; };
        listen-on-v6 { localhost; };
	port 5300;
        allow-query { any; };
        allow-transfer { localhost; };
        allow-recursion { none; };
        recursion no;
};

key rndc_key {
        secret "1234abcd8765";
        algorithm hmac-md5;
};

controls {
        inet 127.0.0.1 port 9953 allow { any; } keys { rndc_key; };
};

logging {
        channel basic {
                file "`pwd`/named.log" versions 3 size 100m;
                severity info;
                print-time yes;
                print-severity no;
                print-category no;
        };
        category default {
                basic;
        };
};

EOF

$PERL makenames.pl $nzones | while read zonename; do
    if [ $single_file ]; then
        echo "zone $zonename { type master; file \"smallzone.db\"; };"
    else
        [ -d zones ] || mkdir zones
        $PERL mkzonefile.pl $zonename $nrecords > zones/$zonename.db
        echo "zone $zonename { type master; file \"zones/$zonename.db\"; };"
    fi
done
