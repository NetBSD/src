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

set -e

. ../conf.sh

DIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC -c ../_common/rndc.conf -p ${CONTROLPORT} -s"

#
# Uncomment when creating credential cache files.
#
# KRB5_CONFIG="$(pwd)/krb/krb5.conf"
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

nextpartreset ns3/named.run

# wait for zone transfer to complete
tries=0
while true; do
  if [ $tries -eq 10 ]; then
    exit 1
  fi

  if grep "example.nil/IN.*Transfer status" ns2/named.run >/dev/null; then
    break
  else
    echo_i "zones are not fully loaded, waiting..."
    tries=$((tries + 1))
    sleep 1
  fi
done

has_positive_response() {
  zone=$1
  type=$2
  ns=$3
  $DIG $DIGOPTS +tcp +norec $zone $type @$ns >dig.out.post.test$n || return 1
  grep "status: NOERROR" dig.out.post.test$n >/dev/null || return 1
  grep "ANSWER: 0," dig.out.post.test$n >/dev/null && return 1
  return 0
}

ret=0
echo_i "fetching first copy of zone before update"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil. @10.53.0.1 axfr >dig.out.ns1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "fetching second copy of zone before update"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil. @10.53.0.2 axfr >dig.out.ns2 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "comparing pre-update copies to known good data"
digcomp knowngood.ns1.before dig.out.ns1 || ret=1
digcomp knowngood.ns1.before dig.out.ns2 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "ensure an unrelated zone is mentioned in its NOTAUTH log"
$NSUPDATE -k ns1/ddns.key >nsupdate.out 2>&1 <<END && ret=1
server 10.53.0.1 ${PORT}
zone unconfigured.test
update add unconfigured.test 600 IN A 10.53.0.1
send
END
grep NOTAUTH nsupdate.out >/dev/null 2>&1 || ret=1
grep ' unconfigured.test: not authoritative' ns1/named.run \
  >/dev/null 2>&1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "ensure a subdomain is mentioned in its NOTAUTH log"
$NSUPDATE -k ns1/ddns.key >nsupdate.out 2>&1 <<END && ret=1
server 10.53.0.1 ${PORT}
zone sub.sub.example.nil
update add sub.sub.sub.example.nil 600 IN A 10.53.0.1
send
END
grep NOTAUTH nsupdate.out >/dev/null 2>&1 || ret=1
grep ' sub.sub.example.nil: not authoritative' ns1/named.run \
  >/dev/null 2>&1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "updating zone"
# nsupdate will print a ">" prompt to stdout as it gets each input line.
$NSUPDATE -k ns1/ddns.key <<END >/dev/null || ret=1
server 10.53.0.1 ${PORT}
update add updated.example.nil. 600 A 10.10.10.1
add updated.example.nil. 600 TXT Foo
delete t.example.nil.

END
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

echo_i "sleeping 5 seconds for server to incorporate changes"
sleep 5

ret=0
echo_i "fetching first copy of zone after update"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil. @10.53.0.1 axfr >dig.out.ns1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "fetching second copy of zone after update"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil. @10.53.0.2 axfr >dig.out.ns2 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "comparing post-update copies to known good data"
digcomp knowngood.ns1.after dig.out.ns1 || ret=1
digcomp knowngood.ns1.after dig.out.ns2 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "testing local update policy"
pre=$($DIG $DIGOPTS +short new.other.nil. @10.53.0.1 a) || ret=1
[ -z "$pre" ] || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "updating zone"
# nsupdate will print a ">" prompt to stdout as it gets each input line.
$NSUPDATE -4 -l -p ${PORT} -k ns1/session.key >/dev/null <<END || ret=1
zone other.nil.
update add new.other.nil. 600 IN A 10.10.10.1
send
END
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

echo_i "sleeping 5 seconds for server to incorporate changes"
sleep 5

ret=0
echo_i "checking result of update"
post=$($DIG $DIGOPTS +short new.other.nil. @10.53.0.1 a) || ret=1
[ "$post" = "10.10.10.1" ] || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "comparing post-update copy to known good data"
digcomp knowngood.ns1.after dig.out.ns1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "testing zone consistency checks"
# inserting an NS record without a corresponding A or AAAA record should fail
$NSUPDATE -4 -l -p ${PORT} -k ns1/session.key >nsupdate.out 2>&1 <<END && ret=1
update add other.nil. 600 in ns ns3.other.nil.
send
END
grep REFUSED nsupdate.out >/dev/null 2>&1 || ret=1
# ...but should work if an A record is inserted first:
$NSUPDATE -4 -l -p ${PORT} -k ns1/session.key >nsupdate.out 2>&1 <<END || ret=1
update add ns4.other.nil 600 in a 10.53.0.1
send
update add other.nil. 600 in ns ns4.other.nil.
send
END
grep REFUSED nsupdate.out >/dev/null 2>&1 && ret=1
# ...or if an AAAA record does:
$NSUPDATE -4 -l -p ${PORT} -k ns1/session.key >nsupdate.out 2>&1 <<END || ret=1
update add ns5.other.nil 600 in aaaa 2001:db8::1
send
update add other.nil. 600 in ns ns5.other.nil.
send
END
grep REFUSED nsupdate.out >/dev/null 2>&1 && ret=1
# ...or if the NS and A/AAAA are inserted together:
$NSUPDATE -4 -l -p ${PORT} -k ns1/session.key >nsupdate.out 2>&1 <<END || ret=1
update add other.nil. 600 in ns ns6.other.nil.
update add ns6.other.nil 600 in a 10.53.0.1
send
END
grep REFUSED nsupdate.out >/dev/null 2>&1 && ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

echo_i "sleeping 5 seconds for server to incorporate changes"
sleep 5

ret=0
echo_i "checking result of update"
$DIG $DIGOPTS +short @10.53.0.1 ns other.nil >dig.out.ns1 || ret=1
grep ns3.other.nil dig.out.ns1 >/dev/null 2>&1 && ret=1
grep ns4.other.nil dig.out.ns1 >/dev/null 2>&1 || ret=1
grep ns5.other.nil dig.out.ns1 >/dev/null 2>&1 || ret=1
grep ns6.other.nil dig.out.ns1 >/dev/null 2>&1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "ensure 'check-mx ignore' allows adding MX records containing an address without a warning"
$NSUPDATE -k ns1/ddns.key >nsupdate.out 2>&1 <<END || ret=1
server 10.53.0.1 ${PORT}
update add mx03.example.nil 600 IN MX 10 10.53.0.1
send
END
grep REFUSED nsupdate.out >/dev/null 2>&1 && ret=1
grep "mx03.example.nil/MX:.*MX is an address" ns1/named.run >/dev/null 2>&1 && ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "ensure 'check-mx warn' allows adding MX records containing an address with a warning"
$NSUPDATE -4 -l -p ${PORT} -k ns1/session.key >nsupdate.out 2>&1 <<END || ret=1
update add mx03.other.nil 600 IN MX 10 10.53.0.1
send
END
grep REFUSED nsupdate.out >/dev/null 2>&1 && ret=1
grep "mx03.other.nil/MX:.*MX is an address" ns1/named.run >/dev/null 2>&1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "ensure 'check-mx fail' prevents adding MX records containing an address with a warning"
$NSUPDATE >nsupdate.out 2>&1 <<END && ret=1
server 10.53.0.1 ${PORT}
update add mx03.update.nil 600 IN MX 10 10.53.0.1
send
END
grep REFUSED nsupdate.out >/dev/null 2>&1 || ret=1
grep "mx03.update.nil/MX:.*MX is an address" ns1/named.run >/dev/null 2>&1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "check SIG(0) key is accepted"
key=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -T KEY -n ENTITY xxx)
echo "" | $NSUPDATE -k ${key}.private >/dev/null 2>&1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check TYPE=0 update is rejected by nsupdate ($n)"
$NSUPDATE <<END >nsupdate.out 2>&1 && ret=1
    server 10.53.0.1 ${PORT}
    ttl 300
    update add example.nil. in type0 ""
    send
END
grep "unknown class/type" nsupdate.out >/dev/null 2>&1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check TYPE=0 prerequisite is handled ($n)"
$NSUPDATE -k ns1/ddns.key <<END >nsupdate.out 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    prereq nxrrset example.nil. type0
    send
END
$DIG $DIGOPTS +tcp version.bind txt ch @10.53.0.1 >dig.out.ns1.$n || ret=1
grep "status: NOERROR" dig.out.ns1.$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that TYPE=0 update is handled ($n)"
echo "a0e4280000010000000100000000060001c00c000000fe000000000000" \
  | $PERL ../packet.pl -a 10.53.0.1 -p ${PORT} -t tcp >/dev/null || ret=1
$DIG $DIGOPTS +tcp version.bind txt ch @10.53.0.1 >dig.out.ns1.$n || ret=1
grep "status: NOERROR" dig.out.ns1.$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that TYPE=0 additional data is handled ($n)"
echo "a0e4280000010000000000010000060001c00c000000fe000000000000" \
  | $PERL ../packet.pl -a 10.53.0.1 -p ${PORT} -t tcp >/dev/null || ret=1
$DIG $DIGOPTS +tcp version.bind txt ch @10.53.0.1 >dig.out.ns1.$n || ret=1
grep "status: NOERROR" dig.out.ns1.$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that update to undefined class is handled ($n)"
echo "a0e4280000010001000000000000060101c00c000000fe000000000000" \
  | $PERL ../packet.pl -a 10.53.0.1 -p ${PORT} -t tcp >/dev/null || ret=1
$DIG $DIGOPTS +tcp version.bind txt ch @10.53.0.1 >dig.out.ns1.$n || ret=1
grep "status: NOERROR" dig.out.ns1.$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that address family mismatch is handled ($n)"
$NSUPDATE <<END >/dev/null 2>&1 && ret=1
server ::1
local 127.0.0.1
update add 600 txt.example.nil in txt "test"
send
END
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that unixtime serial number is correctly generated ($n)"
$DIG $DIGOPTS +short unixtime.nil. soa @10.53.0.1 >dig.out.old.test$n || ret=1
oldserial=$(awk '{print $3}' dig.out.old.test$n) || ret=1
start=$($PERL -e 'print time()."\n";')
$NSUPDATE <<END >/dev/null 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    ttl 600
    update add new.unixtime.nil in a 1.2.3.4
    send
END
now=$($PERL -e 'print time()."\n";')
sleep 1
$DIG $DIGOPTS +short unixtime.nil. soa @10.53.0.1 >dig.out.new.test$n || ret=1
serial=$(awk '{print $3}' dig.out.new.test$n) || ret=1
[ "$oldserial" = "$serial" ] && {
  echo_i "oldserial == serial"
  ret=1
}
if [ "$serial" -lt "$start" ]; then
  echo_i "out-of-range serial=$serial < start=$start"
  ret=1
