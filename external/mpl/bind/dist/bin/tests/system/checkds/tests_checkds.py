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


from typing import NamedTuple

import os
import sys
import time

import dns.name
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import pytest

import isctest

pytestmark = [
    pytest.mark.skipif(
        sys.version_info < (3, 7), reason="Python >= 3.7 required [GL #3001]"
    ),
    pytest.mark.extra_artifacts(
        [
            "*.out",
            "ns*/*.db",
            "ns*/*.db.infile",
            "ns*/*.db.signed",
            "ns*/*.jnl",
            "ns*/*.jbk",
            "ns*/*.keyname",
            "ns*/dsset-*",
            "ns*/K*",
            "ns*/keygen.out*",
            "ns*/settime.out*",
            "ns*/signer.out*",
            "ns*/trusted.conf",
            "ns*/zones",
        ]
    ),
]


def has_signed_apex_nsec(zone, response):
    has_nsec = False
    has_rrsig = False

    ttl = 300
    nextname = "a."
    labelcount = zone.count(".")  # zone is specified as FQDN
    types = "NS SOA RRSIG NSEC DNSKEY"
    match = f"{zone} {ttl} IN NSEC {nextname}{zone} {types}"
    sig = f"{zone} {ttl} IN RRSIG NSEC 13 {labelcount} 300"

    for rr in response.answer:
        if match in rr.to_text():
            has_nsec = True
        if sig in rr.to_text():
            has_rrsig = True

    if not has_nsec:
        isctest.log.error("missing apex NSEC record in response")
    if not has_rrsig:
        isctest.log.error("missing NSEC signature in response")

    return has_nsec and has_rrsig


def do_query(server, qname, qtype, tcp=False):
    msg = isctest.query.create(qname, qtype)
    query_func = isctest.query.tcp if tcp else isctest.query.udp
    response = query_func(msg, server.ip, expected_rcode=dns.rcode.NOERROR)
    return response


def verify_zone(zone, transfer):
    verify = os.getenv("VERIFY")
    assert verify is not None

    filename = f"{zone}out"
    with open(filename, "w", encoding="utf-8") as file:
        for rr in transfer.answer:
            file.write(rr.to_text())
            file.write("\n")

    # dnssec-verify command with default arguments.
    verify_cmd = [verify, "-z", "-o", zone, filename]

    verifier = isctest.run.cmd(verify_cmd)

    if verifier.rc != 0:
        isctest.log.error(f"dnssec-verify {zone} failed")

    return verifier.rc == 0


def read_statefile(server, zone):
    count = 0
    keyid = 0
    state = {}

    response = do_query(server, zone, "DS", tcp=True)
    # fetch key id from response.
    for rr in response.answer:
        if rr.match(
            dns.name.from_text(zone),
            dns.rdataclass.IN,
            dns.rdatatype.DS,
            dns.rdatatype.NONE,
        ):
            if count == 0:
                keyid = list(dict(rr.items).items())[0][0].key_tag
            count += 1

    assert (
        count == 1
    ), f"expected a single DS in response for {zone} from {server.ip}, got {count}"

    filename = f"ns9/K{zone}+013+{keyid:05d}.state"
    isctest.log.debug(f"read state file {filename}")

    try:
        with open(filename, "r", encoding="utf-8") as file:
            for line in file:
                if line.startswith(";"):
                    continue
                key, val = line.strip().split(":", 1)
                state[key.strip()] = val.strip()
    except FileNotFoundError:
        # file may not be written just yet.
        return {}

    return state


def zone_check(server, zone):
    fqdn = f"{zone}."

    # check zone is fully signed.
    response = do_query(server, fqdn, "NSEC")
    assert has_signed_apex_nsec(fqdn, response)

    # check if zone if DNSSEC valid.
    transfer = do_query(server, fqdn, "AXFR", tcp=True)
    assert verify_zone(fqdn, transfer)


def keystate_check(server, zone, key):
    fqdn = f"{zone}."
    val = 0
    deny = False

    search = key
    if key.startswith("!"):
        deny = True
        search = key[1:]

    for _ in range(10):
        state = read_statefile(server, fqdn)
        try:
            val = state[search]
        except KeyError:
            pass

        if not deny and val != 0:
            break
        if deny and val == 0:
            break

        time.sleep(1)

    if deny:
        assert val == 0
    else:
        assert val != 0


