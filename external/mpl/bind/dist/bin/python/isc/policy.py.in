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

import re
import ply.lex as lex
import ply.yacc as yacc
from string import *
from copy import copy


############################################################################
# PolicyLex: a lexer for the policy file syntax.
############################################################################
class PolicyLex:
    reserved = ('POLICY',
                'ALGORITHM_POLICY',
                'ZONE',
                'ALGORITHM',
                'DIRECTORY',
                'KEYTTL',
                'KEY_SIZE',
                'ROLL_PERIOD',
                'PRE_PUBLISH',
                'POST_PUBLISH',
                'COVERAGE',
                'STANDBY',
                'NONE')

    tokens = reserved + ('DATESUFFIX',
                         'KEYTYPE',
                         'ALGNAME',
                         'STR',
                         'QSTRING',
                         'NUMBER',
                         'LBRACE',
                         'RBRACE',
                         'SEMI')
    reserved_map = {}

    t_ignore           = ' \t'
    t_ignore_olcomment = r'(//|\#).*'

    t_LBRACE           = r'\{'
    t_RBRACE           = r'\}'
    t_SEMI             = r';';

    def t_newline(self, t):
        r'\n+'
        t.lexer.lineno += t.value.count("\n")

    def t_comment(self, t):
        r'/\*(.|\n)*?\*/'
        t.lexer.lineno += t.value.count('\n')

    def t_DATESUFFIX(self, t):
        r'(?i)(?<=[0-9 \t])(y(?:ears|ear|ea|e)?|mo(?:nths|nth|nt|n)?|w(?:eeks|eek|ee|e)?|d(?:ays|ay|a)?|h(?:ours|our|ou|o)?|mi(?:nutes|nute|nut|nu|n)?|s(?:econds|econd|econ|eco|ec|e)?)\b'
        t.value = re.match(r'(?i)(y|mo|w|d|h|mi|s)([a-z]*)', t.value).group(1).lower()
        return t

    def t_KEYTYPE(self, t):
        r'(?i)\b(KSK|ZSK)\b'
        t.value = t.value.upper()
        return t

    def t_ALGNAME(self, t):
        r'(?i)\b(RSAMD5|DH|DSA|NSEC3DSA|ECC|RSASHA1|NSEC3RSASHA1|RSASHA256|RSASHA512|ECCGOST|ECDSAP256SHA256|ECDSAP384SHA384|ED25519|ED448)\b'
        t.value = t.value.upper()
        return t

    def t_STR(self, t):
        r'[A-Za-z._-][\w._-]*'
        t.type = self.reserved_map.get(t.value, "STR")
        return t

    def t_QSTRING(self, t):
        r'"([^"\n]|(\\"))*"'
        t.type = self.reserved_map.get(t.value, "QSTRING")
        t.value = t.value[1:-1]
        return t

    def t_NUMBER(self, t):
        r'\d+'
        t.value = int(t.value)
        return t

    def t_error(self, t):
        print("Illegal character '%s'" % t.value[0])
        t.lexer.skip(1)

    def __init__(self, **kwargs):
        if 'maketrans' in dir(str):
            trans = str.maketrans('_', '-')
        else:
            trans = maketrans('_', '-')
        for r in self.reserved:
            self.reserved_map[r.lower().translate(trans)] = r
        self.lexer = lex.lex(object=self, **kwargs)

    def test(self, text):
        self.lexer.input(text)
        while True:
            t = self.lexer.token()
            if not t:
                break
            print(t)