elif [ "$serial" -gt "$now" ]; then
  echo_i "out-of-range serial=$serial > now=$now"
  ret=1
fi
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

if $PERL -e 'use Net::DNS;' 2>/dev/null; then
  n=$((n + 1))
  ret=0
  echo_i "running update.pl test ($n)"
  $PERL update_test.pl -s 10.53.0.1 -p ${PORT} update.nil. >perl.update_test.out || ret=1
  [ $ret -eq 1 ] && {
    echo_i "failed"
    status=1
  }

  if $PERL -e 'use Net::DNS; die "Net::DNS too old ($Net::DNS::VERSION < 1.01)" if ($Net::DNS::VERSION < 1.01)' >/dev/null; then
    n=$((n + 1))
    ret=0
    echo_i "check for too many NSEC3 iterations log ($n)"
    grep "updating zone 'update.nil/IN': too many NSEC3 iterations (51)" ns1/named.run >/dev/null || ret=1
    [ $ret -eq 1 ] && {
      echo_i "failed"
      status=1
    }
  fi
else
  echo_i "The second part of this test requires the Net::DNS library." >&2
fi

n=$((n + 1))
ret=0
echo_i "fetching first copy of test zone ($n)"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil. @10.53.0.1 axfr >dig.out.ns1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "fetching second copy of test zone ($n)"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil. @10.53.0.2 axfr >dig.out.ns2 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "comparing zones ($n)"
digcomp dig.out.ns1 dig.out.ns2 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

echo_i "SIGKILL and restart server ns1"
cd ns1
kill -KILL $(cat named.pid)
rm named.pid
cd ..
sleep 10
if
  start_server --noclean --restart --port ${PORT} ns1
then
  echo_i "restarted server ns1"
else
  echo_i "could not restart server ns1"
  exit 1
fi
sleep 10

n=$((n + 1))
ret=0
echo_i "fetching ns1 after hard restart ($n)"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd example.nil. @10.53.0.1 axfr >dig.out.ns1.after || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "comparing zones ($n)"
digcomp dig.out.ns1 dig.out.ns1.after || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

echo_i "begin RT #482 regression test"

n=$((n + 1))
ret=0
echo_i "update primary ($n)"
$NSUPDATE -k ns1/ddns.key <<END >/dev/null || ret=1
server 10.53.0.1 ${PORT}
update add updated2.example.nil. 600 A 10.10.10.2
update add updated2.example.nil. 600 TXT Bar
update delete c.example.nil.
send
END
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

sleep 5

echo_i "SIGHUP secondary"
kill -HUP $(cat ns2/named.pid)

sleep 5

n=$((n + 1))
ret=0
echo_i "update primary again ($n)"
$NSUPDATE -k ns1/ddns.key <<END >/dev/null || ret=1
server 10.53.0.1 ${PORT}
update add updated3.example.nil. 600 A 10.10.10.3
update add updated3.example.nil. 600 TXT Zap
del d.example.nil.
send
END
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

sleep 5

echo_i "SIGHUP secondary again"
kill -HUP $(cat ns2/named.pid)

sleep 5

n=$((n + 1))
echo_i "check to 'out of sync' message ($n)"
if grep "out of sync" ns2/named.run >/dev/null; then
  echo_i "failed (found 'out of sync')"
  status=1
fi

echo_i "end RT #482 regression test"

n=$((n + 1))
ret=0
echo_i "remove nonexistent PTR record ($n)"
$NSUPDATE -k ns1/ddns.key -d <<EOF >nsupdate.out.test$n 2>&1 || ret=1
server 10.53.0.1 ${PORT}
zone example.nil.
update delete nonexistent.example.nil. 0 IN PTR foo.
send
EOF
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "remove nonexistent SRV record ($n)"
$NSUPDATE -k ns1/ddns.key -d <<EOF >nsupdate.out.test$n 2>&1 || ret=1
server 10.53.0.1 ${PORT}
zone example.nil.
update delete nonexistent.example.nil. 0 IN SRV 0 0 0 foo.
send
EOF
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
i=0
echo_i "check that nsupdate does not hang when processing a large number of updates interactively ($n)"
{
  echo "server 10.53.0.3 ${PORT}"
  echo "zone many-updates.test."
  while [ $i -le 2000 ]; do
    echo "update add host$i.many-updates.test. 3600 IN TXT \"host $i\""
    i=$((i + 1))
  done
  echo "send"
} | $NSUPDATE
echo_i "query for host2000.many-updates.test ($n)"
retry_quiet 5 has_positive_response host2000.many-updates.test TXT 10.53.0.3 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "start NSEC3PARAM changes via UPDATE on a unsigned zone test ($n)"
$NSUPDATE <<EOF
server 10.53.0.3 ${PORT}
update add example 3600 nsec3param 1 0 0 -
send
EOF

# the zone is not signed.  The nsec3param records should be removed.
# this also proves that the server is still running.
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocmd +norec example. @10.53.0.3 nsec3param >dig.out.ns3.$n || ret=1
grep "ANSWER: 0," dig.out.ns3.$n >/dev/null || ret=1
grep "flags:[^;]* aa[ ;]" dig.out.ns3.$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "change the NSEC3PARAM ttl via update ($n)"
$NSUPDATE <<EOF
server 10.53.0.3 ${PORT}
update add nsec3param.test 3600 NSEC3PARAM 1 0 1 -
send
EOF

