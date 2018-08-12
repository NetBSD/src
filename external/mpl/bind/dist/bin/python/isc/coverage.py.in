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

from __future__ import print_function
import os
import sys
import argparse
import glob
import re
import time
import calendar
import pprint
from collections import defaultdict

prog = 'dnssec-coverage'

from isc import dnskey, eventlist, keydict, keyevent, keyzone, utils

############################################################################
# print a fatal error and exit
############################################################################
def fatal(*args, **kwargs):
    print(*args, **kwargs)
    sys.exit(1)


############################################################################
# output:
############################################################################
_firstline = True
def output(*args, **kwargs):
    """output text, adding a vertical space this is *not* the first
    first section being printed since a call to vreset()"""
    global _firstline
    if 'skip' in kwargs:
        skip = kwargs['skip']
        kwargs.pop('skip', None)
    else:
        skip = True
    if _firstline:
        _firstline = False
    elif skip:
        print('')
    if args:
        print(*args, **kwargs)


def vreset():
    """reset vertical spacing"""
    global _firstline
    _firstline = True


############################################################################
# parse_time
############################################################################
def parse_time(s):
    """ convert a formatted time (e.g., 1y, 6mo, 15mi, etc) into seconds
    :param s: String with some text representing a time interval
    :return: Integer with the number of seconds in the time interval
    """
    s = s.strip()

    # if s is an integer, we're done already
    try:
        return int(s)
    except ValueError:
        pass

    # try to parse as a number with a suffix indicating unit of time
    r = re.compile(r'([0-9][0-9]*)\s*([A-Za-z]*)')
    m = r.match(s)
    if not m:
        raise ValueError("Cannot parse %s" % s)
    n, unit = m.groups()
    n = int(n)
    unit = unit.lower()
    if unit.startswith('y'):
        return n * 31536000
    elif unit.startswith('mo'):
        return n * 2592000
    elif unit.startswith('w'):
        return n * 604800
    elif unit.startswith('d'):
        return n * 86400
    elif unit.startswith('h'):
        return n * 3600
    elif unit.startswith('mi'):
        return n * 60
    elif unit.startswith('s'):
        return n
    else:
        raise ValueError("Invalid suffix %s" % unit)


############################################################################
# set_path:
############################################################################
def set_path(command, default=None):
    """ find the location of a specified command.  if a default is supplied
    and it works, we use it; otherwise we search PATH for a match.
    :param command: string with a command to look for in the path
    :param default: default location to use
    :return: detected location for the desired command
    """

    fpath = default
    if not fpath or not os.path.isfile(fpath) or not os.access(fpath, os.X_OK):
        path = os.environ["PATH"]
        if not path:
            path = os.path.defpath
        for directory in path.split(os.pathsep):
            fpath = os.path.join(directory, command)
            if os.path.isfile(fpath) and os.access(fpath, os.X_OK):
                break
            fpath = None

    return fpath


