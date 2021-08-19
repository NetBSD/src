#!/usr/bin/python
############################################################################
# Copyright (C) 2015  Internet Systems Consortium, Inc. ("ISC")
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
# kasp2policy.py
# This translates the Keys section of a KASP XML file into a dnssec.policy
# file that can be used by dnssec-keymgr.
############################################################################

from xml.etree import cElementTree as ET
from collections import defaultdict
import re
from ply import yacc
from ply import lex
from isc import dnskey


############################################################################
# Translate KASP duration values into seconds
############################################################################
class KaspTime:
    # pylint: disable=invalid-name
    class KTLex:
        # pylint: disable=invalid-name

        tokens = ('P', 'T', 'Y', 'M', 'D', 'H', 'S', 'NUM')

        t_P = r'(?i)P'
        t_T = r'(?i)T'
        t_Y = r'(?i)Y'
        t_M = r'(?i)M'
        t_D = r'(?i)D'
        t_H = r'(?i)H'
        t_S = r'(?i)S'

        @staticmethod
        def t_NUM(t):
            r'\d+'
            t.value = int(t.value)
            return t

        @staticmethod
        def t_error(t):
            print("Illegal character '%s'" % t.value[0])
            t.lexer.skip(1)

        def __init__(self):
            self.lexer = lex.lex(object=self)

    def __init__(self):
        self.lexer = self.KTLex()
        self.tokens = self.lexer.tokens
        self.parser = yacc.yacc(debug=False, write_tables=False, module=self)

    def parse(self, text):
        self.lexer.lexer.lineno = 0
        return self.parser.parse(text)

    @staticmethod
    def p_ktime_4(p):
        "ktime : P periods T times"
        p[0] = p[2] + p[4]

    @staticmethod
    def p_ktime_3(p):
        "ktime : P T times"
        p[0] = p[3]

    @staticmethod
    def p_ktime_2(p):
        "ktime : P periods"
        p[0] = p[2]

    @staticmethod
    def p_periods_1(p):
        "periods : period"
        p[0] = p[1]

    @staticmethod
    def p_periods_2(p):
        "periods : periods period"
        p[0] = p[1] + p[2]

    @staticmethod
    def p_times_1(p):
        "times : time"
        p[0] = p[1]

    @staticmethod
    def p_times_2(p):
        "times : times time"
        p[0] = p[1] + p[2]

    @staticmethod
    def p_period(p):
        '''period : NUM Y
                  | NUM M
                  | NUM D'''
        if p[2].lower() == 'y':
            p[0] = int(p[1]) * 31536000
        elif p[2].lower() == 'm':
            p[0] = int(p[1]) * 2592000
        elif p[2].lower() == 'd':
            p[0] += int(p[1]) * 86400

    @staticmethod
    def p_time(p):
        '''time : NUM H
                | NUM M
                | NUM S'''
        if p[2].lower() == 'h':
            p[0] = int(p[1]) * 3600
        elif p[2].lower() == 'm':
            p[0] = int(p[1]) * 60
        elif p[2].lower() == 's':
            p[0] = int(p[1])

    @staticmethod
    def p_error():
        print("Syntax error")


############################################################################
# Load the contents of a KASP XML file as a python dictionary
############################################################################
class Kasp():
    # pylint: disable=invalid-name

    @staticmethod
    def _todict(t):
        d = {t.tag: {} if t.attrib else None}
        children = list(t)
        if children:
            dd = defaultdict(list)
            for dc in map(Kasp._todict, children):
                for k, v in dc.iteritems():
                    dd[k].append(v)
            k = {k: v[0] if len(v) == 1 else v for k, v in dd.items()}
            d = {t.tag: k}
        if t.attrib:
            d[t.tag].update(('@' + k, v) for k, v in t.attrib.iteritems())
        if t.text:
            text = t.text.strip()
            if children or t.attrib:
                if text:
                    d[t.tag]['#text'] = text
            else:
                d[t.tag] = text
        return d

    def __init__(self, filename):
        self._dict = Kasp._todict(ET.parse(filename).getroot())

    def __getitem__(self, key):
        return self._dict[key]

    def __len__(self):
        return len(self._dict)

    def __iter__(self):
        return self._dict.__iter__()

    def __repr__(self):
        return repr(self._dict)


############################################################################
# Load the contents of a KASP XML file as a python dictionary
############################################################################
if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print("Usage: kasp2policy <filename>")
        sys.exit(1)

    KINFO = Kasp(sys.argv[1])
    try:
        KINFO = Kasp(sys.argv[1])
    except FileNotFoundError:
        print("%s: unable to load KASP file '%s'" % (sys.argv[0], sys.argv[1]))
        sys.exit(1)

    KT = KaspTime()
    FIRST = True

    for policy in KINFO['KASP']['Policy']:
        if not policy['@name'] or not policy['Keys']:
            continue
        if not FIRST:
            print("")
        FIRST = False
        if policy['Description']:
            desc = policy['Description'].strip()
            print("# %s" % re.sub(r"\n\s*", "\n# ", desc))
        print("policy %s {" % policy['@name'])
        ksk = policy['Keys']['KSK']
        zsk = policy['Keys']['ZSK']
        kalg = ksk['Algorithm']
        zalg = zsk['Algorithm']
        algnum = kalg['#text'] or zalg['#text']
        if algnum:
            print("\talgorithm %s;" % dnskey.algstr(int(algnum)))
        if policy['Keys']['TTL']:
            print("\tkeyttl %d;" % KT.parse(policy['Keys']['TTL']))
        if kalg['@length']:
            print("\tkey-size ksk %d;" % int(kalg['@length']))
        if zalg['@length']:
            print("\tkey-size zsk %d;" % int(zalg['@length']))
        if ksk['Lifetime']:
            print("\troll-period ksk %d;" % KT.parse(ksk['Lifetime']))
        if zsk['Lifetime']:
            print("\troll-period zsk %d;" % KT.parse(zsk['Lifetime']))
        if ksk['Standby']:
            print("\tstandby ksk %d;" % int(ksk['Standby']))
        if zsk['Standby']:
            print("\tstandby zsk %d;" % int(zsk['Standby']))
        print("};")