$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocmd +norec nsec3param.test. @10.53.0.3 nsec3param >dig.out.ns3.$n || ret=1
grep "ANSWER: 1," dig.out.ns3.$n >/dev/null || ret=1
grep "3600.*NSEC3PARAM" dig.out.ns3.$n >/dev/null || ret=1
grep "flags:[^;]* aa[ ;]" dig.out.ns3.$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "testing that rndc stop updates the file"
$NSUPDATE -k ns1/ddns.key <<END >/dev/null || ret=1
server 10.53.0.1 ${PORT}
update add updated4.example.nil. 600 A 10.10.10.3
send
END
sleep 3
stop_server --use-rndc --port ${CONTROLPORT} ns1
sleep 3
# Removing the journal file and restarting the server means
# that the data served by the new server process are exactly
# those dumped to the file by "rndc stop".
rm -f ns1/*jnl
start_server --noclean --restart --port ${PORT} ns1
for try in 0 1 2 3 4 5 6 7 8 9; do
  iret=0
  $DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
    updated4.example.nil. @10.53.0.1 a >dig.out.ns1 || iret=1
  digcomp knowngood.ns1.afterstop dig.out.ns1 || iret=1
  [ "$iret" -eq 0 ] && break
  sleep 1
done
[ "$iret" -ne 0 ] && ret=1
[ "$ret" -eq 0 ] || {
  echo_i "failed"
  status=1
}

ret=0
echo_i "check that 'nsupdate -l' with a missing keyfile reports the missing file"
$NSUPDATE -4 -p ${PORT} -l -k ns1/nonexistent.key 2>nsupdate.out </dev/null && ret=1
grep ns1/nonexistent.key nsupdate.out >/dev/null || ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check that 'update-policy local' works from localhost address ($n)"
$NSUPDATE -k ns5/session.key >nsupdate.out.$n 2>&1 <<END || ret=1
server 10.53.0.5 ${PORT}
local 127.0.0.1
update add fromlocal.local.nil. 600 A 1.2.3.4
send
END
grep REFUSED nsupdate.out.$n >/dev/null 2>&1 && ret=1
$DIG $DIGOPTS @10.53.0.5 \
  +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
  fromlocal.local.nil. >dig.out.ns5.$n || ret=1
grep fromlocal dig.out.ns5.$n >/dev/null 2>&1 || ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check that 'update-policy local' fails from non-localhost address ($n)"
grep 'match on session key not from localhost' ns5/named.run >/dev/null && ret=1
$NSUPDATE -k ns5/session.key >nsupdate.out.$n 2>&1 <<END && ret=1
server 10.53.0.5 ${PORT}
local 10.53.0.1
update add nonlocal.local.nil. 600 A 4.3.2.1
send
END
grep REFUSED nsupdate.out.$n >/dev/null 2>&1 || ret=1
grep 'match on session key not from localhost' ns5/named.run >/dev/null || ret=1
$DIG $DIGOPTS @10.53.0.5 \
  +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
  nonlocal.local.nil. >dig.out.ns5.$n || ret=1
grep nonlocal dig.out.ns5.$n >/dev/null 2>&1 && ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check that 'update-policy tcp-self' refuses update of records via UDP ($n)"
$NSUPDATE >nsupdate.out.$n 2>&1 <<END && ret=1
server 10.53.0.6 ${PORT}
local 127.0.0.1
update add 1.0.0.127.in-addr.arpa. 600 PTR localhost.
send
END
grep REFUSED nsupdate.out.$n >/dev/null 2>&1 || ret=1
$DIG $DIGOPTS @10.53.0.6 \
  +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
  -x 127.0.0.1 >dig.out.ns6.$n
grep localhost. dig.out.ns6.$n >/dev/null 2>&1 && ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check that 'update-policy tcp-self' permits update of records for the client's own address via TCP ($n)"
$NSUPDATE -v >nsupdate.out.$n 2>&1 <<END || ret=1
server 10.53.0.6 ${PORT}
local 127.0.0.1
update add 1.0.0.127.in-addr.arpa. 600 PTR localhost.
send
END
grep REFUSED nsupdate.out.$n >/dev/null 2>&1 && ret=1
$DIG $DIGOPTS @10.53.0.6 \
  +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
  -x 127.0.0.1 >dig.out.ns6.$n || ret=1
grep localhost. dig.out.ns6.$n >/dev/null 2>&1 || ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check that 'update-policy tcp-self' refuses update of records for a different address from the client's own address via TCP ($n)"
$NSUPDATE -v >nsupdate.out.$n 2>&1 <<END && ret=1
server 10.53.0.6 ${PORT}
local 127.0.0.1
update add 1.0.168.192.in-addr.arpa. 600 PTR localhost.
send
END
grep REFUSED nsupdate.out.$n >/dev/null 2>&1 || ret=1
$DIG $DIGOPTS @10.53.0.6 \
  +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
  -x 192.168.0.1 >dig.out.ns6.$n
grep localhost. dig.out.ns6.$n >/dev/null 2>&1 && ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check that 'update-policy 6to4-self' refuses update of records via UDP over IPv4 ($n)"
REVERSE_NAME=6.0.0.0.5.3.a.0.2.0.0.2.ip6.arpa
$NSUPDATE >nsupdate.out.$n 2>&1 <<END && ret=1
server 10.53.0.6 ${PORT}
local 10.53.0.6
zone 2.0.0.2.ip6.arpa
update add ${REVERSE_NAME} 600 NS localhost.
send
END
grep REFUSED nsupdate.out.$n >/dev/null 2>&1 || ret=1
$DIG $DIGOPTS @10.53.0.6 \
  +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
  $REVERSE_NAME NS >dig.out.ns6.$n
grep localhost. dig.out.ns6.$n >/dev/null 2>&1 && ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
echo_i "check that 'update-policy 6to4-self' permits update of records for the client's own address via TCP over IPv4 ($n)"
ret=0
REVERSE_NAME=6.0.0.0.5.3.a.0.2.0.0.2.ip6.arpa
$NSUPDATE -v >nsupdate.out.$n 2>&1 <<END || ret=1
server 10.53.0.6 ${PORT}
local 10.53.0.6
zone 2.0.0.2.ip6.arpa
update add ${REVERSE_NAME} 600 NS localhost.
send
END
grep REFUSED nsupdate.out.$n >/dev/null 2>&1 && ret=1
$DIG $DIGOPTS @10.53.0.6 \
  +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
  $REVERSE_NAME NS >dig.out.ns6.$n || ret=1
grep localhost. dig.out.ns6.$n >/dev/null 2>&1 || ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check that 'update-policy 6to4-self' refuses update of records via UDP over IPv6 ($n)"
REVERSE_NAME=7.0.0.0.5.3.a.0.2.0.0.2.ip6.arpa
$NSUPDATE >nsupdate.out.$n 2>&1 <<END && ret=1
server fd92:7065:b8e:ffff::6 ${PORT}
local 2002:a35:7::1
zone 2.0.0.2.ip6.arpa
update add ${REVERSE_NAME} 600 NS localhost.
send
END
grep REFUSED nsupdate.out.$n >/dev/null 2>&1 || ret=1
$DIG $DIGOPTS @fd92:7065:b8e:ffff::6 \
  +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
  $REVERSE_NAME NS >dig.out.ns6.$n
grep localhost. dig.out.ns6.$n >/dev/null 2>&1 && ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
echo_i "check that 'update-policy 6to4-self' permits update of records for the client's own address via TCP over IPv6 ($n)"
ret=0
REVERSE_NAME=7.0.0.0.5.3.a.0.2.0.0.2.ip6.arpa
$NSUPDATE -v >nsupdate.out.$n 2>&1 <<END || ret=1
server fd92:7065:b8e:ffff::6 ${PORT}
local 2002:a35:7::1
zone 2.0.0.2.ip6.arpa
update add ${REVERSE_NAME} 600 NS localhost.
send
END
grep REFUSED nsupdate.out.$n >/dev/null 2>&1 && ret=1
$DIG $DIGOPTS @fd92:7065:b8e:ffff::6 \
  +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
  $REVERSE_NAME NS >dig.out.ns6.$n || ret=1
grep localhost. dig.out.ns6.$n >/dev/null 2>&1 || ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check that 'update-policy subdomain' is properly enforced ($n)"
# "restricted.example.nil" matches "grant ... subdomain restricted.example.nil"
# and thus this UPDATE should succeed.
$NSUPDATE -d <<END >nsupdate.out1-$n 2>&1 || ret=1
server 10.53.0.1 ${PORT}
key $DEFAULT_HMAC:restricted.example.nil 1234abcd8765
update add restricted.example.nil 0 IN TXT everywhere.
send
END
$DIG $DIGOPTS +tcp @10.53.0.1 restricted.example.nil TXT >dig.out.1.test$n || ret=1
grep "TXT.*everywhere" dig.out.1.test$n >/dev/null || ret=1
# "example.nil" does not match "grant ... subdomain restricted.example.nil" and
# thus this UPDATE should fail.
$NSUPDATE -d <<END >nsupdate.out2-$n 2>&1 && ret=1
server 10.53.0.1 ${PORT}
key $DEFAULT_HMAC:restricted.example.nil 1234abcd8765
update add example.nil 0 IN TXT everywhere.
send
END
$DIG $DIGOPTS +tcp @10.53.0.1 example.nil TXT >dig.out.2.test$n || ret=1
grep "TXT.*everywhere" dig.out.2.test$n >/dev/null && ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that 'update-policy zonesub' is properly enforced ($n)"
# grant zonesub-key.example.nil zonesub TXT;
# the A record update should be rejected as it is not in the type list
$NSUPDATE -d <<END >nsupdate.out1-$n 2>&1 && ret=1
server 10.53.0.1 ${PORT}
key $DEFAULT_HMAC:zonesub-key.example.nil 1234subk8765
update add zonesub.example.nil 0 IN A 1.2.3.4
send
END
$DIG $DIGOPTS +tcp @10.53.0.1 zonesub.example.nil A >dig.out.1.test$n || ret=1
grep "status: REFUSED" nsupdate.out1-$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.1.test$n >/dev/null || ret=1
# the TXT record update should be accepted as it is in the type list
$NSUPDATE -d <<END >nsupdate.out2-$n 2>&1 || ret=1
server 10.53.0.1 ${PORT}
key $DEFAULT_HMAC:zonesub-key.example.nil 1234subk8765
update add zonesub.example.nil 0 IN TXT everywhere.
send
END
$DIG $DIGOPTS +tcp @10.53.0.1 zonesub.example.nil TXT >dig.out.2.test$n || ret=1
grep "status: REFUSED" nsupdate.out2-$n >/dev/null && ret=1
grep "ANSWER: 1," dig.out.2.test$n >/dev/null || ret=1
grep "TXT.*everywhere" dig.out.2.test$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check 'grant' in deny name + grant subdomain ($n)"
$NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 || ret=1
key $DEFAULT_HMAC:subkey 1234abcd8765
server 10.53.0.9 ${PORT}
zone denyname.example
update add foo.denyname.example 3600 IN TXT added
send
EOF
$DIG $DIGOPTS +tcp @10.53.0.9 foo.denyname.example TXT >dig.out.ns9.test$n || ret=1
grep "added" dig.out.ns9.test$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check 'deny' in deny name + grant subdomain ($n)"
$NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
key $DEFAULT_HMAC:subkey 1234abcd8765
server 10.53.0.9 ${PORT}
zone denyname.example
update add denyname.example 3600 IN TXT added
send
EOF
$DIG $DIGOPTS +tcp @10.53.0.9 denyname.example TXT >dig.out.ns9.test$n || ret=1
grep "added" dig.out.ns9.test$n >/dev/null && ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that changes to the DNSKEY RRset TTL do not have side effects ($n)"
$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd dnskey.test. \
  @10.53.0.3 dnskey \
  | awk -v port="${PORT}" 'BEGIN { print "server 10.53.0.3", port; }
	$2 == 10 && $3 == "IN" && $4 == "DNSKEY" { $2 = 600; print "update add", $0 }
	END { print "send" }' >update.in.$n || ret=1
$NSUPDATE update.in.$n

$DIG $DIGOPTS +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd dnskey.test. \
  @10.53.0.3 any >dig.out.ns3.$n || ret=1

grep "600.*DNSKEY" dig.out.ns3.$n >/dev/null || ret=1
grep TYPE65534 dig.out.ns3.$n >/dev/null && ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
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

n=$((n + 1))
ret=0
echo_i "check type list options ($n)"
$NSUPDATE -T >typelist.out.T.${n} || {
  ret=1
  echo_i "nsupdate -T failed"
}
$NSUPDATE -P >typelist.out.P.${n} || {
  ret=1
  echo_i "nsupdate -P failed"
}
$NSUPDATE -TP >typelist.out.TP.${n} || {
  ret=1
  echo_i "nsupdate -TP failed"
}
grep ANY typelist.out.T.${n} >/dev/null && {
  ret=1
  echo_i "failed: ANY found (-T)"
}
grep ANY typelist.out.P.${n} >/dev/null && {
  ret=1
  echo_i "failed: ANY found (-P)"
}
grep ANY typelist.out.TP.${n} >/dev/null && {
  ret=1
  echo_i "failed: ANY found (-TP)"
}
grep KEYDATA typelist.out.T.${n} >/dev/null && {
  ret=1
  echo_i "failed: KEYDATA found (-T)"
}
grep KEYDATA typelist.out.P.${n} >/dev/null && {
  ret=1
  echo_i "failed: KEYDATA found (-P)"
}
grep KEYDATA typelist.out.TP.${n} >/dev/null && {
  ret=1
  echo_i "failed: KEYDATA found (-TP)"
}
grep AAAA typelist.out.T.${n} >/dev/null || {
  ret=1
  echo_i "failed: AAAA not found (-T)"
}
grep AAAA typelist.out.P.${n} >/dev/null && {
  ret=1
  echo_i "failed: AAAA found (-P)"
}
grep AAAA typelist.out.TP.${n} >/dev/null || {
  ret=1
  echo_i "failed: AAAA not found (-TP)"
}
if [ $ret -ne 0 ]; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check command list ($n)"
(
  while read cmd; do
    {
      echo "$cmd" | $NSUPDATE >/dev/null 2>&1
      rc=$?
    } || true
    if test $rc -gt 1; then
      echo_i "failed ($cmd)"
      ret=1
    fi
    {
      echo "$cmd " | $NSUPDATE >/dev/null 2>&1
      rc=$?
    } || true
    if test $rc -gt 1; then
      echo_i "failed ($cmd)"
      ret=1
    fi
  done
  exit $ret
) <commandlist || ret=1
if [ $ret -ne 0 ]; then
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check DoT (opportunistic-tls) ($n)"
if $FEATURETEST --have-fips-dh; then
  $NSUPDATE -D -S -O -k ns1/ddns.key <<END >nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${TLSPORT}
    update add dot-non-auth-client-o.example.nil. 600 A 10.10.10.3
    send
END
  sleep 2
  $DIG $DIGOPTS +short @10.53.0.1 dot-non-auth-client-o.example.nil >dig.out.test$n 2>&1 || ret=1
  grep -F "10.10.10.3" dig.out.test$n >/dev/null 2>&1 || ret=1
  if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
  fi
else
  echo_i "skipped: DH not supported in FIPS mode"
fi

n=$((n + 1))
ret=0
echo_i "check DoT (strict-tls) with an implicit hostname (by IP address) ($n)"
if $FEATURETEST --have-fips-dh; then
  $NSUPDATE -D -S -A CA/CA.pem -k ns1/ddns.key <<END >nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${EXTRAPORT1}
    update add dot-non-auth-client.example.nil. 600 A 10.10.10.3
    send
END
  sleep 2
  $DIG $DIGOPTS +short @10.53.0.1 dot-non-auth-client.example.nil >dig.out.test$n 2>&1 || ret=1
  grep -F "10.10.10.3" dig.out.test$n >/dev/null 2>&1 || ret=1
  if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
  fi
else
  echo_i "skipped: DH not supported in FIPS mode"
fi

n=$((n + 1))
ret=0
echo_i "check DoT (strict-tls) with an implicit hostname (by IP address) ($n)"
if $FEATURETEST --have-fips-dh; then
  $NSUPDATE -D -S -A CA/CA.pem -k ns1/ddns.key <<END >nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${EXTRAPORT1}
    update add dot-fs.example.nil. 600 A 10.10.10.3
    send
END
  sleep 2
  $DIG $DIGOPTS +short @10.53.0.1 dot-fs.example.nil >dig.out.test$n 2>&1 || ret=1
  grep -F "10.10.10.3" dig.out.test$n >/dev/null 2>&1 || ret=1
  if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
  fi
else
  echo_i "skipped: DH not supported in FIPS mode"
fi

n=$((n + 1))
ret=0
echo_i "check DoT (strict-tls) with a correct hostname ($n)"
if $FEATURETEST --have-fips-dh; then
  $NSUPDATE -D -S -A CA/CA.pem -H srv01.crt01.example.nil -k ns1/ddns.key <<END >nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${EXTRAPORT1}
    update add dot-fs-h.example.nil. 600 A 10.10.10.3
    send
END
  sleep 2
  $DIG $DIGOPTS +short @10.53.0.1 dot-fs-h.example.nil >dig.out.test$n 2>&1 || ret=1
  grep -F "10.10.10.3" dig.out.test$n >/dev/null 2>&1 || ret=1
  if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
  fi
else
  echo_i "skipped: DH not supported in FIPS mode"
fi

n=$((n + 1))
ret=0
echo_i "check DoT (strict-tls) with an incorrect hostname (failure expected) ($n)"
if $FEATURETEST --have-fips-dh; then
  $NSUPDATE -D -S -A CA/CA.pem -H srv01.crt01.example.bad -k ns1/ddns.key <<END >nsupdate.out.test$n 2>&1 && ret=1
    server 10.53.0.1 ${EXTRAPORT1}
    update add dot-fs-h-bad.example.nil. 600 A 10.10.10.3
    send
END
  sleep 2
  $DIG $DIGOPTS +short @10.53.0.1 dot-fs-h-bad.example.nil >dig.out.test$n 2>&1 || ret=1
  grep -F "10.10.10.3" dig.out.test$n >/dev/null 2>&1 && ret=1
  if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
  fi
else
  echo_i "skipped: DH not supported in FIPS mode"
fi

n=$((n + 1))
ret=0
echo_i "check DoT (strict-tls) with a wrong authority (failure expected) ($n)"
if $FEATURETEST --have-fips-dh; then
  $NSUPDATE -D -S -A CA/CA-other.pem -k ns1/ddns.key <<END >nsupdate.out.test$n 2>&1 && ret=1
    server 10.53.0.1 ${EXTRAPORT1}
    update add dot-fs-auth-bad.example.nil. 600 A 10.10.10.3
    send
END
  sleep 2
  $DIG $DIGOPTS +short @10.53.0.1 dot-fs-auth-bad.example.nil >dig.out.test$n 2>&1 || ret=1
  grep -F "10.10.10.3" dig.out.test$n >/dev/null 2>&1 && ret=1
  if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
  fi
else
  echo_i "skipped: DH not supported in FIPS mode"
fi

n=$((n + 1))
ret=0
echo_i "check DoT (mutual-tls) with a valid client certificate ($n)"
if $FEATURETEST --have-fips-dh; then
  $NSUPDATE -D -S -A CA/CA.pem -K CA/certs/srv01.client01.example.nil.key -E CA/certs/srv01.client01.example.nil.pem -k ns1/ddns.key <<END >nsupdate.out.test$n 2>&1 || ret=1
    server 10.53.0.1 ${EXTRAPORT2}
    update add dot-fsmt.example.nil. 600 A 10.10.10.3
    send
END
  sleep 2
  $DIG $DIGOPTS +short @10.53.0.1 dot-fsmt.example.nil >dig.out.test$n 2>&1 || ret=1
  grep -F "10.10.10.3" dig.out.test$n >/dev/null 2>&1 || ret=1
  if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
  fi
else
  echo_i "skipped: DH not supported in FIPS mode"
fi

n=$((n + 1))
ret=0
echo_i "check DoT (mutual-tls) with a valid client certificate but with an incorrect hostname (failure expected) ($n)"
if $FEATURETEST --have-fips-dh; then
  $NSUPDATE -D -S -A CA/CA.pem -K CA/certs/srv01.client01.example.nil.key -E CA/certs/srv01.client01.example.nil.pem -H srv01.crt01.example.bad -k ns1/ddns.key <<END >nsupdate.out.test$n 2>&1 && ret=1
    server 10.53.0.1 ${EXTRAPORT2}
    update add dot-fsmt-h-bad.example.nil. 600 A 10.10.10.3
    send
END
  sleep 2
  $DIG $DIGOPTS +short @10.53.0.1 dot-fsmt-h-bad.example.nil >dig.out.test$n 2>&1 || ret=1
  grep -F "10.10.10.3" dig.out.test$n >/dev/null 2>&1 && ret=1
  if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
  fi
else
  echo_i "skipped: DH not supported in FIPS mode"
fi

n=$((n + 1))
ret=0
echo_i "check DoT (mutual-tls) with a valid client certificate but with a wrong authority (failure expected) ($n)"
if $FEATURETEST --have-fips-dh; then
  $NSUPDATE -D -S -A CA/CA-other.pem -K CA/certs/srv01.client01.example.nil.key -E CA/certs/client01.crt01.example.nil.pem -k ns1/ddns.key <<END >nsupdate.out.test$n 2>&1 && ret=1
    server 10.53.0.1 ${EXTRAPORT2}
    update add dot-fsmt-auth-bad.example.nil. 600 A 10.10.10.3
    send
END
  sleep 2
  $DIG $DIGOPTS +short @10.53.0.1 dot-fsmt-auth-bad.example.nil >dig.out.test$n 2>&1 || ret=1
  grep -F "10.10.10.3" dig.out.test$n >/dev/null 2>&1 && ret=1
  if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
  fi
else
  echo_i "skipped: DH not supported in FIPS mode"
fi

n=$((n + 1))
ret=0
echo_i "check DoT (mutual-tls) with an expired client certificate (failure expected) ($n)"
if $FEATURETEST --have-fips-dh; then
  $NSUPDATE -D -S -A CA/CA.pem -K CA/certs/srv01.client02-expired.example.nil.key -E CA/certs/srv01.client02-expired.example.nil.pem -k ns1/ddns.key <<END >nsupdate.out.test$n 2>&1 && ret=1
    server 10.53.0.1 ${EXTRAPORT2}
    update add dot-fsmt-exp-bad.example.nil. 600 A 10.10.10.3
    send
END
  sleep 2
  $DIG $DIGOPTS +short @10.53.0.1 dot-fsmt-exp-bad.example.nil >dig.out.test$n 2>&1 || ret=1
  grep -F "10.10.10.3" dig.out.test$n >/dev/null 2>&1 && ret=1
  if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
  fi
else
  echo_i "skipped: DH not supported in FIPS mode"
fi

n=$((n + 1))
ret=0
echo_i "check DoT (mutual-tls) with a valid client certificate and an expired server certificate (failure expected) ($n)"
if $FEATURETEST --have-fips-dh; then
  $NSUPDATE -D -S -A CA/CA.pem -K CA/certs/srv01.client01.example.nil.key -E CA/certs/srv01.client01.example.nil.pem -k ns1/ddns.key <<END >nsupdate.out.test$n 2>&1 && ret=1
    server 10.53.0.1 ${EXTRAPORT3}
    update add dot-fsmt-exp-bad.example.nil. 600 A 10.10.10.3
    send
END
  sleep 2
  $DIG $DIGOPTS +short @10.53.0.1 dot-fsmt-exp-bad.example.nil >dig.out.test$n 2>&1 || ret=1
  grep -F "10.10.10.3" dig.out.test$n >/dev/null 2>&1 && ret=1
  if [ $ret -ne 0 ]; then
    echo_i "failed"
    status=1
  fi
else
  echo_i "skipped: DH not supported in FIPS mode"
fi

n=$((n + 1))
ret=0
echo_i "check TSIG key algorithms using legacy K file pairs (nsupdate -k) ($n)"
if $FEATURETEST --md5; then
  ALGS="157 161 162 163 164 165"
else
  ALGS="161 162 163 164 165"
  echo_i "skipping disabled md5 (157) algorithm"
fi
for alg in $ALGS; do
  $NSUPDATE -k ns1/legacy/Klegacy-${alg}.+${alg}+*.key <<END >nsupdate.alg-$alg.out 2>&1 || ret=1
server 10.53.0.1 ${PORT}
update add ${alg}.keytests.nil. 600 A 10.10.10.3
send
END
done
sleep 2
for alg in $ALGS; do
  $DIG $DIGOPTS +short @10.53.0.1 ${alg}.keytests.nil | grep 10.10.10.3 >/dev/null 2>&1 || ret=1
  grep "Use of K\* file pairs for HMAC is deprecated" nsupdate.alg-$alg.out >/dev/null || ret=1
done
if [ $ret -ne 0 ]; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check TSIG key algorithms (nsupdate -k) ($n)"
if $FEATURETEST --md5; then
  ALGS="md5 sha1 sha224 sha256 sha384 sha512"
else
  ALGS="sha1 sha224 sha256 sha384 sha512"
  echo_i "skipping disabled md5 algorithm"
fi
for alg in $ALGS; do
  $NSUPDATE -k ns1/${alg}.key <<END >/dev/null || ret=1
server 10.53.0.1 ${PORT}
update add ${alg}.keytests.nil. 600 A 10.10.10.3
send
END
done
sleep 2
for alg in $ALGS; do
  $DIG $DIGOPTS +short @10.53.0.1 ${alg}.keytests.nil | grep 10.10.10.3 >/dev/null 2>&1 || ret=1
done
if [ $ret -ne 0 ]; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check TSIG key algorithms (nsupdate -y) ($n)"
for alg in $ALGS; do
  secret=$(sed -n 's/.*secret "\(.*\)";.*/\1/p' ns1/${alg}.key)
  $NSUPDATE -y "hmac-${alg}:${alg}-key:$secret" <<END >/dev/null || ret=1
