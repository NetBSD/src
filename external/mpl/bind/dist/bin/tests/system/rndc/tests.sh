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

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd"
DIGCMD="$DIG $DIGOPTS @10.53.0.2 -p ${PORT}"
RNDCCMD="$RNDC -p ${CONTROLPORT} -c ../common/rndc.conf -s"

status=0
n=0

n=`expr $n + 1`
echo_i "preparing ($n)"
ret=0
$NSUPDATE -p ${PORT} -k ns2/session.key > /dev/null 2>&1 <<END || ret=1
server 10.53.0.2
zone nil.
update add text1.nil. 600 IN TXT "addition 1"
send
zone other.
update add text1.other. 600 IN TXT "addition 1"
send
END
[ -s ns2/nil.db.jnl ] || {
	echo_i "'test -s ns2/nil.db.jnl' failed when it shouldn't have"; ret=1;
}
[ -s ns2/other.db.jnl ] || {
	echo_i "'test -s ns2/other.db.jnl' failed when it shouldn't have"; ret=1;
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "rndc freeze"
$RNDCCMD 10.53.0.2 freeze | sed 's/^/ns2 /' | cat_i | cat_i

n=`expr $n + 1`
echo_i "checking zone was dumped ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10
do
	grep "addition 1" ns2/nil.db > /dev/null && break
	sleep 1
done
grep "addition 1" ns2/nil.db > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking journal file is still present ($n)"
ret=0
[ -s ns2/nil.db.jnl ] || {
	echo_i "'test -s ns2/nil.db.jnl' failed when it shouldn't have"; ret=1;
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking zone not writable ($n)"
ret=0
$NSUPDATE -p ${PORT} -k ns2/session.key > /dev/null 2>&1 <<END && ret=1
server 10.53.0.2
zone nil.
update add text2.nil. 600 IN TXT "addition 2"
send
END

$DIGCMD text2.nil. TXT > dig.out.1.test$n
grep 'addition 2' dig.out.1.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "rndc thaw"
$RNDCCMD 10.53.0.2 thaw | sed 's/^/ns2 /' | cat_i

n=`expr $n + 1`
echo_i "checking zone now writable ($n)"
ret=0
$NSUPDATE -p ${PORT} -k ns2/session.key > nsupdate.out.1.test$n 2>&1 <<END || ret=1
server 10.53.0.2
zone nil.
update add text3.nil. 600 IN TXT "addition 3"
send
END
$DIGCMD text3.nil. TXT > dig.out.1.test$n
grep 'addition 3' dig.out.1.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "rndc sync"
ret=0
$RNDCCMD 10.53.0.2 sync nil | sed 's/^/ns2 /' | cat_i

n=`expr $n + 1`
echo_i "checking zone was dumped ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10
do
	grep "addition 3" ns2/nil.db > /dev/null && break
	sleep 1
done
grep "addition 3" ns2/nil.db > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking journal file is still present ($n)"
ret=0
[ -s ns2/nil.db.jnl ] || {
	echo_i "'test -s ns2/nil.db.jnl' failed when it shouldn't have"; ret=1;
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking zone is still writable ($n)"
ret=0
$NSUPDATE -p ${PORT} -k ns2/session.key > nsupdate.out.1.test$n 2>&1 <<END || ret=1
server 10.53.0.2
zone nil.
update add text4.nil. 600 IN TXT "addition 4"
send
END

$DIGCMD text4.nil. TXT > dig.out.1.test$n
grep 'addition 4' dig.out.1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "rndc sync -clean"
ret=0
$RNDCCMD 10.53.0.2 sync -clean nil | sed 's/^/ns2 /' | cat_i

n=`expr $n + 1`
echo_i "checking zone was dumped ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9 10
do
	grep "addition 4" ns2/nil.db > /dev/null && break
	sleep 1
done
grep "addition 4" ns2/nil.db > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking journal file is deleted ($n)"
ret=0
[ -s ns2/nil.db.jnl ] && {
	echo_i "'test -s ns2/nil.db.jnl' failed when it shouldn't have"; ret=1;
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking zone is still writable ($n)"
ret=0
$NSUPDATE -p ${PORT} -k ns2/session.key > /dev/null 2>&1 <<END || ret=1
server 10.53.0.2
zone nil.
update add text5.nil. 600 IN TXT "addition 5"
send
END

$DIGCMD text4.nil. TXT > dig.out.1.test$n
grep 'addition 4' dig.out.1.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking other journal files not removed ($n)"
ret=0
[ -s ns2/other.db.jnl ] || {
	echo_i "'test -s ns2/other.db.jnl' failed when it shouldn't have"; ret=1;
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "cleaning all zones ($n)"
$RNDCCMD 10.53.0.2 sync -clean | sed 's/^/ns2 /' | cat_i

n=`expr $n + 1`
echo_i "checking all journals removed ($n)"
ret=0
[ -s ns2/nil.db.jnl ] && {
	echo_i "'test -s ns2/nil.db.jnl' succeeded when it shouldn't have"; ret=1;
}
[ -s ns2/other.db.jnl ] && {
	echo_i "'test -s ns2/other.db.jnl' succeeded when it shouldn't have"; ret=1;
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that freezing static zones is not allowed ($n)"
ret=0
$RNDCCMD 10.53.0.2 freeze static > rndc.out.1.test$n 2>&1
grep 'not dynamic' rndc.out.1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that journal is removed when serial is changed before thaw ($n)"
ret=0
sleep 1
$NSUPDATE -p ${PORT} -k ns2/session.key > nsupdate.out.1.test$n 2>&1 <<END || ret=1
server 10.53.0.2
zone other.
update add text6.other. 600 IN TXT "addition 6"
send
END
[ -s ns2/other.db.jnl ] || {
	echo_i "'test -s ns2/other.db.jnl' failed when it shouldn't have"; ret=1;
}
$RNDCCMD 10.53.0.2 freeze other 2>&1 | sed 's/^/ns2 /' | cat_i
for i in 1 2 3 4 5 6 7 8 9 10
do
	grep "addition 6" ns2/other.db > /dev/null && break
	sleep 1
done
serial=`awk '$3 == "serial" {print $1}' ns2/other.db`
newserial=`expr $serial + 1`
sed s/$serial/$newserial/ ns2/other.db > ns2/other.db.new
echo 'frozen TXT "frozen addition"' >> ns2/other.db.new
mv -f ns2/other.db.new ns2/other.db
$RNDCCMD 10.53.0.2 thaw 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 1
[ -f ns2/other.db.jnl ] && {
	echo_i "'test -f ns2/other.db.jnl' succeeded when it shouldn't have"; ret=1;
}
$NSUPDATE -p ${PORT} -k ns2/session.key > nsupdate.out.2.test$n 2>&1 <<END || ret=1
server 10.53.0.2
zone other.
update add text7.other. 600 IN TXT "addition 7"
send
END
$DIGCMD text6.other. TXT > dig.out.1.test$n
grep 'addition 6' dig.out.1.test$n >/dev/null || ret=1
$DIGCMD text7.other. TXT > dig.out.2.test$n
grep 'addition 7' dig.out.2.test$n >/dev/null || ret=1
$DIGCMD frozen.other. TXT > dig.out.3.test$n
grep 'frozen addition' dig.out.3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that journal is kept when ixfr-from-differences is in use ($n)"
ret=0
$NSUPDATE -p ${PORT} -k ns2/session.key > nsupdate.out.1.test$n 2>&1 <<END || ret=1
server 10.53.0.2
zone nil.
update add text6.nil. 600 IN TXT "addition 6"
send
END
[ -s ns2/nil.db.jnl ] || {
	echo_i "'test -s ns2/nil.db.jnl' failed when it shouldn't have"; ret=1;
}
$RNDCCMD 10.53.0.2 freeze nil 2>&1 | sed 's/^/ns2 /' | cat_i
for i in 1 2 3 4 5 6 7 8 9 10
do
	grep "addition 6" ns2/nil.db > /dev/null && break
	sleep 1
done
serial=`awk '$3 == "serial" {print $1}' ns2/nil.db`
newserial=`expr $serial + 1`
sed s/$serial/$newserial/ ns2/nil.db > ns2/nil.db.new
echo 'frozen TXT "frozen addition"' >> ns2/nil.db.new
mv -f ns2/nil.db.new ns2/nil.db
$RNDCCMD 10.53.0.2 thaw 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 1
[ -s ns2/nil.db.jnl ] || {
	echo_i "'test -s ns2/nil.db.jnl' failed when it shouldn't have"; ret=1;
}
$NSUPDATE -p ${PORT} -k ns2/session.key > nsupdate.out.2.test$n 2>&1 <<END || ret=1
server 10.53.0.2
zone nil.
update add text7.nil. 600 IN TXT "addition 7"
send
END
$DIGCMD text6.nil. TXT > dig.out.1.test$n
grep 'addition 6' dig.out.1.test$n > /dev/null || ret=1
$DIGCMD text7.nil. TXT > dig.out.2.test$n
grep 'addition 7' dig.out.2.test$n > /dev/null || ret=1
$DIGCMD frozen.nil. TXT > dig.out.3.test$n
grep 'frozen addition' dig.out.3.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# temp test
echo_i "dumping stats ($n)"
$RNDCCMD 10.53.0.2 stats
n=`expr $n + 1`
echo_i "verifying adb records in named.stats ($n)"
grep "ADB stats" ns2/named.stats > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test using second key ($n)"
ret=0
$RNDC -s 10.53.0.2 -p ${CONTROLPORT} -c ns2/secondkey.conf status > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test 'rndc dumpdb' on a empty cache ($n)"
ret=0
$RNDCCMD 10.53.0.3 dumpdb > /dev/null || ret=1
for i in 1 2 3 4 5 6 7 8 9
do
	tmp=0
	grep "Dump complete" ns3/named_dump.db > /dev/null || tmp=1
	[ $tmp -eq 0 ] && break
	sleep 1
done
[ $tmp -eq 1 ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test 'rndc reload' on a zone with include files ($n)"
ret=0
grep "incl/IN: skipping load" ns2/named.run > /dev/null && ret=1
loads=`grep "incl/IN: starting load" ns2/named.run | wc -l`
[ "$loads" -eq 1 ] || ret=1
$RNDCCMD 10.53.0.2 reload > /dev/null || ret=1
for i in 1 2 3 4 5 6 7 8 9
do
    tmp=0
    grep "incl/IN: skipping load" ns2/named.run > /dev/null || tmp=1
    [ $tmp -eq 0 ] && break
    sleep 1
done
[ $tmp -eq 1 ] && ret=1
touch ns2/static.db
$RNDCCMD 10.53.0.2 reload > /dev/null || ret=1
for i in 1 2 3 4 5 6 7 8 9
do
    tmp=0
    loads=`grep "incl/IN: starting load" ns2/named.run | wc -l`
    [ "$loads" -eq 2 ] || tmp=1
    [ $tmp -eq 0 ] && break
    sleep 1
done
[ $tmp -eq 1 ] && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing rndc with hmac-md5 ($n)"
ret=0
$RNDC -s 10.53.0.4 -p ${EXTRAPORT1} -c ns4/key1.conf status > /dev/null 2>&1 || ret=1
for i in 2 3 4 5 6
do
        $RNDC -s 10.53.0.4 -p ${EXTRAPORT1} -c ns4/key${i}.conf status > /dev/null 2>&1 && ret=1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing rndc with hmac-sha1 ($n)"
ret=0
$RNDC -s 10.53.0.4 -p ${EXTRAPORT2} -c ns4/key2.conf status > /dev/null 2>&1 || ret=1
for i in 1 3 4 5 6
do
        $RNDC -s 10.53.0.4 -p ${EXTRAPORT2} -c ns4/key${i}.conf status > /dev/null 2>&1 && ret=1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing rndc with hmac-sha224 ($n)"
ret=0
$RNDC -s 10.53.0.4 -p ${EXTRAPORT3} -c ns4/key3.conf status > /dev/null 2>&1 || ret=1
for i in 1 2 4 5 6
do
        $RNDC -s 10.53.0.4 -p ${EXTRAPORT3} -c ns4/key${i}.conf status > /dev/null 2>&1 && ret=1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing rndc with hmac-sha256 ($n)"
ret=0
$RNDC -s 10.53.0.4 -p ${EXTRAPORT4} -c ns4/key4.conf status > /dev/null 2>&1 || ret=1
for i in 1 2 3 5 6
do
        $RNDC -s 10.53.0.4 -p ${EXTRAPORT4} -c ns4/key${i}.conf status > /dev/null 2>&1 && ret=1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing rndc with hmac-sha384 ($n)"
ret=0
$RNDC -s 10.53.0.4 -p ${EXTRAPORT5} -c ns4/key5.conf status > /dev/null 2>&1 || ret=1
for i in 1 2 3 4 6
do
        $RNDC -s 10.53.0.4 -p ${EXTRAPORT5} -c ns4/key${i}.conf status > /dev/null 2>&1 && ret=1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing rndc with hmac-sha512 ($n)"
ret=0
$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf status > /dev/null 2>&1 || ret=1
for i in 1 2 3 4 5
do
        $RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key${i}.conf status > /dev/null 2>&1 2>&1 && ret=1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing automatic zones are reported ($n)"
ret=0
$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf status > rndc.out.1.test$n || ret=1
grep "number of zones: 200 (198 automatic)" rndc.out.1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing rndc with null command ($n)"
ret=0
$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing rndc with unknown control channel command ($n)"
ret=0
$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf obviouslynotacommand >/dev/null 2>&1 && ret=1
# rndc: 'obviouslynotacommand' failed: unknown command
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing rndc with querylog command ($n)"
ret=0
# first enable it with querylog on option
$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf querylog on >/dev/null 2>&1 || ret=1
grep "query logging is now on" ns4/named.run > /dev/null || ret=1
# query for builtin and check if query was logged (without +subnet)
$DIG @10.53.0.4 -p ${PORT} -c ch -t txt foo12345.bind > /dev/null || ret=1
grep "query: foo12345.bind CH TXT.*(.*)$" ns4/named.run > /dev/null || ret=1
# query for another builtin zone and check if query was logged (with +subnet=127.0.0.1)
$DIG +subnet=127.0.0.1 @10.53.0.4 -p ${PORT} -c ch -t txt foo12346.bind > /dev/null || ret=1
grep "query: foo12346.bind CH TXT.*\[ECS 127\.0\.0\.1\/32\/0]" ns4/named.run > /dev/null || ret=1
# query for another builtin zone and check if query was logged (with +subnet=127.0.0.1/24)
$DIG +subnet=127.0.0.1/24 @10.53.0.4 -p ${PORT} -c ch -t txt foo12347.bind > /dev/null || ret=1
grep "query: foo12347.bind CH TXT.*\[ECS 127\.0\.0\.0\/24\/0]" ns4/named.run > /dev/null || ret=1
# query for another builtin zone and check if query was logged (with +subnet=::1)
$DIG +subnet=::1 @10.53.0.4 -p ${PORT} -c ch -t txt foo12348.bind > /dev/null || ret=1
grep "query: foo12348.bind CH TXT.*\[ECS \:\:1\/128\/0]" ns4/named.run > /dev/null || ret=1
# toggle query logging and check again
$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf querylog > /dev/null 2>&1 || ret=1
grep "query logging is now off" ns4/named.run > /dev/null || ret=1
# query for another builtin zone and check if query was logged (without +subnet)
$DIG @10.53.0.4 -p ${PORT} -c ch -t txt foo9876.bind > /dev/null || ret=1
grep "query: foo9876.bind CH TXT.*(.*)$" ns4/named.run > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

RNDCCMD4="$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf"
n=`expr $n + 1`
echo_i "testing rndc nta time limits ($n)"
ret=0
$RNDCCMD4 nta -l 2h nta1.example > rndc.out.1.test$n 2>&1
grep "Negative trust anchor added" rndc.out.1.test$n > /dev/null || ret=1
$RNDCCMD4 nta -l 1d nta2.example > rndc.out.2.test$n 2>&1
grep "Negative trust anchor added" rndc.out.2.test$n > /dev/null || ret=1
$RNDCCMD4 nta -l 1w nta3.example > rndc.out.3.test$n 2>&1
grep "Negative trust anchor added" rndc.out.3.test$n > /dev/null || ret=1
$RNDCCMD4 nta -l 8d nta4.example > rndc.out.4.test$n 2>&1
grep "NTA lifetime cannot exceed one week" rndc.out.4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

for i in 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288
do
	n=`expr $n + 1`
	echo_i "testing rndc buffer size limits (size=${i}) ($n)"
	ret=0
	$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf testgen ${i} 2>&1 > rndc.out.$i.test$n || ret=1
	actual_size=`$GENCHECK rndc.out.$i.test$n`
	if [ "$?" = "0" ]; then
	    expected_size=`expr $i + 1`
	    if [ $actual_size != $expected_size ]; then ret=1; fi
	else
	    ret=1
	fi

	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`
done

n=`expr $n + 1`
echo_i "testing rndc -r (show result) ($n)"
ret=0
$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf -r testgen 0 2>&1 > rndc.out.1.test$n || ret=1
grep "ISC_R_SUCCESS 0" rndc.out.1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "testing rndc with a token containing a space ($n)"
ret=0
$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf -r flush '"view with a space"' 2>&1 > rndc.out.1.test$n || ret=1
grep "not found" rndc.out.1.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test 'rndc reconfig' with a broken config ($n)"
ret=0
$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf reconfig > /dev/null || ret=1
sleep 1
mv ns4/named.conf ns4/named.conf.save
echo "error error error" >> ns4/named.conf
$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf reconfig > rndc.out.1.test$n 2>&1 && ret=1
grep "rndc: 'reconfig' failed: unexpected token" rndc.out.1.test$n > /dev/null || ret=1
mv ns4/named.conf.save ns4/named.conf
sleep 1
$RNDC -s 10.53.0.4 -p ${EXTRAPORT6} -c ns4/key6.conf reconfig > /dev/null || ret=1
sleep 1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test read-only control channel access ($n)"
ret=0
$RNDCCMD 10.53.0.5 status > rndc.out.1.test$n 2>&1 || ret=1
$RNDCCMD 10.53.0.5 nta -dump > rndc.out.2.test$n 2>&1 || ret=1
$RNDCCMD 10.53.0.5 reconfig > rndc.out.3.test$n 2>&1 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test rndc status shows running on ($n)"
ret=0
$RNDCCMD 10.53.0.5 status > rndc.out.1.test$n 2>&1 || ret=1
grep "^running on " rndc.out.1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "test 'rndc reconfig' with loading of a large zone ($n)"
ret=0
cur=`awk 'BEGIN {l=0} /^/ {l++} END { print l }' ns6/named.run`
cp ns6/named.conf ns6/named.conf.save
echo "zone \"huge.zone\" { type master; file \"huge.zone.db\"; };" >> ns6/named.conf
echo_i "reloading config"
$RNDCCMD 10.53.0.6 reconfig > rndc.out.1.test$n 2>&1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
sleep 1
n=`expr $n + 1`
echo_i "check if zone load was scheduled ($n)"
grep "scheduled loading new zones" ns6/named.run > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check if query for the zone returns SERVFAIL ($n)"
$DIG @10.53.0.6 -p ${PORT} -t soa huge.zone > dig.out.1.test$n
grep "SERVFAIL" dig.out.1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed (ignored)"; ret=0; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "wait for the zones to be loaded ($n)"
ret=1
try=0
while test $try -lt 100
do
    sleep 1
    sed -n "$cur,"'$p' < ns6/named.run | grep "any newly configured zones are now loaded" > /dev/null && {
        ret=0
        break
    }
    try=`expr $try + 1`
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check if query for the zone returns NOERROR ($n)"
$DIG @10.53.0.6 -p ${PORT} -t soa huge.zone > dig.out.1.test$n
grep "NOERROR" dig.out.1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "verify that the full command is logged ($n)"
ret=0
$RNDCCMD 10.53.0.2 null with extra arguments > /dev/null 2>&1
grep "received control channel command 'null with extra arguments'" ns2/named.run > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

mv ns6/named.conf.save ns6/named.conf
sleep 1
$RNDCCMD 10.53.0.6 reconfig > /dev/null || ret=1
sleep 1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ -x "$PYTHON" ]; then
    n=`expr $n + 1`
    echo_i "test rndc python bindings ($n)"
    ret=0
    $PYTHON > python.out.1.test$n << EOF
import sys
sys.path.insert(0, '../../../../bin/python')
from isc import *
r = rndc(('10.53.0.5', ${CONTROLPORT}), 'hmac-sha256', '1234abcd8765')
result = r.call('status')
print(result['text'])
EOF
    grep 'server is up and running' python.out.1.test$n > /dev/null 2>&1 || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=`expr $status + $ret`
fi

n=`expr $n + 1`
echo_i "check 'rndc \"\"' is handled ($n)"
ret=0
$RNDCCMD 10.53.0.2 "" > rndc.out.1.test$n 2>&1 && ret=1
grep "rndc: '' failed: failure" rndc.out.1.test$n > /dev/null
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check rndc -4 -6 ($n)"
ret=0
$RNDCCMD 10.53.0.2 -4 -6 status > rndc.out.1.test$n 2>&1 && ret=1
grep "only one of -4 and -6 allowed" rndc.out.1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "check rndc -4 with an IPv6 server address ($n)"
ret=0
$RNDCCMD fd92:7065:b8e:ffff::2 -4 status > rndc.out.1.test$n 2>&1 && ret=1
grep "address family not supported" rndc.out.1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
