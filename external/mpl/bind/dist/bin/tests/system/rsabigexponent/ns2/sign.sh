#!/bin/sh

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

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

zone=example.
infile=example.db.in
outfile=example.db.bad

for i in Xexample.+008+51650.key Xexample.+008+51650.private \
	 Xexample.+008+52810.key Xexample.+008+52810.private
do
	cp $i `echo $i | sed s/X/K/`
done

$SIGNER -g -s 20000101000000 -e 20361231235959 -o $zone \
	$infile Kexample.+008+52810.key \
	> /dev/null 2> signer.err || true
