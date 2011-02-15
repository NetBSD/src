#!/bin/sh
#
# Copyright (C) 2010  Internet Systems Consortium, Inc. ("ISC")
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

# Id: prereq.sh,v 1.4 2010-12-20 23:47:20 tbox Exp

TOP=${SYSTEMTESTTOP:=.}/../../../..

# enable the dlzexternal test only if it builds and dlz-dlopen was enabled
$TOP/bin/named/named -V | grep with.dlz.dlopen | grep -v with.dlz.dlopen=no > /dev/null || {
    echo "I:not built with --with-dlz-dlopen=yes - skipping dlzexternal test"
    exit 255
}

cd ../../../../contrib/dlz/example && make all > /dev/null || {
    echo "I:build of dlz_example.so failed - skipping dlzexternal test"
    exit 1
}
exit 0


