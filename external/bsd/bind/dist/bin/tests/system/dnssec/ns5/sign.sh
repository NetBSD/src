#!/bin/sh -e
#
# Copyright (C) 2015  Internet Systems Consortium, Inc. ("ISC")
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

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

zone=.
infile=../ns1/root.db.in
zonefile=root.db.signed

keyname=`$KEYGEN -r $RANDFILE -qfk $zone`

# copy the KSK out first, then revoke it
cat $keyname.key | grep -v '^; ' | $PERL -n -e '
local ($dn, $class, $type, $flags, $proto, $alg, @rest) = split;
local $key = join("", @rest);
print <<EOF
managed-keys {
    "$dn" initial-key $flags $proto $alg "$key";
};
EOF
' > revoked.conf

$SETTIME -R now ${keyname}.key > /dev/null

# create a current set of keys, and sign the root zone
$KEYGEN -r $RANDFILE -q $zone > /dev/null
$KEYGEN -r $RANDFILE -qfk $zone > /dev/null
$SIGNER -S -r $RANDFILE -o $zone -f $zonefile $infile > /dev/null 2>&1
