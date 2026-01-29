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

from datetime import datetime, timedelta, timezone
from functools import total_ordering
import glob
import os
from pathlib import Path
import re
from re import compile as Re
import time
from typing import Dict, List, Optional, Tuple, Union

import dns
import dns.dnssec
import dns.rdatatype
import dns.rrset
import dns.tsig

import pytest

import isctest.log
import isctest.query
import isctest.util
from isctest.compat import DSDigest
from isctest.instance import NamedInstance
from isctest.template import TrustAnchor
from isctest.vars.algorithms import Algorithm, ALL_ALGORITHMS_BY_NUM

DEFAULT_TTL = 300

NEXT_KEY_EVENT_THRESHOLD = 100


def _query(server, qname, qtype, tsig=None):
    query = isctest.query.create(qname, qtype)

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


def Ipub(config):
    return (
        config["dnskey-ttl"]
        + config["zone-propagation-delay"]
        + config["publish-safety"]
    )


def IpubC(config, rollover=True):
    ttl1 = config["dnskey-ttl"] + config["publish-safety"]
    ttl2 = timedelta(0)

    if not rollover:
        # If this is the first key, we also need to wait until the zone
        # signatures are omnipresent. Use max-zone-ttl instead of
        # dnskey-ttl, and no publish-safety (because we are looking at
        # signatures here, not the public key).
        ttl2 = config["max-zone-ttl"]

    return config["zone-propagation-delay"] + max(ttl1, ttl2)


