#!/bin/sh
#
#	$NetBSD: updatedb.sh,v 1.11 2006/04/23 03:04:08 christos Exp $
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

LIBDIR="/usr/libexec"			# for subprograms
					# for temp files
TMPDIR=/tmp
FCODES="/var/db/locate.database"	# the database
CONF=/etc/locate.conf			# configuration file

PATH="/bin:/usr/bin"

ignorefs='! -fstype local -o -fstype cd9660 -o -fstype fdesc -o -fstype kernfs -o -fstype procfs'
ignore=
SRCHPATHS=

# read configuration file
if [ -f "$CONF" ]; then
	exec 5<&0 < "$CONF"
	while read com args; do
		case "$com/$args" in /) continue;; esac	# skip blank lines
		case "$com" in
		'#'*)	;;			# lines start with # is comment
		searchpath)
			SRCHPATHS="$SRCHPATHS $args";;
		ignorefs)
			for i in $args; do
				case "$i" in
				none)	ignorefs=;;
				*)	fs=`echo "$i" | sed -e 's/^!/! -fstype /' -e t -e 's/^/-fstype /'`
					ignorefs="${ignorefs:+${ignorefs} -o }${fs}"
				esac
			done;;
		ignore)
			set -f
			for i in $args; do
				ignore="${ignore:+${ignore} -o }-path ${i}"
			done
			set +f;;
		ignorecontents)
			set -f
			for i in $args; do
				ignore="${ignore:+${ignore} -o }-path ${i} -print"
			done
			set +f;;
		workdir)
			if [ -d "$args" ]; then
				TMPDIR="$args"
			else
				echo "$CONF: workdir: $args nonexistent" >&2
			fi;;
		*)
			echo "$CONF: $com: unknown config command"	>&2
			exit 1;;
		esac
	done
	exec <&5 5>&-
fi

: ${SRCHPATHS:=/}			# directories to be put in the database
export TMPDIR

case "$ignorefs/$ignore" in
/)	lp= ;   rp= ;;
*)	lp='('; rp=') -prune -o' ;;
esac

# insert '-o' if neither $ignorefs or $ignore are empty
case "$ignorefs $ignore" in
' '*|*' ')	;;
*)		ignore="-o $ignore";;
esac

FILELIST=$(mktemp -t locate.list) || exit 1
trap "rm -f '$FILELIST'" EXIT
trap "rm -f '$FILELIST'; exit 1" INT QUIT TERM

# Make a file list and compute common bigrams.
# Entries of each directory shall be sorted (find -s).

set -f
(find -s ${SRCHPATHS} $lp $ignorefs $ignore $rp -print; true) | cat  >> "$FILELIST"
if [ $? != 0 ]
then
	exit 1
fi

BIGRAMS=$($LIBDIR/locate.bigram <"$FILELIST")

# code the file list
if [ -z "$BIGRAMS" ]; then
	echo 'locate: updatedb failed' >&2
else
	$LIBDIR/locate.code "$BIGRAMS" <"$FILELIST" >"$FCODES"
	chmod 644 "$FCODES"
fi
