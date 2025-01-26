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

from functools import total_ordering
import os
from pathlib import Path
import re
import subprocess
import time
from typing import Optional, Union

from datetime import datetime, timedelta, timezone

import dns
import isctest.log
import isctest.query

DEFAULT_TTL = 300


def _query(server, qname, qtype):
    query = dns.message.make_query(qname, qtype, use_edns=True, want_dnssec=True)
    try:
        response = isctest.query.tcp(query, server.ip, server.ports.dns, timeout=3)
    except dns.exception.Timeout:
        isctest.log.debug(f"query timeout for query {qname} {qtype} to {server.ip}")
        return None

    return response


@total_ordering
class KeyTimingMetadata:
    """
    Represent a single timing information for a key.

    These objects can be easily compared, support addition and subtraction of
    timedelta objects or integers(value in seconds). A lack of timing metadata
    in the key (value 0) should be represented with None rather than an
    instance of this object.
    """

    FORMAT = "%Y%m%d%H%M%S"

    def __init__(self, timestamp: str):
        if int(timestamp) <= 0:
            raise ValueError(f'invalid timing metadata value: "{timestamp}"')
        self.value = datetime.strptime(timestamp, self.FORMAT).replace(
            tzinfo=timezone.utc
        )

    def __repr__(self):
        return self.value.strftime(self.FORMAT)

    def __str__(self) -> str:
        return self.value.strftime(self.FORMAT)

    def __add__(self, other: Union[timedelta, int]):
        if isinstance(other, int):
            other = timedelta(seconds=other)
        result = KeyTimingMetadata.__new__(KeyTimingMetadata)
        result.value = self.value + other
        return result

    def __sub__(self, other: Union[timedelta, int]):
        if isinstance(other, int):
            other = timedelta(seconds=other)
        result = KeyTimingMetadata.__new__(KeyTimingMetadata)
        result.value = self.value - other
        return result

    def __iadd__(self, other: Union[timedelta, int]):
        if isinstance(other, int):
            other = timedelta(seconds=other)
        self.value += other

    def __isub__(self, other: Union[timedelta, int]):
        if isinstance(other, int):
            other = timedelta(seconds=other)
        self.value -= other

    def __lt__(self, other: "KeyTimingMetadata"):
        return self.value < other.value

    def __eq__(self, other: object):
        return isinstance(other, KeyTimingMetadata) and self.value == other.value

    @staticmethod
    def now() -> "KeyTimingMetadata":
        result = KeyTimingMetadata.__new__(KeyTimingMetadata)
        result.value = datetime.now(timezone.utc)
        return result


@total_ordering
class Key:
    """
    Represent a key from a keyfile.

    This object keeps track of its origin (keydir + name), can be used to
    retrieve metadata from the underlying files and supports convenience
    operations for KASP tests.
    """

    def __init__(self, name: str, keydir: Optional[Union[str, Path]] = None):
        self.name = name
        if keydir is None:
            self.keydir = Path()
        else:
            self.keydir = Path(keydir)
        self.path = str(self.keydir / name)
        self.keyfile = f"{self.path}.key"
        self.statefile = f"{self.path}.state"
        self.tag = int(self.name[-5:])

    def get_timing(
        self, metadata: str, must_exist: bool = True
    ) -> Optional[KeyTimingMetadata]:
        regex = rf";\s+{metadata}:\s+(\d+).*"
        with open(self.keyfile, "r", encoding="utf-8") as file:
            for line in file:
                match = re.match(regex, line)
                if match is not None:
                    try:
                        return KeyTimingMetadata(match.group(1))
                    except ValueError:
                        break
        if must_exist:
            raise ValueError(
                f'timing metadata "{metadata}" for key "{self.name}" invalid'
            )
        return None

    def get_metadata(self, metadata: str, must_exist=True) -> str:
        value = "undefined"
        regex = rf"{metadata}:\s+(.*)"
        with open(self.statefile, "r", encoding="utf-8") as file:
            for line in file:
                match = re.match(regex, line)
                if match is not None:
                    value = match.group(1)
                    break
        if must_exist and value == "undefined":
            raise ValueError(
                'state metadata "{metadata}" for key "{self.name}" undefined'
            )
        return value

    def is_ksk(self) -> bool:
        return self.get_metadata("KSK") == "yes"

    def is_zsk(self) -> bool:
        return self.get_metadata("ZSK") == "yes"

    def dnskey_equals(self, value, cdnskey=False):
        dnskey = value.split()

        if cdnskey:
            # fourth element is the rrtype
            assert dnskey[3] == "CDNSKEY"
            dnskey[3] = "DNSKEY"

        dnskey_fromfile = []
        rdata = " ".join(dnskey[:7])

        with open(self.keyfile, "r", encoding="utf-8") as file:
            for line in file:
                if f"{rdata}" in line:
                    dnskey_fromfile = line.split()

        pubkey_fromfile = "".join(dnskey_fromfile[7:])
        pubkey_fromwire = "".join(dnskey[7:])

        return pubkey_fromfile == pubkey_fromwire

    def cds_equals(self, value, alg):
        cds = value.split()

        dsfromkey_command = [
            os.environ.get("DSFROMKEY"),
            "-T",
            "3600",
            "-a",
            alg,
            "-C",
            "-w",
            str(self.keyfile),
        ]

        out = isctest.run.cmd(dsfromkey_command, log_stdout=True)
        dsfromkey = out.stdout.decode("utf-8").split()

        rdata_fromfile = " ".join(dsfromkey[:7])
        rdata_fromwire = " ".join(cds[:7])
        if rdata_fromfile != rdata_fromwire:
            isctest.log.debug(
                f"CDS RDATA MISMATCH: {rdata_fromfile} - {rdata_fromwire}"
            )
            return False

        digest_fromfile = "".join(dsfromkey[7:]).lower()
        digest_fromwire = "".join(cds[7:]).lower()
        if digest_fromfile != digest_fromwire:
            isctest.log.debug(
                f"CDS DIGEST MISMATCH: {digest_fromfile} - {digest_fromwire}"
            )
            return False

        return digest_fromfile == digest_fromwire

    def __lt__(self, other: "Key"):
        return self.name < other.name

    def __eq__(self, other: object):
        return isinstance(other, Key) and self.path == other.path

    def __repr__(self):
        return self.path


