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

import dns, dns.message
from dns.rcode import *

modes = [
    b"silent",  # Do not respond
    b"close",  # UDP: same as silent; TCP: also close the connection
    b"servfail",  # Always respond with SERVFAIL
    b"unstable",  # Constantly switch between "silent" and "servfail"
]
mode = modes[0]
n = 0


def ctrl_channel(msg):
    global modes, mode, n

    msg = msg.splitlines().pop(0)
    print("Received control message: %s" % msg)

    if msg in modes:
        mode = msg
        n = 0
        print("New mode: %s" % str(mode))


def create_servfail(msg):
    m = dns.message.from_wire(msg)
    qname = m.question[0].name.to_text()
    rrtype = m.question[0].rdtype
    typename = dns.rdatatype.to_text(rrtype)

    with open("query.log", "a") as f:
        f.write("%s %s\n" % (typename, qname))
        print("%s %s" % (typename, qname), end=" ")

    r = dns.message.make_response(m)
    r.set_rcode(SERVFAIL)
    return r


def sigterm(signum, frame):
    print("Shutting down now...")
    os.remove("ans.pid")
    running = False
    sys.exit(0)


ip4 = "10.53.0.8"

try:
    port = int(os.environ["PORT"])
except:
    port = 5300

try:
    ctrlport = int(os.environ["EXTRAPORT1"])
except:
    ctrlport = 5300

query4_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
query4_udp.bind((ip4, port))

query4_tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
query4_tcp.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
query4_tcp.bind((ip4, port))
query4_tcp.listen(100)

ctrl4_tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
ctrl4_tcp.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
ctrl4_tcp.bind((ip4, ctrlport))
ctrl4_tcp.listen(100)

signal.signal(signal.SIGTERM, sigterm)

f = open("ans.pid", "w")
pid = os.getpid()
print(pid, file=f)
f.close()

running = True

print("Listening on %s port %d" % (ip4, port))
print("Listening on %s port %d" % (ip4, ctrlport))
print("Ctrl-c to quit")

input = [query4_udp, query4_tcp, ctrl4_tcp]

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
        if s == query4_udp:
            n = n + 1
            print("UDP query received on %s" % ip4, end=" ")
            msg = s.recvfrom(65535)
            if (
                mode == b"silent"
                or mode == b"close"
                or (mode == b"unstable" and n % 2 == 1)
            ):
                # Do not respond.
                print("NO RESPONSE (%s)" % str(mode))
                continue
            elif mode == b"servfail" or (mode == b"unstable" and n % 2 == 0):
                rsp = create_servfail(msg[0])
                if rsp:
                    print(dns.rcode.to_text(rsp.rcode()))
                    s.sendto(rsp.to_wire(), msg[1])
                else:
                    print("NO RESPONSE (can not create a response)")
            else:
                raise (Exception("unsupported mode: %s" % mode))
        elif s == query4_tcp:
            n = n + 1
            print("TCP query received on %s" % ip4, end=" ")
            conn = None
            try:
                if mode == b"silent" or (mode == b"unstable" and n % 2 == 1):
                    conn, addr = s.accept()
                    # Do not respond and hang the connection.
                    print("NO RESPONSE (%s)" % str(mode))
                    hung_conns.append(conn)
                    continue
                elif mode == b"close":
                    conn, addr = s.accept()
                    # Do not respond and close the connection.
                    print("NO RESPONSE (%s)" % str(mode))
                    conn.close()
                    continue
                elif mode == b"servfail" or (mode == b"unstable" and n % 2 == 0):
                    conn, addr = s.accept()
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
                    rsp = create_servfail(msg)
                    if rsp:
                        print(dns.rcode.to_text(rsp.rcode()))
                        wire = rsp.to_wire()
                        conn.send(struct.pack(">H", len(wire)))
                        conn.send(wire)
                    else:
                        print("NO RESPONSE (can not create a response)")
                else:
                    raise (Exception("unsupported mode: %s" % mode))
            except socket.error as e:
                print("NO RESPONSE (error: %s)" % str(e))
            if conn:
                conn.close()
        elif s == ctrl4_tcp:
            print("Control channel connected")
            conn = None
            try:
                # Handle control channel input
                conn, addr = s.accept()
                msg = conn.recv(1024)
                if msg:
                    ctrl_channel(msg)
                conn.close()
            except s.timeout:
                pass
            if conn:
                conn.close()

    if not running:
        break
