#!/bin/sh
#
# Copyright (C) 2004, 2007-2017  Internet Systems Consortium, Inc. ("ISC")
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

rm -f */K* */keyset-* */dsset-* */dlvset-* */signedkey-* */*.signed
rm -f */example.bk
rm -f */named.memstats
rm -f */named.run
rm -f */named.secroots
rm -f */tmp* */*.jnl */*.bk */*.jbk
rm -f */trusted.conf */managed.conf */revoked.conf
rm -f Kexample.*
rm -f canonical?.*
rm -f delv.out*
rm -f delve.out*
rm -f dig.out.*
rm -f dsfromkey.out.*
rm -f keygen.err
rm -f named.secroots.test*
rm -f nosign.before
rm -f ns*/*.nta
rm -f ns*/named.lock
rm -f ns1/managed.key.id
rm -f ns1/root.db ns2/example.db ns3/secure.example.db
rm -f ns2/algroll.db
rm -f ns2/badparam.db ns2/badparam.db.bad
rm -f ns2/cdnskey-update.secure.db
rm -f ns2/cdnskey.secure.db
rm -f ns2/cds-auto.secure.db ns2/cds-auto.secure.db.jnl
rm -f ns2/cds-update.secure.db ns2/cds-update.secure.db.jnl
rm -f ns2/cds.secure.db
rm -f ns2/dlv.db
rm -f ns2/in-addr.arpa.db
rm -f ns2/nsec3chain-test.db
rm -f ns2/private.secure.example.db
rm -f ns2/single-nsec3.db
rm -f ns3/auto-nsec.example.db ns3/auto-nsec3.example.db
rm -f ns3/badds.example.db
rm -f ns3/dnskey-nsec3-unknown.example.db
rm -f ns3/dnskey-nsec3-unknown.example.db.tmp
rm -f ns3/dnskey-unknown.example.db
rm -f ns3/dnskey-unknown.example.db.tmp
rm -f ns3/dynamic.example.db ns3/dynamic.example.db.signed.jnl
rm -f ns3/expired.example.db ns3/update-nsec3.example.db
rm -f ns3/expiring.example.db ns3/nosign.example.db
rm -f ns3/future.example.db ns3/trusted-future.key
rm -f ns3/inline.example.db.signed
rm -f ns3/kskonly.example.db
rm -f ns3/lower.example.db ns3/upper.example.db ns3/upper.example.db.lower
rm -f ns3/multiple.example.db ns3/nsec3-unknown.example.db ns3/nsec3.example.db
rm -f ns3/nsec3.nsec3.example.db
rm -f ns3/nsec3.optout.example.db
rm -f ns3/optout-unknown.example.db ns3/optout.example.db
rm -f ns3/optout.nsec3.example.db
rm -f ns3/optout.optout.example.db
rm -f ns3/publish-inactive.example.db
rm -f ns3/rsasha256.example.db ns3/rsasha512.example.db
rm -f ns3/secure.below-cname.example.db
rm -f ns3/secure.nsec3.example.db
rm -f ns3/secure.optout.example.db
rm -f ns3/siginterval.conf
rm -f ns3/siginterval.example.db
rm -f ns3/split-dnssec.example.db
rm -f ns3/split-smart.example.db
rm -f ns3/ttlpatch.example.db ns3/ttlpatch.example.db.signed
rm -f ns3/ttlpatch.example.db.patched
rm -f ns3/unsecure.example.db ns3/bogus.example.db ns3/keyless.example.db
rm -f ns3/revkey.example.db
rm -f ns3/managed-future.example.db
rm -f ns4/managed-keys.bind*
rm -f ns4/named.conf
rm -f ns4/named.conf ns5/named.conf
rm -f ns4/named_dump.db
rm -f ns6/optout-tld.db
rm -f ns7/multiple.example.bk ns7/nsec3.example.bk ns7/optout.example.bk
rm -f ns7/split-rrsig.db ns7/split-rrsig.db.unsplit
rm -f nsupdate.out*
rm -f rndc.out.*
rm -f signer/*.db
rm -f signer/example.db.after signer/example.db.before
rm -f signer/example.db.changed
rm -f signer/nsec3param.out
rm -f signer/signer.out.*
rm -f signing.out*
rm -f signer/*.signed.pre*
rm -f signer/*.signed.post*
