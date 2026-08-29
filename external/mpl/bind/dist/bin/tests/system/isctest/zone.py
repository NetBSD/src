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

from __future__ import annotations

from abc import ABC, abstractmethod
from collections.abc import Callable
from pathlib import Path
from typing import TypeAlias

import base64
import shutil

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec, ed448, ed25519, rsa
from dns.rdtypes.dnskeybase import Flag

import dns.dnssec
import dns.name
import dns.rdataclass
import dns.rdatatype
import dns.rrset
import dns.zonefile

from .algorithms import (
    ALL_ALGORITHMS_BY_NUM,
    ECDSAP256SHA256,
    ECDSAP384SHA384,
    ED448,
    ED25519,
    Algorithm,
)
from .log import debug
from .run import EnvCmd
from .template import NS1, Nameserver, TemplateEngine, TrustAnchor

KEYDIR = "keys"
DNSKEY_TTL = 3600

PrivateKey: TypeAlias = (
    ec.EllipticCurvePrivateKey
    | ed25519.Ed25519PrivateKey
    | ed448.Ed448PrivateKey
    | rsa.RSAPrivateKey
)


class ZoneKey(ABC):
    """
    Abstract base for a DNSSEC key attached to a Zone.

    Two concrete implementations exist:
      FileZoneKey    — reads a dnssec-keygen-managed key file pair.
      PythonZoneKey  — holds a Python-native (private_key, dnskey_rdata) pair
                       required for dnspython-based operations and signing.

    The interface covers the zone-infrastructure subset: trust anchor
    generation and DS propagation to parent zones.
    """

    @property
    @abstractmethod
    def dnskey(self) -> dns.rrset.RRset:
        """
        The DNSKEY RRset for this key (single-record, with TTL).
        """

    @property
    @abstractmethod
    def private_key(self) -> PrivateKey:
        """
        The dnspython private-key object, for use with dns.dnssec signing.
        """

    def is_ksk(self) -> bool:
        return bool(self.dnskey[0].flags & int(Flag.SEP))

    @abstractmethod
    def write_dsset(
        self,
        target_dir: Path | str,
        dsdigest: dns.dnssec.DSDigest = dns.dnssec.DSDigest.SHA256,
    ) -> None:
        """
        Write or copy dsset-{zone}. into target_dir.

        For FileZoneKey: copies the dsset file produced by dnssec-signzone.
        For PythonZoneKey: derives the DS from the in-memory key and writes it.
        """

    def into_ta(
        self,
        ta_type: str = "static-ds",
        dsdigest: dns.dnssec.DSDigest = dns.dnssec.DSDigest.SHA256,
    ) -> TrustAnchor:
        """
        Build a named.conf trust-anchor stanza from this key.

        ta_type must be one of: static-ds, initial-ds, static-key, initial-key.
        Implemented once here; both subclasses inherit it via self.dnskey.
        """
        dnskey = self.dnskey
        if ta_type in ["static-ds", "initial-ds"]:
            ds = dns.dnssec.make_ds(dnskey.name, dnskey[0], dsdigest)
            parts = str(ds).split()
            contents = " ".join(parts[:3]) + f' "{parts[3]}"'
        elif ta_type in ["static-key", "initial-key"]:
            parts = str(dnskey).split()
            contents = " ".join(parts[4:7]) + f' "{"".join(parts[7:])}"'
        else:
            raise ValueError(f"invalid trust anchor type: {ta_type!r}")
        return TrustAnchor(str(dnskey.name), ta_type, contents)


