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
import glob
import os
from pathlib import Path
import re
import subprocess
import time
from typing import Dict, List, Optional, Tuple, Union

from datetime import datetime, timedelta, timezone

import dns
import dns.tsig
import isctest.log
import isctest.query
import isctest.util

DEFAULT_TTL = 300

NEXT_KEY_EVENT_THRESHOLD = 100


def _query(server, qname, qtype, tsig=None):
    query = dns.message.make_query(qname, qtype, use_edns=True, want_dnssec=True)

    if tsig is not None:
        tsigkey = tsig.split(":")
        keyring = dns.tsig.Key(tsigkey[1], tsigkey[2], tsigkey[0])
        query.use_tsig(keyring)

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


class KeyProperties:
    """
    Represent the (expected) properties a key should have.
    """

    def __init__(
        self,
        name: str,
        properties: dict,
        metadata: dict,
        timing: Dict[str, KeyTimingMetadata],
    ):
        self.name = name
        self.key = None
        self.properties = properties
        self.metadata = metadata
        self.timing = timing

    def __repr__(self):
        return self.name

    def __str__(self) -> str:
        return self.name

    @staticmethod
    def default(with_state=True) -> "KeyProperties":
        properties = {
            "expect": True,
            "private": True,
            "legacy": False,
            "role": "csk",
            "role_full": "key-signing",
            "dnskey_ttl": 3600,
            "flags": 257,
        }
        metadata = {
            "Algorithm": isctest.vars.algorithms.ECDSAP256SHA256.number,
            "Length": 256,
            "Lifetime": 0,
            "KSK": "yes",
            "ZSK": "yes",
        }
        timing: Dict[str, KeyTimingMetadata] = {}

        result = KeyProperties(
            name="DEFAULT", properties=properties, metadata=metadata, timing=timing
        )
        result.name = "DEFAULT"
        result.key = None
        if with_state:
            result.metadata["GoalState"] = "omnipresent"
            result.metadata["DNSKEYState"] = "rumoured"
            result.metadata["KRRSIGState"] = "rumoured"
            result.metadata["ZRRSIGState"] = "rumoured"
            result.metadata["DSState"] = "hidden"

        return result

    def Ipub(self, config):
        ipub = timedelta(0)

        if self.key.get_metadata("Predecessor", must_exist=False) != "undefined":
            # Ipub = Dprp + TTLkey
            ipub = (
                config["dnskey-ttl"]
                + config["zone-propagation-delay"]
                + config["publish-safety"]
            )

        self.timing["Active"] = self.timing["Published"] + ipub

    def IpubC(self, config):
        if not self.key.is_ksk():
            return

        ttl1 = config["dnskey-ttl"] + config["publish-safety"]
        ttl2 = timedelta(0)

        if self.key.get_metadata("Predecessor", must_exist=False) == "undefined":
            # If this is the first key, we also need to wait until the zone
            # signatures are omnipresent. Use max-zone-ttl instead of
            # dnskey-ttl, and no publish-safety (because we are looking at
            # signatures here, not the public key).
            ttl2 = config["max-zone-ttl"]

        # IpubC = DprpC + TTLkey
        ipubc = config["zone-propagation-delay"] + max(ttl1, ttl2)

        self.timing["PublishCDS"] = self.timing["Published"] + ipubc

        if self.metadata["Lifetime"] != 0:
            self.timing["DeleteCDS"] = (
                self.timing["PublishCDS"] + self.metadata["Lifetime"]
            )

    def Iret(self, config):
        if self.metadata["Lifetime"] == 0:
            return

        sign_delay = config["signatures-validity"] - config["signatures-refresh"]
        safety_interval = config["retire-safety"]

        iretKSK = timedelta(0)
        iretZSK = timedelta(0)
        if self.key.is_ksk():
            # Iret = DprpP + TTLds
            iretKSK = (
                config["parent-propagation-delay"] + config["ds-ttl"] + safety_interval
            )
        if self.key.is_zsk():
            # Iret = Dsgn + Dprp + TTLsig
            iretZSK = (
                sign_delay
                + config["zone-propagation-delay"]
                + config["max-zone-ttl"]
                + safety_interval
            )

        self.timing["Removed"] = self.timing["Retired"] + max(iretKSK, iretZSK)

    def set_expected_keytimes(self, config, offset=None, pregenerated=False):
        if self.key is None:
            raise ValueError("KeyProperties must be attached to a Key")

        if self.properties["legacy"]:
            return

        if offset is None:
            offset = self.properties["offset"]

        self.timing["Generated"] = self.key.get_timing("Created")

        self.timing["Published"] = self.timing["Generated"]
        if pregenerated:
            self.timing["Published"] = self.key.get_timing("Publish")
        self.timing["Published"] = self.timing["Published"] + offset
        self.Ipub(config)

        # Set Retired timing metadata if key has lifetime.
        if self.metadata["Lifetime"] != 0:
            self.timing["Retired"] = self.timing["Active"] + self.metadata["Lifetime"]

        self.IpubC(config)
        self.Iret(config)

        # Key state change times must exist, but since we cannot reliably tell
        # when named made the actual state change, we don't care what the
        # value is. Set it to None will verify that the metadata exists, but
        # without actual checking the value.
        self.timing["DNSKEYChange"] = None

        if self.key.is_ksk():
            self.timing["DSChange"] = None
            self.timing["KRRSIGChange"] = None

        if self.key.is_zsk():
            self.timing["ZRRSIGChange"] = None


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
        self.privatefile = f"{self.path}.private"
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

    def get_metadata(
        self, metadata: str, file=None, comment=False, must_exist=True
    ) -> str:
        if file is None:
            file = self.statefile
        value = "undefined"
        regex = rf"{metadata}:\s+(\S+).*"
        if comment:
            # The expected metadata is prefixed with a ';'.
            regex = rf";\s+{metadata}:\s+(\S+).*"
        with open(file, "r", encoding="utf-8") as fp:
            for line in fp:
                match = re.match(regex, line)
                if match is not None:
                    value = match.group(1)
                    break
        if must_exist and value == "undefined":
            raise ValueError(
                f'metadata "{metadata}" for key "{self.name}" in file "{file}" undefined'
            )
        return value

    def get_signing_state(
        self, offline_ksk=False, zsk_missing=False
    ) -> Tuple[bool, bool]:
        """
        This returns the signing state derived from the key states, KRRSIGState
        and ZRRSIGState.

        If 'offline_ksk' is set to True, we determine the signing state from
        the timing metadata. If 'zsigning' is True, ensure the current time is
        between the Active and Retired timing metadata.

        If 'zsk_missing' is set to True, it means the ZSK private key file is
        missing, and the KSK should take over signing the RRset, and the
        expected zone signing state (zsigning) is reversed.
        """
        # Fetch key timing metadata.
        now = KeyTimingMetadata.now()
        activate = self.get_timing("Activate")
        assert activate is not None  # to silence mypy - its implied by line above
        inactive = self.get_timing("Inactive", must_exist=False)

        active = now >= activate
        retired = inactive is not None and inactive <= now
        signing = active and not retired

        # Fetch key state metadata.
        krrsigstate = self.get_metadata("KRRSIGState", must_exist=False)
        ksigning = krrsigstate in ["rumoured", "omnipresent"]
        zrrsigstate = self.get_metadata("ZRRSIGState", must_exist=False)
        zsigning = zrrsigstate in ["rumoured", "omnipresent"]

        if ksigning:
            assert self.is_ksk()
        if zsigning:
            assert self.is_zsk()

        # If the ZSK private key file is missing, revers the zone signing state.
        if zsk_missing:
            zsigning = not zsigning

        # If testing offline KSK, retrieve the signing state from the key timing
        # metadata.
        if offline_ksk and signing and self.is_zsk():
            assert zsigning
        if offline_ksk and signing and self.is_ksk():
            ksigning = signing

        return ksigning, zsigning

    def ttl(self) -> int:
        with open(self.keyfile, "r", encoding="utf-8") as file:
            for line in file:
                if line.startswith(";"):
                    continue
                return int(line.split()[1])
        return 0

    def dnskey(self):
        with open(self.keyfile, "r", encoding="utf-8") as file:
            for line in file:
                if "DNSKEY" in line:
                    return line.strip()
        return "undefined"

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
            str(self.ttl()),
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

    def is_metadata_consistent(self, key, metadata, checkval=True):
        """
        If 'key' exists in 'metadata' then it must also exist in the state
        meta data. Otherwise, it must not exist in the state meta data.
        If 'checkval' is True, the meta data values must also match.
        """
        if key in metadata:
            if checkval:
                value = self.get_metadata(key)
                if value != f"{metadata[key]}":
                    isctest.log.debug(
                        f"{self.name} {key} METADATA MISMATCH: {value} - {metadata[key]}"
                    )
                return value == f"{metadata[key]}"

            return self.get_metadata(key) != "undefined"

        value = self.get_metadata(key, must_exist=False)
        if value != "undefined":
            isctest.log.debug(f"{self.name} {key} METADATA UNEXPECTED: {value}")
        return value == "undefined"

    def is_timing_consistent(self, key, timing, file, comment=False):
        """
        If 'key' exists in 'timing' then it must match the value in the state
        timing data. Otherwise, it must also not exist in the state timing data.
        """
        if key in timing:
            value = self.get_metadata(key, file=file, comment=comment)
            if value != str(timing[key]):
                isctest.log.debug(
                    f"{self.name} {key} TIMING MISMATCH: {value} - {timing[key]}"
                )
            return value == str(timing[key])

        value = self.get_metadata(key, file=file, comment=comment, must_exist=False)
        if value != "undefined":
            isctest.log.debug(f"{self.name} {key} TIMING UNEXPECTED: {value}")
        return value == "undefined"

    def match_properties(self, zone, properties):
        """
        Check the key with given properties.
        """
        if not properties.properties["expect"]:
            return False

        # Check file existence.
        # Noop. If file is missing then the get_metadata calls will fail.

        # Check the public key file.
        role = properties.properties["role_full"]
        comment = f"This is a {role} key, keyid {self.tag}, for {zone}."
        if not isctest.util.file_contents_contain(self.keyfile, comment):
            isctest.log.debug(f"{self.name} COMMENT MISMATCH: expected '{comment}'")
            return False

        ttl = properties.properties["dnskey_ttl"]
        flags = properties.properties["flags"]
        alg = properties.metadata["Algorithm"]
        dnskey = f"{zone}. {ttl} IN DNSKEY {flags} 3 {alg}"
        if not isctest.util.file_contents_contain(self.keyfile, dnskey):
            isctest.log.debug(f"{self.name} DNSKEY MISMATCH: expected '{dnskey}'")
            return False

        # Now check the private key file.
        if properties.properties["private"]:
            # Retrieve creation date.
            created = self.get_metadata("Generated")

            pval = self.get_metadata("Created", file=self.privatefile)
            if pval != created:
                isctest.log.debug(
                    f"{self.name} Created METADATA MISMATCH: {pval} - {created}"
                )
                return False
            pval = self.get_metadata("Private-key-format", file=self.privatefile)
            if pval != "v1.3":
                isctest.log.debug(
                    f"{self.name} Private-key-format METADATA MISMATCH: {pval} - v1.3"
                )
                return False
            pval = self.get_metadata("Algorithm", file=self.privatefile)
            if pval != f"{alg}":
                isctest.log.debug(
                    f"{self.name} Algorithm METADATA MISMATCH: {pval} - {alg}"
                )
                return False

        # Now check the key state file.
        if properties.properties["legacy"]:
            return True

        comment = f"This is the state of key {self.tag}, for {zone}."
        if not isctest.util.file_contents_contain(self.statefile, comment):
            isctest.log.debug(f"{self.name} COMMENT MISMATCH: expected '{comment}'")
            return False

        attributes = [
            "Lifetime",
            "Algorithm",
            "Length",
            "KSK",
            "ZSK",
            "GoalState",
            "DNSKEYState",
            "KRRSIGState",
            "ZRRSIGState",
            "DSState",
        ]
        for key in attributes:
            if not self.is_metadata_consistent(key, properties.metadata):
                return False

        # A match is found.
        return True

    def match_timingmetadata(self, timings, file=None, comment=False):
        if file is None:
            file = self.statefile

        attributes = [
            "Generated",
            "Created",
            "Published",
            "Publish",
            "PublishCDS",
            "SyncPublish",
            "Active",
            "Activate",
            "Retired",
            "Inactive",
            "Revoked",
            "Removed",
            "Delete",
        ]
        for key in attributes:
            if not self.is_timing_consistent(key, timings, file, comment=comment):
                isctest.log.debug(f"{self.name} TIMING METADATA MISMATCH: {key}")
                return False

        return True

    def __lt__(self, other: "Key"):
        return self.name < other.name

    def __eq__(self, other: object):
        return isinstance(other, Key) and self.path == other.path

    def __repr__(self):
        return self.path