def Iret(config, zsk=True, ksk=False, rollover=True, smooth=True):
    sign_delay = timedelta(0)
    safety_interval = timedelta(0)
    if rollover:
        if smooth:
            sign_delay = config["signatures-validity"] - config["signatures-refresh"]
        safety_interval = config["retire-safety"]

    iretKSK = timedelta(0)
    if ksk:
        # KSK: Double-KSK Method: Iret = DprpP + TTLds
        iretKSK = (
            config["parent-propagation-delay"] + config["ds-ttl"] + safety_interval
        )

    iretZSK = timedelta(0)
    if zsk:
        # ZSK: Pre-Publication Method: Iret = Dsgn + Dprp + TTLsig
        iretZSK = (
            sign_delay
            + config["zone-propagation-delay"]
            + config["max-zone-ttl"]
            + safety_interval
        )

    return max(iretKSK, iretZSK)


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
        metadata: dict,
        timing: Dict[str, KeyTimingMetadata],
        private: bool = True,
        legacy: bool = False,
        role: str = "csk",
        ttl: int = 3600,
        flags: int = 257,
        keytag_min: int = 0,
        keytag_max: int = 65535,
        offset: Union[timedelta, int] = 0,
    ):
        self.name = name
        self.key = None
        self.metadata = metadata
        self.timing = timing
        # Properties
        self.private = private
        self.legacy = legacy
        self.role = role
        self.ttl = ttl
        self.flags = flags
        self.keytag_min = keytag_min
        self.keytag_max = keytag_max
        self.offset = offset

    def __repr__(self):
        return self.name

    def __str__(self) -> str:
        return self.name

    @staticmethod
    def default(with_state=True) -> "KeyProperties":
        metadata = {
            "Algorithm": isctest.vars.algorithms.ECDSAP256SHA256.number,
            "Length": 256,
            "Lifetime": 0,
            "KSK": "yes",
            "ZSK": "yes",
        }
        timing: Dict[str, KeyTimingMetadata] = {}

        result = KeyProperties(name="DEFAULT", metadata=metadata, timing=timing)
        result.name = "DEFAULT"
        result.key = None
        if with_state:
            result.metadata["GoalState"] = "omnipresent"
            result.metadata["DNSKEYState"] = "rumoured"
            result.metadata["KRRSIGState"] = "rumoured"
            result.metadata["ZRRSIGState"] = "rumoured"
            result.metadata["DSState"] = "hidden"

        return result

    def role_full(self) -> str:
        if self.flags == 256:
            return "zone-signing"
        return "key-signing"

    def Ipub(self, config):
        ipub = timedelta(0)

        if self.key.get_metadata("Predecessor", must_exist=False) != "undefined":
            ipub = Ipub(config)

        self.timing["Active"] = self.timing["Published"] + ipub

    def IpubC(self, config):
        if not self.key.is_ksk():
            return

        rollover = self.key.get_metadata("Predecessor", must_exist=False) != "undefined"
        ipubc = IpubC(config, rollover)

        self.timing["PublishCDS"] = self.timing["Published"] + ipubc

        if "Lifetime" in self.metadata and self.metadata["Lifetime"] != 0:
            self.timing["DeleteCDS"] = (
                self.timing["PublishCDS"] + self.metadata["Lifetime"]
            )

    def Iret(self, config):
        if "Lifetime" not in self.metadata or self.metadata["Lifetime"] == 0:
            return

        sigdel = self.key.get_timing("SigRemoved", must_exist=False)
        smooth = sigdel is None
        iret = Iret(config, zsk=self.key.is_zsk(), ksk=self.key.is_ksk(), smooth=smooth)
        self.timing["Removed"] = self.timing["Retired"] + iret

    def set_expected_keytimes(
        self, config, offset=None, pregenerated=False, migrate=False
    ):
        if self.key is None:
            raise ValueError("KeyProperties must be attached to a Key")

        if self.legacy:
            return

        if offset is None:
            offset = self.offset

        self.timing["Generated"] = self.key.get_timing("Created")
        self.timing["Published"] = self.key.get_timing("Created")
        if pregenerated:
            self.timing["Published"] = self.key.get_timing("Publish")

        if migrate:
            self.timing["Published"] = self.key.get_timing("Publish")
            if self.key.is_ksk():
                self.timing["PublishCDS"] = self.key.get_timing("SyncPublish")
            self.timing["Active"] = self.key.get_timing("Activate")
        else:
            self.timing["Published"] = self.timing["Published"] + offset
            self.Ipub(config)
            self.IpubC(config)

        # Set Retired timing metadata if key has lifetime.
        if "Lifetime" in self.metadata and self.metadata["Lifetime"] != 0:
            self.timing["Retired"] = self.timing["Active"] + self.metadata["Lifetime"]

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
        self.external = False

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
        self, offline_ksk=False, zsk_missing=False, smooth=False
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

        If 'smooth' is set to True, it means a smooth ZSK rollover is
        initiated. Signatures are being replaced gradually during a ZSK
        rollover, so the existing signatures of the predecessor ZSK are still
        being used, thus the predecessor is expected to be signing.
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
        if smooth:
            zsigning = zrrsigstate in ["unretentive", "omnipresent"]

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

    @property
    def dnskey(self) -> dns.rrset.RRset:
        pytest.importorskip("dns", minversion="2.2.0")  # dns.zonefile.read_rrsets
        with open(self.keyfile, "r", encoding="utf-8") as file:
            rrsets = dns.zonefile.read_rrsets(
                file.read(),
                rdclass=None,  # read rdclass from the file
                default_ttl=DEFAULT_TTL,  # use this TTL if not present
            )
        assert len(rrsets) == 1, f"{self.keyfile} has multiple RRsets"
        dnskey_rr = rrsets[0]
        assert len(dnskey_rr) == 1, f"{self.keyfile} has multiple RRs"
        assert (
            dnskey_rr.rdtype == dns.rdatatype.DNSKEY
        ), f"DNSKEY not found in {self.keyfile}"
        return dnskey_rr

    def into_ta(self, ta_type: str, dsdigest=DSDigest.SHA256) -> TrustAnchor:
        dnskey = self.dnskey
        if ta_type in ["static-ds", "initial-ds"]:
            ds = dns.dnssec.make_ds(dnskey.name, dnskey[0], dsdigest)
            parts = str(ds).split()
            contents = " ".join(parts[:3]) + f' "{parts[3]}"'
        elif ta_type in ["static-key", "initial-key"]:
            parts = str(dnskey).split()
            contents = " ".join(parts[4:7]) + f' "{"".join(parts[7:])}"'
        else:
            raise ValueError(f"invalid trust anchor type: {ta_type}")
        return TrustAnchor(str(dnskey.name), ta_type, contents)

    def is_ksk(self) -> bool:
        return self.get_metadata("KSK") == "yes"

    def is_zsk(self) -> bool:
        return self.get_metadata("ZSK") == "yes"

    @property
    def algorithm(self) -> Algorithm:
        num = int(self.get_metadata("Algorithm"))
        return ALL_ALGORITHMS_BY_NUM[num]

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

        cmd = isctest.run.cmd(dsfromkey_command)
        dsfromkey = cmd.out.split()

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
        # Check file existence.
        # Noop. If file is missing then the get_metadata calls will fail.

        # Check the public key file.
        role = properties.role_full()
        comment = f"This is a {role} key, keyid {self.tag}, for {zone}."
        if not isctest.util.file_contents_contain(self.keyfile, comment):
            isctest.log.debug(f"{self.name} COMMENT MISMATCH: expected '{comment}'")
            return False

        ttl = properties.ttl
        flags = properties.flags
        alg = properties.metadata["Algorithm"]
        dnskey = f"{zone}. {ttl} IN DNSKEY {flags} 3 {alg}"
        if not isctest.util.file_contents_contain(self.keyfile, dnskey):
            isctest.log.debug(f"{self.name} DNSKEY MISMATCH: expected '{dnskey}'")
            return False

        # Now check the private key file.
        if properties.private:
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
        if properties.legacy:
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

        # Check tag range.
        if self.tag < properties.keytag_min:
            return False
        if self.tag > properties.keytag_max:
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


