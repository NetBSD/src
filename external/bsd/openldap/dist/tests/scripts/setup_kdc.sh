#! /bin/sh
# $OpenLDAP$
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 1998-2021 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.

KRB5_TRACE=$TESTDIR/k5_trace
KRB5_CONFIG=$TESTDIR/krb5.conf
KRB5_KDC_PROFILE=$KRB5_CONFIG
KRB5_KTNAME=$TESTDIR/server.kt
KRB5_CLIENT_KTNAME=$TESTDIR/client.kt
KRB5CCNAME=$TESTDIR/client.ccache

export KRB5_TRACE KRB5_CONFIG KRB5_KDC_PROFILE KRB5_KTNAME KRB5_CLIENT_KTNAME KRB5CCNAME

KDCLOG=$TESTDIR/setup_kdc.log
KSERVICE=ldap/$LOCALHOST
KUSER=kuser

. $CONFFILTER < $DATADIR/krb5.conf > $KRB5_CONFIG

PATH=${PATH}:/usr/lib/heimdal-servers:/usr/sbin:/usr/local/sbin

echo "Trying Heimdal KDC..."

command -v kdc >/dev/null 2>&1
if test $? = 0 ; then
	kstash --random-key > $KDCLOG 2>&1
	RC=$?
	if test $RC != 0 ; then
		echo "Heimdal: kstash failed, skipping GSSAPI tests"
		exit 0
	fi

	flags="--realm-max-ticket-life=1h --realm-max-renewable-life=1h"
	kadmin -l init $flags $KRB5REALM > $KDCLOG 2>&1
	RC=$?
	if test $RC != 0 ; then
		echo "Heimdal: kadmin init failed, skipping GSSAPI tests"
		exit 0
	fi

	kadmin -l add --random-key --use-defaults $KSERVICE > $KDCLOG 2>&1
	RC=$?
	if test $RC != 0 ; then
		echo "Heimdal: kadmin add failed, skipping GSSAPI tests"
		exit 0
	fi

	kadmin -l ext -k $KRB5_KTNAME $KSERVICE > $KDCLOG 2>&1
	RC=$?
	if test $RC != 0 ; then
		echo "Heimdal: kadmin ext failed, skipping GSSAPI tests"
		exit 0
	fi

	kadmin -l add --random-key --use-defaults $KUSER > $KDCLOG 2>&1
	RC=$?
	if test $RC != 0 ; then
		echo "Heimdal: kadmin add failed, skipping GSSAPI tests"
		exit 0
	fi

	kadmin -l ext -k $KRB5_CLIENT_KTNAME $KUSER > $KDCLOG 2>&1
	RC=$?
	if test $RC != 0 ; then
		echo "Heimdal: kadmin ext failed, skipping GSSAPI tests"
		exit 0
	fi

	kdc --addresses=$LOCALIP --ports="$KDCPORT/udp" > $KDCLOG 2>&1 &
else
	echo "Trying MIT KDC..."

	command -v krb5kdc >/dev/null 2>&1
	if test $? != 0; then
		echo "No KDC available, skipping GSSAPI tests"
		exit 0
	fi

	kdb5_util create -r $KRB5REALM -s -P password > $KDCLOG 2>&1
	RC=$?
	if test $RC != 0 ; then
		echo "MIT: kdb5_util create failed, skipping GSSAPI tests"
		exit 0
	fi

	kadmin.local -q "addprinc -randkey $KSERVICE" > $KDCLOG 2>&1
	RC=$?
	if test $RC != 0 ; then
		echo "MIT: admin addprinc failed, skipping GSSAPI tests"
		exit 0
	fi

	kadmin.local -q "ktadd -k $KRB5_KTNAME $KSERVICE" > $KDCLOG 2>&1
	RC=$?
	if test $RC != 0 ; then
		echo "MIT: kadmin ktadd failed, skipping GSSAPI tests"
		exit 0
	fi

	kadmin.local -q "addprinc -randkey $KUSER" > $KDCLOG 2>&1
	RC=$?
	if test $RC != 0 ; then
		echo "MIT: kadmin addprinc failed, skipping GSSAPI tests"
		exit 0
	fi

	kadmin.local -q "ktadd -k $KRB5_CLIENT_KTNAME $KUSER" > $KDCLOG 2>&1
	RC=$?
	if test $RC != 0 ; then
		echo "MIT: kadmin ktadd failed, skipping GSSAPI tests"
		exit 0
	fi

	krb5kdc -n > $KDCLOG 2>&1 &
fi

KDCPROC=$!
sleep 1

kinit -kt $KRB5_CLIENT_KTNAME $KUSER > $KDCLOG 2>&1
RC=$?
if test $RC != 0 ; then
	kill $KDCPROC
	echo "SASL/GSSAPI: kinit failed, skipping GSSAPI tests"
	exit 0
fi

pluginviewer -m GSSAPI > $TESTDIR/plugin_out 2>/dev/null
RC=$?
if test $RC != 0 ; then

	saslpluginviewer -m GSSAPI > $TESTDIR/plugin_out 2>/dev/null
	RC=$?
	if test $RC != 0 ; then
		kill $KDCPROC
		echo "cyrus-sasl has no GSSAPI support, test skipped"
		exit 0
	fi
fi

HAVE_SASL_GSS_CBIND=no

grep CHANNEL_BINDING $TESTDIR/plugin_out > /dev/null 2>&1
RC=$?
if test $RC = 0 ; then
	HAVE_SASL_GSS_CBIND=yes
fi
