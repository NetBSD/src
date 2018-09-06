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

import os
import sys
import re
from subprocess import Popen, PIPE

########################################################################
# Exceptions
########################################################################
class KeyZoneException(Exception):
    pass

########################################################################
# class keyzone
########################################################################
class keyzone:
    """reads a zone file to find data relevant to keys"""

    def __init__(self, name, filename, czpath):
        self.maxttl = None
        self.keyttl = None

        if not name:
            return

        if not czpath or not os.path.isfile(czpath) \
                or not os.access(czpath, os.X_OK):
            raise KeyZoneException('"named-compilezone" not found')
            return

        maxttl = keyttl = None

        fp, _ = Popen([czpath, "-o", "-", name, filename],
                      stdout=PIPE, stderr=PIPE).communicate()
        for line in fp.splitlines():
            if re.search('^[:space:]*;', line):
                continue
            fields = line.split()
            if not maxttl or int(fields[1]) > maxttl:
                maxttl = int(fields[1])
            if fields[3] == "DNSKEY":
                keyttl = int(fields[1])

        self.keyttl = keyttl
        self.maxttl = maxttl
