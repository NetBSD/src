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

status=0

#
echo_i "checking that we detect a NS which refers to a CNAME"
if $CHECKZONE . cname.db > cname.out 2>&1
then
	echo_i "failed (status)"; status=`expr $status + 1`
else
	if grep "is a CNAME" cname.out > /dev/null
	then
		:
	else
		echo_i "failed (message)"; status=`expr $status + 1`
	fi
fi

#
echo_i "checking that we detect a NS which is below a DNAME"
if $CHECKZONE . dname.db > dname.out 2>&1
then
	echo_i "failed (status)"; status=`expr $status + 1`
else
	if grep "is below a DNAME" dname.out > /dev/null
	then
		:
	else
		echo_i "failed (message)"; status=`expr $status + 1`
	fi
fi

#
echo_i "checking that we detect a NS which has no address records (A/AAAA)"
if $CHECKZONE . noaddress.db > noaddress.out
then
	echo_i "failed (status)"; status=`expr $status + 1`
else
	if grep "has no address records" noaddress.out > /dev/null
	then
		:
	else
		echo_i "failed (message)"; status=`expr $status + 1`
	fi
fi

#
echo_i "checking that we detect a NS which has no records"
if $CHECKZONE . nxdomain.db > nxdomain.out
then
	echo_i "failed (status)"; status=`expr $status + 1`
else
	if grep "has no address records" noaddress.out > /dev/null
	then
		:
	else
		echo_i "failed (message)"; status=`expr $status + 1`
	fi
fi

#
echo_i "checking that we detect a NS which looks like a A record (fail)"
if $CHECKZONE -n fail . a.db > a.out 2>&1
then
	echo_i "failed (status)"; status=`expr $status + 1`
else
	if grep "appears to be an address" a.out > /dev/null
	then
		:
	else
		echo_i "failed (message)"; status=`expr $status + 1`
	fi
fi

#
echo_i "checking that we detect a NS which looks like a A record (warn=default)"
if $CHECKZONE . a.db > a.out 2>&1
then
	if grep "appears to be an address" a.out > /dev/null
	then
		:
	else
		echo_i "failed (message)"; status=`expr $status + 1`
	fi
else
	echo_i "failed (status)"; status=`expr $status + 1`
fi

#
echo_i "checking that we detect a NS which looks like a A record (ignore)"
if $CHECKZONE -n ignore . a.db > a.out 2>&1
then
	if grep "appears to be an address" a.out > /dev/null
	then
		echo_i "failed (message)"; status=`expr $status + 1`
	else
		:
	fi
else
	echo_i "failed (status)"; status=`expr $status + 1`
fi

#
echo_i "checking that we detect a NS which looks like a AAAA record (fail)"
if $CHECKZONE -n fail . aaaa.db > aaaa.out 2>&1
then
	echo_i "failed (status)"; status=`expr $status + 1`
else
	if grep "appears to be an address" aaaa.out > /dev/null
	then
		:
	else
		echo_i "failed (message)"; status=`expr $status + 1`
	fi
fi

#
echo_i "checking that we detect a NS which looks like a AAAA record (warn=default)"
if $CHECKZONE . aaaa.db > aaaa.out 2>&1
then
	if grep "appears to be an address" aaaa.out > /dev/null
	then
		:
	else
		echo_i "failed (message)"; status=`expr $status + 1`
	fi
else
	echo_i "failed (status)"; status=`expr $status + 1`
fi

#
echo_i "checking that we detect a NS which looks like a AAAA record (ignore)"
if $CHECKZONE -n ignore . aaaa.db > aaaa.out 2>&1
then
	if grep "appears to be an address" aaaa.out > /dev/null
	then
		echo_i "failed (message)"; status=`expr $status + 1`
	else
		:
	fi
else
	echo_i "failed (status)"; status=`expr $status + 1`
fi

#
echo_i "checking 'rdnc zonestatus' output"
ret=0 
for i in 0 1 2 3 4 5 6 7 8 9
do
	$RNDCCMD 10.53.0.1 zonestatus master.example > rndc.out.master 2>&1
	grep "zone not loaded" rndc.out.master > /dev/null || break
	sleep 1
