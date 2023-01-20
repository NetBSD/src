#!/bin/sh
#
#	$NetBSD: updatedb.sh,v 1.18 2023/01/20 13:07:09 uwe Exp $
#
# Copyright (c) 1989, 1993
#	The Regents of the University of California.  All rights reserved.
#
# This code is derived from software contributed to Berkeley by
# James A. Woods.
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
#	This product includes software developed by the University of
#	California, Berkeley and its contributors.
# 4. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
#	@(#)updatedb.csh	8.4 (Berkeley) 10/27/94
#

PROG="$(basename "${0}")"
LIBDIR="/usr/libexec"			# for subprograms
TMPDIR="/tmp"				# for temp files
FCODES="/var/db/locate.database"	# the database
CONF="/etc/locate.conf"			# configuration file

PATH="/bin:/usr/bin"

ignorefs='! -fstype local -o -fstype cd9660 -o -fstype fdesc -o -fstype kernfs -o -fstype procfs'
ignore=
SRCHPATHS=

# Quote args to make them safe in the shell.
# Usage: quotedlist="$(shell_quote args...)"
#
# After building up a quoted list, use it by evaling it inside
# double quotes, like this:
#    eval "set -- $quotedlist"
# or like this:
#    eval "\$command $quotedlist \$filename"
#
shell_quote()
{(
	local result=''
	local arg qarg
	LC_COLLATE=C ; export LC_COLLATE # so [a-zA-Z0-9] works in ASCII
	for arg in "$@" ; do
		case "${arg}" in
		'')
			qarg="''"
			;;
		*[!-./a-zA-Z0-9]*)
			# Convert each embedded ' to '\'',
			# then insert ' at the beginning of the first line,
			# and append ' at the end of the last line.
			# Finally, elide unnecessary '' pairs at the
			# beginning and end of the result and as part of
			# '\'''\'' sequences that result from multiple
			# adjacent quotes in he input.
			qarg="$(printf "%s\n" "$arg" | \
			    ${SED:-sed} -e "s/'/'\\\\''/g" \
				-e "1s/^/'/" -e "\$s/\$/'/" \
				-e "1s/^''//" -e "\$s/''\$//" \
				-e "s/'''/'/g"
				)"
			;;
		*)
			# Arg is not the empty string, and does not contain
			# any unsafe characters.  Leave it unchanged for
			# readability.
			qarg="${arg}"
			;;
		esac
		result="${result}${result:+ }${qarg}"
	done
	printf "%s\n" "$result"
)}

usage()
{
	echo "usage: $PROG [-c config]" >&2
	exit 1
}

while getopts c: f; do
	case "$f" in
	c)
		CONF="$OPTARG" ;;
	*)
		usage ;;
	esac
done
shift $((OPTIND - 1))
[ $# -ne 0 ] && usage

# read configuration file
if [ -f "$CONF" ]; then
	while read -r com args; do
		case "$com" in
		''|'#'*)
			continue ;;	# skip blank lines and comment lines
		esac
		eval "set -- $args"	# set "$@" from $args, obeying quotes
		case "$com" in
		searchpath)
			SRCHPATHS="${SRCHPATHS}${SRCHPATHS:+ }$(shell_quote "$@")"
			;;
		ignorefs)
			for i in "$@"; do
				q="$(shell_quote "${i#\!}")"
				fs=
				case "$i" in
				none)	ignorefs=
					;;
				\!*)	fs="! -fstype $q"
					;;
				*)	fs="-fstype $q"
					;;
				esac
				case "$fs" in
				'')	;;
				*)	ignorefs="${ignorefs:+${ignorefs} -o }${fs}"
					;;
				esac
			done
			;;
		ignore)
			for i in "$@"; do
				q="$(shell_quote "$i")"
				ignore="${ignore:+${ignore} -o }-path ${q}"
			done
			;;
		ignorecontents)
			for i in "$@"; do
				q="$(shell_quote "$i")"
				ignore="${ignore:+${ignore} -o }-path ${q} -print"
			done
			;;
		workdir)
			if [ $# -ne 1 ]; then
				echo "$CONF: workdir takes exactly one argument" >&2
			elif [ -d "$1" ]; then
				TMPDIR="$1"
			else
				echo "$CONF: workdir: $1 nonexistent" >&2
			fi
			;;
		database)
			if [ $# -ne 1 ]; then
				echo "$CONF: database takes exactly one argument" >&2
			else
				FCODES="$1"
			fi
			;;
		*)
			echo "$CONF: $com: unknown config command" >&2
			exit 1
			;;
		esac
	done < "$CONF"
fi

: ${SRCHPATHS:=/}			# directories to be put in the database
export TMPDIR

case "$ignorefs/$ignore" in
/)	lp= ;   rp= ;;
*)	lp='\('; rp='\) -prune -o' ;;
esac

# insert '-o' if neither $ignorefs or $ignore are empty
case "$ignorefs $ignore" in
' '*|*' ')	;;
*)		ignore="-o $ignore" ;;
esac

FILELIST=$(mktemp -t locate.list) || exit 1
trap 'rm -f "$FILELIST"' EXIT
trap 'rm -f "$FILELIST"; exit 1' INT QUIT TERM

# Make a file list and compute common bigrams.
# Entries of each directory shall be sorted (find -s).
# The "cat" process is not useless; it lets us detect write errors.

(eval "find -s ${SRCHPATHS} $lp $ignorefs $ignore $rp -print"; true) \
	| cat >> "$FILELIST"
if [ $? != 0 ]; then
	exit 1
fi

BIGRAMS=$($LIBDIR/locate.bigram <"$FILELIST")

# code the file list
if [ -z "$BIGRAMS" ]; then
	echo 'locate: updatedb failed' >&2
	exit 1
else
	$LIBDIR/locate.code -- "$BIGRAMS" <"$FILELIST" >"$FCODES"
	chmod 644 "$FCODES"
fi
