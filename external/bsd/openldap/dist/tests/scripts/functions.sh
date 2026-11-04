#! /bin/sh
# $OpenLDAP$
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 2022-2024 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.

timer() {
	if [ -n "$STARTTIME" ]; then
		now=`date +%s`
		delta=`expr $now - $STARTTIME`
		date -u $DATEOPT$delta +%T
	fi
}