def check_zone_is_signed(server, zone):
    addr = server.ip
    fqdn = f"{zone}."

    # wait until zone is fully signed
    signed = False
    for _ in range(10):
        response = _query(server, fqdn, dns.rdatatype.NSEC)
        if not isinstance(response, dns.message.Message):
            isctest.log.debug(f"no response for {fqdn} NSEC from {addr}")
        elif response.rcode() != dns.rcode.NOERROR:
            rcode = dns.rcode.to_text(response.rcode())
            isctest.log.debug(f"{rcode} response for {fqdn} NSEC from {addr}")
        else:
            has_nsec = False
            has_rrsig = False
            for rr in response.answer:
                if not has_nsec:
                    has_nsec = rr.match(
                        dns.name.from_text(fqdn),
                        dns.rdataclass.IN,
                        dns.rdatatype.NSEC,
                        dns.rdatatype.NONE,
                    )
                if not has_rrsig:
                    has_rrsig = rr.match(
                        dns.name.from_text(fqdn),
                        dns.rdataclass.IN,
                        dns.rdatatype.RRSIG,
                        dns.rdatatype.NSEC,
                    )

            if not has_nsec:
                isctest.log.debug(
                    f"missing apex {fqdn} NSEC record in response from {addr}"
                )
            if not has_rrsig:
                isctest.log.debug(
                    f"missing {fqdn} NSEC signature in response from {addr}"
                )

            signed = has_nsec and has_rrsig

        if signed:
            break

        time.sleep(1)

    assert signed


def check_dnssec_verify(server, zone):
    # Check if zone if DNSSEC valid with dnssec-verify.
    fqdn = f"{zone}."

    verified = False
    for _ in range(10):
        transfer = _query(server, fqdn, dns.rdatatype.AXFR)
        if not isinstance(transfer, dns.message.Message):
            isctest.log.debug(f"no response for {fqdn} AXFR from {server.ip}")
        elif transfer.rcode() != dns.rcode.NOERROR:
            rcode = dns.rcode.to_text(transfer.rcode())
            isctest.log.debug(f"{rcode} response for {fqdn} AXFR from {server.ip}")
        else:
            zonefile = f"{zone}.axfr"
            with open(zonefile, "w", encoding="utf-8") as file:
                for rr in transfer.answer:
                    file.write(rr.to_text())
                    file.write("\n")

            try:
                verify_command = [os.environ.get("VERIFY"), "-z", "-o", zone, zonefile]
                verified = isctest.run.cmd(verify_command)
            except subprocess.CalledProcessError:
                pass

        if verified:
            break

        time.sleep(1)

    assert verified


