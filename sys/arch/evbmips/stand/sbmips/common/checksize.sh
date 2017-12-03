#!/bin/sh
# $NetBSD: checksize.sh,v 1.1.1.1.6.2 2017/12/03 11:36:11 jdolecek Exp $
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
#	maximum total (load + bss) size

progname=$0
if [ $# != 4 ] || [ ! -f $1 ] || [ ! -f $2 ]; then
	echo "$progname bootobj bootfile maxload maxtotal" 1>&2
	exit 1
fi

bootobj=$1
bootfile=$2
maxloadsize=$3
maxtotalsize=$4

if [ "$SIZE" = "" ]; then
	SIZE=size
fi

size_data=`$SIZE $bootobj`
if [ $? != 0 ]; then
	echo "$progname: couldn't get size of $bootobj" 2>&1
	exit 1
fi
bss_size=`echo "$size_data" | awk ' NR == 2 { print $3 } '`

load_size=`ls -l $bootfile | awk ' { print $5 }'`

echo -n "checking sizes for $bootfile/$bootobj... "

if expr $load_size \> $maxloadsize >/dev/null 2>&1; then
	echo "MAXIMUM LOAD SIZE EXCEEDED ($load_size > $maxloadsize)"
	exit 1
fi

if expr $load_size + $bss_size \> $maxtotalsize >/dev/null 2>&1; then
	echo "MAXIMUM TOTAL SIZE EXCEEDED ($load_size + $bss_size > $maxtotalsize)"
	exit 1
fi

echo "OK"
exit 0
