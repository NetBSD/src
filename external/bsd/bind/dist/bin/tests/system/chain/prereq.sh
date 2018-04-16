#!/bin/sh
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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

$SHELL ../testcrypto.sh || exit 255

if test -n "$PYTHON"
then
    if $PYTHON -c "import dns" 2> /dev/null
    then
        :
    else
        echo "I:This test requires the dnspython module." >&2
        exit 1
    fi
else
    echo "I:This test requires Python and the dnspython module." >&2
    exit 1
fi

if $PERL -e 'use Net::DNS;' 2>/dev/null
then
    if $PERL -e 'use Net::DNS; die if ($Net::DNS::VERSION >= 0.69 && $Net::DNS::VERSION <= 0.74);' 2>/dev/null
    then
        :
    else
        echo "I:Net::DNS versions 0.69 to 0.74 have bugs that cause this test to fail: please update." >&2
        exit 1
    fi
else
    echo "I:This test requires the perl Net::DNS library." >&2
    exit 1
fi
if $PERL -e 'use Net::DNS::Nameserver;' 2>/dev/null
then
	:
else
    echo "I:This test requires the Net::DNS::Nameserver library." >&2
    exit 1
fi
