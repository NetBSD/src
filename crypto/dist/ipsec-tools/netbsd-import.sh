#! /bin/sh
#
#	$NetBSD: netbsd-import.sh,v 1.3.6.1 2009/02/08 18:42:14 snj Exp $
#
# Copyright (c) 2000-2005 The NetBSD Foundation, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the NetBSD
#	Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# netbsd-import: prepare ipsec-tools distribution for import 
# in the NetBSD tree, under src/crypto/dist/ipsec-tools
# Based on bind2netbsd.
#
# Instructions for importing a newer ipsec-tools release:
#
#	$ tag=ipsec-tools-0_6-20050224
#	$ cd /tmp
#	$ cvs -danoncvs@cvs.sf.net:/cvsroot/ipsec-tools co -r $tag ipsec-tools
#	$ cd ipsec-tools
#	$ /usr/src/crypto/dist/ipsec-tools/netbsd-import.sh $tag `pwd` /usr/src
#	$ cvs -d`whoami`@cvs.netbsd.org:/cvsroot import -m 	\
#	  "Import ipsec-tools $tag" src/crypto/dist/ipsec-tools \
#	  IPSEC_TOOLS $tag
#	$ cd /usr/src/lib/libipsec
#	$ cvs -d`whoami`@cvs.netbsd.org:/cvsroot commit -m 	\
#	  "update ipsec-tools version" package_version.h 
#

test $# -ne 3 && 							\
    echo "usage: netbsd-import.sh tag ipsec-tools-src netbsdsrc" &&	\
    exit

SCRIPTNAME=$0
RELEASE=`echo $1|sed 's/^ipsec-tools-//; s/_/\./'`
DISTSRC=$2
NETBSDSRC=$3

### Remove CVS directories and .cvsignore files
find ${DISTSRC} -type d -name CVS -print | while read d ; do 		\
    rm -R $d && echo "removed $d" ;					\
done
find ${DISTSRC} -type f -name .cvsignore -print | while read f ; do	\
    rm $f && echo "removed $f" ;					\
done

### Remove the $'s around RCS tags
find ${DISTSRC} -type f -print | 				\
    xargs egrep -l '\$(Id|Created|Header)' | while read f; do
	sed -e 's/\$\(Id.*\) \$/\1/' \
	    -e 's/\$\(Created.*\) \$/\1/' \
	    -e 's/\$\(Header.*\) \$/\1/' \
	    < $f > /tmp/ipsec1f$$ && mv /tmp/ipsec1f$$ $f && \
	echo "removed \$RCS tag from $f"
done

### Add our NetBSD RCS Id
find ${DISTSRC}  -type f -name '*.[chly]' -print | while read c; do
	sed 1q < $c | grep -q '\$NetBSD' || (
echo "/*	\$NetBSD\$	*/" >/tmp/ipsec3n$$
echo "" >>/tmp/ipsec3n$$
cat $c  >> /tmp/ipsec3n$$
mv /tmp/ipsec3n$$ $c && echo "added NetBSD RCS tag to $c"
	)
done

find ${DISTSRC} -type f -name '*.[0-9]' -print | while read m; do
	sed 1q < $m | grep -q '\$NetBSD' || (
echo ".\\\"	\$NetBSD\$" >/tmp/ipsec2m$$
echo ".\\\"" >>/tmp/ipsec2m$$
cat $m >> /tmp/ipsec2m$$
mv /tmp/ipsec2m$$ $m && echo "added NetBSD RCS tag to $m"
	)
done

sed "									\
    s/^\(#define TOP_PACKAGE_VERSION \).*/\1 \"${RELEASE}\"/;		\
    s/^\(#define TOP_PACKAGE_STRING \).*/\1 \"ipsec-tools ${RELEASE}\"/;\
" ${NETBSDSRC}/lib/libipsec/package_version.h > /tmp/ipsec5		
mv /tmp/ipsec5 ${NETBSDSRC}/lib/libipsec/package_version.h &&		\
    echo "Updated version in lib/libipsec/package_version.h"

cp ${SCRIPTNAME} ${DISTSRC} && echo "copied ${SCRIPTNAME} to ${DISTSRC}" 

echo "done, don't forget to cvs commit src/lib/libipsec/package_version.h"

