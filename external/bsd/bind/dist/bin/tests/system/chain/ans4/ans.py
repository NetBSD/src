############################################################################
# Copyright (C) 2017  Internet Systems Consortium, Inc. ("ISC")
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

############################################################################
# ans.py: See README.anspy for details.
############################################################################

from __future__ import print_function
import os
import sys
import signal
import socket
import select
from datetime import datetime, timedelta
import functools

import dns, dns.message, dns.query
from dns.rdatatype import *
from dns.rdataclass import *
from dns.rcode import *
from dns.name import *

############################################################################
# set up the RRs to be returned in the next answer
#
# the message contains up to two pipe-separated ('|') fields.
#
# the first field of the message is a comma-separated list
# of actions indicating what to put into the answer set
# (e.g., a dname, a cname, another cname, etc)
#
# supported actions:
# - cname (cname from the current name to a new one in the same domain)
# - dname (dname to a new domain, plus a synthesized cname)
# - xname ("external" cname, to a new name in a new domain)
#
# example: xname, dname, cname represents a CNAME to an external
# domain which is then answered by a DNAME and synthesized
# CNAME pointing to yet another domain, which is then answered
# by a CNAME within the same domain, and finally an answer
# to the query. each RR in the answer set has a corresponding
# RRSIG. these signatures are not valid, but will exercise the
# response parser.
#
# the second field is a comma-separated list of which RRs in the
# answer set to include in the answer, in which order. if prepended
# with 's', the number indicates which signature to include.
#
# examples: for the answer set "cname, cname, cname", an rr set
# '1, s1, 2, s2, 3, s3, 4, s4' indicates that all four RRs should
# be included in the answer, with siagntures, in the origninal
# order, while 4, s4, 3, s3, 2, s2, 1, s1' indicates the order
# should be reversed, 's3, s3, s3, s3' indicates that the third
# RRSIG should be repeated four times and everything else should
# be omitted, and so on.
#
# if there is no second field (i.e., no pipe symbol appears in
# the line) , the default is to send all answers and signatures.
# if a pipe symbol exists but the second field is empty, then
# nothing is sent at all.
############################################################################
actions = []
rrs = []
def ctl_channel(msg):
    global actions, rrs

    msg = msg.splitlines().pop(0)
    print ('received control message: %s' % msg)

    msg = msg.split(b'|')
    if len(msg) == 0:
        return

    actions = [x.strip() for x in msg[0].split(b',')]
    n = functools.reduce(lambda n, act: (n + (2 if act == b'dname' else 1)), [0] + actions)

    if len(msg) == 1:
        rrs = []
        for i in range(n):
            for b in [False, True]:
                rrs.append((i, b))
        return

    rlist = [x.strip() for x in msg[1].split(b',')]
    rrs = []
    for item in rlist:
        if item[0] == b's'[0]:
            i = int(item[1:].strip()) - 1
            if i > n:
                print ('invalid index %d' + (i + 1))
                continue
            rrs.append((int(item[1:]) - 1, True))
        else:
            i = int(item) - 1
            if i > n:
                print ('invalid index %d' % (i + 1))
                continue
            rrs.append((i, False))