def check_keys(zone, keys, expected):
    """
    Checks keys for a configured zone. This verifies:
    1. The expected number of keys exist in 'keys'.
    2. The keys match the expected properties.
    """

    def _verify_keys():
        # check number of keys matches expected.
        if len(keys) != len(expected):
            assert (
                False
            ), f"check_keys(): mismatched number of keys, expected {len(expected)}, got {len(keys)}"

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
                        key.external = expected[i].legacy
                        expected[i].key = key
                i += 1
            if not found:
                assert False, f"check_keys(): key {key} not found"

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
            if expect.legacy:
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
            if expect.private:
                assert key.match_timingmetadata(synonyms, file=key.privatefile)
            if not expect.legacy:
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
            if expect.legacy:
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

            verify_command = [os.environ.get("VERIFY"), "-z", "-o", zone, zonefile]
            verified = isctest.run.cmd(verify_command, raise_on_exception=False)
            if verified.rc == 0:
                return

        time.sleep(1)

    assert False, "zone not verified"


def check_dnssecstatus(server, zone, keys, policy=None, view=None):
    # Call rndc dnssec -status on 'server' for 'zone'. Expect 'policy' in
    # the output. This is a loose verification, it just tests if the right
    # policy name is returned, and if all expected keys are listed.
    response = ""
    if view is None:
        response = server.rndc(f"dnssec -status {zone}")
    else:
        response = server.rndc(f"dnssec -status {zone} in {view}")

    if policy is None:
        assert "Zone does not have dnssec-policy" in response.out
        return

    assert f"dnssec-policy: {policy}" in response.out

    for key in keys:
        if not key.external:
            assert f"key: {key.tag}" in response.out


def _check_signatures(
    signatures,
    covers,
    fqdn,
    keys,
    offline_ksk=False,
    zsk_missing=False,
    smooth=False,
):
    numsigs = 0
    zrrsig = True
    if covers in [dns.rdatatype.DNSKEY, dns.rdatatype.CDNSKEY, dns.rdatatype.CDS]:
        zrrsig = False
    krrsig = not zrrsig

    for key in keys:
        if key.external:
            continue

        ksigning, zsigning = key.get_signing_state(
            offline_ksk=offline_ksk, zsk_missing=zsk_missing, smooth=smooth
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
    rrset, covers, fqdn, ksks, zsks, offline_ksk=False, zsk_missing=False, smooth=False
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
        signatures,
        covers,
        fqdn,
        ksks,
        offline_ksk=offline_ksk,
        zsk_missing=zsk_missing,
        smooth=smooth,
    )
    numsigs += _check_signatures(
        signatures,
        covers,
        fqdn,
        zsks,
        offline_ksk=offline_ksk,
        zsk_missing=zsk_missing,
        smooth=smooth,
    )

    assert numsigs == len(signatures)


