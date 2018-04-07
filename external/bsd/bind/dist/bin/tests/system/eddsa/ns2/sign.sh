#!/bin/sh -e
#
# Copyright (C) 2017  Internet Systems Consortium, Inc. ("ISC")
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
