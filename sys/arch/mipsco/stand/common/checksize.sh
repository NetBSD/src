#!/bin/sh
# $NetBSD: checksize.sh,v 1.2 2000/09/26 09:06:50 wdk Exp $
#
# Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
#      This product includes software developed by Christopher G. Demetriou
#	for the NetBSD Project.
# 4. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission
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

# four arguments:
#	boot object with headers, to check bss size
#	boot file (just text+data), to check load and total sizes
#	maximum load size

progname=$0
if [ $# != 2 ] || [ ! -f $1 ]; then
	echo "usage: $progname bootfile maxload" 1>&2
	exit 1
fi

bootfile=$1
max_size=$2

prog_size=`wc -c $bootfile | awk '{print $1}'`

if [ $? != 0 ]; then
	echo "$progname: couldn't get size of $bootfile" 2>&1
	exit 1
fi

echo -n "checking sizes for $bootfile... "

if expr $prog_size \> $max_size >/dev/null 2>&1; then
	echo "MAXIMUM FILE SIZE EXCEEDED ($prog_size > $max_size)"
	exit 1
fi

echo "OK - `expr $max_size - $prog_size` bytes free"
exit 0