class FileZoneKey(ZoneKey):
    """
    A ZoneKey backed by dnssec-keygen-managed key files.

    Reads key material directly from the .key/.private file pair.  Normally
    constructed via FileZoneKey.generate() (or Zone.add_keys());
    isctest.kasp.Key subclasses it to add KASP timing/state operations.
    """

    def __init__(
        self,
        name: str,
        keydir: str | Path | None = None,
        zone: Zone | None = None,
    ) -> None:
        self.name = name
        self.keydir = Path() if keydir is None else Path(keydir)
        self.path = str(self.keydir / name)
        self.privatefile = f"{self.path}.private"
        self.keyfile = f"{self.path}.key"
        self.tag = int(self.name[-5:])
        self.zone = zone

    @property
    def dnskey(self) -> dns.rrset.RRset:
        with open(self.keyfile, "r", encoding="utf-8") as file:
            rrsets = dns.zonefile.read_rrsets(
                file.read(),
                rdclass=None,  # read rdclass from the file
                default_ttl=DNSKEY_TTL,  # use this TTL if not present
            )
        assert len(rrsets) == 1, f"{self.keyfile} has multiple RRsets"
        dnskey_rr = rrsets[0]
        assert len(dnskey_rr) == 1, f"{self.keyfile} has multiple RRs"
        assert (
            dnskey_rr.rdtype == dns.rdatatype.DNSKEY
        ), f"DNSKEY not found in {self.keyfile}"
        return dnskey_rr

    @property
    def private_key(self) -> PrivateKey:
        """
        Read the .private file and build the corresponding cryptography
        private-key object, dispatching on the DNSKEY algorithm.

        Supports the RSA, ECDSA and EdDSA families; other algorithms raise
        NotImplementedError.
        """
        alg = ALL_ALGORITHMS_BY_NUM[self.dnskey[0].algorithm]

        fields = {}
        with open(self.privatefile, "r", encoding="utf-8") as file:
            for line in file:
                name, sep, value = line.partition(":")
                if sep:
                    fields[name.strip()] = value.strip()

        def field(name: str) -> bytes:
            return base64.b64decode(fields[name])

        if "Modulus" in fields:  # RSA family, including the private-OID variants
            public = rsa.RSAPublicNumbers(
                int.from_bytes(field("PublicExponent"), "big"),
                int.from_bytes(field("Modulus"), "big"),
            )
            return rsa.RSAPrivateNumbers(
                p=int.from_bytes(field("Prime1"), "big"),
                q=int.from_bytes(field("Prime2"), "big"),
                d=int.from_bytes(field("PrivateExponent"), "big"),
                dmp1=int.from_bytes(field("Exponent1"), "big"),
                dmq1=int.from_bytes(field("Exponent2"), "big"),
                iqmp=int.from_bytes(field("Coefficient"), "big"),
                public_numbers=public,
            ).private_key()

        secret = field("PrivateKey")
        if alg == ECDSAP256SHA256:
            return ec.derive_private_key(int.from_bytes(secret, "big"), ec.SECP256R1())
        if alg == ECDSAP384SHA384:
            return ec.derive_private_key(int.from_bytes(secret, "big"), ec.SECP384R1())
        if alg == ED25519:
            return ed25519.Ed25519PrivateKey.from_private_bytes(secret)
        if alg == ED448:
            return ed448.Ed448PrivateKey.from_private_bytes(secret)
        raise NotImplementedError(f"dnskey algorithm {alg.name} not implemented")

    def write_dsset(
        self,
        target_dir: Path | str,
        dsdigest: dns.dnssec.DSDigest = dns.dnssec.DSDigest.SHA256,
    ) -> None:
        """
        Copy the dnssec-signzone-produced dsset-{zone}. into target_dir.

        dsdigest is accepted for interface compatibility but ignored: the dsset
        file is produced by dnssec-signzone (SHA-256). This copy overwrites any
        existing dsset file, so a zone must not mix FileZoneKey and
        PythonZoneKey KSKs (PythonZoneKey appends to the same file);
        Zone.copy_dssets enforces this.
        """
        assert self.zone is not None, "write_dsset requires a zone-attached key"
        src = Path(self.zone.ns.name) / f"dsset-{self.zone.name}."
        dst = Path(target_dir) / src.name
        if src.resolve() == dst.resolve():
            debug(f"{self.zone.name}: dsset already in {target_dir}")
            return
        shutil.copy(src, dst)
        debug(f"{self.zone.name}: dsset copied to {target_dir}")

    @staticmethod
    def generate(
        zone: Zone,
        params: str = "",
        alg: Algorithm | None = None,
    ) -> FileZoneKey:
        """
        Generate a DNSSEC key via dnssec-keygen for zone and return it.

        Runs dnssec-keygen in zone.ns.name/keys/, stores the key there, and
        returns the resulting FileZoneKey.  Pass params="-f KSK" to generate a
        Key Signing Key; omit it (or pass "") for a Zone Signing Key.
        """
        debug(f"{zone.name}: generating key using dnssec-keygen")
        keydir = Path(zone.ns.name) / KEYDIR
        keydir.mkdir(exist_ok=True)
        if alg is None:
            alg = Algorithm.default()
        keygen = EnvCmd(
            "KEYGEN", f"-q -a {alg.number} -b {alg.bits} -K {KEYDIR} -L {DNSKEY_TTL}"
        )
        key_name = keygen(f"{params} {zone.name}", cwd=zone.ns.name).out.strip()
        return FileZoneKey(key_name, keydir=keydir, zone=zone)


