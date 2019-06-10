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

DIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

#
# Uncomment when creating credential cache files.
#
# KRB5_CONFIG=`pwd`/krb/krb5.conf
#
# Cd krb and run krb/setup.sh to create new keys.
# Run nsupdate system test.
# Kill the krb5kdc server started by krb/setup.sh.
# Check the expiry date on the cached machine.ccache with klist is in 2038.
# Comment out KRB5_CONFIG.
# Re-run nsupdate system test to confirm everything still works.
# git add and commit the resulting ns*/machine.ccache and ns*/dns.keytab files.
# Clean up krb.
#

status=0
n=0

# wait for zone transfer to complete
tries=0
while true; do
    if [ $tries -eq 10 ]
    then
        exit 1
    fi

    if grep "example.nil/IN.*Transfer status" ns2/named.run > /dev/null
    then
        break
    else
        echo_i "zones are not fully loaded, waiting..."
        tries=`expr $tries + 1`
        sleep 1
    fi
done

ret=0
echo_i "fetching first copy of zone before update"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil.\
	@10.53.0.1 axfr > dig.out.ns1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "fetching second copy of zone before update"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil.\
	@10.53.0.2 axfr > dig.out.ns2 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "comparing pre-update copies to known good data"
digcomp knowngood.ns1.before dig.out.ns1 || ret=1
digcomp knowngood.ns1.before dig.out.ns2 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "updating zone"
# nsupdate will print a ">" prompt to stdout as it gets each input line.
$NSUPDATE -k ns1/ddns.key <<END > /dev/null || ret=1
server 10.53.0.1 ${PORT}
update add updated.example.nil. 600 A 10.10.10.1
add updated.example.nil. 600 TXT Foo
delete t.example.nil.

END
[ $ret = 0 ] || { echo_i "failed"; status=1; }

echo_i "sleeping 5 seconds for server to incorporate changes"
sleep 5

ret=0
echo_i "fetching first copy of zone after update"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil.\
	@10.53.0.1 axfr > dig.out.ns1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "fetching second copy of zone after update"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil.\
	@10.53.0.2 axfr > dig.out.ns2 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "comparing post-update copies to known good data"
digcomp knowngood.ns1.after dig.out.ns1 || ret=1
digcomp knowngood.ns1.after dig.out.ns2 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "testing local update policy"
pre=`$DIG $DIGOPTS +short new.other.nil. @10.53.0.1 a` || ret=1
[ -z "$pre" ] || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "updating zone"
# nsupdate will print a ">" prompt to stdout as it gets each input line.
$NSUPDATE -4 -l -p ${PORT} -k ns1/session.key > /dev/null <<END || ret=1
zone other.nil.
update add new.other.nil. 600 IN A 10.10.10.1
send
END
[ $ret = 0 ] || { echo_i "failed"; status=1; }

echo_i "sleeping 5 seconds for server to incorporate changes"
sleep 5

ret=0
echo_i "checking result of update"
post=`$DIG $DIGOPTS +short new.other.nil. @10.53.0.1 a` || ret=1
[ "$post" = "10.10.10.1" ] || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "comparing post-update copy to known good data"
digcomp knowngood.ns1.after dig.out.ns1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "testing zone consistency checks"
# inserting an NS record without a corresponding A or AAAA record should fail
$NSUPDATE -4 -l -p ${PORT} -k ns1/session.key > nsupdate.out 2>&1 << END && ret=1
update add other.nil. 600 in ns ns3.other.nil.
send
END
grep REFUSED nsupdate.out > /dev/null 2>&1 || ret=1
# ...but should work if an A record is inserted first:
$NSUPDATE -4 -l -p ${PORT} -k ns1/session.key > nsupdate.out 2>&1 << END || ret=1
update add ns4.other.nil 600 in a 10.53.0.1
send
update add other.nil. 600 in ns ns4.other.nil.
send
END
grep REFUSED nsupdate.out > /dev/null 2>&1 && ret=1
# ...or if an AAAA record does:
$NSUPDATE -4 -l -p ${PORT} -k ns1/session.key > nsupdate.out 2>&1 << END || ret=1
update add ns5.other.nil 600 in aaaa 2001:db8::1
send
update add other.nil. 600 in ns ns5.other.nil.
send
END
grep REFUSED nsupdate.out > /dev/null 2>&1 && ret=1
# ...or if the NS and A/AAAA are inserted together:
$NSUPDATE -4 -l -p ${PORT} -k ns1/session.key > nsupdate.out 2>&1 << END || ret=1
update add other.nil. 600 in ns ns6.other.nil.
update add ns6.other.nil 600 in a 10.53.0.1
send
END
grep REFUSED nsupdate.out > /dev/null 2>&1 && ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

echo_i "sleeping 5 seconds for server to incorporate changes"
sleep 5

ret=0
echo_i "checking result of update"
$DIG $DIGOPTS +short @10.53.0.1 ns other.nil > dig.out.ns1 || ret=1
grep ns3.other.nil dig.out.ns1 > /dev/null 2>&1 && ret=1
grep ns4.other.nil dig.out.ns1 > /dev/null 2>&1 || ret=1
grep ns5.other.nil dig.out.ns1 > /dev/null 2>&1 || ret=1
grep ns6.other.nil dig.out.ns1 > /dev/null 2>&1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "ensure 'check-mx ignore' allows adding MX records containing an address without a warning"
$NSUPDATE -k ns1/ddns.key > nsupdate.out 2>&1 << END || ret=1
server 10.53.0.1 ${PORT}
update add mx03.example.nil 600 IN MX 10 10.53.0.1
send
END
grep REFUSED nsupdate.out > /dev/null 2>&1 && ret=1
grep "mx03.example.nil/MX:.*MX is an address" ns1/named.run > /dev/null 2>&1 && ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "ensure 'check-mx warn' allows adding MX records containing an address with a warning"
$NSUPDATE -4 -l -p ${PORT} -k ns1/session.key > nsupdate.out 2>&1 << END || ret=1
update add mx03.other.nil 600 IN MX 10 10.53.0.1
send
END
grep REFUSED nsupdate.out > /dev/null 2>&1 && ret=1
grep "mx03.other.nil/MX:.*MX is an address" ns1/named.run > /dev/null 2>&1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "ensure 'check-mx fail' prevents adding MX records containing an address with a warning"
$NSUPDATE > nsupdate.out 2>&1 << END && ret=1
server 10.53.0.1 ${PORT}
update add mx03.update.nil 600 IN MX 10 10.53.0.1
send
END
grep REFUSED nsupdate.out > /dev/null 2>&1 || ret=1
grep "mx03.update.nil/MX:.*MX is an address" ns1/named.run > /dev/null 2>&1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "check SIG(0) key is accepted"
key=`$KEYGEN -q -a NSEC3RSASHA1 -b 1024 -T KEY -n ENTITY xxx`
echo "" | $NSUPDATE -k ${key}.private > /dev/null 2>&1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "check TYPE=0 update is rejected by nsupdate ($n)"
$NSUPDATE <<END > nsupdate.out 2>&1 && ret=1
    server 10.53.0.1 ${PORT}
    ttl 300
    update add example.nil. in type0 ""
    send
