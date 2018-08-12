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
import time
import calendar
from subprocess import Popen, PIPE

########################################################################
# Class dnskey
########################################################################
class TimePast(Exception):
    def __init__(self, key, prop, value):
        super(TimePast, self).__init__('%s time for key %s (%d) is already past'
                                       % (prop, key, value))

class dnskey:
    """An individual DNSSEC key.  Identified by path, name, algorithm, keyid.
    Contains a dictionary of metadata events."""

    _PROPS = ('Created', 'Publish', 'Activate', 'Inactive', 'Delete',
              'Revoke', 'DSPublish', 'SyncPublish', 'SyncDelete')
    _OPTS = (None, '-P', '-A', '-I', '-D', '-R', None, '-Psync', '-Dsync')

    _ALGNAMES = (None, 'RSAMD5', 'DH', 'DSA', 'ECC', 'RSASHA1',
                 'NSEC3DSA', 'NSEC3RSASHA1', 'RSASHA256', None,
                 'RSASHA512', None, 'ECCGOST', 'ECDSAP256SHA256',
                 'ECDSAP384SHA384', 'ED25519', 'ED448')

    def __init__(self, key, directory=None, keyttl=None):
        # this makes it possible to use algname as a class or instance method
        if isinstance(key, tuple) and len(key) == 3:
            self._dir = directory or '.'
            (name, alg, keyid) = key
            self.fromtuple(name, alg, keyid, keyttl)

        self._dir = directory or os.path.dirname(key) or '.'
        key = os.path.basename(key)
        (name, alg, keyid) = key.split('+')
        name = name[1:-1]
        alg = int(alg)
        keyid = int(keyid.split('.')[0])
        self.fromtuple(name, alg, keyid, keyttl)

    def fromtuple(self, name, alg, keyid, keyttl):
        if name.endswith('.'):
            fullname = name
            name = name.rstrip('.')
        else:
            fullname = name + '.'

        keystr = "K%s+%03d+%05d" % (fullname, alg, keyid)
        key_file = self._dir + (self._dir and os.sep or '') + keystr + ".key"
        private_file = (self._dir + (self._dir and os.sep or '') +
                        keystr + ".private")

        self.keystr = keystr

        self.name = name
        self.alg = int(alg)
        self.keyid = int(keyid)
        self.fullname = fullname

        kfp = open(key_file, "r")
        for line in kfp:
            if line[0] == ';':
                continue
            tokens = line.split()
            if not tokens:
                continue

            if tokens[1].lower() in ('in', 'ch', 'hs'):
                septoken = 3
                self.ttl = keyttl
            else:
                septoken = 4
                self.ttl = int(tokens[1]) if not keyttl else keyttl

            if (int(tokens[septoken]) & 0x1) == 1:
                self.sep = True
            else:
                self.sep = False
        kfp.close()

        pfp = open(private_file, "rU")

        self.metadata = dict()
        self._changed = dict()
        self._delete = dict()
        self._times = dict()
        self._fmttime = dict()
        self._timestamps = dict()
        self._original = dict()
        self._origttl = None

        for line in pfp:
            line = line.strip()
            if not line or line[0] in ('!#'):
                continue
            punctuation = [line.find(c) for c in ':= '] + [len(line)]
            found = min([pos for pos in punctuation if pos != -1])
            name = line[:found].rstrip()
            value = line[found:].lstrip(":= ").rstrip()
            self.metadata[name] = value

        for prop in dnskey._PROPS:
            self._changed[prop] = False
            if prop in self.metadata:
                t = self.parsetime(self.metadata[prop])
                self._times[prop] = t
                self._fmttime[prop] = self.formattime(t)
                self._timestamps[prop] = self.epochfromtime(t)
                self._original[prop] = self._timestamps[prop]
            else:
                self._times[prop] = None
                self._fmttime[prop] = None
                self._timestamps[prop] = None
                self._original[prop] = None

        pfp.close()

    def commit(self, settime_bin, **kwargs):
        quiet = kwargs.get('quiet', False)
        cmd = []
        first = True

        if self._origttl is not None:
            cmd += ["-L", str(self.ttl)]

        for prop, opt in zip(dnskey._PROPS, dnskey._OPTS):
            if not opt or not self._changed[prop]:
                continue

            delete = False
            if prop in self._delete and self._delete[prop]:
                delete = True

            when = 'none' if delete else self._fmttime[prop]
            cmd += [opt, when]
            first = False

        if cmd:
            fullcmd = [settime_bin, "-K", self._dir] + cmd + [self.keystr,]
            if not quiet:
                print('# ' + ' '.join(fullcmd))
            try:
                p = Popen(fullcmd, stdout=PIPE, stderr=PIPE)
                stdout, stderr = p.communicate()
                if stderr:
                    raise Exception(str(stderr))
            except Exception as e:
                raise Exception('unable to run %s: %s' %
                                (settime_bin, str(e)))
            self._origttl = None
            for prop in dnskey._PROPS:
                self._original[prop] = self._timestamps[prop]
                self._changed[prop] = False

    @classmethod
    def generate(cls, keygen_bin, randomdev, keys_dir, name, alg, keysize, sep,
                 ttl, publish=None, activate=None, **kwargs):
        quiet = kwargs.get('quiet', False)

        keygen_cmd = [keygen_bin, "-q", "-K", keys_dir, "-L", str(ttl)]

        if randomdev:
            keygen_cmd += ["-r", randomdev]

        if sep:
            keygen_cmd.append("-fk")

        if alg:
            keygen_cmd += ["-a", alg]

        if keysize:
            keygen_cmd += ["-b", str(keysize)]

        if publish:
            t = dnskey.timefromepoch(publish)
            keygen_cmd += ["-P", dnskey.formattime(t)]

        if activate:
            t = dnskey.timefromepoch(activate)
            keygen_cmd += ["-A", dnskey.formattime(activate)]

        keygen_cmd.append(name)

        if not quiet:
            print('# ' + ' '.join(keygen_cmd))

        p = Popen(keygen_cmd, stdout=PIPE, stderr=PIPE)
        stdout, stderr = p.communicate()
        if stderr:
            raise Exception('unable to generate key: ' + str(stderr))

        try:
            keystr = stdout.splitlines()[0].decode('ascii')
            newkey = dnskey(keystr, keys_dir, ttl)
            return newkey
        except Exception as e:
            raise Exception('unable to parse generated key: %s' % str(e))

    def generate_successor(self, keygen_bin, randomdev, prepublish, **kwargs):
        quiet = kwargs.get('quiet', False)

        if not self.inactive():
            raise Exception("predecessor key %s has no inactive date" % self)

        keygen_cmd = [keygen_bin, "-q", "-K", self._dir, "-S", self.keystr]

        if self.ttl:
            keygen_cmd += ["-L", str(self.ttl)]

        if randomdev:
            keygen_cmd += ["-r", randomdev]

        if prepublish:
            keygen_cmd += ["-i", str(prepublish)]

        if not quiet:
            print('# ' + ' '.join(keygen_cmd))

        p = Popen(keygen_cmd, stdout=PIPE, stderr=PIPE)
        stdout, stderr = p.communicate()
        if stderr:
            raise Exception('unable to generate key: ' + stderr)

        try:
            keystr = stdout.splitlines()[0].decode('ascii')
            newkey = dnskey(keystr, self._dir, self.ttl)
            return newkey
        except:
            raise Exception('unable to generate successor for key %s' % self)

    @staticmethod
    def algstr(alg):
        name = None
        if alg in range(len(dnskey._ALGNAMES)):
            name = dnskey._ALGNAMES[alg]
        return name if name else ("%03d" % alg)

    @staticmethod
    def algnum(alg):
        if not alg:
            return None
        alg = alg.upper()
        try:
            return dnskey._ALGNAMES.index(alg)
        except ValueError:
            return None

    def algname(self, alg=None):
        return self.algstr(alg or self.alg)

    @staticmethod
    def timefromepoch(secs):
        return time.gmtime(secs)

    @staticmethod
    def parsetime(string):
        return time.strptime(string, "%Y%m%d%H%M%S")

    @staticmethod
    def epochfromtime(t):
        return calendar.timegm(t)

    @staticmethod
    def formattime(t):
        return time.strftime("%Y%m%d%H%M%S", t)

    def setmeta(self, prop, secs, now, **kwargs):
        force = kwargs.get('force', False)

        if self._timestamps[prop] == secs:
            return

        if self._original[prop] is not None and \
           self._original[prop] < now and not force:
            raise TimePast(self, prop, self._original[prop])

        if secs is None:
            self._changed[prop] = False \
                if self._original[prop] is None else True

            self._delete[prop] = True
            self._timestamps[prop] = None
            self._times[prop] = None
            self._fmttime[prop] = None
            return

        t = self.timefromepoch(secs)
        self._timestamps[prop] = secs
        self._times[prop] = t
        self._fmttime[prop] = self.formattime(t)
        self._changed[prop] = False if \
            self._original[prop] == self._timestamps[prop] else True

    def gettime(self, prop):
        return self._times[prop]

    def getfmttime(self, prop):
        return self._fmttime[prop]

    def gettimestamp(self, prop):
        return self._timestamps[prop]

    def created(self):
        return self._timestamps["Created"]

    def syncpublish(self):
        return self._timestamps["SyncPublish"]

    def setsyncpublish(self, secs, now=time.time(), **kwargs):
        self.setmeta("SyncPublish", secs, now, **kwargs)

    def publish(self):
        return self._timestamps["Publish"]

    def setpublish(self, secs, now=time.time(), **kwargs):
        self.setmeta("Publish", secs, now, **kwargs)

    def activate(self):
        return self._timestamps["Activate"]

    def setactivate(self, secs, now=time.time(), **kwargs):
        self.setmeta("Activate", secs, now, **kwargs)

    def revoke(self):
        return self._timestamps["Revoke"]

    def setrevoke(self, secs, now=time.time(), **kwargs):
        self.setmeta("Revoke", secs, now, **kwargs)

    def inactive(self):
        return self._timestamps["Inactive"]

    def setinactive(self, secs, now=time.time(), **kwargs):
        self.setmeta("Inactive", secs, now, **kwargs)

    def delete(self):
        return self._timestamps["Delete"]

    def setdelete(self, secs, now=time.time(), **kwargs):
        self.setmeta("Delete", secs, now, **kwargs)

    def syncdelete(self):
        return self._timestamps["SyncDelete"]

    def setsyncdelete(self, secs, now=time.time(), **kwargs):
        self.setmeta("SyncDelete", secs, now, **kwargs)

    def setttl(self, ttl):
        if ttl is None or self.ttl == ttl:
            return
        elif self._origttl is None:
            self._origttl = self.ttl
            self.ttl = ttl
        elif self._origttl == ttl:
            self._origttl = None
            self.ttl = ttl
        else:
            self.ttl = ttl

    def keytype(self):
        return ("KSK" if self.sep else "ZSK")

    def __str__(self):
        return ("%s/%s/%05d"
                % (self.name, self.algname(), self.keyid))

    def __repr__(self):
        return ("%s/%s/%05d (%s)"
                % (self.name, self.algname(), self.keyid,
                   ("KSK" if self.sep else "ZSK")))

    def date(self):
        return (self.activate() or self.publish() or self.created())

    # keys are sorted first by zone name, then by algorithm. within
    # the same name/algorithm, they are sorted according to their
    # 'date' value: the activation date if set, OR the publication
    # if set, OR the creation date.
    def __lt__(self, other):
        if self.name != other.name:
            return self.name < other.name
        if self.alg != other.alg:
            return self.alg < other.alg
        return self.date() < other.date()

    def check_prepub(self, output=None):
        def noop(*args, **kwargs): pass
        if not output:
            output = noop

        now = int(time.time())
        a = self.activate()
        p = self.publish()

        if not a:
            return False

        if not p:
            if a > now:
                output("WARNING: Key %s is scheduled for\n"
                       "\t activation but not for publication."
                       % repr(self))
            return False

        if p <= now and a <= now:
            return True

        if p == a:
            output("WARNING: %s is scheduled to be\n"
                   "\t published and activated at the same time. This\n"
                   "\t could result in a coverage gap if the zone was\n"
                   "\t previously signed. Activation should be at least\n"
                   "\t %s after publication."
                   % (repr(self),
                       dnskey.duration(self.ttl) or 'one DNSKEY TTL'))
            return True

        if a < p:
            output("WARNING: Key %s is active before it is published"
                   % repr(self))
            return False

        if self.ttl is not None and a - p < self.ttl:
            output("WARNING: Key %s is activated too soon\n"
                   "\t after publication; this could result in coverage \n"
                   "\t gaps due to resolver caches containing old data.\n"
                   "\t Activation should be at least %s after\n"
                   "\t publication."
                   % (repr(self),
                      dnskey.duration(self.ttl) or 'one DNSKEY TTL'))
            return False

        return True

    def check_postpub(self, output = None, timespan = None):
        def noop(*args, **kwargs): pass
        if output is None:
            output = noop

        if timespan is None:
            timespan = self.ttl

        now = time.time()
        d = self.delete()
        i = self.inactive()

        if not d:
            return False

        if not i:
            if d > now:
                output("WARNING: Key %s is scheduled for\n"
                       "\t deletion but not for inactivation." % repr(self))
            return False

        if d < now and i < now:
            return True

        if d < i:
            output("WARNING: Key %s is scheduled for\n"
                   "\t deletion before inactivation."
                   % repr(self))
            return False

        if d - i < timespan:
            output("WARNING: Key %s scheduled for\n"
                   "\t deletion too soon after deactivation; this may \n"
                   "\t result in coverage gaps due to resolver caches\n"
                   "\t containing old data.  Deletion should be at least\n"
                   "\t %s after inactivation."
                   % (repr(self), dnskey.duration(timespan)))
            return False

        return True

    @staticmethod
    def duration(secs):
        if not secs:
            return None

        units = [("year", 60*60*24*365),
                 ("month", 60*60*24*30),
                 ("day", 60*60*24),
                 ("hour", 60*60),
                 ("minute", 60),
                 ("second", 1)]

        output = []
        for unit in units:
            v, secs = secs // unit[1], secs % unit[1]
            if v > 0:
                output.append("%d %s%s" % (v, unit[0], "s" if v > 1 else ""))

        return ", ".join(output)

