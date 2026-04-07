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

############################################################################
#
# This tool allows an arbitrary number of TCP connections to be made to the
# specified service and to keep them open until told otherwise.  It is
# controlled by writing text commands to a TCP socket (default port: 5309).
#
# Currently supported commands:
#
#   - open <COUNT> <HOST> <PORT>
#
#     Opens <COUNT> TCP connections to <HOST>:<PORT> and keeps them open.
#     <HOST> must be an IP address (IPv4 or IPv6).
#
#   - close <COUNT>
#
#     Close the oldest <COUNT> previously established connections.
#
############################################################################

from __future__ import print_function

import datetime
import errno
import os
import select
import signal
import socket
import sys
import time

# Timeout for establishing all connections requested by a single 'open' command.
OPEN_TIMEOUT = 2
VERSION_QUERY = b"\x00\x1e\xaf\xb8\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00\x07version\x04bind\x00\x00\x10\x00\x03"


def log(msg):
    print(datetime.datetime.now().strftime("%d-%b-%Y %H:%M:%S.%f ") + msg)


def open_connections(active_conns, count, host, port):
    queued = []
    errors = []

    try:
        socket.inet_aton(host)
        family = socket.AF_INET
    except socket.error:
        family = socket.AF_INET6

    log(f"Opening {count} connections...")

    for _ in range(count):
        sock = socket.socket(family, socket.SOCK_STREAM)
        sock.setblocking(0)
        err = sock.connect_ex((host, port))
        if err not in (0, errno.EINPROGRESS):
            log(f"{errno.errorcode[err]} on connect for socket {sock}")
            errors.append(sock)
        else:
            queued.append(sock)

    start = time.monotonic()
    while queued:
        now = time.monotonic()
        time_left = OPEN_TIMEOUT - (now - start)
        if time_left <= 0:
            break
        _, wsocks, _ = select.select([], queued, [], time_left)
        for sock in wsocks:
            queued.remove(sock)
            err = sock.getsockopt(socket.SOL_SOCKET, socket.SO_ERROR)
            if err:
                log(f"{errno.errorcode[err]} for socket {sock}")
                errors.append(sock)
            else:
                sock.send(VERSION_QUERY)
                active_conns.append(sock)

    if errors:
        log(f"result=FAIL: {len(errors)} connection(s) failed")
    elif queued:
        log(f"result=FAIL: Timed out, aborting {len(queued)} pending connections")
        for sock in queued:
            sock.close()
    else:
        log(f"result=OK: Successfully opened {count} connections")


def close_connections(active_conns, count):
    log(f"Closing {'all' if count == 0 else count} connections...")
    if count == 0:
        count = len(active_conns)
    for _ in range(count):
        sock = active_conns.pop(0)
        sock.close()
    log(f"result=OK: Successfully closed {count} connections")


def sigterm(*_):
    log("SIGTERM received, shutting down")
    os.remove("ans.pid")
    sys.exit(0)


def main():
    active_conns = []

    signal.signal(signal.SIGTERM, sigterm)

    with open("ans.pid", "w", encoding="utf-8") as pidfile:
        print(os.getpid(), file=pidfile)

    listenip = "10.53.0.6"
    try:
        port = int(os.environ["CONTROLPORT"])
    except KeyError:
        port = 5309

    log(f"Listening on {listenip}:{port}")

    ctlsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ctlsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    ctlsock.bind((listenip, port))
    ctlsock.listen(1)

    while True:
        clientsock, _ = ctlsock.accept()
        log(f"Accepted control connection from {clientsock}")
        cmdline = clientsock.recv(512).decode("ascii").strip()
        if cmdline:
            log(f"Received command: {cmdline}")
            cmd = cmdline.split()
            if cmd[0] == "open":
                count, host, port = cmd[1:]
                open_connections(active_conns, int(count), host, int(port))
            elif cmd[0] == "close":
                (count,) = cmd[1:]
                close_connections(active_conns, int(count))
            else:
                log("result=FAIL: Unknown command")
        clientsock.close()


if __name__ == "__main__":
    main()
