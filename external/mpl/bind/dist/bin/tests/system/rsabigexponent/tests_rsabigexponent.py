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

import os
import subprocess

import dns.message
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "dig.out.*",
        "options.conf",
        "ns*/dsset-*",
        "ns*/K*",
        "ns*/trusted.conf",
        "ns*/*.signed",
        "ns1/root.db",
        "ns2/signer.err",
    ]
)

CHECKCONF = os.environ["CHECKCONF"]


@pytest.mark.parametrize("exponent_size", [0, 35, 666, 1024, 2048, 3072, 4096])
def test_max_rsa_exponent_size_good(exponent_size, templates):
    templates.render("options.conf", {"max_rsa_exponent_size": exponent_size})
    isctest.run.cmd([CHECKCONF, "options.conf"])


@pytest.mark.parametrize("exponent_size", [1, 34, 4097])
def test_max_rsa_exponent_size_bad(exponent_size, templates):
    templates.render("options.conf", {"max_rsa_exponent_size": exponent_size})
    with pytest.raises(subprocess.CalledProcessError):
        isctest.run.cmd([CHECKCONF, "options.conf"])


def test_rsa_big_exponent_keys_cant_load():
    with open("ns2/signer.err", encoding="utf-8") as file:
        assert (
            "dnssec-signzone: fatal: cannot load dnskey Kexample.+008+52810.key: out of range"
            in file.read()
        )


def test_rsa_big_exponent_keys_cant_validate():
    msg = dns.message.make_query("a.example.", "A")
    res2 = isctest.query.tcp(msg, "10.53.0.2")
    isctest.check.noerror(res2)
    res3 = isctest.query.tcp(msg, "10.53.0.3")
    isctest.check.servfail(res3)
