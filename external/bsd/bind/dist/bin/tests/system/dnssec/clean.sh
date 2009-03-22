#!/bin/sh
#
# Copyright (C) 2004, 2007, 2008  Internet Systems Consortium, Inc. ("ISC")
# Copyright (C) 2000-2002  Internet Software Consortium.
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

# Id: clean.sh,v 1.23 2008/09/25 04:02:38 tbox Exp

rm -f */K* */keyset-* */dsset-* */dlvset-* */signedkey-* */*.signed */trusted.conf */tmp* */*.jnl */*.bk
rm -f ns1/root.db ns2/example.db ns3/secure.example.db
rm -f ns3/unsecure.example.db ns3/bogus.example.db ns3/keyless.example.db
rm -f ns3/dynamic.example.db ns3/dynamic.example.db.signed.jnl
rm -f ns2/private.secure.example.db
rm -f */example.bk
rm -f dig.out.*
rm -f random.data
rm -f ns2/dlv.db
rm -f ns3/multiple.example.db ns3/nsec3-unknown.example.db ns3/nsec3.example.db
rm -f ns3/optout-unknown.example.db ns3/optout.example.db
rm -f ns7/multiple.example.bk ns7/nsec3.example.bk ns7/optout.example.bk
rm -f */named.memstats
rm -f ns3/nsec3.nsec3.example.db
rm -f ns3/nsec3.optout.example.db
rm -f ns3/optout.nsec3.example.db
rm -f ns3/optout.optout.example.db
rm -f ns3/secure.nsec3.example.db
rm -f ns3/secure.optout.example.db
