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

############################################################################
#
# This tool acts as a TCP/UDP proxy and delays all incoming packets by 500
# milliseconds.
#
# We use it to check pipelining - a client sents 8 questions over a
# pipelined connection - that require asking a normal (examplea) and a
# slow-responding (exampleb) servers:
# a.examplea
# a.exampleb
# b.examplea
# b.exampleb
# c.examplea
# c.exampleb
# d.examplea
# d.exampleb
#
# If pipelining works properly the answers will be returned out of order
# with all answers from examplea returned first, and then all answers
# from exampleb.
#
############################################################################

from __future__ import print_function

import datetime
import os
import select
import signal
import socket
import sys
import time
import threading
import struct

DELAY = 0.5
THREADS = []

def log(msg):
    print(datetime.datetime.now().strftime('%d-%b-%Y %H:%M:%S.%f ') + msg)


def sigterm(*_):
    log('SIGTERM received, shutting down')
    for thread in THREADS:
        thread.close()
        thread.join()
    os.remove('ans.pid')
    sys.exit(0)

class TCPDelayer(threading.Thread):
    """ For a given TCP connection conn we open a connection to (ip, port),
        and then we delay each incoming packet by DELAY by putting it in a
        queue.
        In the pipelined test TCP should not be used, but it's here for
        completnes.
    """
    def __init__(self, conn, ip, port):
        threading.Thread.__init__(self)
        self.conn = conn
        self.cconn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.cconn.connect((ip, port))
        self.queue = []
        self.running = True

    def close(self):
        self.running = False

    def run(self):
        while self.running:
            curr_timeout = 0.5
            try:
                curr_timeout = self.queue[0][0]-time.time()
            except StopIteration:
                pass
            if curr_timeout > 0:
                if curr_timeout == 0:
                    curr_timeout = 0.5
                rfds, _, _ = select.select([self.conn, self.cconn], [], [], curr_timeout)
                if self.conn in rfds:
                    data = self.conn.recv(65535)
                    if not data:
                        return
                    self.queue.append((time.time() + DELAY, data))
                if self.cconn in rfds:
                    data = self.cconn.recv(65535)
                    if not data == 0:
                        return
                    self.conn.send(data)
            try:
                while self.queue[0][0]-time.time() < 0:
                    _, data = self.queue.pop(0)
                    self.cconn.send(data)
            except StopIteration:
                pass

class UDPDelayer(threading.Thread):
    """ Every incoming UDP packet is put in a queue for DELAY time, then
        it's sent to (ip, port). We remember the query id to send the
        response we get to a proper source, responses are not delayed.
    """
    def __init__(self, usock, ip, port):
        threading.Thread.__init__(self)
        self.sock = usock
        self.csock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.dst = (ip, port)
        self.queue = []
        self.qid_mapping = {}
        self.running = True

    def close(self):
        self.running = False

    def run(self):
        while self.running:
            curr_timeout = 0.5
            if self.queue:
                curr_timeout = self.queue[0][0]-time.time()
            if curr_timeout >= 0:
                if curr_timeout == 0:
                    curr_timeout = 0.5
                rfds, _, _ = select.select([self.sock, self.csock], [], [], curr_timeout)
                if self.sock in rfds:
                    data, addr = self.sock.recvfrom(65535)
                    if not data:
                        return
                    self.queue.append((time.time() + DELAY, data))
                    qid = struct.unpack('>H', data[:2])[0]
                    log('Received a query from %s, queryid %d' % (str(addr), qid))
                    self.qid_mapping[qid] = addr
                if self.csock in rfds:
                    data, addr = self.csock.recvfrom(65535)
                    if not data:
                        return
                    qid = struct.unpack('>H', data[:2])[0]
                    dst = self.qid_mapping.get(qid)
                    if dst is not None:
                        self.sock.sendto(data, dst)
                        log('Received a response from %s, queryid %d, sending to %s' % (str(addr), qid, str(dst)))
            while self.queue and self.queue[0][0]-time.time() < 0:
                _, data = self.queue.pop(0)
                qid = struct.unpack('>H', data[:2])[0]
                log('Sending a query to %s, queryid %d' % (str(self.dst), qid))
                self.csock.sendto(data, self.dst)

def main():
    signal.signal(signal.SIGTERM, sigterm)
    signal.signal(signal.SIGINT, sigterm)

    with open('ans.pid', 'w') as pidfile:
        print(os.getpid(), file=pidfile)

    listenip = '10.53.0.5'
    serverip = '10.53.0.2'

    try:
        port = int(os.environ['PORT'])
    except KeyError:
        port = 5300

    log('Listening on %s:%d' % (listenip, port))

    usock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    usock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    usock.bind((listenip, port))
    thread = UDPDelayer(usock, serverip, port)
    thread.start()
    THREADS.append(thread)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((listenip, port))
    sock.listen(1)
    sock.settimeout(1)

    while True:
        try:
            (clientsock, _) = sock.accept()
            log('Accepted connection from %s' % clientsock)
            thread = TCPDelayer(clientsock, serverip, port)
            thread.start()
            THREADS.append(thread)
        except socket.timeout:
            pass

if __name__ == '__main__':
    main()
