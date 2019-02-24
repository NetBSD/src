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


class eventlist:
    _K = defaultdict(lambda: defaultdict(list))
    _Z = defaultdict(lambda: defaultdict(list))
    _zones = set()
    _kdict = None

    def __init__(self, kdict):
        properties = ["SyncPublish", "Publish", "SyncDelete",
                      "Activate", "Inactive", "Delete"]
        self._kdict = kdict
        for zone in kdict.zones():
            self._zones.add(zone)
            for alg, keys in kdict[zone].items():
                for k in keys.values():
                    for prop in properties:
                        t = k.gettime(prop)
                        if not t:
                            continue
                        e = keyevent(prop, k, t)
                        if k.sep:
                            self._K[zone][alg].append(e)
                        else:
                            self._Z[zone][alg].append(e)

                self._K[zone][alg] = sorted(self._K[zone][alg],
                                            key=lambda event: event.when)
                self._Z[zone][alg] = sorted(self._Z[zone][alg],
                                            key=lambda event: event.when)

    # scan events per zone, algorithm, and key type, in order of
    # occurrance, noting inconsistent states when found
    def coverage(self, zone, keytype, until, output = None):
        def noop(*args, **kwargs): pass
        if not output:
            output = noop

        no_zsk = True if (keytype and keytype == "KSK") else False
        no_ksk = True if (keytype and keytype == "ZSK") else False
        kok = zok = True
        found = False

        if zone and not zone in self._zones:
            output("ERROR: No key events found for %s" % zone)
            return False

        if zone:
            found = True
            if not no_ksk:
                kok = self.checkzone(zone, "KSK", until, output)
            if not no_zsk:
                zok = self.checkzone(zone, "ZSK", until, output)
        else:
            for z in self._zones:
                if not no_ksk and z in self._K.keys():
                    found = True
                    kok = self.checkzone(z, "KSK", until, output)
                if not no_zsk and z in self._Z.keys():
                    found = True
                    zok = self.checkzone(z, "ZSK", until, output)

        if not found:
            output("ERROR: No key events found")
            return False

        return (kok and zok)

    def checkzone(self, zone, keytype, until, output):
        allok = True
        if keytype == "KSK":
            kz = self._K[zone]
        else:
            kz = self._Z[zone]

        for alg in kz.keys():
            output("Checking scheduled %s events for zone %s, "
                   "algorithm %s..." %
                   (keytype, zone, dnskey.algstr(alg)))
            ok = eventlist.checkset(kz[alg], keytype, until, output)
            if ok:
                output("No errors found")
            allok = allok and ok

        return allok

    @staticmethod
    def showset(eventset, output):
        if not eventset:
            return
        output("  " + eventset[0].showtime() + ":", skip=False)
        for event in eventset:
            output("    %s: %s" % (event.what, repr(event.key)), skip=False)

    @staticmethod
    def checkset(eventset, keytype, until, output):
        groups = list()
        group = list()

        # collect up all events that have the same time
        eventsfound = False
        for event in eventset:
            # we found an event
            eventsfound = True

            # add event to current group
            if (not group or group[0].when == event.when):
                group.append(event)

            # if we're at the end of the list, we're done.  if
            # we've found an event with a later time, start a new group
            if (group[0].when != event.when):
                groups.append(group)
                group = list()
                group.append(event)

        if group:
            groups.append(group)

        if not eventsfound:
            output("ERROR: No %s events found" % keytype)
            return False

        active = published = None
        for group in groups:
            if (until and calendar.timegm(group[0].when) > until):
                output("Ignoring events after %s" %
                      time.strftime("%a %b %d %H:%M:%S UTC %Y", 
                          time.gmtime(until)))
                return True

            for event in group:
                (active, published) = event.status(active, published)

            eventlist.showset(group, output)

            # and then check for inconsistencies:
            if not active:
                output("ERROR: No %s's are active after this event" % keytype)
                return False
            elif not published:
                output("ERROR: No %s's are published after this event"
                        % keytype)
                return False
            elif not published.intersection(active):
                output("ERROR: No %s's are both active and published "
                       "after this event" % keytype)
                return False

        return True