END
grep "unknown class/type" nsupdate.out > /dev/null 2>&1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "check TYPE=0 prerequisite is handled ($n)"
$NSUPDATE -k ns1/ddns.key <<END > nsupdate.out 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    prereq nxrrset example.nil. type0
    send
END
$DIG $DIGOPTS +tcp version.bind txt ch @10.53.0.1 > dig.out.ns1.$n
grep "status: NOERROR" dig.out.ns1.$n > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "check that TYPE=0 update is handled ($n)"
echo "a0e4280000010000000100000000060001c00c000000fe000000000000" |
$PERL ../packet.pl -a 10.53.0.1 -p ${PORT} -t tcp > /dev/null
$DIG $DIGOPTS +tcp version.bind txt ch @10.53.0.1 > dig.out.ns1.$n
grep "status: NOERROR" dig.out.ns1.$n > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "check that TYPE=0 additional data is handled ($n)"
echo "a0e4280000010000000000010000060001c00c000000fe000000000000" |
$PERL ../packet.pl -a 10.53.0.1 -p ${PORT} -t tcp > /dev/null
$DIG $DIGOPTS +tcp version.bind txt ch @10.53.0.1 > dig.out.ns1.$n
grep "status: NOERROR" dig.out.ns1.$n > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "check that update to undefined class is handled ($n)"
echo "a0e4280000010001000000000000060101c00c000000fe000000000000" |
$PERL ../packet.pl -a 10.53.0.1 -p ${PORT} -t tcp > /dev/null
$DIG $DIGOPTS +tcp version.bind txt ch @10.53.0.1 > dig.out.ns1.$n
grep "status: NOERROR" dig.out.ns1.$n > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "check that address family mismatch is handled ($n)"
$NSUPDATE <<END > /dev/null 2>&1 && ret=1
server ::1
local 127.0.0.1
update add 600 txt.example.nil in txt "test"
send
END
[ $ret = 0 ] || { echo_i "failed"; status=1; }


n=`expr $n + 1`
ret=0
echo_i "check that unixtime serial number is correctly generated ($n)"
$DIG $DIGOPTS +short unixtime.nil. soa @10.53.0.1 > dig.out.old.test$n || ret=1
oldserial=`awk '{print $3}' dig.out.old.test$n` || ret=1
start=`$PERL -e 'print time()."\n";'`
$NSUPDATE <<END > /dev/null 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    ttl 600
    update add new.unixtime.nil in a 1.2.3.4
    send
END
now=`$PERL -e 'print time()."\n";'`
sleep 1
$DIG $DIGOPTS +short unixtime.nil. soa @10.53.0.1 > dig.out.new.test$n || ret=1
serial=`awk '{print $3}' dig.out.new.test$n` || ret=1
[ "$oldserial" = "$serial" ] && { echo_i "oldserial == serial"; ret=1; }
if [ "$serial" -lt "$start" ]; then
    echo_i "out-of-range serial=$serial < start=$start"; ret=1;
elif [ "$serial" -gt "$now" ]; then
    echo_i "out-of-range serial=$serial > now=$now"; ret=1;
fi
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
if $PERL -e 'use Net::DNS;' 2>/dev/null
then
    echo_i "running update.pl test"
    {
      $PERL update_test.pl -s 10.53.0.1 -p ${PORT} update.nil. || ret=1
    } | cat_i
    [ $ret -eq 1 ] && { echo_i "failed"; status=1; }
else
    echo_i "The second part of this test requires the Net::DNS library." >&2
fi

ret=0
echo_i "fetching first copy of test zone"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil.\
	@10.53.0.1 axfr > dig.out.ns1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "fetching second copy of test zone"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil.\
	@10.53.0.2 axfr > dig.out.ns2 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "comparing zones"
digcomp dig.out.ns1 dig.out.ns2 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

echo_i "SIGKILL and restart server ns1"
cd ns1
$KILL -KILL `cat named.pid`
rm named.pid
cd ..
sleep 10
if
	$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} nsupdate ns1
then
	echo_i "restarted server ns1"
else
	echo_i "could not restart server ns1"
	exit 1
fi
sleep 10

ret=0
echo_i "fetching ns1 after hard restart"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil.\
	@10.53.0.1 axfr > dig.out.ns1.after || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "comparing zones"
digcomp dig.out.ns1 dig.out.ns1.after || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

echo_i "begin RT #482 regression test"

ret=0
echo_i "update master"
$NSUPDATE -k ns1/ddns.key <<END > /dev/null || ret=1
server 10.53.0.1 ${PORT}
update add updated2.example.nil. 600 A 10.10.10.2
update add updated2.example.nil. 600 TXT Bar
update delete c.example.nil.
send
END
[ $ret = 0 ] || { echo_i "failed"; status=1; }

sleep 5

if [ ! "$CYGWIN" ]; then
    echo_i "SIGHUP slave"
    $KILL -HUP `cat ns2/named.pid`
else
    echo_i "reload slave"
    rndc_reload ns2 10.53.0.2
fi

sleep 5

ret=0
echo_i "update master again"
$NSUPDATE -k ns1/ddns.key <<END > /dev/null || ret=1
server 10.53.0.1 ${PORT}
update add updated3.example.nil. 600 A 10.10.10.3
update add updated3.example.nil. 600 TXT Zap
del d.example.nil.
send
END
[ $ret = 0 ] || { echo_i "failed"; status=1; }

sleep 5

if [ ! "$CYGWIN" ]; then
    echo_i "SIGHUP slave again"
    $KILL -HUP `cat ns2/named.pid`
else
    echo_i "reload slave again"
    rndc_reload ns2 10.53.0.2
fi

sleep 5

