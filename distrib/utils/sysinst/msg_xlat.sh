#! /bin/sh
#	$NetBSD: msg_xlat.sh,v 1.2 2003/07/07 12:30:22 dsl Exp $

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
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
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

nl="
"

rval=0

# Read header file and set up map of message names to numbers

exec 3<&0 <msg_defs.h

while read define MSG_name number rest
do
	[ -z "$number" -o -n "$rest" ] && continue
	[ "$define" = "#define" ] || continue
	name="${MSG_name#MSG_}"
	[ "$name" = "${MSG_name}" ] && continue
	msg_number="${number#((msg)(long)}"
	[ "$msg_number" = "$number" ] && continue
	msg_number="${msg_number%)}"

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
		l=${line#*\{}
		[ "$l" = "$line" ] && continue
		line="{$l"
	}
	[ -z "$msg" ] && {
		l="${line#\{}"
		[ "$l" = "$line" ] && continue
		msg="$line"
	} || msg="$msg$nl$line"
	m="${msg%\}}"
	[ "$m" = "$msg" ] && {
		# Allow <tab>*/* comment */ (eg XXX translate)
		m="${msg%%\}*/\**\*/}"
		[ "$m" = "$msg" ] &&
			continue
	}
	# We need the %b to expand the \n that exist in the message file
	msg="$(printf "%bz" "${m#\{}")"
	msg="${msg%z}"
	eval old=\"\$MSGTEXT_$number\"
	[ -n "$old" ] && {
		echo "ERROR: Two translations for message \"$name\"" >&2
		[ -n "$IGNORE_MISSING_TRANSLATIONS" ] || rval=1
	}
	eval MSGTEXT_$number=\"\${msg}\"
	# echo $number $msg
	name=
	msg=
done


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