############################################################################
# Policy: this object holds a set of DNSSEC policy settings.
############################################################################
class Policy:
    is_zone = False
    is_alg = False
    is_constructed = False
    ksk_rollperiod = None
    zsk_rollperiod = None
    ksk_prepublish = None
    zsk_prepublish = None
    ksk_postpublish = None
    zsk_postpublish = None
    ksk_keysize = None
    zsk_keysize = None
    ksk_standby = None
    zsk_standby = None
    keyttl = None
    coverage = None
    directory = None
    valid_key_sz_per_algo = {'DSA': [512, 1024],
                             'NSEC3DSA': [512, 1024],
                             'RSAMD5': [1024, 4096],
                             'RSASHA1': [1024, 4096],
                             'NSEC3RSASHA1': [512, 4096],
                             'RSASHA256': [1024, 4096],
                             'RSASHA512': [1024, 4096],
                             'ECCGOST': None,
                             'ECDSAP256SHA256': None,
                             'ECDSAP384SHA384': None,
                             'ED25519': None,
                             'ED448': None}

    def __init__(self, name=None, algorithm=None, parent=None):
        self.name = name
        self.algorithm = algorithm
        self.parent = parent
        pass

    def __repr__(self):
        return ("%spolicy %s:\n"
                "\tinherits %s\n"
                "\tdirectory %s\n"
                "\talgorithm %s\n"
                "\tcoverage %s\n"
                "\tksk_keysize %s\n"
                "\tzsk_keysize %s\n"
                "\tksk_rollperiod %s\n"
                "\tzsk_rollperiod %s\n"
                "\tksk_prepublish %s\n"
                "\tksk_postpublish %s\n"
                "\tzsk_prepublish %s\n"
                "\tzsk_postpublish %s\n"
                "\tksk_standby %s\n"
                "\tzsk_standby %s\n"
                "\tkeyttl %s\n"
                %
                ((self.is_constructed and 'constructed ' or \
                  self.is_zone and 'zone ' or \
                  self.is_alg and 'algorithm ' or ''),
                 self.name or 'UNKNOWN',
                 self.parent and self.parent.name or 'None',
                 self.directory and ('"' + str(self.directory) + '"') or 'None',
                 self.algorithm or 'None',
                 self.coverage and str(self.coverage) or 'None',
                 self.ksk_keysize and str(self.ksk_keysize) or 'None',
                 self.zsk_keysize and str(self.zsk_keysize) or 'None',
                 self.ksk_rollperiod and str(self.ksk_rollperiod) or 'None',
                 self.zsk_rollperiod and str(self.zsk_rollperiod) or 'None',
                 self.ksk_prepublish and str(self.ksk_prepublish) or 'None',
                 self.ksk_postpublish and str(self.ksk_postpublish) or 'None',
                 self.zsk_prepublish and str(self.zsk_prepublish) or 'None',
                 self.zsk_postpublish and str(self.zsk_postpublish) or 'None',
                 self.ksk_standby and str(self.ksk_standby) or 'None',
                 self.zsk_standby and str(self.zsk_standby) or 'None',
                 self.keyttl and str(self.keyttl) or 'None'))

    def __verify_size(self, key_size, size_range):
        return (size_range[0] <= key_size <= size_range[1])

    def get_name(self):
        return self.name

    def constructed(self):
        return self.is_constructed

    def validate(self):
        """ Check if the values in the policy make sense
        :return: True/False if the policy passes validation
        """
        if self.ksk_rollperiod and \
           self.ksk_prepublish is not None and \
           self.ksk_prepublish > self.ksk_rollperiod:
            print(self.ksk_rollperiod)
            return (False,
                    ('KSK pre-publish period (%d) exceeds rollover period %d'
                     % (self.ksk_prepublish, self.ksk_rollperiod)))

        if self.ksk_rollperiod and \
           self.ksk_postpublish is not None and \
           self.ksk_postpublish > self.ksk_rollperiod:
            return (False,
                    ('KSK post-publish period (%d) exceeds rollover period %d'
                     % (self.ksk_postpublish, self.ksk_rollperiod)))

        if self.zsk_rollperiod and \
           self.zsk_prepublish is not None and \
           self.zsk_prepublish >= self.zsk_rollperiod:
            return (False,
                    ('ZSK pre-publish period (%d) exceeds rollover period %d'
                     % (self.zsk_prepublish, self.zsk_rollperiod)))

        if self.zsk_rollperiod and \
           self.zsk_postpublish is not None and \
           self.zsk_postpublish >= self.zsk_rollperiod:
            return (False,
                    ('ZSK post-publish period (%d) exceeds rollover period %d'
                     % (self.zsk_postpublish, self.zsk_rollperiod)))

        if self.ksk_rollperiod and \
           self.ksk_prepublish and self.ksk_postpublish and \
           self.ksk_prepublish + self.ksk_postpublish >= self.ksk_rollperiod:
            return (False,
                    (('KSK pre/post-publish periods (%d/%d) ' +
                      'combined exceed rollover period %d') %
                     (self.ksk_prepublish,
                      self.ksk_postpublish,
                      self.ksk_rollperiod)))

        if self.zsk_rollperiod and \
           self.zsk_prepublish and self.zsk_postpublish and \
           self.zsk_prepublish + self.zsk_postpublish >= self.zsk_rollperiod:
            return (False,
                    (('ZSK pre/post-publish periods (%d/%d) ' +
                      'combined exceed rollover period %d') %
                     (self.zsk_prepublish,
                      self.zsk_postpublish,
                      self.zsk_rollperiod)))

        if self.algorithm is not None:
            # Validate the key size
            key_sz_range = self.valid_key_sz_per_algo.get(self.algorithm)
            if key_sz_range is not None:
                # Verify KSK
                if not self.__verify_size(self.ksk_keysize, key_sz_range):
                    return False, 'KSK key size %d outside valid range %s' \
                            % (self.ksk_keysize, key_sz_range)

                # Verify ZSK
                if not self.__verify_size(self.zsk_keysize, key_sz_range):
                    return False, 'ZSK key size %d outside valid range %s' \
                            % (self.zsk_keysize, key_sz_range)

            # Specific check for DSA keys
            if self.algorithm in ['DSA', 'NSEC3DSA'] and \
               self.ksk_keysize % 64 != 0:
                return False, \
                        ('KSK key size %d not divisible by 64 ' +
                         'as required for DSA') % self.ksk_keysize

            if self.algorithm in ['DSA', 'NSEC3DSA'] and \
               self.zsk_keysize % 64 != 0:
                return False, \
                        ('ZSK key size %d not divisible by 64 ' +
                         'as required for DSA') % self.zsk_keysize

            if self.algorithm in ['ECCGOST', \
                                  'ECDSAP256SHA256', \
                                  'ECDSAP384SHA384', \
                                  'ED25519', \
                                  'ED448']:
                self.ksk_keysize = None
                self.zsk_keysize = None

        return True, ''

