#! /bin/sh
## $OpenLDAP$
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 2016-2021 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
##
## ACKNOWLEDGEMENTS:
## This module was written in 2016 by Ondřej Kuzník for Symas Corp.

OVERLAY_CONFIG=${OVERLAY_CONFIG-data/config.ldif}

mkdir -p $TESTDIR $DBDIR1

mkdir $TESTDIR/confdir
. $CONFFILTER $BACKEND $MONITORDB < $CONF > $CONF1

$SLAPPASSWD -g -n >$CONFIGPWF
echo "database config" >>$CONF1
echo "rootpw `$SLAPPASSWD -T $CONFIGPWF`" >>$CONF1

echo "Starting slapd on TCP/IP port $PORT1 for configuration..."
$SLAPD -f $CONF1 -F $TESTDIR/confdir -h $URI1 -d $LVL > $LOG1 2>&1 &
PID=$!
if test $WAIT != 0 ; then
	echo PID $PID
	read foo
fi
KILLPIDS="$PID"

sleep $SLEEP0

for i in 0 1 2 3 4 5; do
	$LDAPSEARCH -s base -b "$MONITOR" -H $URI1 \
		'objectclass=*' > /dev/null 2>&1
	RC=$?
	if test $RC = 0 ; then
		break
	fi
	echo "Waiting ${SLEEP1} seconds for slapd to start..."
	sleep ${SLEEP1}
done

$LDAPSEARCH -D cn=config -H $URI1 -y $CONFIGPWF \
	-s base -b 'cn=module{0},cn=config' 1.1 >$TESTOUT 2>&1
RC=$?
case $RC in
0)
	$LDAPMODIFY -v -D cn=config -H $URI1 -y $CONFIGPWF \
	>> $TESTOUT 2>&1 <<EOMOD
dn: cn=module{0},cn=config
changetype: modify
add: olcModuleLoad
olcModuleLoad: `pwd`/../datamorph.la
EOMOD
	;;
32)
	$LDAPMODIFY -v -D cn=config -H $URI1 -y $CONFIGPWF \
	>> $TESTOUT 2>&1 <<EOMOD
dn: cn=module,cn=config
changetype: add
objectClass: olcModuleList
olcModuleLoad: `pwd`/../datamorph.la
EOMOD
	;;
*)
	echo "Failed testing for module load entry"
	exit $RC;
	;;
esac

RC=$?
if test $RC != 0 ; then
	echo "ldapmodify failed ($RC)!"
	test $KILLSERVERS != no && kill -HUP $KILLPIDS
	exit $RC
fi

echo "Loading test datamorph configuration..."
. $CONFFILTER $BACKEND $MONITORDB < $OVERLAY_CONFIG | \
$LDAPMODIFY -v -D cn=config -H $URI1 -y $CONFIGPWF \
	> $TESTOUT 2>&1
RC=$?
if test $RC != 0 ; then
	echo "ldapmodify failed ($RC)!"
	test $KILLSERVERS != no && kill -HUP $KILLPIDS
	exit $RC
fi

if test $INDEXDB = indexdb ; then
	echo "Configure indexing for transformed attributes..."
	$LDAPMODIFY -v -D cn=config -H $URI1 -y $CONFIGPWF \
		>> $TESTOUT 2>&1 <<EOMOD
dn: olcDatabase={1}$BACKEND,cn=config
changetype: modify
add: olcDbIndex
olcDbIndex: enumerated pres,eq
olcDbIndex: number pres,eq
olcDbIndex: signed pres,eq
EOMOD
	RC=$?
	if test $RC != 0 ; then
		echo "ldapmodify failed ($RC)!"
		test $KILLSERVERS != no && kill -HUP $KILLPIDS
		exit $RC
	fi
else
	echo "Skipping indexing setup for this database"
fi

echo "Stopping slapd on TCP/IP port $PORT1..."
kill -HUP $KILLPIDS
KILLPIDS=""
sleep $SLEEP0

echo "Running slapadd to build slapd database..."
$SLAPADD -F $TESTDIR/confdir -l data/test.ldif
RC=$?
if test $RC != 0 ; then
	echo "slapadd failed ($RC)!"
	exit $RC
fi

echo "Starting slapd on TCP/IP port $PORT1..."
$SLAPD -F $TESTDIR/confdir -h $URI1 -d $LVL >> $LOG1 2>&1 &
PID=$!
if test $WAIT != 0 ; then
	echo PID $PID
	read foo
fi
KILLPIDS="$PID"

sleep $SLEEP0

for i in 0 1 2 3 4 5; do
	$LDAPSEARCH -s base -b "$MONITOR" -H $URI1 \
		'objectclass=*' > /dev/null 2>&1
	RC=$?
	if test $RC = 0 ; then
		break
	fi
	echo "Waiting ${SLEEP1} seconds for slapd to start..."
	sleep ${SLEEP1}
done