def _check_dnskeys(dnskeys, keys, cdnskey=False, manual_mode=False):
    now = KeyTimingMetadata.now()
    numkeys = 0

    publish_md = "Publish"
    delete_md = "Delete"
    if cdnskey:
        publish_md = f"Sync{publish_md}"
        delete_md = f"Sync{delete_md}"

    for key in keys:
        if manual_mode:
            # State transitions may be blocked, preset key timings may not
            # be accurate. Use the state values to determine whether the
            # CDS must be published or not.
            if cdnskey:
                md = key.get_metadata("DSState")
            else:
                md = key.get_metadata("DNSKEYState")
            published = md in ["omnipresent", "rumoured"]
            removed = not published
        else:
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

        if not published or removed:
            if not cdnskey:
                assert has_dnskey

        if has_dnskey:
            numkeys += 1

    return numkeys


def check_dnskeys(rrset, ksks, zsks, cdnskey=False, manual_mode=False):
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

    numkeys += _check_dnskeys(dnskeys, ksks, cdnskey=cdnskey, manual_mode=manual_mode)
    if not cdnskey:
        numkeys += _check_dnskeys(dnskeys, zsks, manual_mode=manual_mode)

    assert numkeys == len(dnskeys)


def check_cds(cdss, keys, alg, manual_mode=False):
    # Check if the correct CDS records are published. If the current time
    # is between the timing metadata 'publish' and 'delete', the key must have
    # a CDS record published.
    now = KeyTimingMetadata.now()
    numcds = 0

    for key in keys:
        assert key.is_ksk()

        if manual_mode:
            # State transitions may be blocked, preset key timings may not
            # be accurate. Use the state values to determine whether the
            # CDS must be published or not.
            md = key.get_metadata("DSState")
            published = md in ["omnipresent", "rumoured"]
            removed = not published
        else:
            publish = key.get_timing("SyncPublish")
            delete = key.get_timing("SyncDelete", must_exist=False)
            published = now >= publish
            removed = delete is not None and delete <= now

        if not published or removed:
            for cds in cdss:
                assert not key.cds_equals(cds, alg)
            continue

        has_cds = False
        for cds in cdss:
            if key.cds_equals(cds, alg):
                has_cds = True
                break

        if not published or removed:
            assert has_cds

        if has_cds:
            numcds += 1

    return numcds


def check_cds_prohibit(cdss, keys, alg):
    # Check if the CDS records are not published. This does not take into
    # account the timing metadata, just making sure that the given algorithm
    # does not get published.
    for key in keys:
        for cds in cdss:
            assert not key.cds_equals(cds, alg)


def check_cdslog(server, zone, key, substr):
    with server.watch_log_from_start() as watcher:
        watcher.wait_for_line(
            f"{substr} for key {zone}/{key.algorithm.name}/{key.tag} is now published"
        )


def check_cdslog_prohibit(server, zone, key, substr):
    msg = f"{substr} for key {zone}/{key.algorithm.name}/{key.tag} is now published"
    assert msg not in server.log


