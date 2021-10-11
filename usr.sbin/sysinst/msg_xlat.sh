#! /bin/sh
#	$NetBSD: msg_xlat.sh,v 1.6 2021/10/11 18:46:34 rillig Exp $

#-
# Copyright (c) 2003 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by David Laight.
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

PROG=$(basename "$0")
usage()
{
	echo "Usage: $PROG [-ci] [-d msg_defs.h] [-f fmt_count]" >&2
	exit 1
}

error()
{
	echo "$PROG: ERROR $@" >&2
}

IGNORE_MISSING_TRANSLATIONS=false
count_fmtargs=false
msg_defs=msg_defs.h

while getopts cd:f:i f
do
	case "$f" in
	c) count_fmtargs=true;;
	d) msg_defs=$OPTARG;;
	f) fmt_count=$OPTARG;;
	i) IGNORE_MISSING_TRANSLATIONS=true;;
	*) usage;;
	esac
done
shift $(($OPTIND - 1))
if [ "$#" -ne 0 ]; then usage; fi

nl='
'
msg_long='((msg)(long)'
close_paren=')'
open_brace='{'
close_brace='}'
slash=/

rval=0

# save stdin while we read the other files
exec 3<&0

# Read existing list of format arg counts
if [ -n "$fmt_count" ]; then
	exec <$fmt_count || exit 2
	while read name count
	do
		eval count_$name=\$count
	done
fi

# Read header file and set up map of message names to numbers

exec <$msg_defs || exit 2

while read define MSG_name number rest
do
	if [ -z "$number" ] || [ -n "$rest" ]; then continue; fi
	if [ "$define" != "#define" ]; then continue; fi

	name="${MSG_name#MSG_}"
	if [ "$name" = "${MSG_name}" ]; then continue; fi

	msg_number="${number#$msg_long}"
	if [ "$msg_number" = "$number" ]; then continue; fi
	msg_number="${msg_number%$close_paren}"

	eval $MSG_name=$msg_number
	eval MSGNUM_$msg_number=\$MSG_name
	# eval echo \$$MSG_name \$MSGNUM_$msg_number
done

last_msg_number="$msg_number"

# Read message definition file and set up map of numbers to strings

exec <&3 3<&-

name=
msg=
while
	IFS=
	read -r line
do
	if [ -z "$name" ]; then
		IFS=" 	"
		set -- $line
		if [ "$1" != message ]; then continue; fi
		name="$2"
		eval number=\$MSG_$name
		if [ -z "$number" ]; then
			error "unknown message \"$name\""
			$IGNORE_MISSING_TRANSLATIONS || rval=1
			number=unknown
		fi
		l=${line#*$open_brace}
		if [ "$l" = "$line" ]; then continue; fi
		line="{$l"
	fi
	if [ -z "$msg" ]; then
		l=${line#$open_brace}
		if [ "$l" = "$line" ]; then continue; fi
		msg="$line"
	else
		msg="$msg$nl$line"
	fi
	m=${msg%$close_brace}
	if [ "$m" = "$msg" ]; then
		# Allow <tab>*/* comment */ (eg XXX translate)
		m=${msg%%$close_brace*$slash[*]*[*]$slash}
		if [ "$m" = "$msg" ]; then continue; fi
	fi
	# We need the %b to expand the \n that exist in the message file
	msg=$(printf %bz "${m#$open_brace}")
	msg=${msg%z}
	eval old=\"\$MSGTEXT_$number\"
	if [ -n "$old" ] &&  [ "$number" != unknown ]; then
		error "Two translations for message \"$name\""
		$IGNORE_MISSING_TRANSLATIONS || rval=1
	fi
	eval MSGTEXT_$number=\"\${msg}\"
	# echo $number $msg
	sv_name="$name"
	sv_msg="$msg"
	name=
	msg=
	if ! $count_fmtargs && [ -z "$fmt_count" ]; then continue; fi

	IFS=%
	set -- $sv_msg

	# For our purposes, empty messages are the same as words without %
	if [ $# -eq 0 ]; then set -- x; fi

	if $count_fmtargs; then
		echo $number $#
		continue
	fi
	eval count=\${count_$number:-unknown}
	if [ "$count" -ne $# ]; then
		error "Wrong number of format specifiers in \"$sv_name\", got $#, expected $count"
		$IGNORE_MISSING_TRANSLATIONS || rval=1
	fi
done
unset IFS

if $count_fmtargs; then exit $rval; fi

# Output the total number of messages and the offset of each in the file.
# Use ascii numbers because generating target-ordered binary numbers
# is just a smidgen tricky in the shell.

offset=$(( 8 + $last_msg_number * 8 + 8 ))
printf 'MSGTXTS\0%-7d\0' $last_msg_number

msgnum=0
while
	msgnum=$(( $msgnum + 1 ))
	[ "$msgnum" -le "$last_msg_number" ]
do
	eval msg=\${MSGTEXT_$msgnum}
	if [ -z "$msg" ]; then
		eval error "No translation for message \$MSGNUM_$msgnum"
		printf '%-7d\0' 0
		$IGNORE_MISSING_TRANSLATIONS || rval=1
		continue
	fi
	printf '%-7d\0' $offset
	offset=$(( $offset + ${#msg} + 1 ))
done

# Finally output and null terminate the messages.

msgnum=0
while
	msgnum=$(( $msgnum + 1 ))
	[ "$msgnum" -le "$last_msg_number" ]
do
	eval msg=\${MSGTEXT_$msgnum}
	if [ -z "$msg" ]; then continue; fi
	printf '%s\0' "$msg"
done

exit $rval
