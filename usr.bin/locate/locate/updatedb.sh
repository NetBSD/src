#!/bin/sh
#
#	$NetBSD: updatedb.sh,v 1.3 2001/03/12 13:29:26 jdolecek Exp $
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

SRCHPATHS="/"				# directories to be put in the database
LIBDIR="/usr/libexec"			# for subprograms
					# for temp files
export TMPDIR="${TMPDIR:-/tmp}"
FCODES="/var/db/locate.database"	# the database

PATH="/bin:/usr/bin"
FILELIST="$TMPDIR/locate.list.$$"
trap 'rm -f $FILELIST' 0
trap 'rm -f $FILELIST; exit 1' 2 15

# Make a file list and compute common bigrams.
# Entries of each directory shall be sorted (find -s).

# search locally or everything
# find -s $SRCHPATHS -print \
find -s $SRCHPATHS \( ! -fstype local -o -fstype fdesc -o -fstype kernfs \) \
		-a -prune -o -print \
	>$FILELIST

BIGRAMS="`$LIBDIR/locate.bigram <$FILELIST`"

# code the file list
if [ -z "$BIGRAMS" ]; then
	echo 'locate: updatedb failed' >&2
else
	$LIBDIR/locate.code "$BIGRAMS" <$FILELIST >$FCODES
	chmod 644 $FCODES
fi
