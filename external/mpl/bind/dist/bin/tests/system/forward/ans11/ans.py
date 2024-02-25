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
import struct
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


# Create a UDP listener
def udp_listen(ip, port, is_ipv6=False):
    try:
        udp = socket.socket(
            socket.AF_INET6 if is_ipv6 else socket.AF_INET, socket.SOCK_DGRAM
        )
        try:
            udp.bind((ip, port))
        except:
            udp.close()
            udp = None
    except:
        udp = None

    if udp is None and not is_ipv6:
        raise socket.error("Can not create an IPv4 UDP listener")

    return udp


# Create a TCP listener
def tcp_listen(ip, port, is_ipv6=False):
    try:
        tcp = socket.socket(
            socket.AF_INET6 if is_ipv6 else socket.AF_INET, socket.SOCK_STREAM
        )
        try:
            tcp.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            tcp.bind((ip, port))
            tcp.listen(100)
        except:
            tcp.close()
            tcp = None
    except:
        tcp = None

    if tcp is None and not is_ipv6:
        raise socket.error("Can not create an IPv4 TCP listener")

    return tcp


############################################################################
# Control channel - send "1" or "0" to enable or disable the "silent" mode.
############################################################################
silent = False


def ctrl_channel(msg):
    global silent

    msg = msg.splitlines().pop(0)
    print("Received control message: %s" % msg)

    if len(msg) != 1:
        return

    if silent:
        if msg == b"0":
            silent = False
            print("Silent mode was disabled")
    else:
        if msg == b"1":
            silent = True
            print("Silent mode was enabled")


############################################################################
# Respond to a DNS query.
############################################################################
def create_response(msg):
    m = dns.message.from_wire(msg)
    qname = m.question[0].name.to_text()
    rrtype = m.question[0].rdtype
    typename = dns.rdatatype.to_text(rrtype)

    with open("query.log", "a") as f:
        f.write("%s %s\n" % (typename, qname))
        print("%s %s" % (typename, qname), end=" ")

    r = dns.message.make_response(m)
    r.set_rcode(NOERROR)
    if rrtype == A:
        tld = qname.split(".")[-2] + "."
        ns = "local." + tld
        r.answer.append(dns.rrset.from_text(qname, 300, IN, A, "10.53.0.11"))
        r.answer.append(dns.rrset.from_text(tld, 300, IN, NS, "local." + tld))
        r.additional.append(dns.rrset.from_text(ns, 300, IN, A, "10.53.0.11"))
    elif rrtype == NS:
        r.answer.append(dns.rrset.from_text(qname, 300, IN, NS, "."))
    elif rrtype == SOA:
        r.answer.append(dns.rrset.from_text(qname, 300, IN, SOA, ". . 0 0 0 0 0"))
    else:
        r.authority.append(dns.rrset.from_text(qname, 300, IN, SOA, ". . 0 0 0 0 0"))
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
ip4 = "10.53.0.11"
ip6 = "fd92:7065:b8e:ffff::11"

try:
    port = int(os.environ["PORT"])
except:
    port = 5300

try:
    ctrlport = int(os.environ["EXTRAPORT1"])
except:
    ctrlport = 5300

ctrl4_tcp = tcp_listen(ip4, ctrlport)
query4_udp = udp_listen(ip4, port)
query6_udp = udp_listen(ip6, port, is_ipv6=True)
query4_tcp = tcp_listen(ip4, port)
query6_tcp = tcp_listen(ip6, port, is_ipv6=True)

havev6 = query6_udp is not None and query6_tcp is not None

signal.signal(signal.SIGTERM, sigterm)

f = open("ans.pid", "w")
pid = os.getpid()
print(pid, file=f)
f.close()

running = True

print("Listening on %s port %d" % (ip4, ctrlport))
print("Listening on %s port %d" % (ip4, port))
if havev6:
    print("Listening on %s port %d" % (ip6, port))

print("Ctrl-c to quit")

if havev6:
    input = [ctrl4_tcp, query4_udp, query6_udp, query4_tcp, query6_tcp]
else:
    input = [ctrl4_tcp, query4_udp, query4_tcp]

hung_conns = []

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
        if s == ctrl4_tcp:
            print("Control channel connected")
            conn = None
            try:
                # Handle control channel input
                conn, addr = s.accept()
                msg = conn.recv(1)
                if msg:
                    ctrl_channel(msg)
                conn.close()
            except s.timeout:
                pass
            if conn:
                conn.close()
        elif s == query4_tcp or s == query6_tcp:
            print(
                "TCP query received on %s" % (ip4 if s == query4_tcp else ip6), end=" "
            )
            conn = None
            try:
                # Handle incoming queries
                conn, addr = s.accept()
                if not silent:
                    # get TCP message length
                    msg = conn.recv(2)
                    if len(msg) != 2:
                        print("NO RESPONSE (can not read the message length)")
                        conn.close()
                        continue
                    length = struct.unpack(">H", msg[:2])[0]
                    msg = conn.recv(length)
                    if len(msg) != length:
                        print("NO RESPONSE (can not read the message)")
                        conn.close()
                        continue
                    rsp = create_response(msg)
                    if rsp:
                        print(dns.rcode.to_text(rsp.rcode()))
                        wire = rsp.to_wire()
                        conn.send(struct.pack(">H", len(wire)))
                        conn.send(wire)
                    else:
                        print("NO RESPONSE (can not create a response)")
                else:
                    # Do not respond and hang the connection.
                    print("NO RESPONSE (silent mode)")
                    hung_conns.append(conn)
                    continue
            except socket.error as e:
                print("NO RESPONSE (error: %s)" % str(e))
            if conn:
                conn.close()
        elif s == query4_udp or s == query6_udp:
            print(
                "UDP query received on %s" % (ip4 if s == query4_udp else ip6), end=" "
            )
            # Handle incoming queries
            msg = s.recvfrom(65535)
            if not silent:
                rsp = create_response(msg[0])
                if rsp:
                    print(dns.rcode.to_text(rsp.rcode()))
                    s.sendto(rsp.to_wire(), msg[1])
                else:
                    print("NO RESPONSE (can not create a response)")
            else:
                # Do not respond.
                print("NO RESPONSE (silent mode)")
    if not running:
        break
