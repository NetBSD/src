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

import time


########################################################################
# Class keyevent
########################################################################
class keyevent:
    """ A discrete key event, e.g., Publish, Activate, Inactive, Delete,
    etc. Stores the date of the event, and identifying information
    about the key to which the event will occur."""

    def __init__(self, what, key, when=None):
        self.what = what
        self.when = when or key.gettime(what)
        self.key = key
        self.sep = key.sep
        self.zone = key.name
        self.alg = key.alg
        self.keyid = key.keyid

    def __repr__(self):
        return repr((self.when, self.what, self.keyid, self.sep,
                     self.zone, self.alg))

    def showtime(self):
        return time.strftime("%a %b %d %H:%M:%S UTC %Y", self.when)

    # update sets of active and published keys, based on
    # the contents of this keyevent
    def status(self, active, published, output = None):
        def noop(*args, **kwargs): pass
        if not output:
            output = noop

        if not active:
            active = set()
        if not published:
            published = set()

        if self.what == "Activate":
            active.add(self.keyid)
        elif self.what == "Publish":
            published.add(self.keyid)
        elif self.what == "Inactive":
            if self.keyid not in active:
                output("\tWARNING: %s scheduled to become inactive "
                       "before it is active"
                       % repr(self.key))
            else:
                active.remove(self.keyid)
        elif self.what == "Delete":
            if self.keyid in published:
                published.remove(self.keyid)
            else:
                output("WARNING: key %s is scheduled for deletion "
                       "before it is published" % repr(self.key))
        elif self.what == "Revoke":
            # We don't need to worry about the logic of this one;
            # just stop counting this key as either active or published
            if self.keyid in published:
                published.remove(self.keyid)
            if self.keyid in active:
                active.remove(self.keyid)

        return active, published
