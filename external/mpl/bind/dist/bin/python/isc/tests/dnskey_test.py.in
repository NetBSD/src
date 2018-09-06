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

kdict = None


def getkey():
    global kdict
    if not kdict:
        kd = keydict(path='testdata')
    for key in kd:
        return key


class DnskeyTest(unittest.TestCase):
    def test_metdata(self):
        key = getkey()
        self.assertEqual(key.created(), 1448055647)
        self.assertEqual(key.publish(), 1445463714)
        self.assertEqual(key.activate(), 1448055714)
        self.assertEqual(key.revoke(), 1479591714)
        self.assertEqual(key.inactive(), 1511127714)
        self.assertEqual(key.delete(), 1542663714)
        self.assertEqual(key.syncpublish(), 1442871714)
        self.assertEqual(key.syncdelete(), 1448919714)

    def test_fmttime(self):
        key = getkey()
        self.assertEqual(key.getfmttime('Created'), '20151120214047')
        self.assertEqual(key.getfmttime('Publish'), '20151021214154')
        self.assertEqual(key.getfmttime('Activate'), '20151120214154')
        self.assertEqual(key.getfmttime('Revoke'), '20161119214154')
        self.assertEqual(key.getfmttime('Inactive'), '20171119214154')
        self.assertEqual(key.getfmttime('Delete'), '20181119214154')
        self.assertEqual(key.getfmttime('SyncPublish'), '20150921214154')
        self.assertEqual(key.getfmttime('SyncDelete'), '20151130214154')

if __name__ == "__main__":
    unittest.main()
