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

import dns
import dns.edns
import dns.flags
import dns.message
import dns.query
import dns.tsig
import dns.tsigkeyring
import dns.version

from dns.edns import *
from dns.name import *
from dns.rcode import *
from dns.rdataclass import *
from dns.rdatatype import *
from dns.tsig import *


# Log query to file
def logquery(type, qname):
    with open("qlog", "a") as f:
        f.write("%s %s\n", type, qname)


# DNS 2.0 keyring specifies the algorithm
try:
    keyring = dns.tsigkeyring.from_text(
        {
            "foo": {"hmac-sha256", "aaaaaaaaaaaa"},
            "fake": {"hmac-sha256", "aaaaaaaaaaaa"},
        }
    )
except:
    keyring = dns.tsigkeyring.from_text({"foo": "aaaaaaaaaaaa", "fake": "aaaaaaaaaaaa"})

dopass2 = False


############################################################################
#
# This server will serve valid and spoofed answers. A spoofed answer will
# have the address 10.53.0.10 included.
#
# When receiving a query over UDP:
#
# A query to "nocookie"/A will result in a spoofed answer with no cookie set.
# A query to "tcponly"/A will result in a spoofed answer with no cookie set.
# A query to "withtsig"/A will result in two responses, the first is a spoofed
# answer that is TSIG signed, the second is a valid answer with a cookie set.
# A query to anything else will result in a valid answer with a cookie set.
#
# When receiving a query over TCP:
#
# A query to "nocookie"/A will result in a valid answer with no cookie set.
# A query to anything else will result in a valid answer with a cookie set.
#
############################################################################
def create_response(msg, tcp, first, ns10):
    global dopass2
    m = dns.message.from_wire(msg, keyring=keyring)
    qname = m.question[0].name.to_text()
    lqname = qname.lower()
    labels = lqname.split(".")
    rrtype = m.question[0].rdtype
    typename = dns.rdatatype.to_text(rrtype)

    with open("query.log", "a") as f:
        f.write("%s %s\n" % (typename, qname))
        print("%s %s" % (typename, qname), end=" ")

    r = dns.message.make_response(m)
    r.set_rcode(NOERROR)
    if rrtype == A:
        # exempt potential nameserver A records.
        if labels[0] == "ns" and ns10:
            r.answer.append(dns.rrset.from_text(qname, 1, IN, A, "10.53.0.10"))
        else:
            r.answer.append(dns.rrset.from_text(qname, 1, IN, A, "10.53.0.9"))
        if not tcp and labels[0] == "nocookie":
            r.answer.append(dns.rrset.from_text(qname, 1, IN, A, "10.53.0.10"))
        if not tcp and labels[0] == "tcponly":
            r.answer.append(dns.rrset.from_text(qname, 1, IN, A, "10.53.0.10"))
        if first and not tcp and labels[0] == "withtsig":
            r.answer.append(dns.rrset.from_text(qname, 1, IN, A, "10.53.0.10"))
            dopass2 = True
    elif rrtype == NS:
        r.answer.append(dns.rrset.from_text(qname, 1, IN, NS, "."))
    elif rrtype == SOA:
        r.answer.append(dns.rrset.from_text(qname, 1, IN, SOA, ". . 0 0 0 0 0"))
    else:
        r.authority.append(dns.rrset.from_text(qname, 1, IN, SOA, ". . 0 0 0 0 0"))
    # Add a server cookie to the response
    if labels[0] != "nocookie":
        for o in m.options:
            if o.otype == 10:  # Use 10 instead of COOKIE
                if first and labels[0] == "withtsig" and not tcp:
                    r.use_tsig(
                        keyring=keyring,
                        keyname=dns.name.from_text("fake"),
                        algorithm=HMAC_SHA256,
                    )
                elif labels[0] != "tcponly" or tcp:
                    cookie = o
                    if len(o.data) == 8:
                        cookie.data = o.data + o.data
                    else:
                        cookie.data = o.data
                    r.use_edns(options=[cookie])
    r.flags |= dns.flags.AA
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
ip4_addr1 = "10.53.0.9"
ip4_addr2 = "10.53.0.10"
ip6_addr1 = "fd92:7065:b8e:ffff::9"
ip6_addr2 = "fd92:7065:b8e:ffff::10"

try:
    port = int(os.environ["PORT"])
except:
    port = 5300