echo_i "check to 'out of sync' message"
if grep "out of sync" ns2/named.run
then
	echo_i "failed (found 'out of sync')"
	status=1
fi

echo_i "end RT #482 regression test"

n=`expr $n + 1`
ret=0
echo_i "start NSEC3PARAM changes via UPDATE on a unsigned zone test ($n)"
$NSUPDATE << EOF
server 10.53.0.3 ${PORT}
update add example 3600 nsec3param 1 0 0 -
send
EOF

sleep 1

# the zone is not signed.  The nsec3param records should be removed.
# this also proves that the server is still running.
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocmd +norec example.\
	@10.53.0.3 nsec3param > dig.out.ns3.$n || ret=1
grep "ANSWER: 0" dig.out.ns3.$n > /dev/null || ret=1
grep "flags:[^;]* aa[ ;]" dig.out.ns3.$n > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "change the NSEC3PARAM ttl via update ($n)"
$NSUPDATE << EOF
server 10.53.0.3 ${PORT}
update add nsec3param.test 3600 NSEC3PARAM 1 0 1 -
send
EOF

sleep 1

$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocmd +norec nsec3param.test.\
        @10.53.0.3 nsec3param > dig.out.ns3.$n || ret=1
grep "ANSWER: 1" dig.out.ns3.$n > /dev/null || ret=1
grep "3600.*NSEC3PARAM" dig.out.ns3.$n > /dev/null || ret=1
grep "flags:[^;]* aa[ ;]" dig.out.ns3.$n > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "add a new the NSEC3PARAM via update ($n)"
$NSUPDATE << EOF
server 10.53.0.3 ${PORT}
update add nsec3param.test 3600 NSEC3PARAM 1 0 4 -
send
EOF

sleep 1

$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocmd +norec nsec3param.test.\
        @10.53.0.3 nsec3param > dig.out.ns3.$n || ret=1
