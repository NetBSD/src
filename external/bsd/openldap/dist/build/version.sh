#! /bin/sh
# $OpenLDAP$
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 1998-2017 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.

DIR=`dirname $0`
. $DIR/version.var

if test $ol_patch != X ; then
	ol_version=${ol_major}.${ol_minor}.${ol_patch}
	ol_api_lib_release=${ol_major}.${ol_minor}
	ol_type=Release
elif test $ol_minor != X ; then
	ol_version=${ol_major}.${ol_minor}.${ol_patch}
	ol_api_lib_release=${ol_major}.${ol_minor}-releng
	ol_type=Engineering
else
	ol_version=${ol_major}.${ol_minor}
	ol_api_lib_release=${ol_major}-devel
	ol_type=Devel
fi

ol_string="${ol_package} ${ol_version}-${ol_type}"
ol_api_lib_version="${ol_api_current}:${ol_api_revision}:${ol_api_age}"

echo OL_PACKAGE=\"${ol_package}\"
echo OL_MAJOR=$ol_major
echo OL_MINOR=$ol_minor
echo OL_PATCH=$ol_patch
echo OL_API_INC=$ol_api_inc
echo OL_API_LIB_RELEASE=$ol_api_lib_release
echo OL_API_LIB_VERSION=$ol_api_lib_version
echo OL_VERSION=$ol_version
echo OL_TYPE=$ol_type
echo OL_STRING=\"${ol_string}\"
echo OL_RELEASE_DATE=\"${ol_release_date}\"
