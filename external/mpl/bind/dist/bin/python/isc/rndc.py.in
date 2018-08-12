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
# rndc.py
# This module implements the RNDC control protocol.
############################################################################

from collections import OrderedDict
import time
import struct
import hashlib
import hmac
import base64
import random
import socket


class rndc(object):
    """RNDC protocol client library"""
    __algos = {'md5':    157,
               'sha1':   161,
               'sha224': 162,
               'sha256': 163,
               'sha384': 164,
               'sha512': 165}

    def __init__(self, host, algo, secret):
        """Creates a persistent connection to RNDC and logs in
        host - (ip, port) tuple
        algo - HMAC algorithm: one of md5, sha1, sha224, sha256, sha384, sha512
               (with optional prefix 'hmac-')
        secret - HMAC secret, base64 encoded"""
        self.host = host
        algo = algo.lower()
        if algo.startswith('hmac-'):
            algo = algo[5:]
        self.algo = algo
        self.hlalgo = getattr(hashlib, algo)
        self.secret = base64.b64decode(secret)
        self.ser = random.randint(0, 1 << 24)
        self.nonce = None
        self.__connect_login()

    def call(self, cmd):
        """Call a RNDC command, all parsing is done on the server side
        cmd - a complete string with a command (eg 'reload zone example.com')
        """
        return dict(self.__command(type=cmd)['_data'])

    def __serialize_dict(self, data, ignore_auth=False):
        rv = bytearray()
        for k, v in data.items():
            if ignore_auth and k == '_auth':
                continue
            rv += struct.pack('B', len(k)) + k.encode('ascii')
            if type(v) == str:
                rv += struct.pack('>BI', 1, len(v)) + v.encode('ascii')
            elif type(v) == bytes:
                rv += struct.pack('>BI', 1, len(v)) + v
            elif type(v) == bytearray:
                rv += struct.pack('>BI', 1, len(v)) + v
            elif type(v) == OrderedDict:
                sd = self.__serialize_dict(v)
                rv += struct.pack('>BI', 2, len(sd)) + sd
            else:
                raise NotImplementedError('Cannot serialize element of type %s'
                                          % type(v))
        return rv

    def __prep_message(self, *args, **kwargs):
        self.ser += 1
        now = int(time.time())
        data = OrderedDict(*args, **kwargs)

        d = OrderedDict()
        d['_auth'] = OrderedDict()
        d['_ctrl'] = OrderedDict()
        d['_ctrl']['_ser'] = str(self.ser)
        d['_ctrl']['_tim'] = str(now)
        d['_ctrl']['_exp'] = str(now+60)
        if self.nonce is not None:
            d['_ctrl']['_nonce'] = self.nonce
        d['_data'] = data

        msg = self.__serialize_dict(d, ignore_auth=True)
        hash = hmac.new(self.secret, msg, self.hlalgo).digest()
        bhash = base64.b64encode(hash)
        if self.algo == 'md5':
            d['_auth']['hmd5'] = struct.pack('22s', bhash)
        else:
            d['_auth']['hsha'] = bytearray(struct.pack('B88s',
                                             self.__algos[self.algo], bhash))
        msg = self.__serialize_dict(d)
        msg = struct.pack('>II', len(msg) + 4, 1) + msg
        return msg

    def __verify_msg(self, msg):
        if self.nonce is not None and msg['_ctrl']['_nonce'] != self.nonce:
            return False
        if self.algo == 'md5':
            bhash = msg['_auth']['hmd5']
        else:
            bhash = msg['_auth']['hsha'][1:]
        if type(bhash) == bytes:
            bhash = bhash.decode('ascii')
        bhash += '=' * (4 - (len(bhash) % 4))
        remote_hash = base64.b64decode(bhash)
        my_msg = self.__serialize_dict(msg, ignore_auth=True)
        my_hash = hmac.new(self.secret, my_msg, self.hlalgo).digest()
        return (my_hash == remote_hash)

    def __command(self, *args, **kwargs):
        msg = self.__prep_message(*args, **kwargs)
        sent = self.socket.send(msg)
        if sent != len(msg):
            raise IOError("Cannot send the message")

        header = self.socket.recv(8)
        if len(header) != 8:
            # What should we throw here? Bad auth can cause this...
            raise IOError("Can't read response header")

        length, version = struct.unpack('>II', header)
        if version != 1:
            raise NotImplementedError('Wrong message version %d' % version)

        # it includes the header
        length -= 4
        data = self.socket.recv(length, socket.MSG_WAITALL)
        if len(data) != length:
            raise IOError("Can't read response data")

        if type(data) == str:
            data = bytearray(data)
        msg = self.__parse_message(data)
        if not self.__verify_msg(msg):
            raise IOError("Authentication failure")

        return msg

    def __connect_login(self):
        self.socket = socket.create_connection(self.host)
        self.nonce = None
        msg = self.__command(type='null')
        self.nonce = msg['_ctrl']['_nonce']

    def __parse_element(self, input):
        pos = 0
        labellen = input[pos]
        pos += 1
        label = input[pos:pos+labellen].decode('ascii')
        pos += labellen
        type = input[pos]
        pos += 1
        datalen = struct.unpack('>I', input[pos:pos+4])[0]
        pos += 4
        data = input[pos:pos+datalen]
        pos += datalen
        rest = input[pos:]

        if type == 1:         # raw binary value
            return label, data, rest
        elif type == 2:       # dictionary
            d = OrderedDict()
            while len(data) > 0:
                ilabel, value, data = self.__parse_element(data)
                d[ilabel] = value
            return label, d, rest
        # TODO type 3 - list
        else:
            raise NotImplementedError('Unknown element type %d' % type)

    def __parse_message(self, input):
        rv = OrderedDict()
        hdata = None
        while len(input) > 0:
            label, value, input = self.__parse_element(input)
            rv[label] = value
        return rv
