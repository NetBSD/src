#!/bin/csh -f
#
#	$NetBSD: updatedb.csh,v 1.10 2000/03/20 19:22:55 jdolecek Exp $
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

set SRCHPATHS = "/"			# directories to be put in the database
set LIBDIR = /usr/libexec		# for subprograms
					# for temp files
if (! $?TMPDIR) setenv TMPDIR /tmp
set FCODES = /var/db/locate.database	# the database

set path = ( /bin /usr/bin )
set filelist = $TMPDIR/locate.list.$$

# Make a file list and compute common bigrams.
# Entries of each directory shall be sorted (find -s).

# search locally or everything
# find -s ${SRCHPATHS} -print \
find -s ${SRCHPATHS} \( ! -fstype local -o -fstype fdesc -o -fstype kernfs \) \
		-a -prune -o -print \
	> $filelist

set bigrams = `$LIBDIR/locate.bigram < $filelist`

# code the file list

if { test -z "$bigrams" } then
	printf 'locate: updatedb failed\n\n'
else
	$LIBDIR/locate.code $bigrams < $filelist > $FCODES
	chmod 644 $FCODES
endif

rm $filelist