query4_udp1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
query4_udp1.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
query4_udp1.bind((ip4_addr1, port))
query4_tcp1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
query4_tcp1.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
query4_tcp1.bind((ip4_addr1, port))
query4_tcp1.listen(1)
query4_tcp1.settimeout(1)

query4_udp2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
query4_udp2.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
query4_udp2.bind((ip4_addr2, port))
query4_tcp2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
query4_tcp2.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
query4_tcp2.bind((ip4_addr2, port))
query4_tcp2.listen(1)
query4_tcp2.settimeout(1)

havev6 = True
query6_udp1 = None
query6_udp2 = None
query6_tcp1 = None
query6_tcp2 = None
try:
    query6_udp1 = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    query6_udp1.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    query6_udp1.bind((ip6_addr1, port))
    query6_tcp1 = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
    query6_tcp1.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    query6_tcp1.bind((ip6_addr1, port))
    query6_tcp1.listen(1)
    query6_tcp1.settimeout(1)

    query6_udp2 = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    query6_udp2.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    query6_udp2.bind((ip6_addr2, port))
    query6_tcp2 = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
    query6_tcp2.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    query6_tcp2.bind((ip6_addr2, port))
    query6_tcp2.listen(1)
    query6_tcp2.settimeout(1)
except:
    if query6_udp1 != None:
        query6_udp1.close()
    if query6_tcp1 != None:
        query6_tcp1.close()
    if query6_udp2 != None:
        query6_udp2.close()
    if query6_tcp2 != None:
        query6_tcp2.close()
    havev6 = False

signal.signal(signal.SIGTERM, sigterm)

f = open("ans.pid", "w")
pid = os.getpid()
print(pid, file=f)
f.close()

running = True

print("Using DNS version %s" % dns.version.version)
print("Listening on %s port %d" % (ip4_addr1, port))
print("Listening on %s port %d" % (ip4_addr2, port))
if havev6:
    print("Listening on %s port %d" % (ip6_addr1, port))
    print("Listening on %s port %d" % (ip6_addr2, port))
print("Ctrl-c to quit")

if havev6:
    input = [
        query4_udp1,
        query6_udp1,
        query4_tcp1,
        query6_tcp1,
        query4_udp2,
        query6_udp2,
        query4_tcp2,
        query6_tcp2,
    ]
else:
    input = [query4_udp1, query4_tcp1, query4_udp2, query4_tcp2]

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
        ns10 = False
        if s == query4_udp1 or s == query6_udp1 or s == query4_udp2 or s == query6_udp2:
            if s == query4_udp1 or s == query6_udp1:
                print(
                    "UDP Query received on %s"
                    % (ip4_addr1 if s == query4_udp1 else ip6_addr1),
                    end=" ",
                )
            if s == query4_udp2 or s == query6_udp2:
                print(
                    "UDP Query received on %s"
                    % (ip4_addr2 if s == query4_udp2 else ip6_addr2),
                    end=" ",
                )
                ns10 = True
            # Handle incoming queries
            msg = s.recvfrom(65535)
            dopass2 = False
            rsp = create_response(msg[0], False, True, ns10)
            print(dns.rcode.to_text(rsp.rcode()))
            s.sendto(rsp.to_wire(), msg[1])
            if dopass2:
                print("Sending second UDP response without TSIG", end=" ")
                rsp = create_response(msg[0], False, False, ns10)
                s.sendto(rsp.to_wire(), msg[1])
                print(dns.rcode.to_text(rsp.rcode()))

        if s == query4_tcp1 or s == query6_tcp1 or s == query4_tcp2 or s == query6_tcp2:
            try:
                (cs, _) = s.accept()
                if s == query4_tcp1 or s == query6_tcp1:
                    print(
                        "TCP Query received on %s"
                        % (ip4_addr1 if s == query4_tcp1 else ip6_addr1),
                        end=" ",
                    )
                if s == query4_tcp2 or s == query6_tcp2:
                    print(
                        "TCP Query received on %s"
                        % (ip4_addr2 if s == query4_tcp2 else ip6_addr2),
                        end=" ",
                    )
                    ns10 = True
                # get TCP message length
                buf = cs.recv(2)
                length = struct.unpack(">H", buf[:2])[0]
                # grep DNS message
                msg = cs.recv(length)
                rsp = create_response(msg, True, True, ns10)
                print(dns.rcode.to_text(rsp.rcode()))
                wire = rsp.to_wire()
                cs.send(struct.pack(">H", len(wire)))
                cs.send(wire)
                cs.close()
            except s.timeout:
                pass
    if not running:
        break
