#!/bin/sh
#
# $NetBSD: rgb_mkdb.sh,v 1.1 2008/09/05 05:20:39 lukem Exp $
#
# Copyright (c) 2008 The NetBSD Foundation, Inc.
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

# set defaults
#
: ${TOOL_AWK=awk}
: ${TOOL_DB=db}
prog=${0##*/}

usage()
{
	cat 1>&2 << _USAGE_
Usage: ${prog} infile.txt outfile.db
_USAGE_
	exit 1
}

[ $# -ne 2 ] && usage
infile=$1
outfile=$2

${TOOL_AWK} < ${infile} '
	/^!/ { next; }
	NF >= 4 {
		printf "%s", tolower($4)
		for (i = 5; i <= NF; i++)
			printf "\\040%s", tolower($i)
		printf " \\%03o\\%03o\\%03o\\%03o\\%03o\\%03o\n",
			$1, $1, $2, $2, $3, $3
	}' \
| ${TOOL_DB} -q -w -N -U b -f - hash ${outfile}
