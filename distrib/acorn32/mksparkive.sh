#!/bin/sh
#
# Copyright (c) 2004 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Gavan Fantom
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

#
# Creates an uncompressed spark format archive. Some metadata is included,
# notably filetypes, but CRC calculations and permissions are not. Filename
# translation is performed according to RISC OS conventions.
# 
# This script is intended to provide sufficient functionality to create
# an archive for distribution of the NetBSD/acorn32 bootloader which can be
# used directly in RISC OS.
#

# Target byte order is little endian.

print2()
{
	lowbyte=`expr $1 % 256 | xargs printf %02x`
	highbyte=`expr $1 / 256 | xargs printf %02x`
	printf "\x$lowbyte\x$highbyte"
}

print4()
{
	print2 `expr $1 % 65536`
	print2 `expr $1 / 65536`
}

makeheader()
{
	filename="$1"
	statfilename="$2"
	realfilename="$3"
	filetype=`printf %03s "$4"`
	length=`wc -c "$filename"`
	eval `stat -s "$statfilename"`
	# centiseconds since 1st Jan 1900
	timestamp=`expr $st_mtime \* 100 + 220898880000`
	lowtype=`echo "$filetype" | sed s/.//`
	hightype=`echo "$filetype" | sed s/..\$//`
	highdate=`expr $timestamp / 4294967296 | xargs printf %02x`
	lowdate=`expr $timestamp % 4294967296`

	# Header version number
	printf \\x82
	# Filename
	printf %-13.13s "$realfilename" | tr " ." \\0/
	# Compressed file length
	print4 $length
	# File date stamp
	print2 0
	# File time stamp
	print2 0
	# CRC
	print2 0
	# Original file length
	print4 $length
	# Load address (FFFtttdd)
	printf \\x$highdate
	printf \\x$lowtype
	printf \\xf$hightype
	printf \\xff
	# Exec address (dddddddd)
	print4 $lowdate
	# Attributes
	# Public read, owner read/write
	print4 19
}

makearchive()
{
	for file in "$@"
	do
		# Archive marker
		printf \\x1a
		if [ -f "$file" ]
		then
			case "$file" in
				*,???)	type=`echo "$file" | \
					    sed "s/.*,\(...\)$/\1/"`
					filename=`echo "$file" | \
					    sed "s/,...$//"`
					;;
				*)	type=fff
					filename="$file"
					;;
			esac
			makeheader "$file" "$file" "$filename" "$type"
			cat "$file"
		fi
		if [ -d "$file" ]
		then
			temp=`mktemp -t $0` || exit 1
			(
				cd "$file"
				makearchive `ls -A` >$temp
			)
			makeheader "$temp" "$file" "$file" ddc
			cat "$temp"
			rm -f "$temp"
		fi
	done

	# Archive marker
	printf \\x1a
	# Archive terminator
	printf \\x00
}

if [ $# -eq 0 ]
then
	name=`basename $0`
	echo "Usage: $name filename"
	echo "$name: Outputs an uncompressed sparkive to stdout."
fi

makearchive "$@"