server 10.53.0.1 ${PORT}
update add ${alg}.keytests.nil. 600 A 10.10.10.50
send
END
done
sleep 2
for alg in $ALGS; do
  $DIG $DIGOPTS +short @10.53.0.1 ${alg}.keytests.nil | grep 10.10.10.50 >/dev/null 2>&1 || ret=1
done
if [ $ret -ne 0 ]; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check that ttl is capped by max-ttl ($n)"
$NSUPDATE <<END >/dev/null || ret=1
server 10.53.0.1 ${PORT}
update add cap.max-ttl.nil. 600 A 10.10.10.3
update add nocap.max-ttl.nil. 150 A 10.10.10.3
send
END
sleep 2
$DIG $DIGOPTS @10.53.0.1 cap.max-ttl.nil | grep "^cap.max-ttl.nil.	300" >/dev/null 2>&1 || ret=1
$DIG $DIGOPTS @10.53.0.1 nocap.max-ttl.nil | grep "^nocap.max-ttl.nil.	150" >/dev/null 2>&1 || ret=1
if [ $ret -ne 0 ]; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
echo_i "check adding more records than max-records-per-type fails ($n)"
ret=0
$NSUPDATE <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.1 ${PORT}
zone max-ttl.nil.
update add a.max-ttl.nil. 60 IN A 192.0.2.1
update add a.max-ttl.nil. 60 IN A 192.0.2.2
update add a.max-ttl.nil. 60 IN A 192.0.2.3
update add a.max-ttl.nil. 60 IN A 192.0.2.4
send
END
grep "update failed: SERVFAIL" nsupdate.out.test$n >/dev/null || ret=1
msg="error updating 'a.max-ttl.nil/A' in 'max-ttl.nil/IN' (zone): too many records (must not exceed 3)"
wait_for_log 10 "$msg" ns1/named.run || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}
nextpart ns1/named.run >/dev/null

