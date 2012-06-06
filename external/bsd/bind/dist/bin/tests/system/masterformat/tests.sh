#!/bin/sh
#
# Copyright (C) 2005, 2007, 2011, 2012  Internet Systems Consortium, Inc. ("ISC")
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

# Id

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

israw () {
    perl -e 'binmode STDIN;
             read(STDIN, $input, 8);
             ($style, $version) = unpack("NN", $input);
             exit 1 if ($style != 2 || $version > 1);' < $1
    return $?
}

rawversion () {
    perl -e 'binmode STDIN;
             read(STDIN, $input, 8);
             if (length($input) < 8) { print "not raw\n"; exit 0; };
             ($style, $version) = unpack("NN", $input);
             print ($style == 2 ? "$version\n" : "not raw\n");' < $1
}

sourceserial () {
    perl -e 'binmode STDIN;
             read(STDIN, $input, 20);
             if (length($input) < 20) { print "UNSET\n"; exit; };
             ($format, $version, $dumptime, $flags, $sourceserial) = 
                     unpack("NNNNN", $input);
             if ($format != 2 || $version <  1) { print "UNSET\n"; exit; };
             if ($flags & 02) {
                     print $sourceserial . "\n";
             } else {
                     print "UNSET\n";
             }' < $1
}

DIGOPTS="+tcp +noauth +noadd +nosea +nostat +noquest +nocomm +nocmd"

status=0

echo "I:checking that master files in raw format loaded"
ret=0
for zone in example example-explicit example-compat; do
    for server in 1 2; do
	for name in ns mx a aaaa cname dname txt rrsig nsec dnskey ds; do
		$DIG $DIGOPTS $name.$zone. $name @10.53.0.$server -p 5300
		echo
	done > dig.out.$zone.$server
    done
    $PERL ../digcomp.pl dig.out.$zone.1 dig.out.$zone.2 || ret=1
done
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:checking raw format versions"
ret=0
israw ns1/example.db.raw || ret=1
israw ns1/example.db.raw1 || ret=1
israw ns1/example.db.compat || ret=1
[ "`rawversion ns1/example.db.raw`" = 1 ] || ret=1
[ "`rawversion ns1/example.db.raw1`" = 1 ] || ret=1
[ "`rawversion ns1/example.db.compat`" = 0 ] || ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:checking source serial numbers"
ret=0
[ "`sourceserial ns1/example.db.raw`" = "UNSET" ] || ret=1
[ "`sourceserial ns1/example.db.serial.raw`" = "3333" ] || ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:waiting for transfers to complete"
for i in 0 1 2 3 4 5 6 7 8 9
do
	test -f ns2/transfer.db.raw -a -f ns2/transfer.db.txt && break
	sleep 1
done

echo "I:checking that slave was saved in raw format by default"
ret=0
israw ns2/transfer.db.raw || ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:checking that slave was saved in text format when configured"
ret=0
israw ns2/transfer.db.txt && ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:checking that slave formerly in text format is now raw"
for i in 0 1 2 3 4 5 6 7 8 9
do
    ret=0
    israw ns2/formerly-text.db > /dev/null 2>&1 || ret=1
    [ "`rawversion ns2/formerly-text.db`" = 1 ] || ret=1
    [ $ret -eq 0 ] && break
    sleep 1
done
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:checking that large rdatasets loaded"
for i in 0 1 2 3 4 5 6 7 8 9
do
ret=0
for a in a b c
do
	$DIG +tcp txt ${a}.large @10.53.0.2 -p 5300 > dig.out
	grep "status: NOERROR" dig.out > /dev/null || ret=1
done
[ $ret -eq 0 ] && break
sleep 1
done
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:exit status: $status"
exit $status
