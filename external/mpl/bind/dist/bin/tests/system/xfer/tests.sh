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

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

status=0
n=0

n=`expr $n + 1`
echo_i "testing basic zone transfer functionality"
$DIG $DIGOPTS example. \
	@10.53.0.2 axfr > dig.out.ns2 || status=1
grep "^;" dig.out.ns2 | cat_i

#
# Spin to allow the zone to tranfer.
#
for i in 1 2 3 4 5
do
tmp=0
$DIG $DIGOPTS example. \
	@10.53.0.3 axfr > dig.out.ns3 || tmp=1
	grep "^;" dig.out.ns3 > /dev/null
	if test $? -ne 0 ; then break; fi
	echo_i "plain zone re-transfer"
	sleep 5
done
if test $tmp -eq 1 ; then status=1; fi
grep "^;" dig.out.ns3 | cat_i

digcomp dig1.good dig.out.ns2 || status=1

digcomp dig1.good dig.out.ns3 || status=1

n=`expr $n + 1`
echo_i "testing TSIG signed zone transfers"
$DIG $DIGOPTS tsigzone. @10.53.0.2 axfr -y tsigzone.:1234abcd8765 > dig.out.ns2 || status=1
grep "^;" dig.out.ns2 | cat_i

#
# Spin to allow the zone to tranfer.
#
for i in 1 2 3 4 5
do
tmp=0
	$DIG $DIGOPTS tsigzone. @10.53.0.3 axfr -y tsigzone.:1234abcd8765 > dig.out.ns3 || tmp=1
	grep "^;" dig.out.ns3 > /dev/null
	if test $? -ne 0 ; then break; fi
	echo_i "plain zone re-transfer"
	sleep 5
done
if test $tmp -eq 1 ; then status=1; fi
grep "^;" dig.out.ns3 | cat_i

digcomp dig.out.ns2 dig.out.ns3 || status=1

echo_i "reload servers for in preparation for ixfr-from-differences tests"

$RNDCCMD 10.53.0.1 reload 2>&1 | sed 's/^/ns1 /' | cat_i
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
$RNDCCMD 10.53.0.3 reload 2>&1 | sed 's/^/ns3 /' | cat_i
$RNDCCMD 10.53.0.6 reload 2>&1 | sed 's/^/ns6 /' | cat_i
$RNDCCMD 10.53.0.7 reload 2>&1 | sed 's/^/ns7 /' | cat_i

sleep 2

echo_i "updating master zones for ixfr-from-differences tests"

$PERL -i -p -e '
	s/0\.0\.0\.0/0.0.0.1/;
	s/1397051952/1397051953/
' ns1/slave.db

$RNDCCMD 10.53.0.1 reload 2>&1 | sed 's/^/ns1 /' | cat_i

$PERL -i -p -e '
	s/0\.0\.0\.0/0.0.0.1/;
	s/1397051952/1397051953/
' ns2/example.db

$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i

$PERL -i -p -e '
	s/0\.0\.0\.0/0.0.0.1/;
	s/1397051952/1397051953/
' ns6/master.db

$RNDCCMD 10.53.0.6 reload 2>&1 | sed 's/^/ns6 /' | cat_i

$PERL -i -p -e '
	s/0\.0\.0\.0/0.0.0.1/;
	s/1397051952/1397051953/
' ns7/master2.db

$RNDCCMD 10.53.0.7 reload 2>&1 | sed 's/^/ns7 /' | cat_i

sleep 3

echo_i "testing zone is dumped after successful transfer"
$DIG $DIGOPTS +noall +answer +multi @10.53.0.2 \
	slave. soa > dig.out.ns2 || tmp=1
grep "1397051952 ; serial" dig.out.ns2 > /dev/null 2>&1 || tmp=1
grep "1397051952 ; serial" ns2/slave.db > /dev/null 2>&1 || tmp=1
if test $tmp != 0 ; then echo_i "failed"; fi
status=`expr $status + $tmp`

n=`expr $n + 1`
echo_i "testing ixfr-from-differences yes;"
tmp=0

