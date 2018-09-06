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

status=0

(
$SHELL -c "while true
           do $RNDC -c ../common/rndc.conf -s 10.53.0.3 -p 9953 reload 2>&1 |
	       sed 's/^/I:ns3 /';
	   sleep 1
       done" & echo $! >reload.pid
) &

for i in 0 1 2 3 4
do
       $PERL update.pl -s 10.53.0.2 -p 5300 zone00000$i.example. &
done

echo "I:waiting for background processes to finish"
wait

echo "I:killing reload loop"
kill `cat reload.pid`

echo "I:exit status: $status"
[ $status -eq 0 ] || exit 1
