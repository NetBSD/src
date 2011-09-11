#!/bin/sh
#
# Copyright (C) 2011  Internet Systems Consortium, Inc. ("ISC")
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

# Id: setup.sh,v 1.3 2011-07-07 23:47:49 tbox Exp

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <number of zones>"
    exit 1
fi

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

$PERL makenames.pl $1 | while read zonename; do
        echo "zone $zonename { type master; file \"smallzone.db\"; };"
done