done
checkfor() {
	grep "$1" $2 > /dev/null || {
		ret=1;
		echo_i "missing string '$1' from '$2'"
	}
}
checkfor "name: master.example" rndc.out.master
checkfor "type: master" rndc.out.master
checkfor "files: master.db, master.db.signed" rndc.out.master
checkfor "serial: " rndc.out.master
checkfor "nodes: " rndc.out.master
checkfor "last loaded: " rndc.out.master
checkfor "secure: yes" rndc.out.master
checkfor "inline signing: no" rndc.out.master
checkfor "key maintenance: automatic" rndc.out.master
checkfor "next key event: " rndc.out.master
checkfor "next resign node: " rndc.out.master
checkfor "next resign time: " rndc.out.master
checkfor "dynamic: yes" rndc.out.master
checkfor "frozen: no" rndc.out.master
for i in 0 1 2 3 4 5 6 7 8 9
do
	$RNDCCMD 10.53.0.2 zonestatus master.example > rndc.out.slave 2>&1
	grep "zone not loaded" rndc.out.slave > /dev/null || break
	sleep 1
done
checkfor "name: master.example" rndc.out.slave
checkfor "type: slave" rndc.out.slave
checkfor "files: slave.db" rndc.out.slave
checkfor "serial: " rndc.out.slave
checkfor "nodes: " rndc.out.slave
checkfor "next refresh: " rndc.out.slave
checkfor "expires: " rndc.out.slave
checkfor "secure: yes" rndc.out.slave
for i in 0 1 2 3 4 5 6 7 8 9
do
	$RNDCCMD 10.53.0.1 zonestatus reload.example > rndc.out.prereload 2>&1
	grep "zone not loaded" rndc.out.prereload > /dev/null || break
	sleep 1
done
checkfor "files: reload.db, soa.db$" rndc.out.prereload
echo "@ 0 SOA . . 2 0 0 0 0" > ns1/soa.db
$RNDCCMD 10.53.0.1 reload reload.example | sed 's/^/ns1 /' | cat_i
for i in 0 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS reload.example SOA @10.53.0.1 > dig.out
	grep " 2 0 0 0 0" dig.out >/dev/null && break
	sleep 1
done
$RNDCCMD 10.53.0.1 zonestatus reload.example > rndc.out.postreload 2>&1
checkfor "files: reload.db, soa.db$" rndc.out.postreload
sleep 1
echo "@ 0 SOA . . 3 0 0 0 0" > ns1/reload.db
echo "@ 0 NS ." >> ns1/reload.db
rndc_reload ns1 10.53.0.1 reload.example
for i in 0 1 2 3 4 5 6 7 8 9
do
	$DIG $DIGOPTS reload.example SOA @10.53.0.1 > dig.out
	grep " 3 0 0 0 0" dig.out >/dev/null && break
	sleep 1
done
$RNDCCMD 10.53.0.1 zonestatus reload.example > rndc.out.removeinclude 2>&1
checkfor "files: reload.db$" rndc.out.removeinclude

if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking 'rdnc zonestatus' with duplicated zone name"
ret=0 
$RNDCCMD 10.53.0.1 zonestatus duplicate.example > rndc.out.duplicate 2>&1
checkfor "zone 'duplicate.example' was found in multiple views" rndc.out.duplicate
$RNDCCMD 10.53.0.1 zonestatus duplicate.example in primary > rndc.out.duplicate 2>&1
checkfor "name: duplicate.example" rndc.out.duplicate
$RNDCCMD 10.53.0.1 zonestatus nosuchzone.example > rndc.out.duplicate 2>&1
checkfor "no matching zone 'nosuchzone.example' in any view" rndc.out.duplicate
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking 'rdnc zonestatus' with big serial value"
ret=0
$RNDCCMD 10.53.0.1 zonestatus bigserial.example > rndc.out.bigserial 2>&1
checkfor "serial: 3003113544" rndc.out.bigserial
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