def check_cdsdelete(rrset, expected):
    numrrs = 0
    for rr in rrset:
        for rdata in rr:
            assert expected in f"{rdata}"
            numrrs += 1
    assert numrrs == 1


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
    server,
    zone,
    ksks,
    zsks,
    cdss=None,
    cds_delete=False,
    manual_mode=False,
    offline_ksk=False,
    zsk_missing=False,
    tsig=None,
):
    # Test the apex of a zone. This checks that the SOA and DNSKEY RRsets
    # are signed correctly and with the appropriate keys.
    fqdn = f"{zone}."

    if cdss is None:
        cdss = ["CDNSKEY", "CDS (SHA-256)"]

    # test dnskey query
    dnskeys, rrsigs = _query_rrset(server, fqdn, dns.rdatatype.DNSKEY, tsig=tsig)
    check_dnskeys(dnskeys, ksks, zsks, manual_mode=manual_mode)
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

    if cds_delete:
        check_cdsdelete(cdnskeys, "0 3 0 AA==")
    else:
        if "CDNSKEY" in cdss:
            check_dnskeys(cdnskeys, ksks, zsks, cdnskey=True, manual_mode=manual_mode)
        else:
            assert len(cdnskeys) == 0

    if len(cdnskeys) > 0:
        assert len(rrsigs) > 0
        check_signatures(
            rrsigs, dns.rdatatype.CDNSKEY, fqdn, ksks, zsks, offline_ksk=offline_ksk
        )

    # test cds query
    cds, rrsigs = _query_rrset(server, fqdn, dns.rdatatype.CDS, tsig=tsig)

    if cds_delete:
        check_cdsdelete(cds, "0 0 0 00")
    else:
        cdsrrs = []
        for rr in cds:
            for rdata in rr:
                rdclass = dns.rdataclass.to_text(rr.rdclass)
                rdtype = dns.rdatatype.to_text(rr.rdtype)
                cds = f"{rr.name} {rr.ttl} {rdclass} {rdtype} {rdata}"
                cdsrrs.append(cds)

        numcds = 0

        for alg in ["SHA-256", "SHA-384"]:
            if f"CDS ({alg})" in cdss:
                numcds += check_cds(cdsrrs, ksks, alg, manual_mode=manual_mode)
            else:
                check_cds_prohibit(cdsrrs, ksks, alg)

        if len(cds) > 0:
            assert len(rrsigs) > 0
            check_signatures(
                rrsigs, dns.rdatatype.CDS, fqdn, ksks, zsks, offline_ksk=offline_ksk
            )

        assert numcds == len(cdsrrs)


def check_subdomain(
    server, zone, ksks, zsks, offline_ksk=False, smooth=False, tsig=None
):
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

    check_signatures(
        rrsigs, qtype, fqdn, ksks, zsks, offline_ksk=offline_ksk, smooth=smooth
    )