class CheckDSTest(NamedTuple):
    zone: str
    logs_to_wait_for: tuple[str]
    expected_parent_state: str


parental_agents_tests = [
    # Using a reference to parental-agents.
    CheckDSTest(
        zone="reference.explicit.dspublish.ns2",
        logs_to_wait_for=("DS response from 10.53.0.8",),
        expected_parent_state="DSPublish",
    ),
    # Using a resolver as parental-agent (ns3).
    CheckDSTest(
        zone="resolver.explicit.dspublish.ns2",
        logs_to_wait_for=("DS response from 10.53.0.3",),
        expected_parent_state="DSPublish",
    ),
    # Using a resolver as parental-agent (ns3).
    CheckDSTest(
        zone="resolver.explicit.dsremoved.ns5",
        logs_to_wait_for=("empty DS response from 10.53.0.3",),
        expected_parent_state="DSRemoved",
    ),
]

no_ent_tests = [
    CheckDSTest(
        zone="no-ent.ns2",
        logs_to_wait_for=("DS response from 10.53.0.2",),
        expected_parent_state="DSPublish",
    ),
    CheckDSTest(
        zone="no-ent.ns5",
        logs_to_wait_for=("DS response from 10.53.0.5",),
        expected_parent_state="DSRemoved",
    ),
]


def dspublished_tests(checkds, addr):
    return [
        #
        # 1.1.1: DS is correctly published in parent.
        # parental-agents: ns2
        #
        # The simple case.
        CheckDSTest(
            zone=f"good.{checkds}.dspublish.ns2",
            logs_to_wait_for=(f"DS response from {addr}",),
            expected_parent_state="DSPublish",
        ),
        #
        # 1.1.2: DS is not published in parent.
        # parental-agents: ns5
        #
        CheckDSTest(
            zone=f"not-yet.{checkds}.dspublish.ns5",
            logs_to_wait_for=("empty DS response from 10.53.0.5",),
            expected_parent_state="!DSPublish",
        ),
        #
        # 1.1.3: The parental agent is badly configured.
        # parental-agents: ns6
        #
        CheckDSTest(
            zone=f"bad.{checkds}.dspublish.ns6",
            logs_to_wait_for=(
                (
                    "bad DS response from 10.53.0.6"
                    if checkds == "explicit"
                    else "error during parental-agents processing"
                ),
            ),
            expected_parent_state="!DSPublish",
        ),
        #
        # 1.1.4: DS is published, but has bogus signature.
        #
        # TBD
        #
        # 1.2.1: DS is correctly published in all parents.
        # parental-agents: ns2, ns4
        #
        CheckDSTest(
            zone=f"good.{checkds}.dspublish.ns2-4",
            logs_to_wait_for=(f"DS response from {addr}", "DS response from 10.53.0.4"),
            expected_parent_state="DSPublish",
        ),
        #
        # 1.2.2: DS is not published in some parents.
        # parental-agents: ns2, ns4, ns5
        #
        CheckDSTest(
            zone=f"incomplete.{checkds}.dspublish.ns2-4-5",
            logs_to_wait_for=(
                f"DS response from {addr}",
                "DS response from 10.53.0.4",
                "empty DS response from 10.53.0.5",
            ),
            expected_parent_state="!DSPublish",
        ),
        #
        # 1.2.3: One parental agent is badly configured.
        # parental-agents: ns2, ns4, ns6
        #
        CheckDSTest(
            zone=f"bad.{checkds}.dspublish.ns2-4-6",
            logs_to_wait_for=(
                f"DS response from {addr}",
                "DS response from 10.53.0.4",
                "bad DS response from 10.53.0.6",
            ),
            expected_parent_state="!DSPublish",
        ),
        #
        # 1.2.4: DS is completely published, bogus signature.
        #
        # TBD
        # TBD: Check with TSIG
        # TBD: Check with TLS
    ]


