#! /bin/sh
# OpenLDAP: pkg/ldap/tests/scripts/acfilter.sh,v 1.11.2.5 2009/01/22 00:01:18 kurt Exp
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 1998-2009 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
#
# Strip comments, sort attributes. Requires GNU awk
#
if [ "$BACKEND" != ndb ]; then
grep -v '^#'
else
grep -v '^#'| awk 'BEGIN{FS="\n";RS=""} {j=0; for (i=1; i<=NF; i++){ if ($i ~ /^ /){ x[j] = x[j] "\n" $i; } else { j++; x[j] = $i } } print x[1]; delete x[1]; j=asort(x); for (i=1; i<=j; i++){ print x[i]; } delete x; print "" }'
fi
