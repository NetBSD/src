#!/bin/sh
#
# $Id: autogen.sh,v 1.1.1.2.10.1 2008/03/23 00:17:05 matt Exp $
#

aclocal
libtoolize --copy --force
autoheader
automake -a -c --foreign
autoconf