def dswithdrawn_tests(checkds, addr):
    return [
        #
        # 2.1.1: DS correctly withdrawn from the parent.
        # parental-agents: ns5
        #
        # The simple case.
        CheckDSTest(
            zone=f"good.{checkds}.dsremoved.ns5",
            logs_to_wait_for=(f"empty DS response from {addr}",),
            expected_parent_state="DSRemoved",
        ),
        #
        # 2.1.2: DS is published in the parent.
        # parental-agents: ns2
        #
        CheckDSTest(
            zone=f"still-there.{checkds}.dsremoved.ns2",
            logs_to_wait_for=("DS response from 10.53.0.2",),
            expected_parent_state="!DSRemoved",
        ),
        #
        # 2.1.3: The parental agent is badly configured.
        # parental-agents: ns6
        #
        CheckDSTest(
            zone=f"bad.{checkds}.dsremoved.ns6",
            logs_to_wait_for=(
                (
                    "bad DS response from 10.53.0.6"
                    if checkds == "explicit"
                    else "error during parental-agents processing"
                ),
            ),
            expected_parent_state="!DSRemoved",
        ),
        #
        # 2.1.4: DS is withdrawn, but has bogus signature.
        #
        # TBD
        #
        # 2.2.1: DS is correctly withdrawn from all parents.
        # parental-agents: ns5, ns7
        #
        CheckDSTest(
            zone=f"good.{checkds}.dsremoved.ns5-7",
            logs_to_wait_for=(
                f"empty DS response from {addr}",
                "empty DS response from 10.53.0.7",
            ),
            expected_parent_state="DSRemoved",
        ),
        #
        # 2.2.2: DS is not withdrawn from some parents.
        # parental-agents: ns2, ns5, ns7
        #
        CheckDSTest(
            zone=f"incomplete.{checkds}.dsremoved.ns2-5-7",
            logs_to_wait_for=(
                "DS response from 10.53.0.2",
                f"empty DS response from {addr}",
                "empty DS response from 10.53.0.7",
            ),
            expected_parent_state="!DSRemoved",
        ),
        #
        # 2.2.3: One parental agent is badly configured.
        # parental-agents: ns5, ns6, ns7
        #
        CheckDSTest(
            zone=f"bad.{checkds}.dsremoved.ns5-6-7",
            logs_to_wait_for=(
                f"empty DS response from {addr}",
                "empty DS response from 10.53.0.7",
                "bad DS response from 10.53.0.6",
            ),
            expected_parent_state="!DSRemoved",
        ),
        #
        # 2.2.4:: DS is removed completely, bogus signature.
        #
        # TBD
    ]


checkds_no_tests = [
    CheckDSTest(
        zone="good.no.dspublish.ns2",
        logs_to_wait_for=(),
        expected_parent_state="!DSPublish",
    ),
    CheckDSTest(
        zone="good.no.dspublish.ns2-4",
        logs_to_wait_for=(),
        expected_parent_state="!DSPublish",
    ),
    CheckDSTest(
        zone="good.no.dsremoved.ns5",
        logs_to_wait_for=(),
        expected_parent_state="!DSRemoved",
    ),
    CheckDSTest(
        zone="good.no.dsremoved.ns5-7",
        logs_to_wait_for=(),
        expected_parent_state="!DSRemoved",
    ),
]


checkds_tests = (
    parental_agents_tests
    + no_ent_tests
    + dspublished_tests("explicit", "10.53.0.8")
    + dspublished_tests("yes", "10.53.0.2")
    + dswithdrawn_tests("explicit", "10.53.0.10")
    + dswithdrawn_tests("yes", "10.53.0.5")
    + checkds_no_tests
)


@pytest.mark.parametrize("params", checkds_tests, ids=lambda t: t.zone)
def test_checkds(ns2, ns9, params):
    # Wait until the provided zone is signed and then verify its DNSSEC data.
    zone_check(ns9, params.zone)

    # Wait up to 10 seconds until all the expected log lines are found in the
    # log file for the provided server.  Rekey every second if necessary.
    time_remaining = 10
    for log_string in params.logs_to_wait_for:
        line = f"zone {params.zone}/IN (signed): checkds: {log_string}"
        while line not in ns9.log:
            ns9.rndc(f"loadkeys {params.zone}")
            time_remaining -= 1
            assert time_remaining, f'Timed out waiting for "{log_string}" to be logged'
            time.sleep(1)

    # Check whether key states on the parent server provided match
    # expectations.
    keystate_check(ns2, params.zone, params.expected_parent_state)