def check_zone_is_signed(server, zone, tsig=None):
    addr = server.ip
    fqdn = f"{zone}."

    # wait until zone is fully signed
    signed = False
    for _ in range(10):
        response = _query(server, fqdn, dns.rdatatype.NSEC, tsig=tsig)
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


def check_keys(zone, keys, expected):
    """
    Checks keys for a configured zone. This verifies:
    1. The expected number of keys exist in 'keys'.
    2. The keys match the expected properties.
    """

    def _verify_keys():
        # check number of keys matches expected.
        if len(keys) != len(expected):
            return False

        if len(keys) == 0:
            return True

        for expect in expected:
            expect.key = None

        for key in keys:
            found = False
            i = 0
            while not found and i < len(expected):
                if expected[i].key is None:
                    found = key.match_properties(zone, expected[i])
                    if found:
                        key.external = expected[i].properties["legacy"]
                        expected[i].key = key
                i += 1
            if not found:
                return False

        return True

    isctest.run.retry_with_timeout(_verify_keys, timeout=10)


def check_keytimes(keys, expected):
    """
    Check the key timing metadata for all keys in 'keys'.
    """
    assert len(keys) == len(expected)

    if len(keys) == 0:
        return

    for key in keys:
        for expect in expected:
            if expect.properties["legacy"]:
                continue

            if not key is expect.key:
                continue

            synonyms = {}
            if "Generated" in expect.timing:
                synonyms["Created"] = expect.timing["Generated"]
            if "Published" in expect.timing:
                synonyms["Publish"] = expect.timing["Published"]
            if "PublishCDS" in expect.timing:
                synonyms["SyncPublish"] = expect.timing["PublishCDS"]
            if "Active" in expect.timing:
                synonyms["Activate"] = expect.timing["Active"]
            if "Retired" in expect.timing:
                synonyms["Inactive"] = expect.timing["Retired"]
            if "DeleteCDS" in expect.timing:
                synonyms["SyncDelete"] = expect.timing["DeleteCDS"]
            if "Revoked" in expect.timing:
                synonyms["Revoked"] = expect.timing["Revoked"]
            if "Removed" in expect.timing:
                synonyms["Delete"] = expect.timing["Removed"]

            assert key.match_timingmetadata(synonyms, file=key.keyfile, comment=True)
            if expect.properties["private"]:
                assert key.match_timingmetadata(synonyms, file=key.privatefile)
            if not expect.properties["legacy"]:
                assert key.match_timingmetadata(expect.timing)

                state_changes = [
                    "DNSKEYChange",
                    "KRRSIGChange",
                    "ZRRSIGChange",
                    "DSChange",
                ]
                for change in state_changes:
                    assert key.is_metadata_consistent(
                        change, expect.timing, checkval=False
                    )