class PythonZoneKey(ZoneKey):
    """
    A ZoneKey holding a Python-native keypair.

    Construct via PythonZoneKey.generate() to create fresh key
    material, or instantiate directly when you already have a private key and
    dnskey rdata (e.g. when loading a saved PEM).

    Attach to a Zone via zone.keys = [key] so that Zone.copy_dssets() can
    generate the dsset-* file for the parent zone and Zone.trust_anchors() can
    produce the correct trust anchor stanzas.

    Zone.sign() raises TypeError if self.keys contains a PythonZoneKey,
    because dnssec-signzone cannot use in-memory keys.  Sign the zone
    with dns.dnssec.sign_zone() directly instead.

    The raw private key object is available as .private_key for callers that
    need it (e.g. to write a PEM file for a custom authoritative server).
    Use write_private_key_pem() as a convenience for the common case.
    """

    def __init__(
        self,
        zone: Zone,
        private_key,
        dnskey_rdata,
        ttl: int = DNSKEY_TTL,
    ) -> None:
        self.zone = zone
        self._private_key = private_key
        self._dnskey_rdata = dnskey_rdata
        self.ttl = ttl

    @property
    def private_key(self) -> PrivateKey:
        return self._private_key

    @property
    def dnskey(self) -> dns.rrset.RRset:
        rrset = dns.rrset.RRset(
            self.zone.dname, dns.rdataclass.IN, dns.rdatatype.DNSKEY
        )
        rrset.update_ttl(self.ttl)
        rrset.add(self._dnskey_rdata)
        return rrset

    def write_dsset(
        self,
        target_dir: Path | str,
        dsdigest: dns.dnssec.DSDigest = dns.dnssec.DSDigest.SHA256,
    ) -> None:
        target = Path(target_dir)
        target.mkdir(parents=True, exist_ok=True)
        ds = dns.dnssec.make_ds(self.zone.dname, self._dnskey_rdata, dsdigest)
        text = (
            f"{self.zone.name}. {self.ttl} IN DS"
            f" {ds.key_tag} {ds.algorithm} {ds.digest_type}"
            f" {ds.digest.hex().upper()}\n"
        )
        with (target / f"dsset-{self.zone.name}.").open("a") as f:
            f.write(text)

    def write_private_key_pem(self, path: Path | str) -> None:
        """Write the private key to path in PKCS8 PEM format (no encryption)."""
        Path(path).write_bytes(
            self.private_key.private_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PrivateFormat.PKCS8,
                encryption_algorithm=serialization.NoEncryption(),
            )
        )

    @staticmethod
    def generate(
        zone: Zone,
        flags: int = int(Flag.ZONE | Flag.SEP),
        alg: Algorithm | None = None,
        ttl: int = DNSKEY_TTL,
    ) -> PythonZoneKey:
        """
        Generate a Python-native DNSSEC keypair for the given algorithm.

        Unlike FileZoneKey.generate(), this does not invoke
        dnssec-keygen and produces no on-disk key files.  The returned
        PythonZoneKey is suitable for use with dns.dnssec.sign(),
        dns.dnssec.sign_zone(), and dns.dnssec.make_ds().

        The algorithm-to-key-type mapping is:
          ECDSAP256SHA256 -> EC P-256
          ECDSAP384SHA384 -> EC P-384
          ED25519         -> Ed25519
          ED448           -> Ed448
          RSASHA1/256/512 -> RSA (key size from alg.bits)

        Args:
            zone:  The Zone to generate the keypair for.
            flags: DNSKEY flags bitmask; defaults to ZONE|SEP (KSK).
            alg:   Algorithm to use; defaults to Algorithm.default().
            ttl:   TTL for the DNSKEY RRset (default DNSKEY_TTL).
        """
        if alg is None:
            alg = Algorithm.default()
        _generators: dict[str, Callable[[], PrivateKey]] = {
            "ECDSAP256SHA256": lambda: ec.generate_private_key(ec.SECP256R1()),
            "ECDSAP384SHA384": lambda: ec.generate_private_key(ec.SECP384R1()),
            "ED25519": ed25519.Ed25519PrivateKey.generate,
            "ED448": ed448.Ed448PrivateKey.generate,
            "RSASHA1": lambda: rsa.generate_private_key(65537, alg.bits),
            "RSASHA256": lambda: rsa.generate_private_key(65537, alg.bits),
            "RSASHA512": lambda: rsa.generate_private_key(65537, alg.bits),
        }
        gen = _generators.get(alg.name)
        if gen is None:
            raise ValueError(
                f"unsupported algorithm for Python-native key generation: {alg.name!r}"
            )
        private_key = gen()
        dnskey_rdata = dns.dnssec.make_dnskey(
            private_key.public_key(),
            dns.dnssec.Algorithm(alg.number),
            flags=flags,
        )
        return PythonZoneKey(zone, private_key, dnskey_rdata, ttl)