############################################################################
# dnssec_policy:
# This class reads a dnssec.policy file and creates a dictionary of
# DNSSEC policy rules from which a policy for a specific zone can
# be generated.
############################################################################
class PolicyException(Exception):
    pass

class dnssec_policy:
    alg_policy = {}
    named_policy = {}
    zone_policy = {}
    current = None
    filename = None
    initial = True

    def __init__(self, filename=None, **kwargs):
        self.plex = PolicyLex()
        self.tokens = self.plex.tokens
        if 'debug' not in kwargs:
            kwargs['debug'] = False
        if 'write_tables' not in kwargs:
            kwargs['write_tables'] = False
        self.parser = yacc.yacc(module=self, **kwargs)

        # set defaults
        self.setup('''policy global { algorithm rsasha256;
                                      key-size ksk 2048;
                                      key-size zsk 2048;
                                      roll-period ksk 0;
                                      roll-period zsk 1y;
                                      pre-publish ksk 1mo;
                                      pre-publish zsk 1mo;
                                      post-publish ksk 1mo;
                                      post-publish zsk 1mo;
                                      standby ksk 0;
                                      standby zsk 0;
                                      keyttl 1h;
                                      coverage 6mo; };
                      policy default { policy global; };''')

        p = Policy()
        p.algorithm = None
        p.is_alg = True
        p.ksk_keysize = 2048;
        p.zsk_keysize = 2048;

        # set default algorithm policies
        # these need a lower default key size:
        self.alg_policy['DSA'] = copy(p)
        self.alg_policy['DSA'].algorithm = "DSA"
        self.alg_policy['DSA'].name = "DSA"
        self.alg_policy['DSA'].ksk_keysize = 1024;

        self.alg_policy['NSEC3DSA'] = copy(p)
        self.alg_policy['NSEC3DSA'].algorithm = "NSEC3DSA"
        self.alg_policy['NSEC3DSA'].name = "NSEC3DSA"
        self.alg_policy['NSEC3DSA'].ksk_keysize = 1024;

        # these can use default settings
        self.alg_policy['RSAMD5'] = copy(p)
        self.alg_policy['RSAMD5'].algorithm = "RSAMD5"
        self.alg_policy['RSAMD5'].name = "RSAMD5"

        self.alg_policy['RSASHA1'] = copy(p)
        self.alg_policy['RSASHA1'].algorithm = "RSASHA1"
        self.alg_policy['RSASHA1'].name = "RSASHA1"

        self.alg_policy['NSEC3RSASHA1'] = copy(p)
        self.alg_policy['NSEC3RSASHA1'].algorithm = "NSEC3RSASHA1"
        self.alg_policy['NSEC3RSASHA1'].name = "NSEC3RSASHA1"

        self.alg_policy['RSASHA256'] = copy(p)
        self.alg_policy['RSASHA256'].algorithm = "RSASHA256"
        self.alg_policy['RSASHA256'].name = "RSASHA256"

        self.alg_policy['RSASHA512'] = copy(p)
        self.alg_policy['RSASHA512'].algorithm = "RSASHA512"
        self.alg_policy['RSASHA512'].name = "RSASHA512"

        self.alg_policy['ECCGOST'] = copy(p)
        self.alg_policy['ECCGOST'].algorithm = "ECCGOST"
        self.alg_policy['ECCGOST'].name = "ECCGOST"

        self.alg_policy['ECDSAP256SHA256'] = copy(p)
        self.alg_policy['ECDSAP256SHA256'].algorithm = "ECDSAP256SHA256"
        self.alg_policy['ECDSAP256SHA256'].name = "ECDSAP256SHA256"
        self.alg_policy['ECDSAP256SHA256'].ksk_keysize = None;
        self.alg_policy['ECDSAP256SHA256'].zsk_keysize = None;

        self.alg_policy['ECDSAP384SHA384'] = copy(p)
        self.alg_policy['ECDSAP384SHA384'].algorithm = "ECDSAP384SHA384"
        self.alg_policy['ECDSAP384SHA384'].name = "ECDSAP384SHA384"
        self.alg_policy['ECDSAP384SHA384'].ksk_keysize = None;
        self.alg_policy['ECDSAP384SHA384'].zsk_keysize = None;

        self.alg_policy['ED25519'] = copy(p)
        self.alg_policy['ED25519'].algorithm = "ED25519"
        self.alg_policy['ED25519'].name = "ED25519"
        self.alg_policy['ED25519'].ksk_keysize = None;
        self.alg_policy['ED25519'].zsk_keysize = None;

        self.alg_policy['ED448'] = copy(p)
        self.alg_policy['ED448'].algorithm = "ED448"
        self.alg_policy['ED448'].name = "ED448"
        self.alg_policy['ED448'].ksk_keysize = None;
        self.alg_policy['ED448'].zsk_keysize = None;

        if filename:
            self.load(filename)

    def load(self, filename):
        self.filename = filename
        self.initial = True
        with open(filename) as f:
            text = f.read()
            self.plex.lexer.lineno = 0
            self.parser.parse(text)

        self.filename = None

    def setup(self, text):
        self.initial = True
        self.plex.lexer.lineno = 0
        self.parser.parse(text)

    def policy(self, zone, **kwargs):
        z = zone.lower()
        p = None

        if z in self.zone_policy:
            p = self.zone_policy[z]

        if p is None:
            p = copy(self.named_policy['default'])
            p.name = zone
            p.is_constructed = True

        if p.algorithm is None:
            parent = p.parent or self.named_policy['default']
            while parent and not parent.algorithm:
                parent = parent.parent
            p.algorithm = parent and parent.algorithm or None

        if p.algorithm in self.alg_policy:
            ap = self.alg_policy[p.algorithm]
        else:
            raise PolicyException('algorithm not found')

        if p.directory is None:
            parent = p.parent or self.named_policy['default']
            while parent is not None and not parent.directory:
                parent = parent.parent
            p.directory = parent and parent.directory

        if p.coverage is None:
            parent = p.parent or self.named_policy['default']
            while parent and not parent.coverage:
                parent = parent.parent
            p.coverage = parent and parent.coverage or ap.coverage

        if p.ksk_keysize is None:
            parent = p.parent or self.named_policy['default']
            while parent.parent and not parent.ksk_keysize:
                parent = parent.parent
            p.ksk_keysize = parent and parent.ksk_keysize or ap.ksk_keysize

        if p.zsk_keysize is None:
            parent = p.parent or self.named_policy['default']
            while parent.parent and not parent.zsk_keysize:
                parent = parent.parent
            p.zsk_keysize = parent and parent.zsk_keysize or ap.zsk_keysize

        if p.ksk_rollperiod is None:
            parent = p.parent or self.named_policy['default']
            while parent.parent and not parent.ksk_rollperiod:
                parent = parent.parent
            p.ksk_rollperiod = parent and \
                parent.ksk_rollperiod or ap.ksk_rollperiod

        if p.zsk_rollperiod is None:
            parent = p.parent or self.named_policy['default']
            while parent.parent and not parent.zsk_rollperiod:
                parent = parent.parent
            p.zsk_rollperiod = parent and \
                parent.zsk_rollperiod or ap.zsk_rollperiod

        if p.ksk_prepublish is None:
            parent = p.parent or self.named_policy['default']
            while parent.parent and not parent.ksk_prepublish:
                parent = parent.parent
            p.ksk_prepublish = parent and \
                parent.ksk_prepublish or ap.ksk_prepublish

        if p.zsk_prepublish is None:
            parent = p.parent or self.named_policy['default']
            while parent.parent and not parent.zsk_prepublish:
                parent = parent.parent
            p.zsk_prepublish = parent and \
                parent.zsk_prepublish or ap.zsk_prepublish

        if p.ksk_postpublish is None:
            parent = p.parent or self.named_policy['default']
            while parent.parent and not parent.ksk_postpublish:
                parent = parent.parent
            p.ksk_postpublish = parent and \
                parent.ksk_postpublish or ap.ksk_postpublish

        if p.zsk_postpublish is None:
            parent = p.parent or self.named_policy['default']
            while parent.parent and not parent.zsk_postpublish:
                parent = parent.parent
            p.zsk_postpublish = parent and \
                parent.zsk_postpublish or ap.zsk_postpublish

        if p.keyttl is None:
            parent = p.parent or self.named_policy['default']
            while parent is not None and not parent.keyttl:
                parent = parent.parent
            p.keyttl = parent and parent.keyttl

        if 'novalidate' not in kwargs or not kwargs['novalidate']:
            (valid, msg) = p.validate()
            if not valid:
                raise PolicyException(msg)
                return None

        return p


    def p_policylist(self, p):
        '''policylist : init policy
                      | policylist policy'''
        pass

    def p_init(self, p):
        "init :"
        self.initial = False

    def p_policy(self, p):
        '''policy : alg_policy
                  | zone_policy
                  | named_policy'''
        pass

    def p_name(self, p):
        '''name : STR
                | KEYTYPE
                | DATESUFFIX'''
        p[0] = p[1]
        pass

    def p_domain(self, p):
        '''domain : STR
                  | QSTRING
                  | KEYTYPE
                  | DATESUFFIX'''
        p[0] = p[1].strip()
        if not re.match(r'^[\w.-][\w.-]*$', p[0]):
            raise PolicyException('invalid domain')
        pass

    def p_new_policy(self, p):
        "new_policy :"
        self.current = Policy()

    def p_alg_policy(self, p):
        "alg_policy : ALGORITHM_POLICY ALGNAME new_policy alg_option_group SEMI"
        self.current.name = p[2]
        self.current.is_alg = True
        self.alg_policy[p[2]] = self.current
        pass

    def p_zone_policy(self, p):
        "zone_policy : ZONE domain new_policy policy_option_group SEMI"
        self.current.name = p[2].rstrip('.')
        self.current.is_zone = True
        self.zone_policy[p[2].rstrip('.').lower()] = self.current
        pass

    def p_named_policy(self, p):
        "named_policy : POLICY name new_policy policy_option_group SEMI"
        self.current.name = p[2]
        self.named_policy[p[2].lower()] = self.current
        pass

    def p_duration_1(self, p):
        "duration : NUMBER"
        p[0] = p[1]
        pass

    def p_duration_2(self, p):
        "duration : NONE"
        p[0] = None
        pass

    def p_duration_3(self, p):
        "duration : NUMBER DATESUFFIX"
        if p[2] == "y":
            p[0] = p[1] * 31536000 # year
        elif p[2] == "mo":
            p[0] = p[1] * 2592000  # month
        elif p[2] == "w":
            p[0] = p[1] * 604800   # week
        elif p[2] == "d":
            p[0] = p[1] * 86400    # day
        elif p[2] == "h":
            p[0] = p[1] * 3600     # hour
        elif p[2] == "mi":
            p[0] = p[1] * 60       # minute
        elif p[2] == "s":
            p[0] = p[1]            # second
        else:
            raise PolicyException('invalid duration')

    def p_policy_option_group(self, p):
        "policy_option_group : LBRACE policy_option_list RBRACE"
        pass

    def p_policy_option_list(self, p):
        '''policy_option_list : policy_option SEMI
                              | policy_option_list policy_option SEMI'''
        pass

    def p_policy_option(self, p):
        '''policy_option : parent_option
                         | directory_option
                         | coverage_option
                         | rollperiod_option
                         | prepublish_option
                         | postpublish_option
                         | keysize_option
                         | algorithm_option
                         | keyttl_option
                         | standby_option'''
        pass

    def p_alg_option_group(self, p):
        "alg_option_group : LBRACE alg_option_list RBRACE"
        pass

    def p_alg_option_list(self, p):
        '''alg_option_list : alg_option SEMI
                           | alg_option_list alg_option SEMI'''
        pass

    def p_alg_option(self, p):
        '''alg_option : coverage_option
                      | rollperiod_option
                      | prepublish_option
                      | postpublish_option
                      | keyttl_option
                      | keysize_option
                      | standby_option'''
        pass

    def p_parent_option(self, p):
        "parent_option : POLICY name"
        self.current.parent = self.named_policy[p[2].lower()]

    def p_directory_option(self, p):
        "directory_option : DIRECTORY QSTRING"
        self.current.directory = p[2]

    def p_coverage_option(self, p):
        "coverage_option : COVERAGE duration"
        self.current.coverage = p[2]

    def p_rollperiod_option(self, p):
        "rollperiod_option : ROLL_PERIOD KEYTYPE duration"
        if p[2] == "KSK":
            self.current.ksk_rollperiod = p[3]
        else:
            self.current.zsk_rollperiod = p[3]

    def p_prepublish_option(self, p):
        "prepublish_option : PRE_PUBLISH KEYTYPE duration"
        if p[2] == "KSK":
            self.current.ksk_prepublish = p[3]
        else:
            self.current.zsk_prepublish = p[3]

    def p_postpublish_option(self, p):
        "postpublish_option : POST_PUBLISH KEYTYPE duration"
        if p[2] == "KSK":
            self.current.ksk_postpublish = p[3]
        else:
            self.current.zsk_postpublish = p[3]

    def p_keysize_option(self, p):
        "keysize_option : KEY_SIZE KEYTYPE NUMBER"
        if p[2] == "KSK":
            self.current.ksk_keysize = p[3]
        else:
            self.current.zsk_keysize = p[3]

    def p_standby_option(self, p):
        "standby_option : STANDBY KEYTYPE NUMBER"
        if p[2] == "KSK":
            self.current.ksk_standby = p[3]
        else:
            self.current.zsk_standby = p[3]

    def p_keyttl_option(self, p):
        "keyttl_option : KEYTTL duration"
        self.current.keyttl = p[2]

    def p_algorithm_option(self, p):
        "algorithm_option : ALGORITHM ALGNAME"
        self.current.algorithm = p[2]

    def p_error(self, p):
        if p:
            print("%s%s%d:syntax error near '%s'" %
                    (self.filename or "", ":" if self.filename else "",
                     p.lineno, p.value))
        else:
            if not self.initial:
                raise PolicyException("%s%s%d:unexpected end of input" %
                    (self.filename or "", ":" if self.filename else "",
                     p and p.lineno or 0))

if __name__ == "__main__":
    import sys
    if sys.argv[1] == "lex":
        file = open(sys.argv[2])
        text = file.read()
        file.close()
        plex = PolicyLex(debug=1)
        plex.test(text)
    elif sys.argv[1] == "parse":
        try:
            pp = dnssec_policy(sys.argv[2], write_tables=True, debug=True)
            print(pp.named_policy['default'])
            print(pp.policy("nonexistent.zone"))
        except Exception as e:
            print(e.args[0])