def check_keyrelationships(keys, expected):
    """
    Check the key relationships (Successor and Predecessor metadata).
    """
    for key in keys:
        for expect in expected:
            if expect.properties["legacy"]:
                continue

            if not key is expect.key:
                continue

            relationship_status = ["Predecessor", "Successor"]
            for status in relationship_status:
                assert key.is_metadata_consistent(status, expect.metadata)


def check_dnssec_verify(server, zone, tsig=None):
    # Check if zone if DNSSEC valid with dnssec-verify.
    fqdn = f"{zone}."

    verified = False
    for _ in range(10):
        transfer = _query(server, fqdn, dns.rdatatype.AXFR, tsig=tsig)
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


def _check_signatures(
    signatures, covers, fqdn, keys, offline_ksk=False, zsk_missing=False
):
    numsigs = 0
    zrrsig = True
    if covers in [dns.rdatatype.DNSKEY, dns.rdatatype.CDNSKEY, dns.rdatatype.CDS]:
        zrrsig = False
    krrsig = not zrrsig

    for key in keys:
        ksigning, zsigning = key.get_signing_state(
            offline_ksk=offline_ksk, zsk_missing=zsk_missing
        )

        alg = key.get_metadata("Algorithm")
        rtype = dns.rdatatype.to_text(covers)

        expect = rf"IN RRSIG {rtype} {alg} (\d) (\d+) (\d+) (\d+) {key.tag} {fqdn}"

        if zrrsig and zsigning:
            has_rrsig = False
            for rrsig in signatures:
                if re.search(expect, rrsig) is not None:
                    has_rrsig = True
                    break
            assert has_rrsig, f"Expected signature but not found: {expect}"
            numsigs += 1

        if zrrsig and not zsigning:
            for rrsig in signatures:
                assert re.search(expect, rrsig) is None

        if krrsig and ksigning:
            has_rrsig = False
            for rrsig in signatures:
                if re.search(expect, rrsig) is not None:
                    has_rrsig = True
                    break
            assert has_rrsig, f"Expected signature but not found: {expect}"
            numsigs += 1

        if krrsig and not ksigning:
            for rrsig in signatures:
                assert re.search(expect, rrsig) is None

    return numsigs


