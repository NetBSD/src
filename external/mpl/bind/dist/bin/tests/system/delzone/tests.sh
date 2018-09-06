#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

DIGOPTS="+tcp +nosea +nostat +nocmd +norec +noques +noauth +noadd +nostats +dnssec -p 5300"
status=0
n=0

echo "I:checking normally loaded zone ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

if [ -x "$PYTHON" ]; then 
echo "I:adding and deleting 20000 new zones ($n)"
ret=0
    time (
        echo "I:adding"
        $PYTHON << EOF
import sys
sys.path.insert(0, '../../../../bin/python')
from isc import rndc
r = rndc(('10.53.0.2', 9953), 'hmac-sha256', '1234abcd8765')
for i in range(20000):
    res = r.call('addzone z%d.example { type master; file "added.db"; };' % i)
    if 'text' in res:
        print ('I:n2:' + res['text'])
EOF
    )
    time (
        echo "I:deleting"
        $PYTHON << EOF
import sys
sys.path.insert(0, '../../../../bin/python')
from isc import rndc
r = rndc(('10.53.0.2', 9953), 'hmac-sha256', '1234abcd8765')
for i in range(20000):
    res = r.call('delzone z%d.example' % i)
    if 'text' in res:
        print ('I:n2:' + res['text'])
EOF
    )
    n=`expr $n + 1`
    if [ $ret != 0 ]; then echo "I:failed"; fi
    status=`expr $status + $ret`
fi

echo "I:exit status: $status"
exit $status