############################################################################
# Respond to a DNS query.
############################################################################
def create_response(msg):
    m = dns.message.from_wire(msg)
    qname = m.question[0].name.to_text()
    labels = qname.lower().split('.')
    wantsigs = True if m.ednsflags & dns.flags.DO else False

    # get qtype
    rrtype = m.question[0].rdtype
    typename = dns.rdatatype.to_text(rrtype)

    # for 'www.example.com.'...
    # - name is 'www'
    # - domain is 'example.com.'
    # - sld is 'example'
    # - tld is 'com.'
    name = labels.pop(0)
    domain = '.'.join(labels)
    sld = labels.pop(0)
    tld = '.'.join(labels)

    print ('query: ' + qname + '/' + typename)
    print ('domain: ' + domain)

    # default answers, depending on QTYPE.
    # currently only A, AAAA, TXT and NS are supported.
    ttl = 86400
    additionalA = '10.53.0.4'
    additionalAAAA = 'fd92:7065:b8e:ffff::4'
    if typename == 'A':
        final = '10.53.0.4'
    elif typename == 'AAAA':
        final = 'fd92:7065:b8e:ffff::4'
    elif typename == 'TXT':
        final = 'Some\ text\ here'
    elif typename == 'NS':
        domain = qname
        final = ('ns1.%s' % domain)
    else:
        final = None

    # RRSIG rdata - won't validate but will exercise response parsing
    t = datetime.now()
    delta = timedelta(30)
    t1 = t - delta
    t2 = t + delta
    inception=t1.strftime('%Y%m%d000000')
    expiry=t2.strftime('%Y%m%d000000')
    sigdata='OCXH2De0yE4NMTl9UykvOsJ4IBGs/ZIpff2rpaVJrVG7jQfmj50otBAp A0Zo7dpBU4ofv0N/F2Ar6LznCncIojkWptEJIAKA5tHegf/jY39arEpO cevbGp6DKxFhlkLXNcw7k9o7DSw14OaRmgAjXdTFbrl4AiAa0zAttFko Tso='

    # construct answer set.
    answers = []
    sigs = []
    curdom = domain
    curname = name
    i = 0

    for action in actions:
        if name != 'test':
            continue
        if action == b'xname':
            owner = curname + '.' + curdom
            newname = 'cname%d' % i
            i += 1
            newdom = 'domain%d.%s' % (i, tld)
            i += 1
            target = newname + '.' + newdom
            print ('add external CNAME %s to %s' % (owner, target))
            answers.append(dns.rrset.from_text(owner, ttl, IN, CNAME, target))
            rrsig = 'CNAME 5 3 %d %s %s 12345 %s %s' % \
               (ttl, expiry, inception, domain, sigdata)
            print ('add external RRISG(CNAME) %s to %s' % (owner, target))
            sigs.append(dns.rrset.from_text(owner, ttl, IN, RRSIG, rrsig))
            curname = newname
            curdom = newdom
            continue

        if action == b'cname':
            owner = curname + '.' + curdom
            newname = 'cname%d' % i
            target = newname + '.' + curdom
            i += 1
            print ('add CNAME %s to %s' % (owner, target))
            answers.append(dns.rrset.from_text(owner, ttl, IN, CNAME, target))
            rrsig = 'CNAME 5 3 %d %s %s 12345 %s %s' % \
                   (ttl, expiry, inception, domain, sigdata)
            print ('add RRSIG(CNAME) %s to %s' % (owner, target))
            sigs.append(dns.rrset.from_text(owner, ttl, IN, RRSIG, rrsig))
            curname = newname
            continue

        if action == b'dname':
            owner = curdom
            newdom = 'domain%d.%s' % (i, tld)
            i += 1
            print ('add DNAME %s to %s' % (owner, newdom))
            answers.append(dns.rrset.from_text(owner, ttl, IN, DNAME, newdom))
            rrsig = 'DNAME 5 3 %d %s %s 12345 %s %s' % \
                   (ttl, expiry, inception, domain, sigdata)
            print ('add RRSIG(DNAME) %s to %s' % (owner, newdom))
            sigs.append(dns.rrset.from_text(owner, ttl, IN, RRSIG, rrsig))
            owner = curname + '.' + curdom
            target = curname + '.' + newdom
            print ('add synthesized CNAME %s to %s' % (owner, target))
            answers.append(dns.rrset.from_text(owner, ttl, IN, CNAME, target))
            rrsig = 'CNAME 5 3 %d %s %s 12345 %s %s' % \
                   (ttl, expiry, inception, domain, sigdata)
            print ('add synthesized RRSIG(CNAME) %s to %s' % (owner, target))
            sigs.append(dns.rrset.from_text(owner, ttl, IN, RRSIG, rrsig))
            curdom = newdom
            continue

    # now add the final answer
    owner = curname + '.' + curdom
    answers.append(dns.rrset.from_text(owner, ttl, IN, rrtype, final))
    rrsig = '%s 5 3 %d %s %s 12345 %s %s' % \
               (typename, ttl, expiry, inception, domain, sigdata)
    sigs.append(dns.rrset.from_text(owner, ttl, IN, RRSIG, rrsig))

    # prepare the response and convert to wire format
    r = dns.message.make_response(m)

    if name != 'test':
        r.answer.append(answers[-1])
        if wantsigs:
            r.answer.append(sigs[-1])
    else:
        for (i, sig) in rrs:
            if sig and not wantsigs:
                continue
            elif sig:
                r.answer.append(sigs[i])
            else:
                r.answer.append(answers[i])

    if typename != 'NS':
        r.authority.append(dns.rrset.from_text(domain, ttl, IN, "NS",
                                               ("ns1.%s" % domain)))
    r.additional.append(dns.rrset.from_text(('ns1.%s' % domain), 86400,
                                             IN, A, additionalA))
    r.additional.append(dns.rrset.from_text(('ns1.%s' % domain), 86400,
                                             IN, AAAA, additionalAAAA))

    r.flags |= dns.flags.AA
    r.use_edns()
    return r.to_wire()

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
ip4 = "10.53.0.4"
ip6 = "fd92:7065:b8e:ffff::4"
sock = 5300

query4_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
query4_socket.bind((ip4, sock))

havev6 = True
try:
    query6_socket = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    try:
        query6_socket.bind((ip6, sock))
    except:
        query6_socket.close()
        havev6 = False
except:
    havev6 = False

ctrl_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
ctrl_socket.bind((ip4, sock + 1))
ctrl_socket.listen(5)

signal.signal(signal.SIGTERM, sigterm)

f = open('ans.pid', 'w')
pid = os.getpid()
print (pid, file=f)
f.close()

running = True

print ("Listening on %s port %d" % (ip4, sock))
if havev6:
    print ("Listening on %s port %d" % (ip6, sock))
print ("Control channel on %s port %d" % (ip4, sock + 1))
print ("Ctrl-c to quit")

if havev6:
    input = [query4_socket, query6_socket, ctrl_socket]
else:
    input = [query4_socket, ctrl_socket]

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
        if s == ctrl_socket:
            # Handle control channel input
            conn, addr = s.accept()
            print ("Control channel connected")
            while True:
                msg = conn.recv(65535)
                if not msg:
                    break
                ctl_channel(msg)
            conn.close()
        if s == query4_socket or s == query6_socket:
            print ("Query received on %s" %
                    (ip4 if s == query4_socket else ip6))
            # Handle incoming queries
            msg = s.recvfrom(65535)
            rsp = create_response(msg[0])
            if rsp:
                s.sendto(rsp, msg[1])
    if not running:
        break