def check_signatures(
    rrset, covers, fqdn, ksks, zsks, offline_ksk=False, zsk_missing=False
):
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

    numsigs += _check_signatures(
        signatures, covers, fqdn, ksks, offline_ksk=offline_ksk, zsk_missing=zsk_missing
    )
    numsigs += _check_signatures(
        signatures, covers, fqdn, zsks, offline_ksk=offline_ksk, zsk_missing=zsk_missing
    )

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
        publish = key.get_timing(publish_md, must_exist=False)
        delete = key.get_timing(delete_md, must_exist=False)
        published = publish is not None and now >= publish
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


def _query_rrset(server, fqdn, qtype, tsig=None):
    response = _query(server, fqdn, qtype, tsig=tsig)
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


def check_apex(
    server, zone, ksks, zsks, offline_ksk=False, zsk_missing=False, tsig=None
):
    # Test the apex of a zone. This checks that the SOA and DNSKEY RRsets
    # are signed correctly and with the appropriate keys.
    fqdn = f"{zone}."

    # test dnskey query
    dnskeys, rrsigs = _query_rrset(server, fqdn, dns.rdatatype.DNSKEY, tsig=tsig)
    check_dnskeys(dnskeys, ksks, zsks)
    check_signatures(
        rrsigs, dns.rdatatype.DNSKEY, fqdn, ksks, zsks, offline_ksk=offline_ksk
    )

    # test soa query
    soa, rrsigs = _query_rrset(server, fqdn, dns.rdatatype.SOA, tsig=tsig)
    assert len(soa) == 1
    assert f"{zone}. {DEFAULT_TTL} IN SOA" in soa[0].to_text()
    check_signatures(
        rrsigs,
        dns.rdatatype.SOA,
        fqdn,
        ksks,
        zsks,
        offline_ksk=offline_ksk,
        zsk_missing=zsk_missing,
    )

    # test cdnskey query
    cdnskeys, rrsigs = _query_rrset(server, fqdn, dns.rdatatype.CDNSKEY, tsig=tsig)
    check_dnskeys(cdnskeys, ksks, zsks, cdnskey=True)
    if len(cdnskeys) > 0:
        assert len(rrsigs) > 0
        check_signatures(
            rrsigs, dns.rdatatype.CDNSKEY, fqdn, ksks, zsks, offline_ksk=offline_ksk
        )

    # test cds query
    cds, rrsigs = _query_rrset(server, fqdn, dns.rdatatype.CDS, tsig=tsig)
    check_cds(cds, ksks)
    if len(cds) > 0:
        assert len(rrsigs) > 0
        check_signatures(
            rrsigs, dns.rdatatype.CDS, fqdn, ksks, zsks, offline_ksk=offline_ksk
        )