def check_rollover_step(server, config, policy, step):
    zone = step["zone"]
    keyprops = step["keyprops"]
    nextev = step.get("nextev", None)
    cdss = step.get("cdss", None)
    keyrelationships = step.get("keyrelationships", None)
    smooth = step.get("smooth", False)
    ds_swap = step.get("ds-swap", True)
    cds_delete = step.get("cds-delete", False)
    check_keytimes_flag = step.get("check-keytimes", True)
    zone_signed = step.get("zone-signed", True)
    manual_mode = step.get("manual-mode", False)

    isctest.log.info(f"check rollover step {zone}")

    if zone_signed:
        check_dnssec_verify(server, zone)

    ttl = int(config["dnskey-ttl"].total_seconds())
    expected = policy_to_properties(ttl, keyprops)
    keys = keydir_to_keylist(zone, server.identifier)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]
    check_keys(zone, keys, expected)

    for kp in expected:
        key = kp.key

        # Set expected key timing metadata.
        kp.set_expected_keytimes(config)

        # Set rollover relationships.
        if keyrelationships is not None:
            prd = keyrelationships[0]
            suc = keyrelationships[1]
            expected[prd].metadata["Successor"] = expected[suc].key.tag
            expected[suc].metadata["Predecessor"] = expected[prd].key.tag
            check_keyrelationships(keys, expected)

        # Policy changes may retire keys, set expected timing metadata.
        if kp.metadata["GoalState"] == "hidden" and "Retired" not in kp.timing:
            retired = kp.key.get_timing("Inactive")
            kp.timing["Retired"] = retired
            kp.timing["Removed"] = retired + Iret(
                config, zsk=key.is_zsk(), ksk=key.is_ksk()
            )

        # Check that CDS publication/withdrawal is logged.
        if "KSK" not in kp.metadata:
            continue
        if kp.metadata["KSK"] == "no":
            continue

        if ds_swap and kp.metadata["DSState"] == "rumoured":
            assert cdss is not None
            for algstr in ["CDNSKEY", "CDS (SHA-256)", "CDS (SHA-384)"]:
                if algstr in cdss:
                    check_cdslog(server, zone, key, algstr)
                else:
                    check_cdslog_prohibit(server, zone, key, algstr)

            # The DS can be introduced. We ignore any parent registration delay,
            # so set the DS publish time to now.
            server.rndc(f"dnssec -checkds -key {key.tag} published {zone}")

        if ds_swap and kp.metadata["DSState"] == "unretentive":
            # The DS can be withdrawn. We ignore any parent registration
            # delay, so set the DS withdraw time to now.
            server.rndc(f"dnssec -checkds -key {key.tag} withdrawn {zone}")

    if check_keytimes_flag:
        check_keytimes(keys, expected)

    check_dnssecstatus(server, zone, keys, policy=policy)
    check_apex(
        server,
        zone,
        ksks,
        zsks,
        cdss=cdss,
        cds_delete=cds_delete,
        manual_mode=manual_mode,
    )
    check_subdomain(server, zone, ksks, zsks, smooth=smooth)

    def check_next_key_event():
        return next_key_event_equals(server, zone, nextev)

    if nextev is not None:
        isctest.run.retry_with_timeout(check_next_key_event, timeout=5)

    return expected


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
        watcher.wait_for_line(Re(waitfor))

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
    - "tag-range", followed by <min>-<max> to test key tag ranges
    """
    proplist = []
    count = 0
    for key in keys:
        count += 1
        line = key.split()

        # defaults
        metadata: Dict[str, Union[str, int]] = {}
        timing: Dict[str, KeyTimingMetadata] = {}
        private = True
        legacy = False
        keytag_min = 0
        keytag_max = 65535
        offset = timedelta(0)

        role = line[0]
        if role == "zsk":
            flags = 256
            metadata["ZSK"] = "yes"
            metadata["KSK"] = "no"
        else:
            flags = 257
            metadata["ZSK"] = "yes" if role == "csk" else "no"
            metadata["KSK"] = "yes"

        metadata["Algorithm"] = line[2]
        metadata["Length"] = line[3]
        if line[1] == "unlimited":
            metadata["Lifetime"] = 0
        elif line[1] != "-":
            metadata["Lifetime"] = int(line[1])

        for i in range(4, len(line)):
            if line[i].startswith("goal:"):
                keyval = line[i].split(":")
                metadata["GoalState"] = keyval[1]
            elif line[i].startswith("dnskey:"):
                keyval = line[i].split(":")
                metadata["DNSKEYState"] = keyval[1]
            elif line[i].startswith("krrsig:"):
                keyval = line[i].split(":")
                metadata["KRRSIGState"] = keyval[1]
            elif line[i].startswith("zrrsig:"):
                keyval = line[i].split(":")
                metadata["ZRRSIGState"] = keyval[1]
            elif line[i].startswith("ds:"):
                keyval = line[i].split(":")
                metadata["DSState"] = keyval[1]
            elif line[i].startswith("offset:"):
                keyval = line[i].split(":")
                offset = timedelta(seconds=int(keyval[1]))
            elif line[i].startswith("tag-range:"):
                keyval = line[i].split(":")
                tagrange = keyval[1].split("-")
                keytag_min = int(tagrange[0])
                keytag_max = int(tagrange[1])
            elif line[i] == "missing":
                private = False
            else:
                assert False, f"undefined optional data {line[i]}"

        keyprop = KeyProperties(
            name=f"KEY{count}",
            metadata=metadata,
            timing=timing,
            private=private,
            legacy=legacy,
            role=role,
            ttl=ttl,
            flags=flags,
            keytag_min=keytag_min,
            keytag_max=keytag_max,
            offset=offset,
        )
        proplist.append(keyprop)

    return proplist


def wait_keymgr_done(server: NamedInstance, zone: str, reconfig: bool = False) -> None:
    """
    Block and wait until the keymgr is done processing zone.
    """
    messages = []
    if reconfig:
        messages.append("received control channel command 'reconfig'")
    messages.append(f"keymgr: {zone} done")
    with server.watch_log_from_start(timeout=60) as watcher:
        watcher.wait_for_sequence(messages)


def private_type_record(zone: str, key: Key, rrtype: int = 65534) -> str:
    """
    Write a private type record recording the state of the signing process for
    a given zone and key, print the private type record with given RRtype,
    indicating that the signing process for this key is completed.
    """
    keyid = key.tag
    secalg = int(key.get_metadata("Algorithm"))
    return f"{zone}. 0 IN TYPE{rrtype} \\# 5 {secalg:02x}{keyid:04x}0000"
