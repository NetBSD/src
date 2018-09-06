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
from .dnskey import *
from .keydict import *
from .keyevent import *
from .policy import *
import time


class keyseries:
    _K = defaultdict(lambda: defaultdict(list))
    _Z = defaultdict(lambda: defaultdict(list))
    _zones = set()
    _kdict = None
    _context = None

    def __init__(self, kdict, now=time.time(), context=None):
        self._kdict = kdict
        self._context = context
        self._zones = set(kdict.missing())

        for zone in kdict.zones():
            self._zones.add(zone)
            for alg, keys in kdict[zone].items():
                for k in keys.values():
                    if k.sep:
                        if not (k.delete() and k.delete() < now):
                          self._K[zone][alg].append(k)
                    else:
                        if not (k.delete() and k.delete() < now):
                          self._Z[zone][alg].append(k)

                self._K[zone][alg].sort()
                self._Z[zone][alg].sort()

    def __iter__(self):
        for zone in self._zones:
            for collection in [self._K, self._Z]:
                if zone not in collection:
                    continue
                for alg, keys in collection[zone].items():
                    for key in keys:
                        yield key

    def dump(self):
        for k in self:
            print("%s" % repr(k))

    def fixseries(self, keys, policy, now, **kwargs):
        force = kwargs.get('force', False)
        if not keys:
            return

        # handle the first key
        key = keys[0]
        if key.sep:
            rp = policy.ksk_rollperiod
            prepub = policy.ksk_prepublish or (30 * 86400)
            postpub = policy.ksk_postpublish or (30 * 86400)
        else:
            rp = policy.zsk_rollperiod
            prepub = policy.zsk_prepublish or (30 * 86400)
            postpub = policy.zsk_postpublish or (30 * 86400)

        # the first key should be published and active
        p = key.publish()
        a = key.activate()
        if not p or p > now:
            key.setpublish(now)
        if not a or a > now:
            key.setactivate(now)

        if not rp:
            key.setinactive(None, **kwargs)
            key.setdelete(None, **kwargs)
        else:
            key.setinactive(a + rp, **kwargs)
            key.setdelete(a + rp + postpub, **kwargs)

        if policy.keyttl != key.ttl:
            key.setttl(policy.keyttl)

        # handle all the subsequent keys
        prev = key
        for key in keys[1:]:
            # if no rollperiod, then all keys after the first in
            # the series kept inactive.
            # (XXX: we need to change this to allow standby keys)
            if not rp:
                key.setpublish(None, **kwargs)
                key.setactivate(None, **kwargs)
                key.setinactive(None, **kwargs)
                key.setdelete(None, **kwargs)
                if policy.keyttl != key.ttl:
                    key.setttl(policy.keyttl)
                continue

            # otherwise, ensure all dates are set correctly based on
            # the initial key
            a = prev.inactive()
            p = a - prepub
            key.setactivate(a, **kwargs)
            key.setpublish(p, **kwargs)
            key.setinactive(a + rp, **kwargs)
            key.setdelete(a + rp + postpub, **kwargs)
            prev.setdelete(a + postpub, **kwargs)
            if policy.keyttl != key.ttl:
                key.setttl(policy.keyttl)
            prev = key

        # if we haven't got sufficient coverage, create successor key(s)
        while rp and prev.inactive() and \
              prev.inactive() < now + policy.coverage:
            # commit changes to predecessor: a successor can only be
            # generated if Inactive has been set in the predecessor key
            prev.commit(self._context['settime_path'], **kwargs)
            key = prev.generate_successor(self._context['keygen_path'],
                                          self._context['randomdev'],
                                          prepub, **kwargs)

            key.setinactive(key.activate() + rp, **kwargs)
            key.setdelete(key.inactive() + postpub, **kwargs)
            keys.append(key)
            prev = key

        # last key? we already know we have sufficient coverage now, so
        # disable the inactivation of the final key (if it was set),
        # ensuring that if dnssec-keymgr isn't run again, the last key
        # in the series will at least remain usable.
        prev.setinactive(None, **kwargs)
        prev.setdelete(None, **kwargs)

        # commit changes
        for key in keys:
            key.commit(self._context['settime_path'], **kwargs)


    def enforce_policy(self, policies, now=time.time(), **kwargs):
        # If zones is provided as a parameter, use that list.
        # If not, use what we have in this object
        zones = kwargs.get('zones', self._zones)
        keys_dir = kwargs.get('dir', self._context.get('keys_path', None))
        force = kwargs.get('force', False)

        for zone in zones:
            collections = []
            policy = policies.policy(zone)
            keys_dir = keys_dir or policy.directory or '.'
            alg = policy.algorithm
            algnum = dnskey.algnum(alg)
            if 'ksk' not in kwargs or not kwargs['ksk']:
                if len(self._Z[zone][algnum]) == 0:
                    k = dnskey.generate(self._context['keygen_path'],
                                        self._context['randomdev'],
                                        keys_dir, zone, alg,
                                        policy.zsk_keysize, False,
                                        policy.keyttl or 3600,
                                        **kwargs)
                    self._Z[zone][algnum].append(k)
                collections.append(self._Z[zone])

            if 'zsk' not in kwargs or not kwargs['zsk']:
                if len(self._K[zone][algnum]) == 0:
                    k = dnskey.generate(self._context['keygen_path'],
                                        self._context['randomdev'],
                                        keys_dir, zone, alg,
                                        policy.ksk_keysize, True,
                                        policy.keyttl or 3600,
                                        **kwargs)
                    self._K[zone][algnum].append(k)
                collections.append(self._K[zone])

            for collection in collections:
                for algorithm, keys in collection.items():
                    if algorithm != algnum:
                        continue
                    try:
                        self.fixseries(keys, policy, now, **kwargs)
                    except Exception as e:
                        raise Exception('%s/%s: %s' %
                                        (zone, dnskey.algstr(algnum), str(e)))
