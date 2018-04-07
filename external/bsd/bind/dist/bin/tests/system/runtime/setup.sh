#!/bin/sh -e
#
# Copyright (C) 2017  Internet Systems Consortium, Inc. ("ISC")
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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

$SHELL clean.sh

cp ns2/named1.conf ns2/named.conf

mkdir ns2/nope

if [ 1 = "${CYGWIN:-0}" ]
then
    setfacl -s user::r-x,group::r-x,other::r-x ns2/nope
else
    chmod 555 ns2/nope
fi

echo "directory \"`pwd`/ns2\";" > ns2/dir
echo "directory \"`pwd`/ns2/nope\";" > ns2/nopedir
echo "managed-keys-directory \"`pwd`/ns2\";" > ns2/mkd
echo "managed-keys-directory \"`pwd`/ns2/nope\";" > ns2/nopemkd