n=$((n + 1))
ret=0
echo_i "add a record which is truncated when logged. ($n)"
$NSUPDATE verylarge || ret=1
$DIG $DIGOPTS +tcp @10.53.0.1 txt txt.update.nil >dig.out.ns1.test$n || ret=1
grep "ANSWER: 1," dig.out.ns1.test$n >/dev/null || ret=1
grep "adding an RR at 'txt.update.nil' TXT .* \[TRUNCATED\]" ns1/named.run >/dev/null || ret=1
if [ $ret -ne 0 ]; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check that yyyymmddvv serial number is correctly generated ($n)"
oldserial=$($DIG $DIGOPTS +short yyyymmddvv.nil. soa @10.53.0.1 | awk '{print $3}') || ret=1
$NSUPDATE <<END >/dev/null 2>&1 || ret=1
    server 10.53.0.1 ${PORT}
    ttl 600
    update add new.yyyymmddvv.nil in a 1.2.3.4
    send
END
now=$($PERL -e '@lt=localtime(); printf "%.4d%0.2d%0.2d00\n",$lt[5]+1900,$lt[4]+1,$lt[3];')
sleep 1
serial=$($DIG $DIGOPTS +short yyyymmddvv.nil. soa @10.53.0.1 | awk '{print $3}') || ret=1
[ "$oldserial" -ne "$serial" ] || ret=1
[ "$serial" -eq "$now" ] || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