def check_subdomain(server, zone, ksks, zsks, offline_ksk=False, tsig=None):
    # Test an RRset below the apex and verify it is signed correctly.
    fqdn = f"{zone}."
    qname = f"a.{zone}."
    qtype = dns.rdatatype.A
    response = _query(server, qname, qtype, tsig=tsig)
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

    check_signatures(rrsigs, qtype, fqdn, ksks, zsks, offline_ksk=offline_ksk)


def verify_update_is_signed(server, fqdn, qname, qtype, rdata, ksks, zsks, tsig=None):
    """
    Test an RRset below the apex and verify it is updated and signed correctly.
    """
    response = _query(server, qname, qtype, tsig=tsig)

    if response.rcode() != dns.rcode.NOERROR:
        return False

    rrtype = dns.rdatatype.to_text(qtype)
    match = f"{qname} {DEFAULT_TTL} IN {rrtype} {rdata}"
    rrsigs = []
    for rrset in response.answer:
        if rrset.match(
            dns.name.from_text(qname), dns.rdataclass.IN, dns.rdatatype.RRSIG, qtype
        ):
            rrsigs.append(rrset)
        elif not match in rrset.to_text():
            return False

    if len(rrsigs) == 0:
        return False

    # Zone is updated, ready to verify the signatures.
    check_signatures(rrsigs, qtype, fqdn, ksks, zsks)

    return True


