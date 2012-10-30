#!/bin/sh
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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh
. ./clean.sh

#  NO-OP is an obsolete synonym for PASSHTRU
for NM in '' -2 -given -disabled -passthru -no-op -nodata -nxdomain -cname -wildcname -garden; do
    sed -e "/SOA/s/blx/bl$NM/g" ns3/base.db >ns3/bl$NM.db
done

../../../tools/genrandom 400 random.data
$KEYGEN -Kns2 -q -r random.data -3 signed-tld2. > /dev/null 2>&1
$KEYGEN -Kns2 -q -r random.data -3fk signed-tld2. > /dev/null 2>&1
$SIGNER -S -Kns2 -o signed-tld2. -f ns2/signed-tld2.db ns2/tld2.db > /dev/null 2>&1
