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

# shellcheck source=conf.sh
SYSTEMTESTTOP=..
. "$SYSTEMTESTTOP/conf.sh"

status=0
n=1

ismap () {
    # shellcheck disable=SC2016
    $PERL -e 'binmode STDIN;
	     read(STDIN, $input, 8);
             ($style, $version) = unpack("NN", $input);
             exit 1 if ($style != 3 || $version > 1);' < "$1"
    return $?
}

israw () {
    # shellcheck disable=SC2016
    $PERL -e 'binmode STDIN;
             read(STDIN, $input, 8);
             ($style, $version) = unpack("NN", $input);
             exit 1 if ($style != 2 || $version > 1);' < "$1"
    return $?
}

isfull () {
    # there should be no whitespace at the beginning of a line
    if grep '^[ 	][ 	]*' "$1" > /dev/null 2>&1; then
	return 1
    else
	return 0
    fi
}

rawversion () {
    # shellcheck disable=SC2016
    $PERL -e 'binmode STDIN;
             read(STDIN, $input, 8);
             if (length($input) < 8) { print "not raw\n"; exit 0; };
             ($style, $version) = unpack("NN", $input);
             print ($style == 2 || $style == 3 ? "$version\n" :
		"not raw or map\n");' < "$1"
}

sourceserial () {
    # shellcheck disable=SC2016
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
             }' < "$1"
}

stomp () {
    # shellcheck disable=SC2016
    $PERL -e 'open(my $file, "+<", $ARGV[0]);
              binmode $file;
              seek($file, $ARGV[1], 0);
              for (my $i = 0; $i < $ARGV[2]; $i++) {
                      print $file pack("C", $ARGV[3]);
              }
              close($file);' "$@"
}

restart () {
    sleep 1
    start_server --noclean --restart --port "${PORT}" ns3
}

dig_with_opts() {
    "$DIG" +tcp +noauth +noadd +nosea +nostat +noquest +nocomm +nocmd -p "${PORT}" "$@"
}

rndccmd() {
    "$RNDC" -c "$SYSTEMTESTTOP/common/rndc.conf" -p "${CONTROLPORT}" -s "$@"
}

status=0

echo_i "checking that files in raw format loaded ($n)"
ret=0
set -- 1 2 3
for zone in example example-explicit example-compat; do
    for server in "$@"; do
	for qname in ns mx a aaaa cname dname txt rrsig nsec \
                dnskey ds cdnskey cds; do
            qtype="$qname"
	    dig_with_opts  @10.53.0.${server} -q ${qname}.${zone}. -t ${qtype}
	    echo
	done > dig.out.${zone}.${server}.test${n}
        for qname in private-dnskey private-cdnskey; do
            qtype=$(expr "$qname" : '.*-\(.*\)')
	    dig_with_opts  @10.53.0.${server} -q ${qname}.${zone}. -t ${qtype}
	done >> dig.out.${zone}.${server}.test${n}
    done
    digcomp dig.out.${zone}.1.test${n} dig.out.${zone}.2.test${n} || ret=1
    if [ "$zone" = "example" ]; then
        set -- 1 2
        digcomp dig.out.${zone}.1.test${n} dig.out.${zone}.3.test${n} || ret=1
    fi
done
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking raw format versions ($n)"
ret=0
israw ns1/example.db.raw || ret=1
israw ns1/example.db.raw1 || ret=1
israw ns1/example.db.compat || ret=1
ismap ns1/example.db.map || ret=1
[ "$(rawversion ns1/example.db.raw)" -eq 1 ] || ret=1
[ "$(rawversion ns1/example.db.raw1)" -eq 1 ] || ret=1
[ "$(rawversion ns1/example.db.compat)" -eq 0 ] || ret=1
[ "$(rawversion ns1/example.db.map)" -eq 1 ] || ret=1
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking source serial numbers ($n)"
ret=0
[ "$(sourceserial ns1/example.db.raw)" = "UNSET" ] || ret=1
[ "$(sourceserial ns1/example.db.serial.raw)" = "3333" ] || ret=1
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "waiting for transfers to complete"
for i in 0 1 2 3 4 5 6 7 8 9
do
	test -f ns2/transfer.db.raw -a -f ns2/transfer.db.txt && break
	sleep 1
