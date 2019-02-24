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

from collections import defaultdict
from . import dnskey
import os
import glob


########################################################################
# Class keydict
########################################################################
class keydict:
    """ A dictionary of keys, indexed by name, algorithm, and key id """

    _keydict = defaultdict(lambda: defaultdict(dict))
    _defttl = None
    _missing = []

    def __init__(self, dp=None, **kwargs):
        self._defttl = kwargs.get('keyttl', None)
        zones = kwargs.get('zones', None)

        if not zones:
            path = kwargs.get('path',None) or '.'
            self.readall(path)
        else:
            for zone in zones:
                if 'path' in kwargs and kwargs['path'] is not None:
                    path = kwargs['path']
                else:
                    path = dp and dp.policy(zone).directory or '.'
                if not self.readone(path, zone):
                    self._missing.append(zone)

    def readall(self, path):
        files = glob.glob(os.path.join(path, '*.private'))

        for infile in files:
            key = dnskey(infile, path, self._defttl)
            self._keydict[key.name][key.alg][key.keyid] = key

    def readone(self, path, zone):
        if not zone.endswith('.'):
            zone += '.'
        match='K' + zone + '+*.private'
        files = glob.glob(os.path.join(path, match))

        found = False
        for infile in files:
            key = dnskey(infile, path, self._defttl)
            if key.fullname != zone: # shouldn't ever happen
                continue
            keyname=key.name if zone != '.' else '.'
            self._keydict[keyname][key.alg][key.keyid] = key
            found = True

        return found

    def __iter__(self):
        for zone, algorithms in self._keydict.items():
            for alg, keys in algorithms.items():
                for key in keys.values():
                    yield key

    def __getitem__(self, name):
        return self._keydict[name]

    def zones(self):
        return (self._keydict.keys())

    def algorithms(self, zone):
        return (self._keydict[zone].keys())

    def keys(self, zone, alg):
        return (self._keydict[zone][alg].keys())

    def missing(self):
        return (self._missing)
