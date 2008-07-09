#! /bin/sh
# $OpenLDAP: pkg/ldap/contrib/ldapc++/version.sh,v 1.1.2.1 2008/07/09 21:59:44 quanah Exp $
#
# Copyright 2008, OpenLDAP Foundation, All Rights Reserved.
# COPYING RESTRICTIONS APPLY, see COPYRIGHT file
DIR=`dirname $0`
. $DIR/version.var

echo OL_CPP_API_VERSION=$ol_cpp_api_current:$ol_cpp_api_revision:$ol_cpp_api_age
echo OL_CPP_API_RELEASE=$ol_cpp_api_rel_major.$ol_cpp_api_rel_minor.$ol_cpp_api_rel_patch
