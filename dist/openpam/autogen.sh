#!/bin/sh
#
# $Id: autogen.sh,v 1.1.1.3 2008/01/27 00:54:48 christos Exp $
#

aclocal
libtoolize --copy --force
autoheader
automake -a -c --foreign
autoconf
