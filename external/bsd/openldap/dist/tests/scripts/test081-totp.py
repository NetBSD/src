# -*- coding: utf-8 -*-
# $OpenLDAP$
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 2016-2021 Ondřej Kuzník, Symas Corp.
## Copyright 2021 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.

from __future__ import print_function

import hashlib
import hmac
import os
import struct
import sys
import time

import ldap
from ldap.cidict import cidict as CIDict
from ldap.ldapobject import LDAPObject

if len(sys.argv) > 1 and sys.argv[1] == "--check":
    raise SystemExit(0)


def get_digits(h, digits):
    offset = h[19] & 15
    number = struct.unpack(">I", h[offset:offset+4])[0] & 0x7fffffff
    number %= (10 ** digits)
    return ("%0*d" % (digits, number)).encode()


def get_hotp_token(secret, interval_no):
    msg = struct.pack(">Q", interval_no)
    h = hmac.new(secret, msg, hashlib.sha1).digest()
    return get_digits(bytearray(h), 6)


def get_interval(period=30):
    return int(time.time() // period)


def get_token_for(connection, dn, typ="totp"):
    result = connection.search_s(dn, ldap.SCOPE_BASE)
    dn, attrs = result[0]
    attrs = CIDict(attrs)

    tokendn = attrs['oath'+typ+'token'][0].decode()

    result = connection.search_s(tokendn, ldap.SCOPE_BASE)
    dn, attrs = result[0]
    attrs = CIDict(attrs)

    return dn, attrs


def main():
    uri = os.environ["URI1"]

    managerdn = os.environ['MANAGERDN']
    passwd = os.environ['PASSWD']

    babsdn = os.environ['BABSDN']
    babspw = b"bjensen"

    bjornsdn = os.environ['BJORNSDN']
    bjornspw = b"bjorn"

    connection = LDAPObject(uri)

    start = time.time()
    connection.bind_s(managerdn, passwd)
    end = time.time()

    if end - start > 1:
        print("It takes more than a second to connect and bind, "
              "skipping potentially unstable test", file=sys.stderr)
        raise SystemExit(0)

    dn, token_entry = get_token_for(connection, babsdn)

    paramsdn = token_entry['oathTOTPParams'][0].decode()
    result = connection.search_s(paramsdn, ldap.SCOPE_BASE)
    _, attrs = result[0]
    params = CIDict(attrs)

    secret = token_entry['oathSecret'][0]
    period = int(params['oathTOTPTimeStepPeriod'][0].decode())

    bind_conn = LDAPObject(uri)

    interval_no = get_interval(period)
    token = get_hotp_token(secret, interval_no-3)

    print("Testing old tokens are not useable")
    bind_conn.bind_s(babsdn, babspw+token)
    try:
        bind_conn.bind_s(babsdn, babspw+token)
    except ldap.INVALID_CREDENTIALS:
        pass
    else:
        raise SystemExit("Bind with an old token should have failed")

    interval_no = get_interval(period)
    token = get_hotp_token(secret, interval_no)

    print("Testing token can only be used once")
    bind_conn.bind_s(babsdn, babspw+token)
    try:
        bind_conn.bind_s(babsdn, babspw+token)
    except ldap.INVALID_CREDENTIALS:
        pass
    else:
        raise SystemExit("Bind with a reused token should have failed")

    token = get_hotp_token(secret, interval_no+1)
    try:
        bind_conn.bind_s(babsdn, babspw+token)
    except ldap.INVALID_CREDENTIALS:
        raise SystemExit("Bind should have succeeded")

    dn, token_entry = get_token_for(connection, babsdn)
    last = int(token_entry['oathTOTPLastTimeStep'][0].decode())
    if last != interval_no+1:
        SystemExit("Unexpected counter value %d (expected %d)" %
                   (last, interval_no+1))

    print("Resetting counter and testing secret sharing between accounts")
    connection.modify_s(dn, [(ldap.MOD_REPLACE, 'oathTOTPLastTimeStep', [])])

    interval_no = get_interval(period)
    token = get_hotp_token(secret, interval_no)

    try:
        bind_conn.bind_s(bjornsdn, bjornspw+token)
    except ldap.INVALID_CREDENTIALS:
        raise SystemExit("Bind should have succeeded")

    try:
        bind_conn.bind_s(babsdn, babspw+token)
    except ldap.INVALID_CREDENTIALS:
        pass
    else:
        raise SystemExit("Bind with a reused token should have failed")

    print("Testing token is retired even with a wrong password")
    connection.modify_s(dn, [(ldap.MOD_REPLACE, 'oathTOTPLastTimeStep', [])])

    interval_no = get_interval(period)
    token = get_hotp_token(secret, interval_no)

    try:
        bind_conn.bind_s(babsdn, b"not the password"+token)
    except ldap.INVALID_CREDENTIALS:
        pass
    else:
        raise SystemExit("Bind with an incorrect password should have failed")

    try:
        bind_conn.bind_s(babsdn, babspw+token)
    except ldap.INVALID_CREDENTIALS:
        pass
    else:
        raise SystemExit("Bind with a reused token should have failed")

    token = get_hotp_token(secret, interval_no+1)
    try:
        bind_conn.bind_s(babsdn, babspw+token)
    except ldap.INVALID_CREDENTIALS:
        raise SystemExit("Bind should have succeeded")


if __name__ == "__main__":
    sys.exit(main())