done

echo_i "checking that secondary was saved in raw format by default ($n)"
ret=0
israw ns2/transfer.db.raw || ret=1
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking that secondary was saved in text format when configured ($n)"
ret=0
israw ns2/transfer.db.txt && ret=1
isfull ns2/transfer.db.txt && ret=1
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking that secondary was saved in 'full' style when configured ($n)"
ret=0
isfull ns2/transfer.db.full > /dev/null 2>&1 || ret=1
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking that secondary formerly in text format is now raw ($n)"
for i in 0 1 2 3 4 5 6 7 8 9
do
    ret=0
    israw ns2/formerly-text.db > /dev/null 2>&1 || ret=1
    [ "$(rawversion ns2/formerly-text.db)" -eq 1 ] || ret=1
    [ $ret -eq 0 ] && break
    sleep 1
done
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking that large rdatasets loaded ($n)"
for i in 0 1 2 3 4 5 6 7 8 9
do
ret=0
for a in a b c
do
	$DIG +tcp txt "${a}.large" @10.53.0.2 -p "${PORT}" > "dig.out.ns2.test$n"
	grep "status: NOERROR" "dig.out.ns2.test$n" > /dev/null || ret=1
done
[ $ret -eq 0 ] && break
sleep 1
done
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking format transitions: text->raw->map->text ($n)"
ret=0
$CHECKZONE -D -f text -F text -o baseline.txt example.nil ns1/example.db > /dev/null
$CHECKZONE -D -f text -F raw -o raw.1 example.nil baseline.txt > /dev/null
$CHECKZONE -D -f raw -F map -o map.1 example.nil raw.1 > /dev/null
$CHECKZONE -D -f map -F text -o text.1 example.nil map.1 > /dev/null
cmp -s baseline.txt text.1 || ret=0
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking format transitions: text->map->raw->text ($n)"
ret=0
$CHECKZONE -D -f text -F map -o map.2 example.nil baseline.txt > /dev/null
$CHECKZONE -D -f map -F raw -o raw.2 example.nil map.2 > /dev/null
$CHECKZONE -D -f raw -F text -o text.2 example.nil raw.2 > /dev/null
cmp -s baseline.txt text.2 || ret=0
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking map format loading with journal file rollforward ($n)"
ret=0
$NSUPDATE <<END > /dev/null || status=1
server 10.53.0.3 ${PORT}
ttl 600
update add newtext.dynamic IN TXT "added text"
update delete aaaa.dynamic
send
END
dig_with_opts @10.53.0.3 newtext.dynamic txt > "dig.out.dynamic1.ns3.test$n"
grep "added text" "dig.out.dynamic1.ns3.test$n" > /dev/null 2>&1 || ret=1
dig_with_opts +comm @10.53.0.3 added.dynamic txt > "dig.out.dynamic2.ns3.test$n"
grep "NXDOMAIN" "dig.out.dynamic2.ns3.test$n" > /dev/null 2>&1 || ret=1
# using "rndc halt" ensures that we don't dump the zone file
stop_server --use-rndc --halt --port ${CONTROLPORT} ns3
restart
check_added_text() {
	dig_with_opts @10.53.0.3 newtext.dynamic txt > "dig.out.dynamic3.ns3.test$n" || return 1
	grep "added text" "dig.out.dynamic3.ns3.test$n" > /dev/null || return 1
	return 0
}
retry_quiet 10 check_added_text || ret=1
dig_with_opts +comm @10.53.0.3 added.dynamic txt > "dig.out.dynamic4.ns3.test$n"
grep "NXDOMAIN" "dig.out.dynamic4.ns3.test$n" > /dev/null 2>&1 || ret=1
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking map format file dumps correctly ($n)"
ret=0
$NSUPDATE <<END > /dev/null || status=1
server 10.53.0.3 ${PORT}
ttl 600
update add moretext.dynamic IN TXT "more text"
send
END
dig_with_opts @10.53.0.3 moretext.dynamic txt > "dig.out.dynamic1.ns3.test$n"
grep "more text" "dig.out.dynamic1.ns3.test$n" > /dev/null 2>&1 || ret=1
# using "rndc stop" will cause the zone file to flush before shutdown
stop_server --use-rndc --port ${CONTROLPORT} ns3
rm ns3/*.jnl
restart
#shellcheck disable=SC2034
for i in 0 1 2 3 4 5 6 7 8 9; do
    lret=0
    dig_with_opts +comm @10.53.0.3 moretext.dynamic txt > "dig.out.dynamic2.ns3.test$n"
    grep "more text" "dig.out.dynamic2.ns3.test$n" > /dev/null 2>&1 || lret=1
    [ $lret -eq 0 ] && break;
done
[ $lret -eq 1 ] && ret=1
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

# stomp on the file header
echo_i "checking corrupt map files fail to load (bad file header) ($n)"
ret=0
$CHECKZONE -D -f text -F map -o map.5 example.nil baseline.txt > /dev/null
cp map.5 badmap
stomp badmap 0 32 99
$CHECKZONE -D -f map -F text -o text.5 example.nil badmap > /dev/null
[ $? = 1 ] || ret=1
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

# stomp on the file data so it hashes differently.
# these are small and subtle changes, so that the resulting file
# would appear to be a legitimate map file and would not trigger an
# assertion failure if loaded into memory, but should still fail to
# load because of a SHA1 hash mismatch.
echo_i "checking corrupt map files fail to load (bad node header) ($n)"
ret=0
cp map.5 badmap
stomp badmap 2754 2 99
$CHECKZONE -D -f map -F text -o text.5 example.nil badmap > /dev/null
[ $? = 1 ] || ret=1
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking corrupt map files fail to load (bad node data) ($n)"
ret=0
cp map.5 badmap
stomp badmap 2897 5 127
$CHECKZONE -D -f map -F text -o text.5 example.nil badmap > /dev/null
[ $? = 1 ] || ret=1
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking map format zone is scheduled for resigning (compilezone) ($n)"
ret=0
rndccmd 10.53.0.1 zonestatus signed > rndc.out 2>&1 || ret=1
grep 'next resign' rndc.out > /dev/null 2>&1 || ret=1
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

echo_i "checking map format zone is scheduled for resigning (signzone) ($n)"
ret=0
rndccmd 10.53.0.1 freeze signed > rndc.out 2>&1 || ret=1
(cd ns1 || exit 1; $SIGNER -S -O map -f signed.db.map -o signed signed.db > /dev/null)
rndc_reload ns1 10.53.0.1 signed
rndccmd 10.53.0.1 zonestatus signed > rndc.out 2>&1 || ret=1
grep 'next resign' rndc.out > /dev/null 2>&1 || ret=1
n=$((n+1))
[ $ret -eq 0 ] || echo_i "failed"
status=$((status+ret))

# The following test is disabled by default because it is very slow.
# It fails on Windows, because a single read() call (specifically
# the one in isc_file_mmap()) cannot process more than INT_MAX (2^31)
# bytes of data.
if [ -n "${TEST_LARGE_MAP}" ]; then
    echo_i "checking map file size > 2GB can be loaded ($n)"
    ret=0
    $PERL ../../startperf/mkzonefile.pl test 9000000 > text.$n
    # convert to map
    $CHECKZONE -D -f text -F map -o map.$n test text.$n > /dev/null || ret=1
    # check map file size is over 2GB to ensure the test is valid
    size=$(ls -l map.$n | awk '{print $5}')
    [ "$size" -gt 2147483648 ] || ret=1
    # convert back to text
    $CHECKZONE -f map test map.$n > /dev/null || ret=1
    n=$((n+1))
    [ $ret -eq 0 ] || echo_i "failed"
    status=$((status+ret))
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