for i in 0 1 2 3 4 5 6 7 8 9
do
	a=0 b=0 c=0 d=0
	echo_i "wait for reloads..."
	$DIG $DIGOPTS @10.53.0.6 +noall +answer soa master > dig.out.soa1.ns6
	grep "1397051953" dig.out.soa1.ns6 > /dev/null && a=1
	$DIG $DIGOPTS @10.53.0.1 +noall +answer soa slave  > dig.out.soa2.ns1
	grep "1397051953" dig.out.soa2.ns1 > /dev/null && b=1
	$DIG $DIGOPTS @10.53.0.2 +noall +answer soa example > dig.out.soa3.ns2
	grep "1397051953" dig.out.soa3.ns2 > /dev/null && c=1
	[ $a -eq 1 -a $b -eq 1 -a $c -eq 1 ] && break
	sleep 2
done

for i in 0 1 2 3 4 5 6 7 8 9
do
	a=0 b=0 c=0 d=0
	echo_i "wait for transfers..."
	$DIG $DIGOPTS @10.53.0.3 +noall +answer soa example > dig.out.soa1.ns3
	grep "1397051953" dig.out.soa1.ns3 > /dev/null && a=1
	$DIG $DIGOPTS @10.53.0.3 +noall +answer soa master > dig.out.soa2.ns3
	grep "1397051953" dig.out.soa2.ns3 > /dev/null && b=1
	$DIG $DIGOPTS @10.53.0.6 +noall +answer soa slave  > dig.out.soa3.ns6
	grep "1397051953" dig.out.soa3.ns6 > /dev/null && c=1
	[ $a -eq 1 -a $b -eq 1 -a $c -eq 1 ] && break

	# re-notify if necessary
	$RNDCCMD 10.53.0.6 notify master 2>&1 | sed 's/^/ns6 /' | cat_i
	$RNDCCMD 10.53.0.1 notify slave 2>&1 | sed 's/^/ns1 /' | cat_i
	$RNDCCMD 10.53.0.2 notify example 2>&1 | sed 's/^/ns2 /' | cat_i
	sleep 2
done

$DIG $DIGOPTS example. \
	@10.53.0.3 axfr > dig.out.ns3 || tmp=1
grep "^;" dig.out.ns3 | cat_i

digcomp dig2.good dig.out.ns3 || tmp=1

# ns3 has a journal iff it received an IXFR.
test -f ns3/example.bk || tmp=1
test -f ns3/example.bk.jnl || tmp=1

if test $tmp != 0 ; then echo_i "failed"; fi
status=`expr $status + $tmp`

n=`expr $n + 1`
echo_i "testing ixfr-from-differences master; (master zone)"
tmp=0

$DIG $DIGOPTS master. \
	@10.53.0.6 axfr > dig.out.ns6 || tmp=1
grep "^;" dig.out.ns6 | cat_i

$DIG $DIGOPTS master. \
	@10.53.0.3 axfr > dig.out.ns3 || tmp=1
grep "^;" dig.out.ns3 > /dev/null && cat_i dig.out.ns3

digcomp dig.out.ns6 dig.out.ns3 || tmp=1

# ns3 has a journal iff it received an IXFR.
test -f ns3/master.bk || tmp=1
test -f ns3/master.bk.jnl || tmp=1

if test $tmp != 0 ; then echo_i "failed"; fi
status=`expr $status + $tmp`

n=`expr $n + 1`
echo_i "testing ixfr-from-differences master; (slave zone)"
tmp=0

$DIG $DIGOPTS slave. \
	@10.53.0.6 axfr > dig.out.ns6 || tmp=1
grep "^;" dig.out.ns6 | cat_i

$DIG $DIGOPTS slave. \
	@10.53.0.1 axfr > dig.out.ns1 || tmp=1
grep "^;" dig.out.ns1 | cat_i

digcomp dig.out.ns6 dig.out.ns1 || tmp=1

# ns6 has a journal iff it received an IXFR.
test -f ns6/slave.bk || tmp=1
test -f ns6/slave.bk.jnl && tmp=1

if test $tmp != 0 ; then echo_i "failed"; fi
status=`expr $status + $tmp`

n=`expr $n + 1`
echo_i "testing ixfr-from-differences slave; (master zone)"
tmp=0

# ns7 has a journal iff it generates an IXFR.
test -f ns7/master2.db || tmp=1
test -f ns7/master2.db.jnl && tmp=1