class Zone:
    """
    Zone providing zone file setup and signing operations.

    This is the operational counterpart to isctest.template.Zone, which is a
    plain data container for template rendering. Use isctest.template.zones()
    to convert a list of Zone instances into template data.

    The normal entrypoint is configure(), which runs the full setup once.
    The individual steps (copy_dssets, add_keys, render, sign) are public so
    that tests needing finer-grained control can drive them directly, but they
    are order-dependent and write to the same on-disk locations as configure().
    Pick one or the other for a given Zone: calling a step method and
    configure() on the same Zone leaves the zone directory in an inconsistent
    state.
    """

    def __init__(
        self,
        name: str | dns.name.Name,
        ns: Nameserver,
        signed: bool = False,
        subdir: str | None = "zones",
        filepath_unsigned: Path | str | None = None,
        filepath_signed: Path | str | None = None,
        zone_type: str = "primary",
    ) -> None:
        self.dname: dns.name.Name = (
            dns.name.from_text(name) if isinstance(name, str) else name
        )
        raw = self.dname.to_text()
        self.name: str = raw if raw == "." else raw.rstrip(".")
        self.basename: str = "root" if self.dname == dns.name.root else self.name
        self.ns = ns
        self.signed = signed
        self.subdir = subdir
        self.type = zone_type
        self._configured = False

        prefix = f"{subdir}/" if subdir else ""
        self.filepath_unsigned: Path = Path(
            filepath_unsigned or f"{prefix}{self.basename}.db"
        )
        self.filepath_signed: Path = Path(
            filepath_signed or f"{prefix}{self.basename}.db.signed"
        )

        self.delegations: list[Zone] = []
        self.keys: list[ZoneKey] = []

    @property
    def filepath(self) -> Path:
        """Actual zone file — filepath_signed if signed, filepath_unsigned otherwise."""
        return self.filepath_signed if self.signed else self.filepath_unsigned

    def add_keys(self, ksk: bool = True, zsk: bool = True) -> None:
        """Generate KSK and/or ZSK via dnssec-keygen and append to self.keys."""
        if ksk:
            self.keys.append(FileZoneKey.generate(self, "-f KSK"))
        if zsk:
            self.keys.append(FileZoneKey.generate(self))

    def copy_dssets(self) -> None:
        """Write dsset-* files for each signed delegation into self.ns dir."""
        for zone in self.delegations:
            ksks = [k for k in zone.keys if k.is_ksk()]
            has_file = any(isinstance(k, FileZoneKey) for k in ksks)
            has_python = any(isinstance(k, PythonZoneKey) for k in ksks)
            if has_file and has_python:
                raise TypeError(
                    f"{zone.name}: cannot mix FileZoneKey and PythonZoneKey KSKs; "
                    "dsset writing is order-dependent (FileZoneKey overwrites, "
                    "PythonZoneKey appends)"
                )
            if ksks:
                for key in ksks:
                    key.write_dsset(Path(self.ns.name))
            else:
                debug(f"{zone.name}: delegation is insecure (no KSK)")

    def render(self, template: str | None = None) -> None:
        """Render the unsigned zone file from a jinja2 template."""
        debug(f"{self.name}: rendering zone file")
        templates = TemplateEngine(".")
        output = Path(self.ns.name) / self.filepath_unsigned
        output.parent.mkdir(exist_ok=True)

        if template is None:
            local = f"{output}.j2.manual"
            common = "_common/zones/template.db.j2.manual"
            template = local if Path(local).is_file() else common

        data = {
            "zone": self,
            "ns": self.ns,
            "delegations": self.delegations,
        }
        templates.render(str(output), data, template=template)

    def sign(self, params: str = "") -> None:
        """
        Sign the rendered zone file via dnssec-signzone.

        Requires self.signed == True.  Raises TypeError if self.keys contains
        any PythonZoneKey — dnssec-signzone cannot use in-memory keys; use
        dns.dnssec.sign_zone() directly for Python-native signing.
        """
        assert self.signed, f"{self.name}: zone is not configured for signing"
        python_keys = [k for k in self.keys if isinstance(k, PythonZoneKey)]
        if python_keys:
            raise TypeError(
                f"{self.name}: Zone.sign() invokes dnssec-signzone which requires "
                "file-backed keys; use dns.dnssec.sign_zone() for Python-native keys"
            )
        debug(f"{self.name}: signing zone")
        signer = EnvCmd("SIGNER", f"-S -g -K {KEYDIR} {params}")
        signer(
            f"-P -x -O full -o {self.name}"
            f" -f {self.filepath_signed} {self.filepath_unsigned}",
            cwd=self.ns.name,
        )

    def configure(self, template: str | None = None, sign_params: str = "") -> None:
        """
        Perform full zone setup: copy DS sets, generate keys, render, sign.

        This is the standard single-call entrypoint and may be called only once
        per Zone. Use the individual step methods directly only when a test
        needs finer-grained control, and do not mix them with configure() on the
        same Zone (see the class docstring).
        """
        assert not self._configured, f"{self.name}: configure() already called"
        self._configured = True
        self.copy_dssets()
        if self.signed:
            self.add_keys()
        self.render(template)
        if self.signed:
            self.sign(sign_params)

    def trust_anchors(self, ta_type: str = "static-ds") -> list[TrustAnchor]:
        """Return a trust-anchor stanza for every KSK in zone.keys."""
        ksks = [k for k in self.keys if k.is_ksk()]
        assert ksks, f"{self.name}: no KSK in zone.keys"
        return [k.into_ta(ta_type) for k in ksks]


def configure_root(
    delegations: list[Zone],
    ns: Nameserver = NS1,
    signed: bool = True,
) -> Zone:
    zone = Zone(".", ns, signed=signed)
    zone.delegations = delegations
    zone.configure(template="_common/zones/root.db.j2.manual")
    return zone
