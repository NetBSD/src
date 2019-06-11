#! /bin/sh
#	$NetBSD: msg_cmp.sh,v 1.2 2019/06/11 15:31:19 martin Exp $

#-
# Copyright (c) 2019 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation.
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


# Compare two (binary) sysinst msg files and report identical messages.
# Used to find untranslated strings.

usage()
{
	echo "usage: msg_cmp.sh msg_defs.h file1 file2" >&2
	exit 1
}

[ "$#" = 3 ] || usage

msg_defs=$1
msg_long="((msg)(long)"
close_paren=")"

TMP1=/tmp/mct1.$$
TMP2=/tmp/mct2.$$
inp1=$2
inp2=$3

# Read header file and set up map of message names to numbers

exec <$msg_defs || exit 2

while read define MSG_name number rest
do
	[ -z "$number" -o -n "$rest" ] && continue
	[ "$define" = "#define" ] || continue
	name="${MSG_name#MSG_}"
	[ "$name" = "${MSG_name}" ] && continue
	msg_number="${number#$msg_long}"
	[ "$msg_number" = "$number" ] && continue
	msg_number="${msg_number%$close_paren}"

	varname=MSGNAME_$msg_number
	eval $varname=$name
done

# Make the sysinst binary message files usable with text tools
set -- $(tr '\000' '\n' < $inp1); off1=$(( $(( $2 + 2 )) \* 8 ))
set -- $(tr '\000' '\n' < $inp2); off2=$(( $(( $2 + 2 )) \* 8 ))
dd bs=1 skip=$off1 if=$inp1 2>/dev/null | tr '\n' '~' | tr '\000' '\n' > $TMP1
dd bs=1 skip=$off2 if=$inp2 2>/dev/null | tr '\n' '~' | tr '\000' '\n' > $TMP2

# Open both input files
exec 3< $TMP1 
exec 4< $TMP2

# Compare lines
IFS=''
NUM=0
HDR="Messages identical to the English version:"

while
	read -r l1 <&3 && read -r l2 <&4
do
	NUM=$(( $NUM + 1 ))
	if [ "$l1" != "$l2" ]; then
		continue
	fi
	[ -z "$hdr_done" ] && hdr_done=1 && printf "%s\n" $HDR
	varname=MSGNAME_$NUM
	eval $( printf "msg=$%s" $varname )
	printf "%s (%d):\t%s\n" $msg $NUM $l1
done

rm $TMP1 $TMP2

