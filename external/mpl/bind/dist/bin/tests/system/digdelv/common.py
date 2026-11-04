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
Helpers shared by the digdelv test modules.
"""

import yaml

ARTIFACTS = [
    "ans*/ans.run",
    "ns*/anchor.*",
    "ns*/dsset-*",
    "ns*/keydata",
    "ns*/keyid",
    "ns*/K*.key",
    "ns*/K*.private",
    "ns1/root.db",
    "ns2/example.db",
    "ns2/example.tld.db",
]


def parse_yaml(text):
    """Parse the tools' +yaml output."""
    return yaml.safe_load(text)


def check_ttl_range(text, rrtype, max_ttl):
    """Check that a record of the given RR type and class IN (or its
    unknown-format spelling CLASS1) is present with a TTL not exceeding
    max_ttl.  A leading ";" token is ignored so that delv's commented
    records ("; name ttl class type ...") are checked too."""
    for line in text.splitlines():
        fields = line.split()
        if fields and fields[0] == ";":
            fields = fields[1:]
        if len(fields) < 4:
            continue
        if fields[2] not in ("IN", "CLASS1") or fields[3] != rrtype:
            continue
        try:
            ttl = int(fields[1])
        except ValueError:
            continue
        if ttl <= max_ttl:
            return True
    return False