def verify_rrsig_is_refreshed(
    server, fqdn, zonefile, qname, qtype, ksks, zsks, tsig=None
):
    """
    Verify signature for RRset has been refreshed.
    """
    response = _query(server, qname, qtype, tsig=tsig)

    if response.rcode() != dns.rcode.NOERROR:
        return False

    rrtype = dns.rdatatype.to_text(qtype)
    match = f"{qname}. {DEFAULT_TTL} IN {rrtype}"
    rrsigs = []
    for rrset in response.answer:
        if rrset.match(
            dns.name.from_text(qname), dns.rdataclass.IN, dns.rdatatype.RRSIG, qtype
        ):
            rrsigs.append(rrset)
        elif not match in rrset.to_text():
            return False

    if len(rrsigs) == 0:
        return False

    tmp_zonefile = f"{zonefile}.tmp"
    isctest.run.cmd(
        [
            os.environ["CHECKZONE"],
            "-D",
            "-q",
            "-o",
            tmp_zonefile,
            "-f",
            "raw",
            fqdn,
            zonefile,
        ],
    )

    zone = dns.zone.from_file(tmp_zonefile, fqdn)
    for rrsig in rrsigs:
        if isctest.util.zone_contains(zone, rrsig):
            return False

    # Zone is updated, ready to verify the signatures.
    check_signatures(rrsigs, qtype, fqdn, ksks, zsks)

    return True


def verify_rrsig_is_reused(server, fqdn, zonefile, qname, qtype, ksks, zsks, tsig=None):
    """
    Verify signature for RRset has been reused.
    """
    response = _query(server, qname, qtype, tsig=tsig)

    assert response.rcode() == dns.rcode.NOERROR

    rrtype = dns.rdatatype.to_text(qtype)
    match = f"{qname}. {DEFAULT_TTL} IN {rrtype}"
    rrsigs = []
    for rrset in response.answer:
        if rrset.match(
            dns.name.from_text(qname), dns.rdataclass.IN, dns.rdatatype.RRSIG, qtype
        ):
            rrsigs.append(rrset)
        else:
            assert match in rrset.to_text()

    tmp_zonefile = f"{zonefile}.tmp"
    isctest.run.cmd(
        [
            os.environ["CHECKZONE"],
            "-D",
            "-q",
            "-o",
            tmp_zonefile,
            "-f",
            "raw",
            fqdn,
            zonefile,
        ],
    )

    zone = dns.zone.from_file(tmp_zonefile, dns.name.from_text(fqdn), relativize=False)
    for rrsig in rrsigs:
        assert isctest.util.zone_contains(zone, rrsig)

    check_signatures(rrsigs, qtype, fqdn, ksks, zsks)


def next_key_event_equals(server, zone, next_event):
    if next_event is None:
        # No next key event check.
        return True

    val = int(next_event.total_seconds())
    if val == 3600:
        waitfor = rf".*zone {zone}.*: next key event in (.*) seconds"
    else:
        # Don't want default loadkeys interval.
        waitfor = rf".*zone {zone}.*: next key event in (?!3600$)(.*) seconds"

    with server.watch_log_from_start() as watcher:
        watcher.wait_for_line(re.compile(waitfor))

    # WMM: The with code below is extracting the line the watcher was
    # waiting for. If WatchLog.wait_for_line()` returned the matched string,
    # we can use it directly on `re.match`.
    next_found = False
    minval = val - NEXT_KEY_EVENT_THRESHOLD
    maxval = val + NEXT_KEY_EVENT_THRESHOLD
    with open(f"{server.identifier}/named.run", "r", encoding="utf-8") as fp:
        for line in fp:
            match = re.match(waitfor, line)
            if match is not None:
                nextval = int(match.group(1))
                if minval <= nextval <= maxval:
                    next_found = True
                    break

                isctest.log.debug(
                    f"check next key event: expected {val} in: {line.strip()}"
                )

    return next_found


