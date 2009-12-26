#!/bin/sh
#
# Copyright (C) 2009  Internet Systems Consortium, Inc. ("ISC")
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

# Id: clean.sh,v 1.3 2009/11/30 23:48:02 tbox Exp

rm -f */K* */dsset-* */*.signed */trusted.conf */tmp* */*.jnl */*.bk
rm -f inact.key del.key unpub.key standby.key rev.key
rm -f ns1/root.db ns2/example.db ns3/secure.example.db
rm -f ns3/rsasha256.example.db ns3/rsasha512.example.db
rm -f ns2/private.secure.example.db
rm -f */core
rm -f */example.bk
rm -f dig.out.*
rm -f random.data
rm -f ns2/dlv.db
rm -f ns3/multiple.example.db ns3/nsec3-unknown.example.db ns3/nsec3.example.db
rm -f ns3/optout-unknown.example.db ns3/optout.example.db
rm -f */named.memstats
rm -f ns3/nsec3.nsec3.example.db
rm -f ns3/nsec3.optout.example.db
rm -f ns3/optout.nsec3.example.db
rm -f ns3/optout.optout.example.db
rm -f ns3/secure.nsec3.example.db
rm -f ns3/secure.optout.example.db
