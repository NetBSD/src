#! /bin/sh
# Copyright (c) 1993 Winning Strategies, Inc.
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
#      This product includes software developed by Winning Strategies, Inc.
# 4. The name of the author may not be used to endorse or promote products
#    derived from this software withough specific prior written permission
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# $Header: /cvsroot/src/libexec/makewhatis/Attic/makewhatis.sh,v 1.1 1993/12/21 03:28:04 cgd Exp $
#

trap "rm -f /tmp/whatis$$; exit 1" 1 2 15

MANDIR=${1-/usr/share/man}
if test ! -d "$MANDIR"; then 
	echo "makewhatis: $MANDIR: not a directory"
	exit 1
fi

find $MANDIR -type f -name '*.0' -print | while read file
do
	sed -n -f /usr/share/man/makewhatis.sed $file;
done | sort -u > /tmp/whatis$$

#install -o bin -g bin -m 444 /tmp/whatis$$ "$MANDIR/whatis.db"
exit 0
