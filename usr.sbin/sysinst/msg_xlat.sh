#! /bin/sh
#	$NetBSD: msg_xlat.sh,v 1.1.2.2 2014/08/10 07:00:24 tls Exp $

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

usage()
{
	echo "usage: msg_xlat.sh [-ci] [-d msg_defs.h] [-f fmt_count]" >&2
	exit 1
}

count_fmtargs=
msg_defs=msg_defs.h
while getopts cd:f:i f
do
	case $f in
	c) count_fmtargs=1;;
	d) msg_defs=$OPTARG;;
	f) fmt_count=$OPTARG;;
	i) IGNORE_MISSING_TRANSLATIONS=y;;
	*) usage;;
	esac
done
shift $(($OPTIND - 1))
[ "$#" = 0 ] || usage

nl="
"
msg_long="((msg)(long)"
close_paren=")"
open_brace="{"
close_brace="}"
slash="/"

rval=0

# save stdin while we read the other files
exec 3<&0

# Read existing list of format arg counts
[ -n "$fmt_count" ] && {
	exec <$fmt_count || exit 2
	while read name count
	do
		eval count_$name=\$count
	done
}

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

	eval $MSG_name=$msg_number
	eval MSGNUM_$msg_number=\$MSG_name
	# eval echo \$$MSG_name \$MSGNUM_$msg_number
done

last_msg_number="$msg_number"

# Read message definition file and set up map of munbers to strings

exec <&3 3<&-

name=
msg=
OIFS="$IFS"
while
	IFS=
	read -r line
do
	[ -z "$name" ] && {
		IFS=" 	"
		set -- $line
		[ "$1" = message ] || continue
		name="$2"
		eval number=\$MSG_$name
		[ -z "$number" ] && {
			echo "ERROR: unknown message \"$name\"" >&2
			[ -n "$IGNORE_MISSING_TRANSLATIONS" ] || rval=1
			number=unknown
		}
		l=${line#*$open_brace}
		[ "$l" = "$line" ] && continue
		line="{$l"
	}
	[ -z "$msg" ] && {
		l="${line#$open_brace}"
		[ "$l" = "$line" ] && continue
		msg="$line"
	} || msg="$msg$nl$line"
	m="${msg%$close_brace}"
	[ "$m" = "$msg" ] && {
		# Allow <tab>*/* comment */ (eg XXX translate)
		m="${msg%%$close_brace*$slash[*]*[*]$slash}"
		[ "$m" = "$msg" ] &&
			continue
	}
	# We need the %b to expand the \n that exist in the message file
	msg="$(printf "%bz" "${m#$open_brace}")"
	msg="${msg%z}"
	eval old=\"\$MSGTEXT_$number\"
	[ -n "$old" -a "$number" != unknown ] && {
		echo "ERROR: Two translations for message \"$name\"" >&2
		[ -n "$IGNORE_MISSING_TRANSLATIONS" ] || rval=1
	}
	eval MSGTEXT_$number=\"\${msg}\"
	# echo $number $msg
	sv_name="$name"
	sv_msg="$msg"
	name=
	msg=
	[ -z "$count_fmtargs" -a -z "$fmt_count" ] && continue

	IFS='%'
	set -- - x$sv_msg
	[ -n "$count_fmtargs" ] && {
		echo $number $#
		continue
	}
	eval count=\${count_$number:-unknown}
	[ "$count" = $# ] || {
		echo "ERROR: Wrong number of format specifiers in \"$sv_name\", got $#, expected $count" >&2
		[ -n "$IGNORE_MISSING_TRANSLATIONS" ] || rval=1
	}
done

[ -n "$count_fmtargs" ] && exit $rval

# Output the total number of messages and the offset of each in the file.
# Use ascii numbers because generating target-ordered binary numbers
# is just a smidgen tricky in the shell.

offset="$(( 8 + $last_msg_number * 8 + 8 ))"
printf 'MSGTXTS\0%-7d\0' $last_msg_number

msgnum=0
while
	msgnum="$(( $msgnum + 1 ))"
	[ "$msgnum" -le "$last_msg_number" ]
do
	eval msg=\${MSGTEXT_$msgnum}
	[ -z "$msg" ] && {
		eval echo "ERROR: No translation for message \$MSGNUM_$msgnum" >&2
		printf '%-7d\0' 0
		[ -n "$IGNORE_MISSING_TRANSLATIONS" ] || rval=1
		continue
	}
	printf '%-7d\0' $offset
	offset="$(( $offset + ${#msg} + 1 ))"
done

# Finally output and null terminate the messages.

msgnum=0
while
	msgnum="$(( $msgnum + 1 ))"
	[ "$msgnum" -le "$last_msg_number" ]
do
	eval msg=\${MSGTEXT_$msgnum}
	[ -z "$msg" ] && continue
	printf '%s\0' $msg
done

exit $rval