if test $tmp != 0 ; then echo_i "failed"; fi
status=`expr $status + $tmp`

n=`expr $n + 1`
echo_i "testing ixfr-from-differences slave; (slave zone)"
tmp=0

$DIG $DIGOPTS slave. \
	@10.53.0.1 axfr > dig.out.ns1 || tmp=1
grep "^;" dig.out.ns1 | cat_i

$DIG $DIGOPTS slave. \
	@10.53.0.7 axfr > dig.out.ns7 || tmp=1
grep "^;" dig.out.ns1 | cat_i

digcomp dig.out.ns7 dig.out.ns1 || tmp=1

# ns7 has a journal iff it generates an IXFR.
test -f ns7/slave.bk || tmp=1
test -f ns7/slave.bk.jnl || tmp=1

if test $tmp != 0 ; then echo_i "failed"; fi
status=`expr $status + $tmp`

echo_i "check that a multi-message uncompressable zone transfers"
$DIG axfr . -p ${PORT} @10.53.0.4 | grep SOA > axfr.out
if test `wc -l < axfr.out` != 2
then
	 echo_i "failed"
	 status=`expr $status + 1`
fi

# now we test transfers with assorted TSIG glitches
DIGCMD="$DIG $DIGOPTS @10.53.0.4"
SENDCMD="$PERL ../send.pl 10.53.0.5 $EXTRAPORT1"

echo_i "testing that incorrectly signed transfers will fail..."
echo_i "initial correctly-signed transfer should succeed"

$SENDCMD < ans5/goodaxfr
sleep 1

# Initially, ns4 is not authoritative for anything.
# Now that ans is up and running with the right data, we make ns4
# a slave for nil.

cat <<EOF >>ns4/named.conf
zone "nil" {
	type slave;
	file "nil.db";
	masters { 10.53.0.5 key tsig_key; };
};
EOF

cur=`awk 'END {print NR}' ns4/named.run`

$RNDCCMD 10.53.0.4 reload | sed 's/^/ns4 /' | cat_i

for i in 0 1 2 3 4 5 6 7 8 9
do
	$DIGCMD nil. SOA > dig.out.ns4
	grep SOA dig.out.ns4 > /dev/null && break
	sleep 1
done

sed -n "$cur,\$p" < ns4/named.run | grep "Transfer status: success" > /dev/null || {
    echo_i "failed: expected status was not logged"
    status=1
}
cur=`awk 'END {print NR}' ns4/named.run`

$DIGCMD nil. TXT | grep 'initial AXFR' >/dev/null || {
    echo_i "failed"
    status=1
}

echo_i "unsigned transfer"

$SENDCMD < ans5/unsigned
sleep 1

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

sed -n "$cur,\$p" < ns4/named.run | grep "Transfer status: expected a TSIG or SIG(0)" > /dev/null || {
    echo_i "failed: expected status was not logged"
    status=1
}
cur=`awk 'END {print NR}' ns4/named.run`

$DIGCMD nil. TXT | grep 'unsigned AXFR' >/dev/null && {
    echo_i "failed"
    status=1
}

echo_i "bad keydata"

$SENDCMD < ans5/badkeydata
sleep 1

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

sed -n "$cur,\$p" < ns4/named.run | grep "Transfer status: tsig verify failure" > /dev/null || {
    echo_i "failed: expected status was not logged"
    status=1
}
cur=`awk 'END {print NR}' ns4/named.run`

$DIGCMD nil. TXT | grep 'bad keydata AXFR' >/dev/null && {
    echo_i "failed"
    status=1
}

echo_i "partially-signed transfer"

$SENDCMD < ans5/partial
sleep 1

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

sed -n "$cur,\$p" < ns4/named.run | grep "Transfer status: expected a TSIG or SIG(0)" > /dev/null || {
    echo_i "failed: expected status was not logged"
    status=1
}
cur=`awk 'END {print NR}' ns4/named.run`

$DIGCMD nil. TXT | grep 'partially signed AXFR' >/dev/null && {
    echo_i "failed"
    status=1
}

echo_i "unknown key"

$SENDCMD < ans5/unknownkey
sleep 1

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

