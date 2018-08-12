#!/bin/sh -e
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

zone=example.com.
zonefile=example.com.db
starttime=20150729220000
endtime=20150819220000

for i in Xexample.com.+015+03613.key Xexample.com.+015+03613.private \
	 Xexample.com.+015+35217.key Xexample.com.+015+35217.private
do
	cp $i `echo $i | sed s/X/K/`
done

$SIGNER -P -z -s $starttime -e $endtime -r $RANDFILE -o $zone $zonefile > /dev/null 2> signer.err || cat signer.err
