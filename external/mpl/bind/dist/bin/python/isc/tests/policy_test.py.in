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
import unittest
sys.path.append('../..')
from isc import *


class PolicyTest(unittest.TestCase):
    def test_keysize(self):
        pol = policy.dnssec_policy()
        pol.load('test-policies/01-keysize.pol')

        p = pol.policy('good_rsa.test', novalidate=True)
        self.assertEqual(p.get_name(), "good_rsa.test")
        self.assertEqual(p.constructed(), False)
        self.assertEqual(p.validate(), (True, ""))

        p = pol.policy('good_dsa.test', novalidate=True)
        self.assertEqual(p.get_name(), "good_dsa.test")
        self.assertEqual(p.constructed(), False)
        self.assertEqual(p.validate(), (True, ""))

        p = pol.policy('bad_dsa.test', novalidate=True)
        self.assertEqual(p.validate(),
                         (False, 'ZSK key size 769 not divisible by 64 as required for DSA'))

    def test_prepublish(self):
        pol = policy.dnssec_policy()
        pol.load('test-policies/02-prepublish.pol')
        p = pol.policy('good_prepublish.test', novalidate=True)
        self.assertEqual(p.validate(), (True, ""))

        p = pol.policy('bad_prepublish.test', novalidate=True)
        self.assertEqual(p.validate(),
                         (False, 'KSK pre/post-publish periods '
                                 '(10368000/5184000) combined exceed '
                                 'rollover period 10368000'))

    def test_postpublish(self):
        pol = policy.dnssec_policy()
        pol.load('test-policies/03-postpublish.pol')

        p = pol.policy('good_postpublish.test', novalidate=True)
        self.assertEqual(p.validate(), (True, ""))

        p = pol.policy('bad_postpublish.test', novalidate=True)
        self.assertEqual(p.validate(),
                         (False, 'KSK pre/post-publish periods '
                                 '(10368000/5184000) combined exceed '
                                 'rollover period 10368000'))

    def test_combined_pre_post(self):
        pol = policy.dnssec_policy()
        pol.load('test-policies/04-combined-pre-post.pol')

        p = pol.policy('good_combined_pre_post_ksk.test', novalidate=True)
        self.assertEqual(p.validate(), (True, ""))

        p = pol.policy('bad_combined_pre_post_ksk.test', novalidate=True)
        self.assertEqual(p.validate(),
                         (False, 'KSK pre/post-publish periods '
                                 '(5184000/5184000) combined exceed '
                                 'rollover period 10368000'))

        p = pol.policy('good_combined_pre_post_zsk.test', novalidate=True)
        self.assertEqual(p.validate(),
                         (True, ""))
        p = pol.policy('bad_combined_pre_post_zsk.test', novalidate=True)
        self.assertEqual(p.validate(),
                         (False, 'ZSK pre/post-publish periods '
                                 '(5184000/5184000) combined exceed '
                                 'rollover period 7776000'))

    def test_numeric_zone(self):
        pol = policy.dnssec_policy()
        pol.load('test-policies/05-numeric-zone.pol')

        p = pol.policy('99example.test', novalidate=True)
        self.assertEqual(p.validate(), (True, ""))

if __name__ == "__main__":
    unittest.main()
