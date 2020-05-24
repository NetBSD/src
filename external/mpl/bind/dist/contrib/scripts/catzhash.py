#!/usr/bin/python
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# catzhash.py: generate the SHA-1 hash of a domain name in wire format.
#
# This can be used to determine the label to use in a catalog zone to
# represent the specified zone. For example, the zone
# "domain.example" can be represented in a catalog zone called
# "catalog.example" by adding the following record:
#
# 5960775ba382e7a4e09263fc06e7c00569b6a05c.zones.catalog.example. \
#    IN PTR domain.example.
#
# The label "5960775ba382e7a4e09263fc06e7c00569b6a05c" is the output of
# this script when run with the argument "domain.example".

import sys
import hashlib
import dns.name

if len(sys.argv) < 2:
    print("Usage: %s name" % sys.argv[0])

NAME = dns.name.from_text(sys.argv[1]).to_wire()
print(hashlib.sha1(NAME).hexdigest())