grep "ANSWER: 2" dig.out.ns3.$n > /dev/null || ret=1
grep "NSEC3PARAM 1 0 4 -" dig.out.ns3.$n > /dev/null || ret=1
grep "flags:[^;]* aa[ ;]" dig.out.ns3.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $ret + $status`; fi

n=`expr $n + 1`
ret=0
echo_i "add, delete and change the ttl of the NSEC3PARAM rrset via update ($n)"
$NSUPDATE << EOF
server 10.53.0.3 ${PORT}
update delete nsec3param.test NSEC3PARAM
update add nsec3param.test 7200 NSEC3PARAM 1 0 5 -
send
EOF

sleep 1

$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocmd +norec nsec3param.test.\
        @10.53.0.3 nsec3param > dig.out.ns3.$n || ret=1
grep "ANSWER: 1" dig.out.ns3.$n > /dev/null || ret=1
grep "7200.*NSEC3PARAM 1 0 5 -" dig.out.ns3.$n > /dev/null || ret=1
grep "flags:[^;]* aa[ ;]" dig.out.ns3.$n > /dev/null || ret=1
$JOURNALPRINT ns3/nsec3param.test.db.signed.jnl > jp.out.ns3.$n
# intermediate TTL changes.
grep "add nsec3param.test.	7200	IN	NSEC3PARAM 1 0 4 -" jp.out.ns3.$n > /dev/null || ret=1
grep "add nsec3param.test.	7200	IN	NSEC3PARAM 1 0 1 -" jp.out.ns3.$n > /dev/null || ret=1
# delayed adds and deletes.
grep "add nsec3param.test.	0	IN	TYPE65534 .# 6 000180000500" jp.out.ns3.$n > /dev/null || ret=1
grep "add nsec3param.test.	0	IN	TYPE65534 .# 6 000140000100" jp.out.ns3.$n > /dev/null || ret=1
grep "add nsec3param.test.	0	IN	TYPE65534 .# 6 000140000400" jp.out.ns3.$n > /dev/null || ret=1
if [ $ret != 0 ] ; then echo_i "failed"; status=`expr $ret + $status`; fi



ret=0
echo_i "testing that rndc stop updates the master file"
$NSUPDATE -k ns1/ddns.key <<END > /dev/null || ret=1
server 10.53.0.1 ${PORT}
update add updated4.example.nil. 600 A 10.10.10.3
send
END
$PERL $SYSTEMTESTTOP/stop.pl --use-rndc --port ${CONTROLPORT} nsupdate ns1
# Removing the journal file and restarting the server means
# that the data served by the new server process are exactly
# those dumped to the master file by "rndc stop".
rm -f ns1/*jnl
$PERL $SYSTEMTESTTOP/start.pl --noclean --restart --port ${PORT} nsupdate ns1
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd updated4.example.nil.\
	@10.53.0.1 a > dig.out.ns1 || status=1
digcomp knowngood.ns1.afterstop dig.out.ns1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

ret=0
echo_i "check that 'nsupdate -l' with a missing keyfile reports the missing file"
$NSUPDATE -4 -p ${PORT} -l -k ns1/nonexistant.key 2> nsupdate.out < /dev/null
grep ns1/nonexistant.key nsupdate.out > /dev/null || ret=1
if test $ret -ne 0
then
echo_i "failed"; status=1
fi

n=`expr $n + 1`
ret=0
echo_i "check that 'update-policy local' works from localhost address ($n)"
$NSUPDATE -k ns5/session.key > nsupdate.out.$n 2>&1 << END || ret=1
server 10.53.0.5 ${PORT}
local 127.0.0.1
update add fromlocal.local.nil. 600 A 1.2.3.4
send
END
grep REFUSED nsupdate.out.$n > /dev/null 2>&1 && ret=1
$DIG $DIGOPTS @10.53.0.5 \
        +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
        fromlocal.local.nil. > dig.out.ns5.$n || ret=1
grep fromlocal dig.out.ns5.$n > /dev/null 2>&1 || ret=1
if test $ret -ne 0
then
echo_i "failed"; status=1
fi

n=`expr $n + 1`
ret=0
echo_i "check that 'update-policy local' fails from non-localhost address ($n)"
grep 'match on session key not from localhost' ns5/named.run > /dev/null && ret=1
$NSUPDATE -k ns5/session.key > nsupdate.out.$n 2>&1 << END && ret=1
server 10.53.0.5 ${PORT}
local 10.53.0.1
update add nonlocal.local.nil. 600 A 4.3.2.1
send
END
grep REFUSED nsupdate.out.$n > /dev/null 2>&1 || ret=1
grep 'match on session key not from localhost' ns5/named.run > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.5 \
        +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
        nonlocal.local.nil. > dig.out.ns5.$n || ret=1
grep nonlocal dig.out.ns5.$n > /dev/null 2>&1 && ret=1
if test $ret -ne 0
then
echo_i "failed"; status=1
fi

n=`expr $n + 1`
ret=0
echo_i "check that 'update-policy tcp-self' refuses update of records via UDP ($n)"
$NSUPDATE > nsupdate.out.$n 2>&1 << END
server 10.53.0.6 ${PORT}
local 127.0.0.1
update add 1.0.0.127.in-addr.arpa. 600 PTR localhost.
send
END
grep REFUSED nsupdate.out.$n > /dev/null 2>&1 || ret=1
$DIG $DIGOPTS @10.53.0.6 \
        +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
        -x 127.0.0.1 > dig.out.ns6.$n
grep localhost. dig.out.ns6.$n > /dev/null 2>&1 && ret=1
if test $ret -ne 0
then
echo_i "failed"; status=1
fi

n=`expr $n + 1`
ret=0
echo_i "check that 'update-policy tcp-self' permits update of records for the client's own address via TCP ($n)"
$NSUPDATE -v > nsupdate.out.$n 2>&1 << END || ret=1
server 10.53.0.6 ${PORT}
local 127.0.0.1
update add 1.0.0.127.in-addr.arpa. 600 PTR localhost.
send
END
grep REFUSED nsupdate.out.$n > /dev/null 2>&1 && ret=1
$DIG $DIGOPTS @10.53.0.6 \
        +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
        -x 127.0.0.1 > dig.out.ns6.$n || ret=1
grep localhost. dig.out.ns6.$n > /dev/null 2>&1 || ret=1
if test $ret -ne 0
then
echo_i "failed"; status=1
fi

n=`expr $n + 1`
ret=0
echo_i "check that 'update-policy tcp-self' refuses update of records for a different address from the client's own address via TCP ($n)"
$NSUPDATE -v > nsupdate.out.$n 2>&1 << END
server 10.53.0.6 ${PORT}
local 127.0.0.1
update add 1.0.168.192.in-addr.arpa. 600 PTR localhost.
send
END
grep REFUSED nsupdate.out.$n > /dev/null 2>&1 || ret=1
$DIG $DIGOPTS @10.53.0.6 \
        +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
        -x 192.168.0.1 > dig.out.ns6.$n
grep localhost. dig.out.ns6.$n > /dev/null 2>&1 && ret=1
if test $ret -ne 0
then
echo_i "failed"; status=1
fi

n=`expr $n + 1`
ret=0
echo_i "check that changes to the DNSKEY RRset TTL do not have side effects ($n)"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd dnskey.test. \
        @10.53.0.3 dnskey | \
	sed -n 's/\(.*\)10.IN/update add \1600 IN/p' |
	(echo server 10.53.0.3 ${PORT}; cat - ; echo send ) |
$NSUPDATE

$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd dnskey.test. \
	@10.53.0.3 any > dig.out.ns3.$n

grep "600.*DNSKEY" dig.out.ns3.$n > /dev/null || ret=1
grep TYPE65534 dig.out.ns3.$n > /dev/null && ret=1
if test $ret -ne 0
then
echo_i "failed"; status=1
fi

n=`expr $n + 1`
ret=0
echo_i "check notify with TSIG worked ($n)"
# if the alternate view received a notify--meaning, the notify was
# validly signed by "altkey"--then the zonefile update.alt.bk will
# will have been created.
[ -f ns2/update.alt.bk ] || ret=1
if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
fi

n=`expr $n + 1`
ret=0
echo_i "check type list options ($n)"
$NSUPDATE -T > typelist.out.T.${n} || { ret=1; echo_i "nsupdate -T failed"; }
$NSUPDATE -P > typelist.out.P.${n} || { ret=1; echo_i "nsupdate -P failed"; }
$NSUPDATE -TP > typelist.out.TP.${n} || { ret=1; echo_i "nsupdate -TP failed"; }
grep ANY typelist.out.T.${n} > /dev/null && { ret=1; echo_i "failed: ANY found (-T)"; }
grep ANY typelist.out.P.${n} > /dev/null && { ret=1; echo_i "failed: ANY found (-P)"; }
grep ANY typelist.out.TP.${n} > /dev/null && { ret=1; echo_i "failed: ANY found (-TP)"; }
grep KEYDATA typelist.out.T.${n} > /dev/null && { ret=1; echo_i "failed: KEYDATA found (-T)"; }
grep KEYDATA typelist.out.P.${n} > /dev/null && { ret=1; echo_i "failed: KEYDATA found (-P)"; }
grep KEYDATA typelist.out.TP.${n} > /dev/null && { ret=1; echo_i "failed: KEYDATA found (-TP)"; }
grep AAAA typelist.out.T.${n} > /dev/null || { ret=1; echo_i "failed: AAAA not found (-T)"; }
grep AAAA typelist.out.P.${n} > /dev/null && { ret=1; echo_i "failed: AAAA found (-P)"; }
grep AAAA typelist.out.TP.${n} > /dev/null || { ret=1; echo_i "failed: AAAA not found (-TP)"; }
if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
fi

n=`expr $n + 1`
ret=0
echo_i "check command list ($n)"
(
while read cmd
do
    echo "$cmd" | $NSUPDATE  > /dev/null 2>&1
    if test $? -gt 1 ; then
	echo_i "failed ($cmd)"
	ret=1
    fi
    echo "$cmd " | $NSUPDATE  > /dev/null 2>&1
    if test $? -gt 1 ; then
	echo_i "failed ($cmd)"
	ret=1
    fi
done
exit $ret
) < commandlist || ret=1
if [ $ret -ne 0 ]; then
    status=1
fi

n=`expr $n + 1`
ret=0
echo_i "check TSIG key algorithms ($n)"
for alg in md5 sha1 sha224 sha256 sha384 sha512; do
    $NSUPDATE -k ns1/${alg}.key <<END > /dev/null || ret=1
server 10.53.0.1 ${PORT}
update add ${alg}.keytests.nil. 600 A 10.10.10.3
send
END
done
sleep 2
for alg in md5 sha1 sha224 sha256 sha384 sha512; do
    $DIG $DIGOPTS +short @10.53.0.1 ${alg}.keytests.nil | grep 10.10.10.3 > /dev/null 2>&1 || ret=1
done
if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
fi

n=`expr $n + 1`
ret=0
echo_i "check that ttl is capped by max-ttl ($n)"
$NSUPDATE <<END > /dev/null || ret=1
server 10.53.0.1 ${PORT}
update add cap.max-ttl.nil. 600 A 10.10.10.3
update add nocap.max-ttl.nil. 150 A 10.10.10.3
send
END
sleep 2
$DIG $DIGOPTS @10.53.0.1  cap.max-ttl.nil | grep "^cap.max-ttl.nil.	300" > /dev/null 2>&1 || ret=1
$DIG $DIGOPTS @10.53.0.1  nocap.max-ttl.nil | grep "^nocap.max-ttl.nil.	150" > /dev/null 2>&1 || ret=1
if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
fi

n=`expr $n + 1`
ret=0
echo_i "add a record which is truncated when logged. ($n)"
$NSUPDATE verylarge || ret=1
$DIG $DIGOPTS +tcp @10.53.0.1 txt txt.update.nil > dig.out.ns1.test$n
grep "ANSWER: 1," dig.out.ns1.test$n > /dev/null || ret=1
grep "adding an RR at 'txt.update.nil' TXT .* \[TRUNCATED\]"  ns1/named.run > /dev/null || ret=1
if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
fi

n=`expr $n + 1`
ret=0
echo_i "check that yyyymmddvv serial number is correctly generated ($n)"
oldserial=`$DIG $DIGOPTS +short yyyymmddvv.nil. soa @10.53.0.1 | awk '{print $3}'` || ret=1
$NSUPDATE <<END > /dev/null 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    ttl 600
    update add new.yyyymmddvv.nil in a 1.2.3.4
    send
END
now=`$PERL -e '@lt=localtime(); printf "%.4d%0.2d%0.2d00\n",$lt[5]+1900,$lt[4]+1,$lt[3];'`
sleep 1
serial=`$DIG $DIGOPTS +short yyyymmddvv.nil. soa @10.53.0.1 | awk '{print $3}'` || ret=1
[ "$oldserial" -ne "$serial" ] || ret=1
[ "$serial" -eq "$now" ] || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

#
#  Refactor to use perl to launch the parallel updates.
#
if false
then
n=`expr $n + 1`
echo_i "send many simultaneous updates via a update forwarder ($n)"
ret=0
for i in 0 1 2 3 4 5 6 7
do
(
    for j in 0 1 2 3 4 5 6 7
    do
    (
	$NSUPDATE << EOF
server 10.53.0.3 ${PORT}
zone many.test
update add $i-$j.many.test 0 IN A 1.2.3.4
send
EOF
    ) &
    done
    wait
) &
done
wait
dig axfr many.test @10.53.0.1 > dig.out.test$n
lines=`awk '$4 == "A" { l++ } END { print l }' dig.out.test$n`
test ${lines:-0} -eq 64 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }
fi

n=`expr $n + 1`
echo_i "check max-journal-size limits ($n)"
ret=0
rm -f nsupdate.out1-$n
# add one record
$NSUPDATE << EOF >> nsupdate.out1-$n 2>&1
server 10.53.0.1 ${PORT}
zone maxjournal.test
update add z.maxjournal.test 300 IN A 10.20.30.40
send
EOF
for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
    # repeatedly add and remove the same set of records to fill up
    # the journal file without changing the zone content
    $NSUPDATE << EOF >> nsupdate.out1-$n 2>&1
server 10.53.0.1 ${PORT}
zone maxjournal.test
update add a.maxjournal.test 300 IN A 1.2.3.4
update add b.maxjournal.test 300 IN A 1.2.3.4
update add c.maxjournal.test 300 IN A 1.2.3.4
update add d.maxjournal.test 300 IN A 1.2.3.4
send
update del a.maxjournal.test
update del b.maxjournal.test
update del c.maxjournal.test
update del d.maxjournal.test
send
EOF
done
# check that the journal is big enough to require truncation.
size=`$PERL -e 'use File::stat; my $sb = stat(@ARGV[0]); printf("%s\n", $sb->size);' ns1/maxjournal.db.jnl`
[ "$size" -gt 6000 ] || ret=1
sleep 1
$RNDCCMD 10.53.0.1 sync maxjournal.test
for i in 1 2 3 4 5 6
do
    sleep 1
    size=`$PERL -e 'use File::stat; my $sb = stat(@ARGV[0]); printf("%s\n", $sb->size);' ns1/maxjournal.db.jnl`
    [ "$size" -lt 5000 ] && break
done
size=`$PERL -e 'use File::stat; my $sb = stat(@ARGV[0]); printf("%s\n", $sb->size);' ns1/maxjournal.db.jnl`
[ "$size" -lt 5000 ] || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
echo_i "check check-names processing ($n)"
ret=0
$NSUPDATE << EOF > nsupdate.out1-$n 2>&1
update add # 0 in a 1.2.3.4
EOF
grep "bad owner" nsupdate.out1-$n > /dev/null || ret=1

$NSUPDATE << EOF > nsupdate.out2-$n 2>&1
check-names off
update add # 0 in a 1.2.3.4
EOF
grep "bad owner" nsupdate.out2-$n > /dev/null && ret=1

$NSUPDATE << EOF > nsupdate.out3-$n 2>&1
update add . 0 in mx 0 #
EOF
grep "bad name" nsupdate.out3-$n > /dev/null || ret=1

$NSUPDATE << EOF > nsupdate.out4-$n 2>&1
check-names off
update add . 0 in mx 0 #
EOF
grep "bad name" nsupdate.out4-$n > /dev/null && ret=1

[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
echo_i "check adding of delegating NS records processing ($n)"
ret=0
$NSUPDATE -v << EOF > nsupdate.out-$n 2>&1 || ret=1
server 10.53.0.3 ${PORT}
zone delegation.test.
update add child.delegation.test. 3600 NS foo.example.net.
update add child.delegation.test. 3600 NS bar.example.net.
send
EOF
$DIG $DIGOPTS +tcp @10.53.0.3 ns child.delegation.test > dig.out.ns1.test$n
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null 2>&1 || ret=1
grep "AUTHORITY: 2" dig.out.ns1.test$n > /dev/null 2>&1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
echo_i "check deleting of delegating NS records processing ($n)"
ret=0
$NSUPDATE -v << EOF > nsupdate.out-$n 2>&1 || ret=1
server 10.53.0.3 ${PORT}
zone delegation.test.
update del child.delegation.test. 3600 NS foo.example.net.
update del child.delegation.test. 3600 NS bar.example.net.
send
EOF
$DIG $DIGOPTS +tcp @10.53.0.3 ns child.delegation.test > dig.out.ns1.test$n
grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null 2>&1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
echo_i "check that adding too many records is blocked ($n)"
ret=0
$NSUPDATE -v << EOF > nsupdate.out-$n 2>&1 && ret=1
server 10.53.0.3 ${PORT}
zone too-big.test.
update add r1.too-big.test 3600 IN TXT r1.too-big.test
send
EOF
grep "update failed: SERVFAIL" nsupdate.out-$n > /dev/null || ret=1
$DIG $DIGOPTS +tcp @10.53.0.3 r1.too-big.test TXT > dig.out.ns3.test$n
grep "status: NXDOMAIN" dig.out.ns3.test$n > /dev/null || ret=1
grep "records in zone (4) exceeds max-records (3)" ns3/named.run > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "check whether valid addresses are used for master failover ($n)"
$NSUPDATE -t 1 <<END > nsupdate.out-$n 2>&1 && ret=1
server 10.53.0.4 ${PORT}
zone unreachable.
update add unreachable. 600 A 192.0.2.1
send
END
grep "; Communication with 10.53.0.4#${PORT} failed: timed out" nsupdate.out-$n > /dev/null 2>&1 || ret=1
grep "not implemented" nsupdate.out-$n > /dev/null 2>&1 && ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "ensure bad owner name is fatal in non-interactive mode ($n)"
$NSUPDATE <<END > nsupdate.out 2>&1 && ret=1
    update add emptylabel..nil. 600 A 10.10.10.1
END
grep "invalid owner name: empty label" nsupdate.out > /dev/null || ret=1
grep "syntax error" nsupdate.out > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "ensure bad owner name is not fatal in interactive mode ($n)"
$NSUPDATE -i <<END > nsupdate.out 2>&1 || ret=1
    update add emptylabel..nil. 600 A 10.10.10.1
END
grep "invalid owner name: empty label" nsupdate.out > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "ensure invalid key type is fatal in non-interactive mode ($n)"
$NSUPDATE <<END > nsupdate.out 2>&1 && ret=1
    key badkeytype:example abcd12345678
END
grep "unknown key type 'badkeytype'" nsupdate.out > /dev/null || ret=1
grep "syntax error" nsupdate.out > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "ensure invalid key type is not fatal in interactive mode ($n)"
$NSUPDATE -i <<END > nsupdate.out 2>&1 || ret=1
    key badkeytype:example abcd12345678
END
grep "unknown key type 'badkeytype'" nsupdate.out > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "ensure unresolvable server name is fatal in non-interactive mode ($n)"
$NSUPDATE <<END > nsupdate.out 2>&1 && ret=1
    server unresolvable..
END
grep "couldn't get address for 'unresolvable..': not found" nsupdate.out > /dev/null || ret=1
grep "syntax error" nsupdate.out > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "ensure unresolvable server name is not fatal in interactive mode ($n)"
$NSUPDATE -i <<END > nsupdate.out 2>&1 || ret=1
    server unresolvable..
END
grep "couldn't get address for 'unresolvable..': not found" nsupdate.out > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "check nsupdate -4 -6 ($n)"
$NSUPDATE -4 -6 <<END > nsupdate.out-$n 2>&1 && ret=1
server 10.53.0.3 ${PORT}
zone delegation.test.
update del child.delegation.test. 3600 NS foo.example.net.
update del child.delegation.test. 3600 NS bar.example.net.
send
END
grep "only one of -4 and -6 allowed" nsupdate.out-$n > /dev/null 2>&1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "check nsupdate -4 with an IPv6 server address ($n)"
$NSUPDATE -4 <<END > nsupdate.out-$n 2>&1 && ret=1
server fd92:7065:b8e:ffff::2 ${PORT}
zone delegation.test.
update del child.delegation.test. 3600 NS foo.example.net.
update del child.delegation.test. 3600 NS bar.example.net.
send
END
grep "address family not supported" nsupdate.out-$n > /dev/null 2>&1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

n=`expr $n + 1`
ret=0
echo_i "check that TKEY in a update is rejected ($n)"
$NSUPDATE -d <<END > nsupdate.out-$n 2>&1 && ret=1
server 10.53.0.3 ${PORT}
update add tkey.example 0 in tkey invalid.algorithm. 1516055980 1516140801 1 0 16 gRof8D2BFKvl/vrr9Lmnjw== 16 gRof8D2BFKvl/vrr9Lmnjw==
send
END
grep "UPDATE, status: NOERROR" nsupdate.out-$n > /dev/null 2>&1 || ret=1
grep "UPDATE, status: FORMERR" nsupdate.out-$n > /dev/null 2>&1 || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }

if $FEATURETEST --gssapi ; then
  n=`expr $n + 1`
  ret=0
  echo_i "check krb5-self match ($n)"
  KRB5CCNAME="FILE:"`pwd`/ns7/machine.ccache
  export KRB5CCNAME
  $NSUPDATE << EOF > nsupdate.out-$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update add machine.example.com 3600 IN A 10.53.0.7
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.7 machine.example.com A > dig.out.ns7.test$n
  grep "status: NOERROR" dig.out.ns7.test$n > /dev/null || ret=1
  grep "machine.example.com..*A.*10.53.0.7" dig.out.ns7.test$n > /dev/null || ret=1
  [ $ret = 0 ] || { echo_i "failed"; status=1; }

  n=`expr $n + 1`
  ret=0
  echo_i "check krb5-self no-match ($n)"
  KRB5CCNAME="FILE:"`pwd`/ns7/machine.ccache
  export KRB5CCNAME
  $NSUPDATE << EOF > nsupdate.out-$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update add foo.example.com 3600 IN A 10.53.0.7
  send
EOF
  grep "update failed: REFUSED" nsupdate.out-$n > /dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.7 foo.example.com A > dig.out.ns7.test$n
  grep "status: NXDOMAIN" dig.out.ns7.test$n > /dev/null || ret=1
  [ $ret = 0 ] || { echo_i "failed"; status=1; }

  n=`expr $n + 1`
  ret=0
  echo_i "check krb5-subdomain match ($n)"
  KRB5CCNAME="FILE:"`pwd`/ns7/machine.ccache
  export KRB5CCNAME
  $NSUPDATE -d << EOF > nsupdate.out-$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update add _xxx._tcp.example.com 3600 IN SRV 0 0 0 machine.example.com
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.7 _xxx._tcp.example.com SRV > dig.out.ns7.test$n
  grep "status: NOERROR" dig.out.ns7.test$n > /dev/null || ret=1
  grep "_xxx._tcp.example.com.*SRV.*0 0 0 machine.example.com" dig.out.ns7.test$n > /dev/null || ret=1
  [ $ret = 0 ] || { echo_i "failed"; status=1; }

  n=`expr $n + 1`
  ret=0
  echo_i "check krb5-subdomain no-match ($n)"
  KRB5CCNAME="FILE:"`pwd`/ns7/machine.ccache
  export KRB5CCNAME
  $NSUPDATE << EOF > nsupdate.out-$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update add _xxx._udp.example.com 3600 IN SRV 0 0 0 machine.example.com
  send
EOF
  grep "update failed: REFUSED" nsupdate.out-$n > /dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.7 _xxx._udp.example.com SRV > dig.out.ns7.test$n
  grep "status: NXDOMAIN" dig.out.ns7.test$n > /dev/null || ret=1
  [ $ret = 0 ] || { echo_i "failed"; status=1; }

  n=`expr $n + 1`
  ret=0
  echo_i "check krb5-selfsub match ($n)"
  KRB5CCNAME="FILE:"`pwd`/ns8/machine.ccache
  export KRB5CCNAME
  $NSUPDATE -d << EOF > nsupdate.out-$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.8 ${PORT}
  zone example.com
  update add xxx.machine.example.com 3600 IN A 10.53.0.8
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.8 xxx.machine.example.com A > dig.out.ns8.test$n
  grep "status: NOERROR" dig.out.ns8.test$n > /dev/null || ret=1
  grep "xxx.machine.example.com..*A.*10.53.0.8" dig.out.ns8.test$n > /dev/null || ret=1
  [ $ret = 0 ] || { echo_i "failed"; status=1; }

  n=`expr $n + 1`
  ret=0
  echo_i "check krb5-selfsub no-match ($n)"
  KRB5CCNAME="FILE:"`pwd`/ns8/machine.ccache
  export KRB5CCNAME
  $NSUPDATE << EOF > nsupdate.out-$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.8 ${PORT}
  zone example.com
  update add foo.example.com 3600 IN A 10.53.0.8
  send
EOF
  grep "update failed: REFUSED" nsupdate.out-$n > /dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.8 foo.example.com A > dig.out.ns8.test$n
  grep "status: NXDOMAIN" dig.out.ns8.test$n > /dev/null || ret=1
  [ $ret = 0 ] || { echo_i "failed"; status=1; }

  n=`expr $n + 1`
  ret=0

  echo_i "check ms-self match ($n)"
  KRB5CCNAME="FILE:"`pwd`/ns9/machine.ccache
  export KRB5CCNAME
  $NSUPDATE << EOF > nsupdate.out-$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.9 ${PORT}
  zone example.com
  update add machine.example.com 3600 IN A 10.53.0.9
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.9 machine.example.com A > dig.out.ns9.test$n
  grep "status: NOERROR" dig.out.ns9.test$n > /dev/null || ret=1
  grep "machine.example.com..*A.*10.53.0.9" dig.out.ns9.test$n > /dev/null || ret=1
  [ $ret = 0 ] || { echo_i "failed"; status=1; }

  n=`expr $n + 1`
  ret=0
  echo_i "check ms-self no-match ($n)"
  KRB5CCNAME="FILE:"`pwd`/ns9/machine.ccache
  export KRB5CCNAME
  $NSUPDATE << EOF > nsupdate.out-$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.9 ${PORT}
  zone example.com
  update add foo.example.com 3600 IN A 10.53.0.9
  send
EOF
  grep "update failed: REFUSED" nsupdate.out-$n > /dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.9 foo.example.com A > dig.out.ns9.test$n
  grep "status: NXDOMAIN" dig.out.ns9.test$n > /dev/null || ret=1
  [ $ret = 0 ] || { echo_i "failed"; status=1; }

  n=`expr $n + 1`
  ret=0
  echo_i "check ms-subdomain match ($n)"
  KRB5CCNAME="FILE:"`pwd`/ns9/machine.ccache
  export KRB5CCNAME
  $NSUPDATE -d << EOF > nsupdate.out-$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.9 ${PORT}
  zone example.com
  update add _xxx._tcp.example.com 3600 IN SRV 0 0 0 machine.example.com
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.9 _xxx._tcp.example.com SRV > dig.out.ns9.test$n
  grep "status: NOERROR" dig.out.ns9.test$n > /dev/null || ret=1
  grep "_xxx._tcp.example.com.*SRV.*0 0 0 machine.example.com" dig.out.ns9.test$n > /dev/null || ret=1
  [ $ret = 0 ] || { echo_i "failed"; status=1; }

  n=`expr $n + 1`
  ret=0
  echo_i "check ms-subdomain no-match ($n)"
  KRB5CCNAME="FILE:"`pwd`/ns9/machine.ccache
  export KRB5CCNAME
  $NSUPDATE << EOF > nsupdate.out-$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.9 ${PORT}
  zone example.com
  update add _xxx._udp.example.com 3600 IN SRV 0 0 0 machine.example.com
  send
EOF
  grep "update failed: REFUSED" nsupdate.out-$n > /dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.9 _xxx._udp.example.com SRV > dig.out.ns9.test$n
  grep "status: NXDOMAIN" dig.out.ns9.test$n > /dev/null || ret=1
  [ $ret = 0 ] || { echo_i "failed"; status=1; }

  n=`expr $n + 1`
  ret=0
  echo_i "check ms-selfsub match ($n)"
  KRB5CCNAME="FILE:"`pwd`/ns10/machine.ccache
  export KRB5CCNAME
  $NSUPDATE -d << EOF > nsupdate.out-$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone example.com
  update add xxx.machine.example.com 3600 IN A 10.53.0.10
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.10 xxx.machine.example.com A > dig.out.ns10.test$n
  grep "status: NOERROR" dig.out.ns10.test$n > /dev/null || ret=1
  grep "xxx.machine.example.com..*A.*10.53.0.10" dig.out.ns10.test$n > /dev/null || ret=1
  [ $ret = 0 ] || { echo_i "failed"; status=1; }

  n=`expr $n + 1`
  ret=0
  echo_i "check ms-selfsub no-match ($n)"
  KRB5CCNAME="FILE:"`pwd`/ns10/machine.ccache
  export KRB5CCNAME
  $NSUPDATE << EOF > nsupdate.out-$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone example.com
  update add foo.example.com 3600 IN A 10.53.0.10
  send
EOF
  grep "update failed: REFUSED" nsupdate.out-$n > /dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.10 foo.example.com A > dig.out.ns10.test$n
  grep "status: NXDOMAIN" dig.out.ns10.test$n > /dev/null || ret=1
  [ $ret = 0 ] || { echo_i "failed"; status=1; }

fi
#
#  Add client library tests here
#

if test unset != "${SAMPLEUPDATE:-unset}" -a -x "${SAMPLEUPDATE}"
then

    n=`expr $n + 1`
    ret=0
    echo_i "check that dns_client_update handles prerequisite NXDOMAIN failure ($n)"
    $SAMPLEUPDATE -P ${PORT} -a 10.53.0.1 -a 10.53.0.2 -p "nxdomain exists.sample" \
	add "nxdomain-exists.sample 0 in a 1.2.3.4" > update.out.test$n 2>&1
    $SAMPLEUPDATE -P ${PORT} -a 10.53.0.2 -p "nxdomain exists.sample" \
	add "check-nxdomain-exists.sample 0 in a 1.2.3.4" > update.out.check$n 2>&1
    $DIG $DIGOPTS +tcp @10.53.0.1 a nxdomain-exists.sample > dig.out.ns1.test$n
    $DIG $DIGOPTS +tcp @10.53.0.2 a nxdomain-exists.sample > dig.out.ns2.test$n
    $DIG $DIGOPTS +tcp @10.53.0.2 a check-nxdomain-exists.sample > check.out.ns2.test$n
    grep "update failed: YXDOMAIN" update.out.test$n > /dev/null || ret=1
    grep "update succeeded" update.out.check$n > /dev/null || ret=1
    grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
    grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
    grep "status: NOERROR" check.out.ns2.test$n > /dev/null || ret=1
    [ $ret = 0 ] || { echo_i "failed"; status=1; }

    n=`expr $n + 1`
    ret=0
    echo_i "check that dns_client_update handles prerequisite YXDOMAIN failure ($n)"
    $SAMPLEUPDATE -P ${PORT} -a 10.53.0.1 -a 10.53.0.2 -p "yxdomain nxdomain.sample" \
	add "yxdomain-nxdomain.sample 0 in a 1.2.3.4" > update.out.test$n 2>&1
    $SAMPLEUPDATE -P ${PORT} -a 10.53.0.2 -p "yxdomain nxdomain.sample" \
	add "check-yxdomain-nxdomain.sample 0 in a 1.2.3.4" > update.out.check$n 2>&1
    $DIG $DIGOPTS +tcp @10.53.0.1 a nxdomain-exists.sample > dig.out.ns1.test$n
    $DIG $DIGOPTS +tcp @10.53.0.2 a nxdomain-exists.sample > dig.out.ns2.test$n
    $DIG $DIGOPTS +tcp @10.53.0.2 a check-nxdomain-exists.sample > check.out.ns2.test$n
    grep "update failed: NXDOMAIN" update.out.test$n > /dev/null || ret=1
    grep "update succeeded" update.out.check$n > /dev/null || ret=1
    grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
    grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
    grep "status: NOERROR" check.out.ns2.test$n > /dev/null || ret=1
    [ $ret = 0 ] || { echo_i "failed"; status=1; }

    n=`expr $n + 1`
    ret=0
    echo_i "check that dns_client_update handles prerequisite NXRRSET failure ($n)"
    $SAMPLEUPDATE -P ${PORT} -a 10.53.0.1 -a 10.53.0.2 -p "nxrrset exists.sample TXT This RRset exists." \
	add "nxrrset-exists.sample 0 in a 1.2.3.4" > update.out.test$n 2>&1
    $SAMPLEUPDATE -P ${PORT} -a 10.53.0.2 -p "nxrrset exists.sample TXT This RRset exists." \
	add "check-nxrrset-exists.sample 0 in a 1.2.3.4" > update.out.check$n 2>&1
    $DIG $DIGOPTS +tcp @10.53.0.1 a nxrrset-exists.sample > dig.out.ns1.test$n
    $DIG $DIGOPTS +tcp @10.53.0.2 a nxrrset-exists.sample > dig.out.ns2.test$n
    $DIG $DIGOPTS +tcp @10.53.0.2 a check-nxrrset-exists.sample > check.out.ns2.test$n
    grep "update failed: YXRRSET" update.out.test$n > /dev/null || ret=1
    grep "update succeeded" update.out.check$n > /dev/null || ret=1
    grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
    grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
    grep "status: NOERROR" check.out.ns2.test$n > /dev/null || ret=1
    [ $ret = 0 ] || { echo_i "failed"; status=1; }

    n=`expr $n + 1`
    ret=0
    echo_i "check that dns_client_update handles prerequisite YXRRSET failure ($n)"
    $SAMPLEUPDATE -s -P ${PORT} -a 10.53.0.1 -a 10.53.0.2 \
	-p "yxrrset no-txt.sample TXT" \
	add "yxrrset-nxrrset.sample 0 in a 1.2.3.4" > update.out.test$n 2>&1
    $SAMPLEUPDATE -P ${PORT} -a 10.53.0.2 -p "yxrrset no-txt.sample TXT" \
	add "check-yxrrset-nxrrset.sample 0 in a 1.2.3.4" > update.out.check$n 2>&1
    $DIG $DIGOPTS +tcp @10.53.0.1 a yxrrset-nxrrset.sample > dig.out.ns1.test$n
    $DIG $DIGOPTS +tcp @10.53.0.2 a yxrrset-nxrrset.sample > dig.out.ns2.test$n
    $DIG $DIGOPTS +tcp @10.53.0.2 a check-yxrrset-nxrrset.sample > check.out.ns2.test$n
    grep "update failed: NXRRSET" update.out.test$n > /dev/null || ret=1
    grep "update succeeded" update.out.check$n > /dev/null || ret=1
    grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
    grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
    grep "status: NOERROR" check.out.ns2.test$n > /dev/null || ret=1
    grep "2nd update failed: NXRRSET" update.out.test$n > /dev/null || ret=1
    [ $ret = 0 ] || { echo_i "failed"; status=1; }

fi

#
# End client library tests here
#

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
