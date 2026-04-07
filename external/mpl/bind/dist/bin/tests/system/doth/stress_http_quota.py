#!/usr/bin/env python3

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

from functools import reduce
from resource import RLIMIT_NOFILE, getrlimit, setrlimit

import os
import random
import socket
import subprocess
import sys
import time

MULTIDIG_INSTANCES = 10
CONNECT_TRIES = 5

random.seed()

# Ensure we have enough file desriptors to work
rlimit_nofile = getrlimit(RLIMIT_NOFILE)
if rlimit_nofile[0] < 1024:
    setrlimit(RLIMIT_NOFILE, (1024, rlimit_nofile[1]))


# Introduce some random delay
def jitter():
    time.sleep((500 + random.randint(0, 250)) / 1000000.0)


# A set of simple procedures to get the test's configuration options
def get_http_port(http_secure=False):
    http_port_env = None
    if http_secure:
        http_port_env = os.getenv("HTTPSPORT")
    else:
        http_port_env = os.getenv("HTTPPORT")
    if http_port_env:
        return int(http_port_env)
    return 443


def get_http_host():
    bind_host = os.getenv("BINDHOST")
    if bind_host:
        return bind_host
    return "localhost"


def get_dig_path():
    dig_path = os.getenv("DIG")
    if dig_path:
        return dig_path
    return "dig"


# A simple class which creates the given number of TCP connections to
# the given host in order to stress the BIND's quota facility
class TCPConnector:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.connections = []

    def connect_one(self):
        tries = CONNECT_TRIES
        while tries > 0:
            try:
                sock = socket.create_connection(
                    address=(self.host, self.port), timeout=None
                )
                self.connections.append(sock)
                break
            except ConnectionResetError:
                # some jitter for BSDs
                jitter()
                continue
            except TimeoutError:
                jitter()
                continue
            finally:
                tries -= 1

    # Close an established connection (randomly)
    def disconnect_random(self):
        pos = random.randint(0, len(self.connections) - 1)
        conn = self.connections[pos]
        try:
            conn.shutdown(socket.SHUT_RDWR)
            conn.close()
        except OSError:
            conn.close()
        finally:
            self.connections.remove(conn)

    def disconnect_all(self):
        while len(self.connections) != 0:
            self.disconnect_random()


# A simple class which allows running a dig instance under control of
# the process
class SubDIG:
    def __init__(self, http_secure=None, extra_args=None):
        self.sub_process = None
        self.dig_path = get_dig_path()
        self.host = get_http_host()
        self.port = get_http_port(http_secure=http_secure)
        if http_secure:
            self.http_secure = True
        else:
            self.http_secure = False
        self.extra_args = extra_args

    # This method constructs a command string
    def get_command(self):
        command = self.dig_path + " -p " + str(self.port) + " "
        command = command + "+noadd +nosea +nostat +noquest +nocmd +time=30 "
        if self.http_secure:
            command = command + "+https "
        else:
            command = command + "+http-plain "
        command = command + "@" + self.host + " "
        if self.extra_args:
            command = command + self.extra_args
        return command

    def run(self):
        with open(os.devnull, "w", encoding="utf-8") as devnull:
            self.sub_process = subprocess.Popen(  # pylint: disable=consider-using-with
                self.get_command(), shell=True, stdout=devnull
            )

    def wait(self, timeout=None):
        res = None
        if timeout is None:
            return self.sub_process.wait()
        try:
            res = self.sub_process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            return None
        return res

    def alive(self):
        return self.sub_process.poll() is None


# A simple wrapper class which allows running multiple dig instances
# and examining their statuses in one logical operation.
class MultiDIG:
    def __init__(self, numdigs, http_secure=None, extra_args=None):
        assert int(numdigs) > 0, f"numdigs={numdigs}"
        digs = []
        for _ in range(1, int(numdigs) + 1):
            digs.append(SubDIG(http_secure=http_secure, extra_args=extra_args))
        self.digs = digs
        assert len(self.digs) == int(numdigs), f"len={len(self.digs)} numdigs={numdigs}"

    def run(self):
        for p in self.digs:
            p.run()

    def wait(self):
        return map(lambda p: (p.wait()), self.digs)

    # Wait for the all instances to terminate with expected given
    # status. Returns true or false.
    def wait_for_result(self, result):
        return reduce(
            lambda a, b: ((a == result or a is True) and b == result), self.wait()
        )

    def alive(self):
        return reduce(lambda a, b: (a and b), map(lambda p: (p.alive()), self.digs))

    def completed(self):
        total = 0
        for p in self.digs:
            if not p.alive():
                total += 1
        return total


# The test's main logic
def run_test(http_secure=True):
    query_args = "SOA ."
    # Let's try to make a successful query
    subdig = SubDIG(http_secure=http_secure, extra_args=query_args)
    subdig.run()
    assert subdig.wait() == 0, "DIG was expected to succeed"
    # Let's create a lot of TCP connections to the server stress the
    # HTTP quota
    connector = TCPConnector(get_http_host(), get_http_port(http_secure=http_secure))
    # Let's make queries until the quota kicks in
    subdig = SubDIG(http_secure=http_secure, extra_args=query_args)
    subdig.run()
    while True:
        connector.connect_one()
        subdig = SubDIG(http_secure=http_secure, extra_args=query_args)
        subdig.run()
        if subdig.wait(timeout=5) is None:
            break

    # At this point quota has kicked in.  Additionally, let's create a
    # bunch of dig processes all trying to make a query against the
    # server with exceeded quota
    multidig = MultiDIG(
        MULTIDIG_INSTANCES, http_secure=http_secure, extra_args=query_args
    )
    multidig.run()
    # Wait for the dig instance to complete. Not a single instance has
    # a chance to complete successfully because of the exceeded quota
    assert (
        subdig.wait(timeout=5) is None
    ), "The single DIG instance has stopped prematurely"
    assert subdig.alive(), "The single DIG instance is expected to be alive"
    assert multidig.alive(), (
        "The DIG instances from the set are all expected to "
        f"be alive, but {multidig.completed()} of them have completed"
    )
    # Let's close opened connections (in random order) to let all dig
    # processes to complete
    connector.disconnect_all()
    # Wait for all processes to complete successfully
    assert subdig.wait() == 0, "Single DIG instance failed"
    assert (
        multidig.wait_for_result(0) is True
    ), "One or more of DIG instances returned unexpected results"


def main():
    run_test(http_secure=True)
    run_test(http_secure=False)
    # If we have reached this point we could safely return 0
    # (success). If the test fails because of an assert, the whole
    # program will return non-zero exit code and produce the backtrace
    return 0


sys.exit(main())