def check_dnssecstatus(server, zone, keys, policy=None, view=None):
    # Call rndc dnssec -status on 'server' for 'zone'. Expect 'policy' in
    # the output. This is a loose verification, it just tests if the right
    # policy name is returned, and if all expected keys are listed.
    response = ""
    if view is None:
        response = server.rndc(f"dnssec -status {zone}", log=False)
    else:
        response = server.rndc(f"dnssec -status {zone} in {view}", log=False)

    if policy is None:
        assert "Zone does not have dnssec-policy" in response
        return

    assert f"dnssec-policy: {policy}" in response

    for key in keys:
        assert f"key: {key.tag}" in response


def _check_signatures(signatures, covers, fqdn, keys):
    now = KeyTimingMetadata.now()
    numsigs = 0
    zrrsig = True
    if covers in [dns.rdatatype.DNSKEY, dns.rdatatype.CDNSKEY, dns.rdatatype.CDS]:
        zrrsig = False
    krrsig = not zrrsig

    for key in keys:
        activate = key.get_timing("Activate")
        inactive = key.get_timing("Inactive", must_exist=False)

        active = now >= activate
        retired = inactive is not None and inactive <= now
        signing = active and not retired
        alg = key.get_metadata("Algorithm")
        rtype = dns.rdatatype.to_text(covers)

        expect = rf"IN RRSIG {rtype} {alg} (\d) (\d+) (\d+) (\d+) {key.tag} {fqdn}"

        if not signing:
            for rrsig in signatures:
                assert re.search(expect, rrsig) is None
            continue

        if zrrsig and key.is_zsk():
            has_rrsig = False
            for rrsig in signatures:
                if re.search(expect, rrsig) is not None:
                    has_rrsig = True
                    break
            assert has_rrsig, f"Expected signature but not found: {expect}"
            numsigs += 1

        if zrrsig and not key.is_zsk():
            for rrsig in signatures:
                assert re.search(expect, rrsig) is None

        if krrsig and key.is_ksk():
            has_rrsig = False
            for rrsig in signatures:
                if re.search(expect, rrsig) is not None:
                    has_rrsig = True
                    break
            assert has_rrsig, f"Expected signature but not found: {expect}"
            numsigs += 1

        if krrsig and not key.is_ksk():
            for rrsig in signatures:
                assert re.search(expect, rrsig) is None

    return numsigs


def check_signatures(rrset, covers, fqdn, ksks, zsks):
    # Check if signatures with covering type are signed with the right keys.
    # The right keys are the ones that expect a signature and have the
    # correct role.
    numsigs = 0

    signatures = []
    for rr in rrset:
        for rdata in rr:
            rdclass = dns.rdataclass.to_text(rr.rdclass)
            rdtype = dns.rdatatype.to_text(rr.rdtype)
            rrsig = f"{rr.name} {rr.ttl} {rdclass} {rdtype} {rdata}"
            signatures.append(rrsig)

    numsigs += _check_signatures(signatures, covers, fqdn, ksks)
    numsigs += _check_signatures(signatures, covers, fqdn, zsks)

    assert numsigs == len(signatures)


def _check_dnskeys(dnskeys, keys, cdnskey=False):
    now = KeyTimingMetadata.now()
    numkeys = 0

    publish_md = "Publish"
    delete_md = "Delete"
    if cdnskey:
        publish_md = f"Sync{publish_md}"
        delete_md = f"Sync{delete_md}"

    for key in keys:
        publish = key.get_timing(publish_md)
        delete = key.get_timing(delete_md, must_exist=False)
        published = now >= publish
        removed = delete is not None and delete <= now

        if not published or removed:
            for dnskey in dnskeys:
                assert not key.dnskey_equals(dnskey, cdnskey=cdnskey)
            continue

        has_dnskey = False
        for dnskey in dnskeys:
            if key.dnskey_equals(dnskey, cdnskey=cdnskey):
                has_dnskey = True
                break

        if not cdnskey:
            assert has_dnskey

        if has_dnskey:
            numkeys += 1

    return numkeys


def check_dnskeys(rrset, ksks, zsks, cdnskey=False):
    # Check if the correct DNSKEY records are published. If the current time
    # is between the timing metadata 'publish' and 'delete', the key must have
    # a DNSKEY record published. If 'cdnskey' is True, check against CDNSKEY
    # records instead.
    numkeys = 0

    dnskeys = []
    for rr in rrset:
        for rdata in rr:
            rdclass = dns.rdataclass.to_text(rr.rdclass)
            rdtype = dns.rdatatype.to_text(rr.rdtype)
            dnskey = f"{rr.name} {rr.ttl} {rdclass} {rdtype} {rdata}"
            dnskeys.append(dnskey)

    numkeys += _check_dnskeys(dnskeys, ksks, cdnskey=cdnskey)
    if not cdnskey:
        numkeys += _check_dnskeys(dnskeys, zsks)

    assert numkeys == len(dnskeys)


