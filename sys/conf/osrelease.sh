#!/bin/sh
#
#	$NetBSD: osrelease.sh,v 1.116 2009/10/26 15:32:38 joerg Exp $
#
# Copyright (c) 1997 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Luke Mewburn.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
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

# We use the number specified in <sys/param.h>

AWK=${AWK:-awk}
SED=${TOOL_SED:-sed}
PARAMH="`dirname $0`"/../sys/param.h
release=`$AWK '/^#define[ 	]*__NetBSD_Version__/ { print $6 }' $PARAMH`

# default: return nn.nn.nn
# -m: return the major number -- -current is the number of the next release
# -s: return nnnnnn (no dots)

case $1 in
-m)
	echo $release | $AWK -F. '{print int($1+$2/100+0.01)}'
	;;
-n)
	echo $release | $AWK -F. '{print $1 "." $2}'
	;;
-s)
	echo $release | $SED -e 's,\.,,g'
	;;
*)
	echo $release
	;;
esac
