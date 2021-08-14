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
## This module was written in 2016-2017 by Ondřej Kuzník for Symas Corp.

OVERLAY_CONFIG=${OVERLAY_CONFIG-data/config.ldif}

mkdir -p $TESTDIR $DBDIR1

echo "Running slapadd to build slapd database..."
. $CONFFILTER $BACKEND $MONITORDB < $CONF > $ADDCONF
$SLAPADD -f $ADDCONF -l $LDIFORDERED
RC=$?
if test $RC != 0 ; then
	echo "slapadd failed ($RC)!"
	exit $RC
fi

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

echo "Making a modification that will be hidden by the test config..."
$LDAPMODIFY -D $MANAGERDN -H $URI1 -w $PASSWD \
	-f data/hidden.ldif >> $TESTOUT 2>&1
RC=$?
if test $RC != 0 ; then
	echo "ldapmodify failed ($RC)!"
	test $KILLSERVERS != no && kill -HUP $KILLPIDS
	exit $RC
fi

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
olcModuleLoad: `pwd`/../variant.la
EOMOD
	;;
32)
	$LDAPMODIFY -v -D cn=config -H $URI1 -y $CONFIGPWF \
	>> $TESTOUT 2>&1 <<EOMOD
dn: cn=module,cn=config
changetype: add
objectClass: olcModuleList
olcModuleLoad: `pwd`/../variant.la
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

echo "Loading test variant configuration..."
. $CONFFILTER $BACKEND $MONITORDB < $OVERLAY_CONFIG | \
$LDAPMODIFY -v -D cn=config -H $URI1 -y $CONFIGPWF \
	> $TESTOUT 2>&1
RC=$?
if test $RC != 0 ; then
	echo "ldapmodify failed ($RC)!"
	test $KILLSERVERS != no && kill -HUP $KILLPIDS
	exit $RC
fi