def keydir_to_keylist(
    zone: Optional[str], keydir: Optional[str] = None, in_use: bool = False
) -> List[Key]:
    """
    Retrieve all keys from the key files in a directory. If 'zone' is None,
    retrieve all keys in the directory, otherwise only those matching the
    zone name. If 'keydir' is None, search the current directory.
    """
    if zone is None:
        zone = ""

    all_keys = []
    if keydir is None:
        regex = rf"(K{zone}\.\+.*\+.*)\.key"
        for filename in glob.glob(f"K{zone}.+*+*.key"):
            match = re.match(regex, filename)
            if match is not None:
                all_keys.append(Key(match.group(1)))
    else:
        regex = rf"{keydir}/(K{zone}\.\+.*\+.*)\.key"
        for filename in glob.glob(f"{keydir}/K{zone}.+*+*.key"):
            match = re.match(regex, filename)
            if match is not None:
                all_keys.append(Key(match.group(1), keydir))

    states = ["GoalState", "DNSKEYState", "KRRSIGState", "ZRRSIGState", "DSState"]

    def used(kk):
        if not in_use:
            return True

        for state in states:
            val = kk.get_metadata(state, must_exist=False)
            if val not in ["undefined", "hidden"]:
                isctest.log.debug(f"key {kk} in use")
                return True

        return False

    return [k for k in all_keys if used(k)]


def keystr_to_keylist(keystr: str, keydir: Optional[str] = None) -> List[Key]:
    return [Key(name, keydir) for name in keystr.split()]


def policy_to_properties(ttl, keys: List[str]) -> List[KeyProperties]:
    """
    Get the policies from a list of specially formatted strings.
    The splitted line should result in the following items:
    line[0]: Role
    line[1]: Lifetime
    line[2]: Algorithm
    line[3]: Length
    Then, optional data for specific tests may follow:
    - "goal", "dnskey", "krrsig", "zrrsig", "ds", followed by a value,
      sets the given state to the specific value
    - "missing", set if the private key file for this key is not available.
    - "offset", an offset for testing key rollover timings
    """
    proplist = []
    count = 0
    for key in keys:
        count += 1
        line = key.split()
        keyprop = KeyProperties(f"KEY{count}", {}, {}, {})
        keyprop.properties["expect"] = True
        keyprop.properties["private"] = True
        keyprop.properties["legacy"] = False
        keyprop.properties["offset"] = timedelta(0)
        keyprop.properties["role"] = line[0]
        if line[0] == "zsk":
            keyprop.properties["role_full"] = "zone-signing"
            keyprop.properties["flags"] = 256
            keyprop.metadata["ZSK"] = "yes"
            keyprop.metadata["KSK"] = "no"
        else:
            keyprop.properties["role_full"] = "key-signing"
            keyprop.properties["flags"] = 257
            keyprop.metadata["ZSK"] = "yes" if line[0] == "csk" else "no"
            keyprop.metadata["KSK"] = "yes"

        keyprop.properties["dnskey_ttl"] = ttl
        keyprop.metadata["Algorithm"] = line[2]
        keyprop.metadata["Length"] = line[3]
        keyprop.metadata["Lifetime"] = 0
        if line[1] != "unlimited":
            keyprop.metadata["Lifetime"] = int(line[1])

        for i in range(4, len(line)):
            if line[i].startswith("goal:"):
                keyval = line[i].split(":")
                keyprop.metadata["GoalState"] = keyval[1]
            elif line[i].startswith("dnskey:"):
                keyval = line[i].split(":")
                keyprop.metadata["DNSKEYState"] = keyval[1]
            elif line[i].startswith("krrsig:"):
                keyval = line[i].split(":")
                keyprop.metadata["KRRSIGState"] = keyval[1]
            elif line[i].startswith("zrrsig:"):
                keyval = line[i].split(":")
                keyprop.metadata["ZRRSIGState"] = keyval[1]
            elif line[i].startswith("ds:"):
                keyval = line[i].split(":")
                keyprop.metadata["DSState"] = keyval[1]
            elif line[i].startswith("offset:"):
                keyval = line[i].split(":")
                keyprop.properties["offset"] = timedelta(seconds=int(keyval[1]))
            elif line[i] == "missing":
                keyprop.properties["private"] = False
            else:
                assert False, f"undefined optional data {line[i]}"

        proplist.append(keyprop)

    return proplist
