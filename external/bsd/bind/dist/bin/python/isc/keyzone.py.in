############################################################################
# Copyright (C) 2013-2016  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
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
