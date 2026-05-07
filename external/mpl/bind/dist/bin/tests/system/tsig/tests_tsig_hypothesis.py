#!/usr/bin/python3

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

# Silence incorrect warnings cause by hypothesis.assume()
# https://github.com/pylint-dev/pylint/issues/10785#issuecomment-3677224217
# pylint: disable=unreachable

import time

from hypothesis import assume, example, given
from hypothesis.strategies import binary, booleans, composite, just, sampled_from

import dns.exception
import dns.message
import dns.name
import dns.rdataclass
import dns.rdatatype
import dns.rdtypes.ANY.TSIG
import dns.rrset
import dns.tsig
import pytest

from isctest.hypothesis.strategies import dns_names, uint

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ans*/ans.run",
        "ns1/named-fips.conf",
    ]
)


@composite
def generate_known_algoritm_and_matching_len_mac(draw):
    candidates = tuple(dns.tsig.mac_sizes.items())
    alg, mac_size = draw(sampled_from(candidates))
    mac = draw(binary(min_size=mac_size, max_size=mac_size))
    return alg, mac


@composite
def generate_known_algoritm_and_wrong_len_mac(draw):
    candidates = tuple(dns.tsig.mac_sizes.items())
    alg, correct_mac_len = draw(sampled_from(candidates))
    mac = draw(binary())
    assume(len(mac) != correct_mac_len)
    return alg, mac


@composite
def generate_unknown_but_likely_algoritm_and_mac(draw):
    alg = draw(dns_names(min_labels=2, max_labels=2))
    mac = draw(binary())
    return alg, mac


@composite
def generate_random_alg_and_mac(draw):
    alg = draw(dns_names())
    mac = draw(binary())
    return alg, mac


@given(
    keyname=dns_names(max_labels=3) | dns_names(),
    alg_and_mac=generate_known_algoritm_and_matching_len_mac()
    | generate_known_algoritm_and_wrong_len_mac()
    | generate_unknown_but_likely_algoritm_and_mac()
    | generate_random_alg_and_mac(),
    time_signed=just(int(time.time())) | uint(48),
    fudge=just(300) | uint(16),
    mangle_orig_id=booleans(),
    error=just(0) | uint(12),
    other=just(b"") | binary(),
)
@example(
    keyname=dns.name.from_text("."),
    alg_and_mac=(dns.name.from_text("."), b""),
    time_signed=0,
    fudge=300,
    mangle_orig_id=False,
    error=0,
    other=b"",
)
def test_tsig_fuzz_rdata(
    keyname,
    alg_and_mac,
    time_signed,
    fudge,
    error,
    mangle_orig_id,
    other,
    ns1,
    named_port,
):
    alg, mac = alg_and_mac
    msg = dns.message.make_query("example.com.", "AXFR")
    msg.keyring = False  # don't validate received TSIG

    tsig_orig_id = msg.id
    if mangle_orig_id:
        tsig_orig_id = (msg.id - 0xABCD) % 0x10000

    tsig = dns.rdtypes.ANY.TSIG.TSIG(
        dns.rdataclass.ANY,
        dns.rdatatype.TSIG,
        alg,
        time_signed,
        fudge,
        mac,
        tsig_orig_id,
        error,
        other,
    )
    rrs = dns.rrset.from_rdata(keyname, 0, tsig)
    msg.additional.append(rrs)

    try:
        isctest.query.tcp(msg, ns1.ip, named_port)
    except dns.tsig.PeerError:
        pass  # any error from named is fine
    except dns.exception.TooBig:
        assume(False)  # some randomly generated value did not fit into message
