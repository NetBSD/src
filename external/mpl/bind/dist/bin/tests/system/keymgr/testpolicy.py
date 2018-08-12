############################################################################
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.
############################################################################

import sys
sys.path.insert(0, '../../../python')
from isc import *

pp = policy.dnssec_policy()
# print the unmodified default and a generated zone policy
print(pp.named_policy['default'])
print(pp.named_policy['global'])
print(pp.policy('example.com'))

if len(sys.argv) > 0:
    for policy_file in sys.argv[1:]:
        pp.load(policy_file)

        # now print the modified default and generated zone policies
        print(pp.named_policy['default'])
        print(pp.policy('example.com'))
        print(pp.policy('example.org'))
        print(pp.policy('example.net'))

        # print algorithm policies
        print(pp.alg_policy['RSASHA1'])
        print(pp.alg_policy['DSA'])

        # print another named policy
        print(pp.named_policy['extra'])
else:
    print("ERROR: Please provide an input file")
