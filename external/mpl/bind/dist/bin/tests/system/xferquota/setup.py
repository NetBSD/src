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

#
# Set up test data for zone transfer quota tests.
#

ZONES = 300

for z in range(ZONES):
    zn = f"zone{z:06d}.example"
    with open(f"ns1/{zn}.db", "w", encoding="utf-8") as f:
        f.write("""$TTL 300
@    IN SOA    ns1 . 1 300 120 3600 86400
        NS      ns1
        NS      ns2
ns1     A       10.53.0.1
ns2     A       10.53.0.2
        MX      10 mail1.isp.example.
        MX      20 mail2.isp.example.
www     A       10.0.0.1
xyzzy   A       10.0.0.2
""")

with open("ns1/zones.conf", "w", encoding="utf-8") as priconf, open(
    "ns2/zones.conf", "w", encoding="utf-8"
) as secconf:
    for z in range(ZONES):
        zn = f"zone{z:06d}.example"
        priconf.write(f'zone "{zn}" {{ type primary; file "{zn}.db"; }};\n')
        secconf.write(
            f'zone "{zn}" {{ type secondary; file "{zn}.bk"; '
            f"masterfile-format text; primaries {{ 10.53.0.1; }}; }};\n"
        )
