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

import time
import os

import pytest

pytest.importorskip("dns", minversion="2.0.0")
import dns.resolver


def wait_for_transfer(ip, port, client_ip, name, rrtype):
    resolver = dns.resolver.Resolver()
    resolver.nameservers = [ip]
    resolver.port = port

    for _ in range(10):
        try:
            resolver.resolve(name, rrtype, source=client_ip)
        except dns.resolver.NoNameservers:
            time.sleep(1)
        else:
            break
    else:
        raise RuntimeError(
            "zone transfer failed: "
            f"client {client_ip} got NXDOMAIN for {name} {rrtype} from @{ip}:{port}"
        )


def test_rpz_multiple_views(named_port):
    resolver = dns.resolver.Resolver()
    resolver.nameservers = ["10.53.0.3"]
    resolver.port = named_port

    wait_for_transfer("10.53.0.3", named_port, "10.53.0.2", "rpz-external.local", "SOA")
    wait_for_transfer("10.53.0.3", named_port, "10.53.0.5", "rpz-external.local", "SOA")

    # For 10.53.0.1 source IP:
    # - baddomain.com isn't allowed (CNAME .), should return NXDOMAIN
    # - gooddomain.com is allowed
    # - allowed. is allowed
    with pytest.raises(dns.resolver.NXDOMAIN):
        resolver.resolve("baddomain.", "A", source="10.53.0.1")

    ans = resolver.resolve("gooddomain.", "A", source="10.53.0.1")
    assert ans[0].address == "10.53.0.2"

    ans = resolver.resolve("allowed.", "A", source="10.53.0.1")
    assert ans[0].address == "10.53.0.2"

    # For 10.53.0.2 source IP:
    # - allowed.com isn't allowed (CNAME .), should return NXDOMAIN
    # - baddomain.com is allowed
    # - gooddomain.com is allowed
    ans = resolver.resolve("baddomain.", "A", source="10.53.0.2")
    assert ans[0].address == "10.53.0.2"

    ans = resolver.resolve("gooddomain.", "A", source="10.53.0.2")
    assert ans[0].address == "10.53.0.2"

    with pytest.raises(dns.resolver.NXDOMAIN):
        resolver.resolve("allowed.", "A", source="10.53.0.2")

    # For 10.53.0.3 source IP:
    # - gooddomain.com is allowed
    # - baddomain.com is allowed
    # - allowed. is allowed
    ans = resolver.resolve("baddomain.", "A", source="10.53.0.3")
    assert ans[0].address == "10.53.0.2"

    ans = resolver.resolve("gooddomain.", "A", source="10.53.0.3")
    assert ans[0].address == "10.53.0.2"

    ans = resolver.resolve("allowed.", "A", source="10.53.0.3")
    assert ans[0].address == "10.53.0.2"

    # For 10.53.0.4 source IP:
    # - gooddomain.com isn't allowed (CNAME .), should return NXDOMAIN
    # - baddomain.com isn't allowed (CNAME .), should return NXDOMAIN
    # - allowed. is allowed
    with pytest.raises(dns.resolver.NXDOMAIN):
        resolver.resolve("baddomain.", "A", source="10.53.0.4")

    with pytest.raises(dns.resolver.NXDOMAIN):
        resolver.resolve("gooddomain.", "A", source="10.53.0.4")

    ans = resolver.resolve("allowed.", "A", source="10.53.0.4")
    assert ans[0].address == "10.53.0.2"

    # For 10.53.0.5 (any) source IP:
    # - baddomain.com is allowed
    # - gooddomain.com isn't allowed (CNAME .), should return NXDOMAIN
    # - allowed.com isn't allowed (CNAME .), should return NXDOMAIN
    ans = resolver.resolve("baddomain.", "A", source="10.53.0.5")
    assert ans[0].address == "10.53.0.2"

    with pytest.raises(dns.resolver.NXDOMAIN):
        resolver.resolve("gooddomain.", "A", source="10.53.0.5")

    with pytest.raises(dns.resolver.NXDOMAIN):
        resolver.resolve("allowed.", "A", source="10.53.0.5")


def test_rpz_passthru_logging(named_port):
    resolver = dns.resolver.Resolver()
    resolver.nameservers = ["10.53.0.3"]
    resolver.port = named_port

    # Should generate a log entry into rpz_passthru.txt
    ans = resolver.resolve("allowed.", "A", source="10.53.0.1")
    assert ans[0].address == "10.53.0.2"

    # baddomain.com isn't allowed (CNAME .), should return NXDOMAIN
    # Should generate a log entry into rpz.txt
    with pytest.raises(dns.resolver.NXDOMAIN):
        resolver.resolve("baddomain.", "A", source="10.53.0.1")

    rpz_passthru_logfile = os.path.join("ns3", "rpz_passthru.txt")
    rpz_logfile = os.path.join("ns3", "rpz.txt")

    assert os.path.isfile(rpz_passthru_logfile)
    assert os.path.isfile(rpz_logfile)

    with open(rpz_passthru_logfile, encoding="utf-8") as log_file:
        line = log_file.read()
        assert "rpz QNAME PASSTHRU rewrite allowed/A/IN" in line

    with open(rpz_logfile, encoding="utf-8") as log_file:
        line = log_file.read()
        assert "rpz QNAME PASSTHRU rewrite allowed/A/IN" not in line
        assert "rpz QNAME NXDOMAIN rewrite baddomain/A/IN" in line
