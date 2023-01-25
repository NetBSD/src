# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

from __future__ import print_function
import os
import sys
import signal
import socket
import select
from datetime import datetime, timedelta
import time
import functools

import dns, dns.message, dns.query, dns.flags
from dns.rdatatype import *
from dns.rdataclass import *
from dns.rcode import *
from dns.name import *


# Log query to file
def logquery(type, qname):
    with open("qlog", "a") as f:
        f.write("%s %s\n", type, qname)


def endswith(domain, labels):
    return domain.endswith("." + labels) or domain == labels


############################################################################
# Respond to a DNS query.
# For good. it serves:
# zoop.boing.good. NS ns3.good.
# icky.ptang.zoop.boing.good. NS a.bit.longer.ns.name.good.
# it responds properly (with NODATA empty response) to non-empty terminals
#
# For slow. it works the same as for good., but each response is delayed by 400 milliseconds
#
# For bad. it works the same as for good., but returns NXDOMAIN to non-empty terminals
#
# For ugly. it works the same as for good., but returns garbage to non-empty terminals
#
# For stale. it serves:
# a.b.stale. IN TXT peekaboo (resolver did not do qname minimization)
############################################################################
def create_response(msg):
    m = dns.message.from_wire(msg)
    qname = m.question[0].name.to_text()
    lqname = qname.lower()
    labels = lqname.split(".")
    suffix = ""

    # get qtype
    rrtype = m.question[0].rdtype
    typename = dns.rdatatype.to_text(rrtype)
    if typename == "A" or typename == "AAAA":
        typename = "ADDR"
    bad = False
    ugly = False
    slow = False

    # log this query
    with open("query.log", "a") as f:
        f.write("%s %s\n" % (typename, lqname))
        print("%s %s" % (typename, lqname), end=" ")

    r = dns.message.make_response(m)
    r.set_rcode(NOERROR)

    ip6req = False

    if endswith(lqname, "bad."):
        bad = True
        suffix = "bad."
        lqname = lqname[:-4]
    elif endswith(lqname, "ugly."):
        ugly = True
        suffix = "ugly."
        lqname = lqname[:-5]
    elif endswith(lqname, "good."):
        suffix = "good."
        lqname = lqname[:-5]
    elif endswith(lqname, "slow."):
        slow = True
        suffix = "slow."
        lqname = lqname[:-5]
    elif endswith(lqname, "8.2.6.0.1.0.0.2.ip6.arpa."):
        ip6req = True
    elif endswith(lqname, "a.b.stale."):
        if lqname == "a.b.stale.":
            if rrtype == TXT:
                # Direct query.
                r.answer.append(dns.rrset.from_text(lqname, 1, IN, TXT, "peekaboo"))
                r.flags |= dns.flags.AA
            elif rrtype == NS:
                # NS a.b.
                r.answer.append(dns.rrset.from_text(lqname, 1, IN, NS, "ns.a.b.stale."))
                r.additional.append(
                    dns.rrset.from_text("ns.a.b.stale.", 1, IN, A, "10.53.0.3")
                )
                r.flags |= dns.flags.AA
            elif rrtype == SOA:
                # SOA a.b.
                r.answer.append(
                    dns.rrset.from_text(
                        lqname, 1, IN, SOA, "a.b.stale. hostmaster.a.b.stale. 1 2 3 4 5"
                    )
                )
                r.flags |= dns.flags.AA
            else:
                # NODATA.
                r.authority.append(
                    dns.rrset.from_text(
                        lqname, 1, IN, SOA, "a.b.stale. hostmaster.a.b.stale. 1 2 3 4 5"
                    )
                )
        else:
            r.authority.append(
                dns.rrset.from_text(
                    lqname, 1, IN, SOA, "a.b.stale. hostmaster.a.b.stale. 1 2 3 4 5"
                )
            )
            r.set_rcode(NXDOMAIN)
            # NXDOMAIN.
        return r
    else:
        r.set_rcode(REFUSED)
        return r

    # Good/bad differs only in how we treat non-empty terminals
    if lqname == "zoop.boing." and rrtype == NS:
        r.answer.append(
            dns.rrset.from_text(lqname + suffix, 1, IN, NS, "ns3." + suffix)
        )
        r.flags |= dns.flags.AA
    elif endswith(lqname, "icky.ptang.zoop.boing."):
        r.authority.append(
            dns.rrset.from_text(
                "icky.ptang.zoop.boing." + suffix,
                1,
                IN,
                NS,
                "a.bit.longer.ns.name." + suffix,
            )
        )
    elif endswith("icky.ptang.zoop.boing.", lqname):
        r.authority.append(
            dns.rrset.from_text(
                "zoop.boing." + suffix,
                1,
                IN,
                SOA,
                "ns3." + suffix + " hostmaster.arpa. 2018050100 1 1 1 1",
            )
        )
        if bad:
            r.set_rcode(NXDOMAIN)
        if ugly:
            r.set_rcode(FORMERR)
    elif endswith(lqname, "zoop.boing."):
        r.authority.append(
            dns.rrset.from_text(
                "zoop.boing." + suffix,
                1,
                IN,
                SOA,
                "ns3." + suffix + " hostmaster.arpa. 2018050100 1 1 1 1",
            )
        )
        r.set_rcode(NXDOMAIN)
    elif ip6req:
        r.authority.append(
            dns.rrset.from_text(
                "1.1.1.1.8.2.6.0.1.0.0.2.ip6.arpa.", 60, IN, NS, "ns4.good."
            )
        )
        r.additional.append(dns.rrset.from_text("ns4.good.", 60, IN, A, "10.53.0.4"))
    else:
        r.set_rcode(REFUSED)

    if slow:
        time.sleep(0.4)
    return r


def sigterm(signum, frame):
    print("Shutting down now...")
    os.remove("ans.pid")
    running = False
    sys.exit(0)


############################################################################
# Main
#
# Set up responder and control channel, open the pid file, and start
# the main loop, listening for queries on the query channel or commands
# on the control channel and acting on them.
############################################################################
ip4 = "10.53.0.3"
ip6 = "fd92:7065:b8e:ffff::3"

try:
    port = int(os.environ["PORT"])
except:
    port = 5300

query4_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
query4_socket.bind((ip4, port))

havev6 = True
try:
    query6_socket = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    try:
        query6_socket.bind((ip6, port))
    except:
        query6_socket.close()
        havev6 = False
except:
    havev6 = False

signal.signal(signal.SIGTERM, sigterm)

f = open("ans.pid", "w")
pid = os.getpid()
print(pid, file=f)
f.close()

running = True

print("Listening on %s port %d" % (ip4, port))
if havev6:
    print("Listening on %s port %d" % (ip6, port))
print("Ctrl-c to quit")

if havev6:
    input = [query4_socket, query6_socket]
else:
    input = [query4_socket]

while running:
    try:
        inputready, outputready, exceptready = select.select(input, [], [])
    except select.error as e:
        break
    except socket.error as e:
        break
    except KeyboardInterrupt:
        break

    for s in inputready:
        if s == query4_socket or s == query6_socket:
            print(
                "Query received on %s" % (ip4 if s == query4_socket else ip6), end=" "
            )
            # Handle incoming queries
            msg = s.recvfrom(65535)
            rsp = create_response(msg[0])
            if rsp:
                print(dns.rcode.to_text(rsp.rcode()))
                s.sendto(rsp.to_wire(), msg[1])
            else:
                print("NO RESPONSE")
    if not running:
        break
