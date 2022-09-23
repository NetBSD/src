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

import sys
from isc import policy

PP = policy.dnssec_policy()
# print the unmodified default and a generated zone policy
print(PP.named_policy["default"])
print(PP.named_policy["global"])
print(PP.policy("example.com"))

if len(sys.argv) > 0:
    for policy_file in sys.argv[1:]:
        PP.load(policy_file)

        # now print the modified default and generated zone policies
        print(PP.named_policy["default"])
        print(PP.policy("example.com"))
        print(PP.policy("example.org"))
        print(PP.policy("example.net"))

        # print algorithm policies
        print(PP.alg_policy["RSASHA1"])
        print(PP.alg_policy["RSASHA256"])
        print(PP.alg_policy["ECDSAP256SHA256"])

        # print another named policy
        print(PP.named_policy["extra"])
else:
    print("ERROR: Please provide an input file")