def check_cds(rrset, keys):
    # Check if the correct CDS records are published. If the current time
    # is between the timing metadata 'publish' and 'delete', the key must have
    # a DNSKEY record published. If 'cdnskey' is True, check against CDNSKEY
    # records instead.
    now = KeyTimingMetadata.now()
    numcds = 0

    cdss = []
    for rr in rrset:
        for rdata in rr:
            rdclass = dns.rdataclass.to_text(rr.rdclass)
            rdtype = dns.rdatatype.to_text(rr.rdtype)
            cds = f"{rr.name} {rr.ttl} {rdclass} {rdtype} {rdata}"
            cdss.append(cds)

    for key in keys:
        assert key.is_ksk()

        publish = key.get_timing("SyncPublish")
        delete = key.get_timing("SyncDelete", must_exist=False)
        published = now >= publish
        removed = delete is not None and delete <= now
        if not published or removed:
            for cds in cdss:
                assert not key.cds_equals(cds, "SHA-256")
            continue

        has_cds = False
        for cds in cdss:
            if key.cds_equals(cds, "SHA-256"):
                has_cds = True
                break

        assert has_cds
        numcds += 1

    assert numcds == len(cdss)


def _query_rrset(server, fqdn, qtype):
    response = _query(server, fqdn, qtype)
    assert response.rcode() == dns.rcode.NOERROR

    rrs = []
    rrsigs = []
    for rrset in response.answer:
        if rrset.match(
            dns.name.from_text(fqdn), dns.rdataclass.IN, dns.rdatatype.RRSIG, qtype
        ):
            rrsigs.append(rrset)
        elif rrset.match(
            dns.name.from_text(fqdn), dns.rdataclass.IN, qtype, dns.rdatatype.NONE
        ):
            rrs.append(rrset)
        else:
            assert False

    return rrs, rrsigs


def check_apex(server, zone, ksks, zsks):
    # Test the apex of a zone. This checks that the SOA and DNSKEY RRsets
    # are signed correctly and with the appropriate keys.
    fqdn = f"{zone}."

    # test dnskey query
    dnskeys, rrsigs = _query_rrset(server, fqdn, dns.rdatatype.DNSKEY)
    assert len(dnskeys) > 0
    check_dnskeys(dnskeys, ksks, zsks)
    assert len(rrsigs) > 0
    check_signatures(rrsigs, dns.rdatatype.DNSKEY, fqdn, ksks, zsks)

    # test soa query
    soa, rrsigs = _query_rrset(server, fqdn, dns.rdatatype.SOA)
    assert len(soa) == 1
    assert f"{zone}. {DEFAULT_TTL} IN SOA" in soa[0].to_text()
    assert len(rrsigs) > 0
    check_signatures(rrsigs, dns.rdatatype.SOA, fqdn, ksks, zsks)

    # test cdnskey query
    cdnskeys, rrsigs = _query_rrset(server, fqdn, dns.rdatatype.CDNSKEY)
    check_dnskeys(cdnskeys, ksks, zsks, cdnskey=True)
    if len(cdnskeys) > 0:
        assert len(rrsigs) > 0
        check_signatures(rrsigs, dns.rdatatype.CDNSKEY, fqdn, ksks, zsks)

    # test cds query
    cds, rrsigs = _query_rrset(server, fqdn, dns.rdatatype.CDS)
    check_cds(cds, ksks)
    if len(cds) > 0:
        assert len(rrsigs) > 0
        check_signatures(rrsigs, dns.rdatatype.CDS, fqdn, ksks, zsks)


def check_subdomain(server, zone, ksks, zsks):
    # Test an RRset below the apex and verify it is signed correctly.
    fqdn = f"{zone}."
    qname = f"a.{zone}."
    qtype = dns.rdatatype.A
    response = _query(server, qname, qtype)
    assert response.rcode() == dns.rcode.NOERROR

    match = f"{qname} {DEFAULT_TTL} IN A 10.0.0.1"
    rrsigs = []
    for rrset in response.answer:
        if rrset.match(
            dns.name.from_text(qname), dns.rdataclass.IN, dns.rdatatype.RRSIG, qtype
        ):
            rrsigs.append(rrset)
        else:
            assert match in rrset.to_text()

    assert len(rrsigs) > 0
    check_signatures(rrsigs, qtype, fqdn, ksks, zsks)