#
#  Refactor to use perl to launch the parallel updates.
#
if false; then
  n=$((n + 1))
  echo_i "send many simultaneous updates via a update forwarder ($n)"
  ret=0
  for i in 0 1 2 3 4 5 6 7; do
    (
      for j in 0 1 2 3 4 5 6 7; do
        (
          $NSUPDATE <<EOF
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
  dig axfr many.test @10.53.0.1 >dig.out.test$n
  lines=$(awk '$4 == "A" { l++ } END { print l }' dig.out.test$n)
  test ${lines:-0} -eq 64 || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }
fi

n=$((n + 1))
echo_i "check max-journal-size limits ($n)"
ret=0
rm -f nsupdate.out1-$n
# add one record
$NSUPDATE <<EOF >>nsupdate.out1-$n 2>&1
server 10.53.0.1 ${PORT}
zone maxjournal.test
update add z.maxjournal.test 300 IN A 10.20.30.40
send
EOF
for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
  # repeatedly add and remove the same set of records to fill up
  # the journal file without changing the zone content
  $NSUPDATE <<EOF >>nsupdate.out1-$n 2>&1
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
size=$($PERL -e 'use File::stat; my $sb = stat(@ARGV[0]); printf("%s\n", $sb->size);' ns1/maxjournal.db.jnl)
[ "$size" -gt 6000 ] || ret=1
sleep 1
$RNDCCMD 10.53.0.1 sync maxjournal.test
check_size_lt_5000() (
  size=$($PERL -e 'use File::stat; my $sb = stat(@ARGV[0]); printf("%s\n", $sb->size);' ns1/maxjournal.db.jnl)
  [ "$size" -lt 5000 ]
)
retry_quiet 20 check_size_lt_5000 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
echo_i "check check-names processing ($n)"
ret=0
$NSUPDATE <<EOF >nsupdate.out1-$n 2>&1 && ret=1
update add # 0 in a 1.2.3.4
EOF
grep "bad owner" nsupdate.out1-$n >/dev/null || ret=1

$NSUPDATE <<EOF >nsupdate.out2-$n 2>&1 || ret=1
check-names off
update add # 0 in a 1.2.3.4
EOF
grep "bad owner" nsupdate.out2-$n >/dev/null && ret=1

$NSUPDATE <<EOF >nsupdate.out3-$n 2>&1 && ret=1
update add . 0 in mx 0 #
EOF
grep "bad name" nsupdate.out3-$n >/dev/null || ret=1

$NSUPDATE <<EOF >nsupdate.out4-$n 2>&1 || ret=1
check-names off
update add . 0 in mx 0 #
EOF
grep "bad name" nsupdate.out4-$n >/dev/null && ret=1

[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
echo_i "check check-svcb processing ($n)"
ret=0
$NSUPDATE <<EOF >nsupdate.out1-$n 2>&1 && ret=1
update add _dns.ns.example 0 in svcb 1 ns.example dohpath=/{?dns}
EOF
grep "check-svcb failed: no ALPN" nsupdate.out1-$n >/dev/null || ret=1

$NSUPDATE <<EOF >nsupdate.out2-$n 2>&1 || ret=1
check-svcb off
update add _dns.ns.example 0 in svcb 1 ns.example dohpath=/{?dns}
EOF
grep "check-svcb failed: no ALPN" nsupdate.out2-$n >/dev/null && ret=1

$NSUPDATE <<EOF >nsupdate.out3-$n 2>&1 && ret=1
update add _dns.ns.example 0 in svcb 1 ns.example alpn=h2
EOF
grep "check-svcb failed: no DOHPATH" nsupdate.out3-$n >/dev/null || ret=1

$NSUPDATE <<EOF >nsupdate.out4-$n 2>&1 || ret=1
check-svcb off
update add _dns.ns.example 0 in svcb 1 ns.example alpn=h2
EOF
grep "check-svcb failed: no DOHPATH" nsupdate.out4-$n >/dev/null && ret=1

[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
echo_i "check adding of delegating NS records processing ($n)"
ret=0
$NSUPDATE -v <<EOF >nsupdate.out.test$n 2>&1 || ret=1
server 10.53.0.3 ${PORT}
zone delegation.test.
update add child.delegation.test. 3600 NS foo.example.net.
update add child.delegation.test. 3600 NS bar.example.net.
send
EOF
$DIG $DIGOPTS +tcp @10.53.0.3 ns child.delegation.test >dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n >/dev/null 2>&1 || ret=1
grep "AUTHORITY: 2" dig.out.ns1.test$n >/dev/null 2>&1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
echo_i "check deleting of delegating NS records processing ($n)"
ret=0
$NSUPDATE -v <<EOF >nsupdate.out.test$n 2>&1 || ret=1
server 10.53.0.3 ${PORT}
zone delegation.test.
update del child.delegation.test. 3600 NS foo.example.net.
update del child.delegation.test. 3600 NS bar.example.net.
send
EOF
$DIG $DIGOPTS +tcp @10.53.0.3 ns child.delegation.test >dig.out.ns1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test$n >/dev/null 2>&1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
echo_i "check that adding too many records is blocked ($n)"
ret=0
$NSUPDATE -v <<EOF >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.3 ${PORT}
zone too-big.test.
update add r1.too-big.test 3600 IN TXT r1.too-big.test
send
EOF
grep "update failed: SERVFAIL" nsupdate.out.test$n >/dev/null || ret=1
$DIG $DIGOPTS +tcp @10.53.0.3 r1.too-big.test TXT >dig.out.ns3.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns3.test$n >/dev/null || ret=1
grep "records in zone (4) exceeds max-records (3)" ns3/named.run >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check whether valid addresses are used for primary failover (UDP with defaults) ($n)"
t1=$($PERL -e 'print time()')
$NSUPDATE <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.4 ${PORT}
zone unreachable.
update add unreachable. 600 A 192.0.2.1
send
END
t2=$($PERL -e 'print time()')
grep "; Communication with 10.53.0.4#${PORT} failed: timed out" nsupdate.out.test$n >/dev/null 2>&1 || ret=1
grep "not implemented" nsupdate.out.test$n >/dev/null 2>&1 && ret=1
elapsed=$((t2 - t1))
# Check that default timeout value is respected, there should be 4 tries with 3 seconds each.
test $elapsed -lt 12 && ret=1
test $elapsed -gt 15 && ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check whether valid addresses are used for primary failover (UDP with -u udptimeout) ($n)"
t1=$($PERL -e 'print time()')
$NSUPDATE -u 4 -r 1 <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.4 ${PORT}
zone unreachable.
update add unreachable. 600 A 192.0.2.1
send
END
t2=$($PERL -e 'print time()')
grep "; Communication with 10.53.0.4#${PORT} failed: timed out" nsupdate.out.test$n >/dev/null 2>&1 || ret=1
grep "not implemented" nsupdate.out.test$n >/dev/null 2>&1 && ret=1
elapsed=$((t2 - t1))
# Check that given timeout value is respected, there should be 2 tries with 4 seconds each.
test $elapsed -lt 8 && ret=1
test $elapsed -gt 12 && ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check whether valid addresses are used for primary failover (UDP with -t timeout) ($n)"
t1=$($PERL -e 'print time()')
$NSUPDATE -u 0 -t 8 -r 1 <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.4 ${PORT}
zone unreachable.
update add unreachable. 600 A 192.0.2.1
send
END
t2=$($PERL -e 'print time()')
grep "; Communication with 10.53.0.4#${PORT} failed: timed out" nsupdate.out.test$n >/dev/null 2>&1 || ret=1
grep "not implemented" nsupdate.out.test$n >/dev/null 2>&1 && ret=1
elapsed=$((t2 - t1))
# Check that given timeout value is respected, there should be 2 tries with 4 seconds each.
test $elapsed -lt 8 && ret=1
test $elapsed -gt 12 && ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check whether valid addresses are used for primary failover (UDP with -u udptimeout -t timeout) ($n)"
t1=$($PERL -e 'print time()')
$NSUPDATE -u 4 -t 30 -r 1 <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.4 ${PORT}
zone unreachable.
update add unreachable. 600 A 192.0.2.1
send
END
t2=$($PERL -e 'print time()')
grep "; Communication with 10.53.0.4#${PORT} failed: timed out" nsupdate.out.test$n >/dev/null 2>&1 || ret=1
grep "not implemented" nsupdate.out.test$n >/dev/null 2>&1 && ret=1
elapsed=$((t2 - t1))
# Check that given timeout value is respected, there should be 2 tries with 4 seconds each, as -u takes precedence over -t.
test $elapsed -lt 8 && ret=1
test $elapsed -gt 12 && ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check whether valid addresses are used for primary failover (TCP with -t timeout) ($n)"
t1=$($PERL -e 'print time()')
$NSUPDATE -t 8 -v <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.4 ${PORT}
zone unreachable.
update add unreachable. 600 A 192.0.2.1
send
END
t2=$($PERL -e 'print time()')
grep "; Communication with 10.53.0.4#${PORT} failed: timed out" nsupdate.out.test$n >/dev/null 2>&1 || ret=1
grep "not implemented" nsupdate.out.test$n >/dev/null 2>&1 && ret=1
elapsed=$((t2 - t1))
# Check that given timeout value is respected, there should be 1 try with 8 seconds.
test $elapsed -lt 8 && ret=1
test $elapsed -gt 12 && ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "ensure bad owner name is fatal in non-interactive mode ($n)"
$NSUPDATE <<END >nsupdate.out 2>&1 && ret=1
    update add emptylabel..nil. 600 A 10.10.10.1
END
grep "invalid owner name: empty label" nsupdate.out >/dev/null || ret=1
grep "syntax error" nsupdate.out >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "ensure bad owner name is not fatal in interactive mode ($n)"
$NSUPDATE -i <<END >nsupdate.out 2>&1 || ret=1
    update add emptylabel..nil. 600 A 10.10.10.1
END
grep "invalid owner name: empty label" nsupdate.out >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "ensure invalid key type is fatal in non-interactive mode ($n)"
$NSUPDATE <<END >nsupdate.out 2>&1 && ret=1
    key badkeytype:example abcd12345678
END
grep "unknown key type 'badkeytype'" nsupdate.out >/dev/null || ret=1
grep "syntax error" nsupdate.out >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "ensure invalid key type is not fatal in interactive mode ($n)"
$NSUPDATE -i <<END >nsupdate.out 2>&1 || ret=1
    key badkeytype:example abcd12345678
END
grep "unknown key type 'badkeytype'" nsupdate.out >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "ensure unresolvable server name is fatal in non-interactive mode ($n)"
$NSUPDATE <<END >nsupdate.out 2>&1 && ret=1
    server unresolvable..
END
grep "couldn't get address for 'unresolvable..':" nsupdate.out >/dev/null || ret=1
grep "syntax error" nsupdate.out >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "ensure unresolvable server name is not fatal in interactive mode ($n)"
$NSUPDATE -i <<END >nsupdate.out 2>&1 || ret=1
    server unresolvable..
END
grep "couldn't get address for 'unresolvable..':" nsupdate.out >/dev/null || ret=1
grep "syntax error" nsupdate.out >/dev/null && ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check nsupdate -4 -6 ($n)"
$NSUPDATE -4 -6 <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.3 ${PORT}
zone delegation.test.
update del child.delegation.test. 3600 NS foo.example.net.
update del child.delegation.test. 3600 NS bar.example.net.
send
END
grep "only one of -4 and -6 allowed" nsupdate.out.test$n >/dev/null 2>&1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check nsupdate -4 with an IPv6 server address ($n)"
$NSUPDATE -4 <<END >nsupdate.out.test$n 2>&1 && ret=1
server fd92:7065:b8e:ffff::2 ${PORT}
zone delegation.test.
update del child.delegation.test. 3600 NS foo.example.net.
update del child.delegation.test. 3600 NS bar.example.net.
send
END
grep "address family not supported" nsupdate.out.test$n >/dev/null 2>&1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that TKEY in a update is rejected ($n)"
$NSUPDATE -d <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.3 ${PORT}
update add tkey.example 0 in tkey invalid.algorithm. 1516055980 1516140801 1 0 16 gRof8D2BFKvl/vrr9Lmnjw== 16 gRof8D2BFKvl/vrr9Lmnjw==
send
END
grep "UPDATE, status: NOERROR" nsupdate.out.test$n >/dev/null 2>&1 || ret=1
grep "UPDATE, status: FORMERR" nsupdate.out.test$n >/dev/null 2>&1 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that max records is enforced ($n)"
nextpart ns6/named.run >/dev/null
$NSUPDATE -v >nsupdate.out.$n 2>&1 <<END
server 10.53.0.6 ${PORT}
local 10.53.0.5
update del 5.0.53.10.in-addr.arpa.
update add 5.0.53.10.in-addr.arpa. 600 PTR localhost.
update add 5.0.53.10.in-addr.arpa. 600 PTR other.
send
END
$DIG $DIGOPTS @10.53.0.6 \
  +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
  -x 10.53.0.5 >dig.out.ns6.$n || ret=1
# the policy is 'grant * tcp-self . PTR(1) ANY(2) A;' so only the
# first PTR record should be added.
grep localhost. dig.out.ns6.$n >/dev/null 2>&1 || ret=1
grep other. dig.out.ns6.$n >/dev/null 2>&1 && ret=1
nextpart ns6/named.run >nextpart.out.$n
grep "attempt to add more records than permitted by policy" nextpart.out.$n >/dev/null || ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check that max records for ANY is enforced ($n)"
nextpart ns6/named.run >/dev/null
$NSUPDATE -v >nsupdate.out.$n 2>&1 <<END
server 10.53.0.6 ${PORT}
local 10.53.0.5
update del 5.0.53.10.in-addr.arpa.
update add 5.0.53.10.in-addr.arpa. 600 A 1.2.3.4
update add 5.0.53.10.in-addr.arpa. 600 A 1.2.3.3
update add 5.0.53.10.in-addr.arpa. 600 A 1.2.3.2
update add 5.0.53.10.in-addr.arpa. 600 AAAA ::ffff:1.2.3.4
update add 5.0.53.10.in-addr.arpa. 600 AAAA ::ffff:1.2.3.3
update add 5.0.53.10.in-addr.arpa. 600 AAAA ::ffff:1.2.3.2
send
END
$DIG $DIGOPTS @10.53.0.6 \
  +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd \
  ANY -x 10.53.0.5 >dig.out.ns6.test$n || ret=1
nextpart ns6/named.run >nextpart.out.test$n
grep "attempt to add more records than permitted by policy" nextpart.out.test$n >/dev/null || ret=1
# the policy is 'grant * tcp-self . PTR(1) ANY(2) A;' so all the A
# records should have been added as there is no limit and the first 2
# of the AAAA records added as they match ANY(2).
c1=$(awk '$4 == "A" { print }' dig.out.ns6.test$n | wc -l)
c2=$(awk '$4 == "AAAA" { print }' dig.out.ns6.test$n | wc -l)
test "$c1" -eq 3 -a "$c2" -eq 2 || ret=1
grep "::ffff:1.2.3.2" dig.out.ns6.test$n >/dev/null && ret=1
if test $ret -ne 0; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
ret=0
echo_i "check that DS to the zone apex is ignored ($n)"
$DIG $DIGOPTS +tcp +norec example DS @10.53.0.3 >dig.out.pre.test$n || ret=1
grep "status: NOERROR" dig.out.pre.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.pre.test$n >/dev/null || ret=1
nextpart ns3/named.run >/dev/null
# specify zone to override the default of adding to parent zone
$NSUPDATE -d <<END >nsupdate.out.test$n 2>&1 || ret=1
server 10.53.0.3 ${PORT}
zone example
update add example 0 in DS 14364 10 2 FD03B2312C8F0FE72C1751EFA1007D743C94EC91594FF0047C23C37CE119BA0C
send
END
msg=": attempt to add a DS record at zone apex ignored"
nextpart ns3/named.run | grep "$msg" >/dev/null || ret=1
$DIG $DIGOPTS +tcp +norec example DS @10.53.0.3 >dig.out.post.test$n || ret=1
grep "status: NOERROR" dig.out.post.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.post.test$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that CDS with mismatched algorithm to DNSSEC multisigner zone is not allowed ($n)"
$DIG $DIGOPTS +tcp +norec multisigner.test CDS @10.53.0.3 >dig.out.pre.test$n || ret=1
grep "status: NOERROR" dig.out.pre.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.pre.test$n >/dev/null || ret=1
$NSUPDATE -d <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.3 ${PORT}
zone multisigner.test
update add multisigner.test 3600 IN CDS 14364 14 2 FD03B2312C8F0FE72C1751EFA1007D743C94EC91594FF0047C23C37CE119BA0C
send
END
msg=": bad CDS RRset"
nextpart ns3/named.run | grep "$msg" >/dev/null || ret=1
$DIG $DIGOPTS +tcp +norec multisigner.test CDS @10.53.0.3 >dig.out.post.test$n || ret=1
grep "status: NOERROR" dig.out.post.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.post.test$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that CDNSKEY with mismatched algorithm to DNSSEC multisigner zone is not allowed ($n)"
$DIG $DIGOPTS +tcp +norec multisigner.test CDNSKEY @10.53.0.3 >dig.out.pre.test$n || ret=1
grep "status: NOERROR" dig.out.pre.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.pre.test$n >/dev/null || ret=1
nextpart ns3/named.run >/dev/null
$NSUPDATE -d <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.3 ${PORT}
zone multisigner.test
update add multisigner.test 3600 IN CDNSKEY 257 3 14 d0NQ5PKmDz6P0B1WPMH9/UKRux/toSFwV2nTJYPA1Cx8pB0sJGTXbVhG U+6gye7VCHDhGIn9CjVfb2RJPW7GnQ==
send
END
msg=": bad CDNSKEY RRset"
nextpart ns3/named.run | grep "$msg" >/dev/null || ret=1
$DIG $DIGOPTS +tcp +norec multisigner.test CDNSKEY @10.53.0.3 >dig.out.post.test$n || ret=1
grep "status: NOERROR" dig.out.post.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.post.test$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that CDS to DNSSEC multisigner zone is allowed ($n)"
$DIG $DIGOPTS +tcp +norec multisigner.test CDS @10.53.0.3 >dig.out.pre.test$n || ret=1
grep "status: NOERROR" dig.out.pre.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.pre.test$n >/dev/null || ret=1
$NSUPDATE -d <<END >nsupdate.out.test$n 2>&1 || ret=1
server 10.53.0.3 ${PORT}
zone multisigner.test
update add multisigner.test 3600 IN CDS 14364 13 2 FD03B2312C8F0FE72C1751EFA1007D743C94EC91594FF0047C23C37CE119BA0C
send
END
retry_quiet 5 has_positive_response multisigner.test CDS 10.53.0.3 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that CDNSKEY to DNSSEC multisigner zone is allowed ($n)"
$DIG $DIGOPTS +tcp +norec multisigner.test CDNSKEY @10.53.0.3 >dig.out.pre.test$n || ret=1
grep "status: NOERROR" dig.out.pre.test$n >/dev/null || ret=1
grep "ANSWER: 0," dig.out.pre.test$n >/dev/null || ret=1
$NSUPDATE -d <<END >nsupdate.out.test$n 2>&1 || ret=1
server 10.53.0.3 ${PORT}
zone multisigner.test
update add multisigner.test 3600 IN CDNSKEY 257 3 13 d0NQ5PKmDz6P0B1WPMH9/UKRux/toSFwV2nTJYPA1Cx8pB0sJGTXbVhG U+6gye7VCHDhGIn9CjVfb2RJPW7GnQ==
send
END
retry_quiet 5 has_positive_response multisigner.test CDNSKEY 10.53.0.3 || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that excessive NSEC3PARAM iterations are rejected by nsupdate ($n)"
$NSUPDATE -d <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.3 ${PORT}
zone example
update add example 0 in NSEC3PARAM 1 0 51 -
END
grep "NSEC3PARAM has excessive iterations (> 50)" nsupdate.out.test$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check nsupdate retries with another server on REFUSED response ($n)"
# resolv.conf uses 10.53.0.1 followed by 10.53.0.3; example is only
# served by 10.53.0.3, so we should fail over to the second server;
# that's what we're testing for. (failure is still expected, however,
# because the address lookup for the primary doesn't use the overridden
# resolv.conf file).
$NSUPDATE -D -C resolv.conf -p ${PORT} <<EOF >nsupdate.out.test$n 2>&1 && ret=1
zone example
update add a 3600 IN A 1.2.3.4
send
EOF
grep '10.53.0.1.*REFUSED' nsupdate.out.test$n >/dev/null || ret=1
grep 'Reply from SOA query' nsupdate.out.test$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that named rejects '_dns' SVCB with missing ALPN ($n)"
nextpart ns3/named.run >/dev/null
$NSUPDATE -d <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.3 ${PORT}
zone example
check-svcb no
update add _dns.ns.example 0 in SVCB 1 ns.example dohpath=/{?dns}
send
END
grep 'status: REFUSED' nsupdate.out.test$n >/dev/null || ret=1
msg="update failed: _dns.ns.example/SVCB: no ALPN (REFUSED)"
nextpart ns3/named.run | grep "$msg" >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that named accepts '_dns' SVCB with missing ALPN (check-svcb no) ($n)"
$NSUPDATE -d <<END >nsupdate.out.test$n 2>&1 || ret=1
server 10.53.0.3 ${PORT}
zone relaxed
check-svcb no
update add _dns.ns.relaxed 0 in SVCB 1 ns.relaxed dohpath=/{?dns}
send
END
$DIG $DIGOPTS +tcp @10.53.0.3 _dns.ns.relaxed SVCB >dig.out.ns3.test$n || ret=1
grep '1 ns.relaxed. key7="/{?dns}"' dig.out.ns3.test$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that named rejects '_dns' SVCB with missing DOHPATH ($n)"
nextpart ns3/named.run >/dev/null
$NSUPDATE -d <<END >nsupdate.out.test$n 2>&1 && ret=1
server 10.53.0.3 ${PORT}
zone example
check-svcb no
update add _dns.ns.example 0 in SVCB 1 ns.example alpn=h2
send
END
grep 'status: REFUSED' nsupdate.out.test$n >/dev/null || ret=1
msg="update failed: _dns.ns.example/SVCB: no DOHPATH (REFUSED)"
nextpart ns3/named.run | grep "$msg" >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that named accepts '_dns' SVCB with missing DOHPATH (check-svcb no) ($n)"
$NSUPDATE -d <<END >nsupdate.out.test$n 2>&1 || ret=1
server 10.53.0.3 ${PORT}
zone relaxed
check-svcb no
update add _dns.ns.relaxed 0 in SVCB 1 ns.relaxed alpn=h2
send
END
$DIG $DIGOPTS +tcp @10.53.0.3 _dns.ns.relaxed SVCB >dig.out.ns3.test$n || ret=1
grep '1 ns.relaxed. alpn="h2"' dig.out.ns3.test$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that update is rejected if query is not allowed ($n)"
{
  $NSUPDATE -d <<END && ret=1
  local 10.53.0.2
  server 10.53.0.1 ${PORT}
  update add reject.other.nil 3600 IN TXT Whatever
  send
END
} >nsupdate.out.test$n 2>&1
grep 'failed: REFUSED' nsupdate.out.test$n >/dev/null || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

n=$((n + 1))
ret=0
echo_i "check that update is rejected if quota is exceeded ($n)"
for loop in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
  {
    $NSUPDATE -4 -l -p ${PORT} -k ns1/session.key >/dev/null 2>&1 <<END
  update add txt-$loop.other.nil 3600 IN TXT Whatever
  send
END
  } &
done
wait_for_log 10 "too many DNS UPDATEs queued" ns1/named.run || ret=1
[ $ret = 0 ] || {
  echo_i "failed"
  status=1
}

if ! $FEATURETEST --gssapi; then
  echo_i "SKIPPED: GSSAPI tests"
else
  n=$((n + 1))
  ret=0
  echo_i "check GSS-API TKEY request rcode against a non configured server ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  send
EOF
  grep "response to GSS-TSIG query was unsuccessful (REFUSED)" nsupdate.out.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  cp ns7/named2.conf ns7/named.conf
  rndc_reload ns7 10.53.0.7

  n=$((n + 1))
  ret=0
  echo_i "check krb5-self match ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update add machine.example.com 3600 IN A 10.53.0.7
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.7 machine.example.com A >dig.out.ns7.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.test$n >/dev/null || ret=1
  grep "machine.example.com..*A.*10.53.0.7" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-self no-match ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update add foo.example.com 3600 IN A 10.53.0.7
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.7 foo.example.com A >dig.out.ns7.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain match ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE -d <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update add _xxx._tcp.example.com 3600 IN SRV 0 0 0 machine.example.com
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.7 _xxx._tcp.example.com SRV >dig.out.ns7.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.test$n >/dev/null || ret=1
  grep "_xxx._tcp.example.com.*SRV.*0 0 0 machine.example.com" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain no-match ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update add _xxx._udp.example.com 3600 IN SRV 0 0 0 machine.example.com
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.7 _xxx._udp.example.com SRV >dig.out.ns7.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs match PTR ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE -d <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone in-addr.arpa
  update add 4.3.2.1.in-addr.arpa 3600 IN PTR machine.example.com
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.7 4.3.2.1.in-addr.arpa PTR >dig.out.ns7.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.test$n >/dev/null || ret=1
  grep "4.3.2.1.in-addr.arpa.*PTR.*machine.example.com" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs no-match PTR ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone in-addr.arpa
  update add 5.3.2.1.in-addr.arpa 3600 IN PTR notme.example.com
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.7 5.3.2.1.in-addr.arpa PTR >dig.out.ns7.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs match SRV ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE -d <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update add _xxx.self-srv.example.com 3600 IN SRV 0 0 0 machine.example.com
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.7 _xxx.self-srv.example.com ANY >dig.out.ns7.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.test$n >/dev/null || ret=1
  grep "_xxx.self-srv.example.com.*SRV.*0 0 0 machine.example.com" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs no listed types match (SRV & TXT) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE -d <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update add _xxx.self-srv-no-type.example.com 3600 IN SRV 0 0 0 machine.example.com
  update add _xxx.self-srv-no-type.example.com 3600 IN TXT a txt record
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.7 _xxx.self-srv-no-type.example.com ANY >dig.out.ns7.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.test$n >/dev/null || ret=1
  grep '_xxx.self-srv-no-type.example.com.*SRV.*0 0 0 machine.example.com' dig.out.ns7.test$n >/dev/null || ret=1
  grep '_xxx.self-srv-no-type.example.com.*TXT.*"a" "txt" "record"' dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs no-match RDATA (SRV) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update add _yyy.self-srv.example.com 3600 IN SRV 0 0 0 notme.example.com
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.7 _yyy.self-srv.example.com SRV >dig.out.ns7.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs no-match TYPE (TXT) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update add _yyy.self-srv.example.com 3600 IN TXT a-txt-record
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.7 _yyy.self-srv.example.com TXT >dig.out.ns7.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs delete PTR (matching PTR) ($n)"
  $DIG $DIGOPTS +tcp @10.53.0.7 single.ptr.self-ptr.in-addr.arpa PTR >dig.out.ns7.pre.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.pre.test$n >/dev/null || ret=1
  grep "ANSWER: 1," dig.out.ns7.pre.test$n >/dev/null || ret=1
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone in-addr.arpa
  update delete single.ptr.self-ptr.in-addr.arpa PTR
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.7 single.ptr.self-ptr.in-addr.arpa PTR >dig.out.ns7.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs delete PTR (matching PTR with non-matching PTR) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone in-addr.arpa
  update delete many.ptr.self-ptr.in-addr.arpa PTR
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.7 many.ptr.self-ptr.in-addr.arpa PTR >dig.out.ns7.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs delete ANY (matching PTR) ($n)"
  $DIG $DIGOPTS +tcp @10.53.0.7 single.any.self-ptr.in-addr.arpa PTR >dig.out.ns7.pre.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.pre.test$n >/dev/null || ret=1
  grep "ANSWER: 1," dig.out.ns7.pre.test$n >/dev/null || ret=1
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone in-addr.arpa
  update delete single.any.self-ptr.in-addr.arpa
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.7 single.any.self-ptr.in-addr.arpa PTR >dig.out.ns7.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs delete ANY (matching PTR with non-matching PTR) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone in-addr.arpa
  update delete many.any.self-ptr.in-addr.arpa
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.7 many.any.self-ptr.in-addr.arpa PTR >dig.out.ns7.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs delete SRV (matching SRV) ($n)"
  $DIG $DIGOPTS +tcp @10.53.0.7 single.srv.self-srv.example.com SRV >dig.out.ns7.pre.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.pre.test$n >/dev/null || ret=1
  grep "ANSWER: 1," dig.out.ns7.pre.test$n >/dev/null || ret=1
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update delete single.srv.self-srv.example.com SRV
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.7 single.srv.self-srv.example.com SRV >dig.out.ns7.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs delete SRV (matching SRV with non-matching SRV) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update delete many.srv.self-srv.example.com SRV
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.7 many.srv.self-srv.example.com SRV >dig.out.ns7.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs delete ANY (matching SRV) ($n)"
  $DIG $DIGOPTS +tcp @10.53.0.7 single.any.self-srv.example.com SRV >dig.out.ns7.pre.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.pre.test$n >/dev/null || ret=1
  grep "ANSWER: 1," dig.out.ns7.pre.test$n >/dev/null || ret=1
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update delete single.any.self-srv.example.com
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.7 single.any.self-srv.example.com SRV >dig.out.ns7.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-subdomain-self-rhs delete ANY (matching SRV with non-matching SRV) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns7/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.7 ${PORT}
  zone example.com
  update delete many.any.self-srv.example.com
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.7 many.any.self-srv.example.com SRV >dig.out.ns7.test$n || ret=1
  grep "status: NOERROR" dig.out.ns7.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns7.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-selfsub match ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns8/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE -d <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.8 ${PORT}
  zone example.com
  update add xxx.machine.example.com 3600 IN A 10.53.0.8
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.8 xxx.machine.example.com A >dig.out.ns8.test$n || ret=1
  grep "status: NOERROR" dig.out.ns8.test$n >/dev/null || ret=1
  grep "xxx.machine.example.com..*A.*10.53.0.8" dig.out.ns8.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check krb5-selfsub no-match ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns8/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.8 ${PORT}
  zone example.com
  update add foo.example.com 3600 IN A 10.53.0.8
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.8 foo.example.com A >dig.out.ns8.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns8.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-self match ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns9/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.9 ${PORT}
  zone example.com
  update add machine.example.com 3600 IN A 10.53.0.9
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.9 machine.example.com A >dig.out.ns9.test$n || ret=1
  grep "status: NOERROR" dig.out.ns9.test$n >/dev/null || ret=1
  grep "machine.example.com..*A.*10.53.0.9" dig.out.ns9.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-self no-match ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns9/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.9 ${PORT}
  zone example.com
  update add foo.example.com 3600 IN A 10.53.0.9
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.9 foo.example.com A >dig.out.ns9.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns9.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain match ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns9/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE -d <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.9 ${PORT}
  zone example.com
  update add _xxx._tcp.example.com 3600 IN SRV 0 0 0 machine.example.com
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.9 _xxx._tcp.example.com SRV >dig.out.ns9.test$n || ret=1
  grep "status: NOERROR" dig.out.ns9.test$n >/dev/null || ret=1
  grep "_xxx._tcp.example.com.*SRV.*0 0 0 machine.example.com" dig.out.ns9.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain no-match ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns9/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.9 ${PORT}
  zone example.com
  update add _xxx._udp.example.com 3600 IN SRV 0 0 0 machine.example.com
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.9 _xxx._udp.example.com SRV >dig.out.ns9.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns9.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain-self-rhs match (PTR) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE -d <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone in-addr.arpa
  update add 4.3.2.1.in-addr.arpa 3600 IN PTR machine.example.com
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.10 4.3.2.1.in-addr.arpa PTR >dig.out.ns10.test$n || ret=1
  grep "status: NOERROR" dig.out.ns10.test$n >/dev/null || ret=1
  grep "4.3.2.1.in-addr.arpa.*PTR.*machine.example.com" dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain-self-rhs no-match (PTR) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone in-addr.arpa
  update add 5.3.2.1.in-addr.arpa 3600 IN PTR notme.example.com
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.10 5.3.2.1.in-addr.arpa PTR >dig.out.ns10.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain-self-rhs match (SRV) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE -d <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone example.com
  update add _xxx.self-srv.example.com 3600 IN SRV 0 0 0 machine.example.com
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.10 _xxx.self-srv.example.com SRV >dig.out.ns10.test$n || ret=1
  grep "status: NOERROR" dig.out.ns10.test$n >/dev/null || ret=1
  grep "_xxx.self-srv.example.com.*SRV.*0 0 0 machine.example.com" dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain-self-rhs no-match (SRV) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone example.com
  update add _yyy.self-srv.example.com 3600 IN SRV 0 0 0 notme.example.com
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.10 _yyy.self-srv.example.com SRV >dig.out.ns10.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain-self-rhs delete SRV (matching SRV) ($n)"
  $DIG $DIGOPTS +tcp @10.53.0.10 single.srv.self-srv.example.com SRV >dig.out.ns10.pre.test$n || ret=1
  grep "status: NOERROR" dig.out.ns10.pre.test$n >/dev/null || ret=1
  grep "ANSWER: 1," dig.out.ns10.pre.test$n >/dev/null || ret=1
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone example.com
  update delete single.srv.self-srv.example.com SRV
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.10 single.srv.self-srv.example.com SRV >dig.out.ns10.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain-self-rhs delete SRV (matching SRV with non-matching SRV) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone example.com
  update delete many.srv.self-srv.example.com SRV
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.10 many.srv.self-srv.example.com SRV >dig.out.ns10.test$n || ret=1
  grep "status: NOERROR" dig.out.ns10.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain-self-rhs delete PTR (matching PTR) ($n)"
  $DIG $DIGOPTS +tcp @10.53.0.10 single.ptr.self-ptr.in-addr.arpa PTR >dig.out.ns10.pre.test$n || ret=1
  grep "status: NOERROR" dig.out.ns10.pre.test$n >/dev/null || ret=1
  grep "ANSWER: 1," dig.out.ns10.pre.test$n >/dev/null || ret=1
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone in-addr.arpa
  update delete single.ptr.self-ptr.in-addr.arpa PTR
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.10 single.ptr.self-ptr.in-addr.arpa PTR >dig.out.ns10.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain-self-rhs delete PTR (matching PTR with non-matching PTR) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone in-addr.arpa
  update delete many.ptr.self-ptr.in-addr.arpa PTR
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.10 many.ptr.self-ptr.in-addr.arpa PTR >dig.out.ns10.test$n || ret=1
  grep "status: NOERROR" dig.out.ns10.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain-self-rhs delete ANY (matching PTR) ($n)"
  $DIG $DIGOPTS +tcp @10.53.0.10 single.any.self-ptr.in-addr.arpa PTR >dig.out.ns10.pre.test$n || ret=1
  grep "status: NOERROR" dig.out.ns10.pre.test$n >/dev/null || ret=1
  grep "ANSWER: 1," dig.out.ns10.pre.test$n >/dev/null || ret=1
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone in-addr.arpa
  update delete single.any.self-ptr.in-addr.arpa
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.10 single.any.self-ptr.in-addr.arpa PTR >dig.out.ns10.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain-self-rhs delete ANY (matching PTR with non-matching PTR) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone in-addr.arpa
  update delete many.any.self-ptr.in-addr.arpa
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.10 many.any.self-ptr.in-addr.arpa PTR >dig.out.ns10.test$n || ret=1
  grep "status: NOERROR" dig.out.ns10.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain-self-rhs delete ANY (matching SRV) ($n)"
  $DIG $DIGOPTS +tcp @10.53.0.10 single.any.self-srv.example.com SRV >dig.out.ns10.pre.test$n || ret=1
  grep "status: NOERROR" dig.out.ns10.pre.test$n >/dev/null || ret=1
  grep "ANSWER: 1," dig.out.ns10.pre.test$n >/dev/null || ret=1
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone example.com
  update delete single.any.self-srv.example.com
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.10 single.any.self-srv.example.com SRV >dig.out.ns10.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-subdomain-self-rhs delete ANY (matching SRV with non-matching SRV) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone example.com
  update delete many.any.self-srv.example.com
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.10 many.any.self-srv.example.com SRV >dig.out.ns10.test$n || ret=1
  grep "status: NOERROR" dig.out.ns10.test$n >/dev/null || ret=1
  grep "ANSWER: 2," dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-selfsub match ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE -d <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone example.com
  update add xxx.machine.example.com 3600 IN A 10.53.0.10
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.10 xxx.machine.example.com A >dig.out.ns10.test$n || ret=1
  grep "status: NOERROR" dig.out.ns10.test$n >/dev/null || ret=1
  grep "xxx.machine.example.com..*A.*10.53.0.10" dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-selfsub no-match ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE <<EOF >nsupdate.out.test$n 2>&1 && ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${PORT}
  zone example.com
  update add foo.example.com 3600 IN A 10.53.0.10
  send
EOF
  grep "update failed: REFUSED" nsupdate.out.test$n >/dev/null || ret=1
  $DIG $DIGOPTS +tcp @10.53.0.10 foo.example.com A >dig.out.ns10.test$n || ret=1
  grep "status: NXDOMAIN" dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }

  n=$((n + 1))
  ret=0
  echo_i "check ms-selfsub match using DoT (opportunistic-tls) ($n)"
  KRB5CCNAME="FILE:$(pwd)/ns10/machine.ccache"
  export KRB5CCNAME
  $NSUPDATE -d -S -O <<EOF >nsupdate.out.test$n 2>&1 || ret=1
  gsstsig
  realm EXAMPLE.COM
  server 10.53.0.10 ${TLSPORT}
  zone example.com
  update add dot.machine.example.com 3600 IN A 10.53.0.10
  send
EOF
  $DIG $DIGOPTS +tcp @10.53.0.10 dot.machine.example.com A >dig.out.ns10.test$n || ret=1
  grep "status: NOERROR" dig.out.ns10.test$n >/dev/null || ret=1
  grep "dot.machine.example.com..*A.*10.53.0.10" dig.out.ns10.test$n >/dev/null || ret=1
  [ $ret = 0 ] || {
    echo_i "failed"
    status=1
  }
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
