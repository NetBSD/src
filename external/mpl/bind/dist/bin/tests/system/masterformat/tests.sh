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

ismap () {
    $PERL -e 'binmode STDIN;
	     read(STDIN, $input, 8);
             ($style, $version) = unpack("NN", $input);
             exit 1 if ($style != 3 || $version > 1);' < $1
    return $?
}

israw () {
    $PERL -e 'binmode STDIN;
             read(STDIN, $input, 8);
             ($style, $version) = unpack("NN", $input);
             exit 1 if ($style != 2 || $version > 1);' < $1
    return $?
}

isfull () {
    # there should be no whitespace at the beginning of a line
    if grep '^[ 	][ 	]*' $1 > /dev/null 2>&1; then
	return 1
    else
	return 0
    fi
}

rawversion () {
    $PERL -e 'binmode STDIN;
             read(STDIN, $input, 8);
             if (length($input) < 8) { print "not raw\n"; exit 0; };
             ($style, $version) = unpack("NN", $input);
             print ($style == 2 || $style == 3 ? "$version\n" : 
		"not raw or map\n");' < $1
}

sourceserial () {
    $PERL -e 'binmode STDIN;
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

stomp () {
        $PERL -e 'open(my $file, "+<", $ARGV[0]);
                 binmode $file;
                 seek($file, $ARGV[1], 0);
                 for (my $i = 0; $i < $ARGV[2]; $i++) {
                         print $file pack('C', $ARGV[3]);
                 }
                 close($file);' $1 $2 $3 $4
}

restart () {
    sleep 1
    (cd ..; $PERL start.pl --noclean --restart --port ${PORT} masterformat ns3)
}

DIGOPTS="+tcp +noauth +noadd +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0

echo_i "checking that master files in raw format loaded"
ret=0
set -- 1 2 3
for zone in example example-explicit example-compat; do
    for server in $*; do
	for name in ns mx a aaaa cname dname txt rrsig nsec \
		    dnskey ds cdnskey cds; do
		$DIG $DIGOPTS $name.$zone. $name @10.53.0.$server
		echo
	done > dig.out.$zone.$server
    done
    digcomp dig.out.$zone.1 dig.out.$zone.2 || ret=1
    if [ $zone = "example" ]; then
        set -- 1 2
        digcomp dig.out.$zone.1 dig.out.$zone.3 || ret=1
    fi
done
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "checking raw format versions"
ret=0
israw ns1/example.db.raw || ret=1
israw ns1/example.db.raw1 || ret=1
israw ns1/example.db.compat || ret=1
ismap ns1/example.db.map || ret=1
[ "`rawversion ns1/example.db.raw`" = 1 ] || ret=1
[ "`rawversion ns1/example.db.raw1`" = 1 ] || ret=1
[ "`rawversion ns1/example.db.compat`" = 0 ] || ret=1
[ "`rawversion ns1/example.db.map`" = 1 ] || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "checking source serial numbers"
ret=0
[ "`sourceserial ns1/example.db.raw`" = "UNSET" ] || ret=1
[ "`sourceserial ns1/example.db.serial.raw`" = "3333" ] || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "waiting for transfers to complete"
for i in 0 1 2 3 4 5 6 7 8 9
do
	test -f ns2/transfer.db.raw -a -f ns2/transfer.db.txt && break
	sleep 1
done

echo_i "checking that slave was saved in raw format by default"
ret=0
israw ns2/transfer.db.raw || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "checking that slave was saved in text format when configured"
ret=0
israw ns2/transfer.db.txt && ret=1
isfull ns2/transfer.db.txt && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "checking that slave was saved in 'full' style when configured"
ret=0
isfull ns2/transfer.db.full > /dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "checking that slave formerly in text format is now raw"
for i in 0 1 2 3 4 5 6 7 8 9
do
    ret=0
    israw ns2/formerly-text.db > /dev/null 2>&1 || ret=1
    [ "`rawversion ns2/formerly-text.db`" = 1 ] || ret=1
    [ $ret -eq 0 ] && break
    sleep 1
done
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "checking that large rdatasets loaded"
for i in 0 1 2 3 4 5 6 7 8 9
do
ret=0
for a in a b c
do
	$DIG +tcp txt ${a}.large @10.53.0.2 -p ${PORT} > dig.out
	grep "status: NOERROR" dig.out > /dev/null || ret=1
done
[ $ret -eq 0 ] && break
sleep 1
done

echo_i "checking format transitions: text->raw->map->text"
ret=0
./named-compilezone -D -f text -F text -o baseline.txt example.nil ns1/example.db > /dev/null
./named-compilezone -D -f text -F raw -o raw.1 example.nil baseline.txt > /dev/null
./named-compilezone -D -f raw -F map -o map.1 example.nil raw.1 > /dev/null
./named-compilezone -D -f map -F text -o text.1 example.nil map.1 > /dev/null
cmp -s baseline.txt text.1 || ret=0
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "checking format transitions: text->map->raw->text"
ret=0
./named-compilezone -D -f text -F map -o map.2 example.nil baseline.txt > /dev/null
./named-compilezone -D -f map -F raw -o raw.2 example.nil map.2 > /dev/null
./named-compilezone -D -f raw -F text -o text.2 example.nil raw.2 > /dev/null
cmp -s baseline.txt text.2 || ret=0
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "checking map format loading with journal file rollforward"
ret=0
$NSUPDATE <<END > /dev/null || status=1
server 10.53.0.3 ${PORT}
ttl 600
update add newtext.dynamic IN TXT "added text"
update delete aaaa.dynamic
send
END
$DIG $DIGOPTS @10.53.0.3 newtext.dynamic txt > dig.out.dynamic.3.1
grep "added text" dig.out.dynamic.3.1 > /dev/null 2>&1 || ret=1
$DIG $DIGOPTS +comm @10.53.0.3 added.dynamic txt > dig.out.dynamic.3.2
grep "NXDOMAIN"  dig.out.dynamic.3.2 > /dev/null 2>&1 || ret=1
# using "rndc halt" ensures that we don't dump the zone file
$RNDCCMD 10.53.0.3 halt 2>&1 | sed 's/^/ns3 /' | cat_i
restart
for i in 0 1 2 3 4 5 6 7 8 9; do
    lret=0
    $DIG $DIGOPTS @10.53.0.3 newtext.dynamic txt > dig.out.dynamic.3.3
    grep "added text" dig.out.dynamic.3.3 > /dev/null 2>&1 || lret=1
    [ $lret -eq 0 ] && break;
done
[ $lret -eq 1 ] && ret=1
$DIG $DIGOPTS +comm @10.53.0.3 added.dynamic txt > dig.out.dynamic.3.4
grep "NXDOMAIN"  dig.out.dynamic.3.4 > /dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "checking map format file dumps correctly"
ret=0
$NSUPDATE <<END > /dev/null || status=1
server 10.53.0.3 ${PORT}
ttl 600
update add moretext.dynamic IN TXT "more text"
send
END
$DIG $DIGOPTS @10.53.0.3 moretext.dynamic txt > dig.out.dynamic.3.5
grep "more text" dig.out.dynamic.3.5 > /dev/null 2>&1 || ret=1
# using "rndc stop" will cause the zone file to flush before shutdown
$RNDCCMD 10.53.0.3 stop 2>&1 | sed 's/^/ns3 /' | cat_i
rm ns3/*.jnl
restart
for i in 0 1 2 3 4 5 6 7 8 9; do
    lret=0
    $DIG $DIGOPTS +comm @10.53.0.3 moretext.dynamic txt > dig.out.dynamic.3.6
    grep "more text" dig.out.dynamic.3.6 > /dev/null 2>&1 || lret=1
    [ $lret -eq 0 ] && break;
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

# stomp on the file header
echo_i "checking corrupt map files fail to load (bad file header)"
ret=0
./named-compilezone -D -f text -F map -o map.5 example.nil baseline.txt > /dev/null
cp map.5 badmap
stomp badmap 0 32 99
./named-compilezone -D -f map -F text -o text.5 example.nil badmap > /dev/null
[ $? = 1 ] || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`
# stomp on the file data so it hashes differently.
# these are small and subtle changes, so that the resulting file
# would appear to be a legitimate map file and would not trigger an
# assertion failure if loaded into memory, but should still fail to
# load because of a SHA1 hash mismatch.
echo_i "checking corrupt map files fail to load (bad node header)"
ret=0
cp map.5 badmap
stomp badmap 2754 2 99
./named-compilezone -D -f map -F text -o text.5 example.nil badmap > /dev/null
[ $? = 1 ] || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "checking corrupt map files fail to load (bad node data)"
ret=0
cp map.5 badmap
stomp badmap 2897 5 127
./named-compilezone -D -f map -F text -o text.5 example.nil badmap > /dev/null
[ $? = 1 ] || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "checking map format zone is scheduled for resigning (compilezone)"
ret=0
$RNDCCMD 10.53.0.1 zonestatus signed > rndc.out 2>&1 || ret=1
grep 'next resign' rndc.out > /dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "checking map format zone is scheduled for resigning (signzone)"
ret=0
$RNDCCMD 10.53.0.1 freeze signed > rndc.out 2>&1 || ret=1
cd ns1
$SIGNER -S -O map -f signed.db.map -o signed signed.db > /dev/null 2>&1
cd ..
$RNDCCMD 10.53.0.1 reload signed > rndc.out 2>&1 || ret=1
$RNDCCMD 10.53.0.1 zonestatus signed > rndc.out 2>&1 || ret=1
grep 'next resign' rndc.out > /dev/null 2>&1 || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
