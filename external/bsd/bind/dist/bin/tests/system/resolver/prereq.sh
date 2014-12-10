#!/bin/sh
#
# Copyright (C) 2004, 2007, 2012, 2014  Internet Systems Consortium, Inc. ("ISC")
# Copyright (C) 2000, 2001  Internet Software Consortium.
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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

if $PERL -e 'use Net::DNS;' 2>/dev/null
then
    if $PERL -e 'use Net::DNS; die if ($Net::DNS::VERSION eq "0.76" || $Net::DNS::VERSION eq "0.77");' 2>/dev/null
    then
	:
    else
	echo "I:Net::DNS version 0.76 and 0.77 have a bug that causes this test to fail: please update." >&2
	exit 1
    fi
else
    echo "I:This test requires the Net::DNS library." >&2
    exit 1
fi

exec $SHELL ../testcrypto.sh