############################################################################
# parse_args:
############################################################################
def parse_args():
    """Read command line arguments, set global 'args' structure"""
    compilezone = set_path('named-compilezone',
                           os.path.join(utils.prefix('sbin'),
                           'named-compilezone'))

    parser = argparse.ArgumentParser(description=prog + ': checks future ' +
                                     'DNSKEY coverage for a zone')

    parser.add_argument('zone', type=str, nargs='*', default=None,
                        help='zone(s) to check' +
                        '(default: all zones in the directory)')
    parser.add_argument('-K', dest='path', default='.', type=str,
                        help='a directory containing keys to process',
                        metavar='dir')
    parser.add_argument('-f', dest='filename', type=str,
                        help='zone master file', metavar='file')
    parser.add_argument('-m', dest='maxttl', type=str,
                        help='the longest TTL in the zone(s)',
                        metavar='time')
    parser.add_argument('-d', dest='keyttl', type=str,
                        help='the DNSKEY TTL', metavar='time')
    parser.add_argument('-r', dest='resign', default='1944000',
                        type=str, help='the RRSIG refresh interval '
                                       'in seconds [default: 22.5 days]',
                        metavar='time')
    parser.add_argument('-c', dest='compilezone',
                        default=compilezone, type=str,
                        help='path to \'named-compilezone\'',
                        metavar='path')
    parser.add_argument('-l', dest='checklimit',
                        type=str, default='0',
                        help='Length of time to check for '
                             'DNSSEC coverage [default: 0 (unlimited)]',
                        metavar='time')
    parser.add_argument('-z', dest='no_ksk',
                        action='store_true', default=False,
                        help='Only check zone-signing keys (ZSKs)')
    parser.add_argument('-k', dest='no_zsk',
                        action='store_true', default=False,
                        help='Only check key-signing keys (KSKs)')
    parser.add_argument('-D', '--debug', dest='debug_mode',
                        action='store_true', default=False,
                        help='Turn on debugging output')
    parser.add_argument('-v', '--version', action='version',
                        version=utils.version)

    args = parser.parse_args()

    if args.no_zsk and args.no_ksk:
        fatal("ERROR: -z and -k cannot be used together.")
    elif args.no_zsk or args.no_ksk:
        args.keytype = "KSK" if args.no_zsk else "ZSK"
    else:
        args.keytype = None

    if args.filename and len(args.zone) > 1:
        fatal("ERROR: -f can only be used with one zone.")

    # convert from time arguments to seconds
    try:
        if args.maxttl:
            m = parse_time(args.maxttl)
            args.maxttl = m
    except ValueError:
        pass

    try:
        if args.keyttl:
            k = parse_time(args.keyttl)
            args.keyttl = k
    except ValueError:
        pass

    try:
        if args.resign:
            r = parse_time(args.resign)
            args.resign = r
    except ValueError:
        pass

    try:
        if args.checklimit:
            lim = args.checklimit
            r = parse_time(args.checklimit)
            if r == 0:
                args.checklimit = None
            else:
                args.checklimit = time.time() + r
    except ValueError:
        pass

    # if we've got the values we need from the command line, stop now
    if args.maxttl and args.keyttl:
        return args

    # load keyttl and maxttl data from zonefile
    if args.zone and args.filename:
        try:
            zone = keyzone(args.zone[0], args.filename, args.compilezone)
            args.maxttl = args.maxttl or zone.maxttl
            args.keyttl = args.maxttl or zone.keyttl
        except Exception as e:
            print("Unable to load zone data from %s: " % args.filename, e)

    if not args.maxttl:
        output("WARNING: Maximum TTL value was not specified.  Using 1 week\n"
               "\t (604800 seconds); re-run with the -m option to get more\n"
               "\t accurate results.")
        args.maxttl = 604800

    return args

############################################################################
# Main
############################################################################
def main():
    args = parse_args()

    print("PHASE 1--Loading keys to check for internal timing problems")

    try:
        kd = keydict(path=args.path, zone=args.zone, keyttl=args.keyttl)
    except Exception as e:
        fatal('ERROR: Unable to build key dictionary: ' + str(e))

    for key in kd:
        key.check_prepub(output)
        if key.sep:
            key.check_postpub(output)
        else:
            key.check_postpub(output, args.maxttl + args.resign)

    output("PHASE 2--Scanning future key events for coverage failures")
    vreset()

    try:
        elist = eventlist(kd)
    except Exception as e:
        fatal('ERROR: Unable to build event list: ' + str(e))

    errors = False
    if not args.zone:
        if not elist.coverage(None, args.keytype, args.checklimit, output):
            errors = True
    else:
        for zone in args.zone:
            try:
                if not elist.coverage(zone, args.keytype,
                                      args.checklimit, output):
                    errors = True
            except:
                output('ERROR: Coverage check failed for zone ' + zone)

    sys.exit(1 if errors else 0)
