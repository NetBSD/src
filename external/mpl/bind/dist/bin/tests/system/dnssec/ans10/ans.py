############################################################################
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.
############################################################################

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

############################################################################
# Respond to a DNS query.
# SOA gets a unsigned response.
# NS gets a unsigned response.
# DNSKEY get a unsigned NODATA response.
# A gets a signed response.
# All other types get a unsigned NODATA response.
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
        now = datetime.today()
        expire = now + timedelta(days=30)
        inception = now - timedelta(days=1)
        rrsig = "A 13 2 60 " + expire.strftime("%Y%m%d%H%M%S") + " " + \
            inception.strftime("%Y%m%d%H%M%S") + " 12345 " + qname + \
            " gB+eISXAhSPZU2i/II0W9ZUhC2SCIrb94mlNvP5092WAeXxqN/vG43/1nmDl" + \
	    "y2Qs7y5VCjSMOGn85bnaMoAc7w=="
        r.answer.append(dns.rrset.from_text(qname, 1, IN, A, "10.53.0.10"))
        r.answer.append(dns.rrset.from_text(qname, 1, IN, RRSIG, rrsig))
    elif rrtype == NS:
        r.answer.append(dns.rrset.from_text(qname, 1, IN, NS, "."))
    elif rrtype == SOA:
        r.answer.append(dns.rrset.from_text(qname, 1, IN, SOA, ". . 0 0 0 0 0"))
    else:
        r.authority.append(dns.rrset.from_text(qname, 1, IN, SOA, ". . 0 0 0 0 0"))
    r.flags |= dns.flags.AA
    return r

def sigterm(signum, frame):
    print ("Shutting down now...")
    os.remove('ans.pid')
    running = False
    sys.exit(0)

############################################################################
# Main
#
# Set up responder and control channel, open the pid file, and start
# the main loop, listening for queries on the query channel or commands
# on the control channel and acting on them.
############################################################################
ip4 = "10.53.0.10"
ip6 = "fd92:7065:b8e:ffff::10"

try: port=int(os.environ['PORT'])
except: port=5300

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

f = open('ans.pid', 'w')
pid = os.getpid()
print (pid, file=f)
f.close()

running = True

print ("Listening on %s port %d" % (ip4, port))
if havev6:
    print ("Listening on %s port %d" % (ip6, port))
print ("Ctrl-c to quit")

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
            print ("Query received on %s" %
                    (ip4 if s == query4_socket else ip6), end=" ")
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