sed -n "$cur,\$p" < ns4/named.run | grep "tsig key 'tsig_key': key name and algorithm do not match" > /dev/null || {
    echo_i "failed: expected status was not logged"
    status=1
}
cur=`awk 'END {print NR}' ns4/named.run`

$DIGCMD nil. TXT | grep 'unknown key AXFR' >/dev/null && {
    echo_i "failed"
    status=1
}

echo_i "incorrect key"

$SENDCMD < ans5/wrongkey
sleep 1

$RNDCCMD 10.53.0.4 retransfer nil | sed 's/^/ns4 /' | cat_i

sleep 2

sed -n "$cur,\$p" < ns4/named.run | grep "tsig key 'tsig_key': key name and algorithm do not match" > /dev/null || {
    echo_i "failed: expected status was not logged"
    status=1
}
cur=`awk 'END {print NR}' ns4/named.run`

$DIGCMD nil. TXT | grep 'incorrect key AXFR' >/dev/null && {
    echo_i "failed"
    status=1
}

n=`expr $n + 1`
echo_i "check that we ask for and get a EDNS EXPIRE response ($n)"
# force a refresh query
$RNDCCMD 10.53.0.7 refresh edns-expire 2>&1 | sed 's/^/ns7 /' | cat_i
sleep 10

# there may be multiple log entries so get the last one.
expire=`awk '/edns-expire\/IN: got EDNS EXPIRE of/ { x=$9 } END { print x }' ns7/named.run`
test ${expire:-0} -gt 0 -a ${expire:-0} -lt 1814400 || {
    echo_i "failed (expire=${expire:-0})"
    status=1
}

n=`expr $n + 1`
echo_i "test smaller transfer TCP message size ($n)"
$DIG $DIGOPTS example. @10.53.0.8 axfr \
	-y key1.:1234abcd8765 > dig.out.msgsize || status=1

$DOS2UNIX dig.out.msgsize >/dev/null

bytes=`wc -c < dig.out.msgsize`
if [ $bytes -ne 459357 ]; then
	echo_i "failed axfr size check"
	status=1
fi

num_messages=`cat ns8/named.run | grep "sending TCP message of" | wc -l`
if [ $num_messages -le 300 ]; then
	echo_i "failed transfer message count check"
	status=1
fi

n=`expr $n + 1`
echo_i "test mapped zone with out of zone data ($n)"
tmp=0
$DIG -p ${PORT} txt mapped @10.53.0.3 > dig.out.1.$n
grep "status: NOERROR," dig.out.1.$n > /dev/null || tmp=1
$PERL $SYSTEMTESTTOP/stop.pl . ns3
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} . ns3
$DIG -p ${PORT} txt mapped @10.53.0.3 > dig.out.2.$n
grep "status: NOERROR," dig.out.2.$n > /dev/null || tmp=1
$DIG -p ${PORT} axfr mapped @10.53.0.3 > dig.out.3.$n
digcomp knowngood.mapped dig.out.3.$n || tmp=1
if test $tmp != 0 ; then echo_i "failed"; fi
status=`expr $status + $tmp`

n=`expr $n + 1`
echo_i "test that a zone with too many records is rejected (AXFR) ($n)"
tmp=0
grep "'axfr-too-big/IN'.*: too many records" ns6/named.run >/dev/null || tmp=1
if test $tmp != 0 ; then echo_i "failed"; fi
status=`expr $status + $tmp`

n=`expr $n + 1`
echo_i "test that a zone with too many records is rejected (IXFR) ($n)"
tmp=0
grep "'ixfr-too-big./IN.*: too many records" ns6/named.run >/dev/null && tmp=1
$NSUPDATE << EOF
zone ixfr-too-big
server 10.53.0.1 ${PORT}
update add the-31st-record.ixfr-too-big 0 TXT this is it
send
EOF
for i in 1 2 3 4 5 6 7 8
do
    grep "'ixfr-too-big/IN'.*: too many records" ns6/named.run >/dev/null && break
    sleep 1
done
grep "'ixfr-too-big/IN'.*: too many records" ns6/named.run >/dev/null || tmp=1
if test $tmp != 0 ; then echo_i "failed"; fi
status=`expr $status + $tmp`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
