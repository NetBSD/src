#!/bin/sh

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

rm -f */K* */dsset-* */*.signed */tmp* */*.jnl */*.bk
rm -f */core
rm -f */example.bk
rm -f */named.conf
rm -f */named.memstats
rm -f */named.run*
rm -f */trusted.conf */private.conf
rm -f activate-now-publish-1day.key
rm -f active.key inact.key del.key delzsk.key unpub.key standby.key rev.key
rm -f delayksk.key delayzsk.key autoksk.key autozsk.key
rm -f dig.out.*
rm -f digcomp.out.test*
rm -f digcomp.out.test*
rm -f noksk-ksk.key nozsk-ksk.key nozsk-zsk.key inaczsk-zsk.key inaczsk-ksk.key
rm -f nopriv.key vanishing.key del1.key del2.key
rm -f ns*/managed-keys.bind*
rm -f ns*/named.lock
rm -f ns*/named.lock
rm -f ns1/root.db
rm -f ns2/example.db
rm -f ns2/private.secure.example.db ns2/bar.db
rm -f ns3/*.nzd ns3/*.nzd-lock ns3/*.nzf
rm -f ns3/*.nzf
rm -f ns3/autonsec3.example.db
rm -f ns3/cdnskey-delete.example.db
rm -f ns3/cds-delete.example.db
rm -f ns3/delzsk.example.db
rm -f ns3/dname-at-apex-nsec3.example.db
rm -f ns3/inacksk2.example.db
rm -f ns3/inacksk3.example.db
rm -f ns3/inaczsk2.example.db
rm -f ns3/inaczsk3.example.db
rm -f ns3/jitter.nsec3.example.db
rm -f ns3/kg.out ns3/s.out ns3/st.out
rm -f ns3/kskonly.example.db
rm -f ns3/named.ns3.prev
rm -f ns3/noksk.example.db
rm -f ns3/nozsk.example.db ns3/inaczsk.example.db
rm -f ns3/nsec-only.example.db
rm -f ns3/nsec3-to-nsec.example.db
rm -f ns3/nsec3.example.db
rm -f ns3/nsec3.nsec3.example.db
rm -f ns3/nsec3.optout.example.db
rm -f ns3/oldsigs.example.db
rm -f ns3/optout.example.db
rm -f ns3/optout.nsec3.example.db
rm -f ns3/optout.optout.example.db
rm -f ns3/prepub.example.db
rm -f ns3/prepub.example.db.in
rm -f ns3/reconf.example.db
rm -f ns3/rsasha256.example.db ns3/rsasha512.example.db
rm -f ns3/secure-to-insecure.example.db
rm -f ns3/secure-to-insecure2.example.db
rm -f ns3/secure.example.db
rm -f ns3/secure.nsec3.example.db
rm -f ns3/secure.optout.example.db
rm -f ns3/settime.out.*
rm -f ns3/sync.example.db
rm -f ns3/ttl*.db
rm -f nsupdate.out
rm -f settime.out.*
rm -f signing.out.*
rm -f sync.key
