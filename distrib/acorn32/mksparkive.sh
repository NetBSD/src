#!/bin/sh -e
#	$NetBSD: mksparkive.sh,v 1.8 2008/04/30 13:10:47 martin Exp $
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
# Creates a spark format archive. Some metadata is included, notably
# filetypes, but permissions are not. Filename translation is performed
# according to RISC OS conventions.
# 
# This script is intended to provide sufficient functionality to create
# an archive for distribution of the NetBSD/acorn32 bootloader which can be
# used directly in RISC OS.
#

if [ -z "${TOOL_SPARKCRC}" ]
then
	TOOL_SPARKCRC=sparkcrc
fi

if [ -z "${TOOL_STAT}" ]
then
	TOOL_STAT=stat
fi

if [ -z "${TOOL_MKTEMP}" ]
then
        TOOL_MKTEMP=mktemp
fi


# Target byte order is little endian.

print2()
{
	if [ -z "$1" ]
	then
		exit 1
	fi
	lowbyte=`expr $1 % 256 | xargs printf %02x`
	highbyte=`expr $1 / 256 | xargs printf %02x`
	printf "\x$lowbyte\x$highbyte"
}

print4()
{
	if [ -z "$1" ]
	then
		exit 1
	fi
	print2 `expr $1 % 65536`
	print2 `expr $1 / 65536`
}

makeheader()
{
	filename="$1"
	statfilename="$2"
	realfilename="$3"
	filetype=`printf %03s "$4"`
	compressed="$5"
	# length is only passed to length4, so we don't need to worry about
	# extracting only the length here.
	length=`wc -c "$filename"`
	eval `${TOOL_STAT} -s "$statfilename"`
	# centiseconds since 1st Jan 1900
	timestamp=`expr $st_mtime \* 100 + 220898880000`
	lowtype=`echo "$filetype" | sed s/.//`
	hightype=`echo "$filetype" | sed s/..\$//`
	highdate=`expr $timestamp / 4294967296 | xargs printf %02x`
	lowdate=`expr $timestamp % 4294967296`

	# Header version number
	if [ "$compressed" -ne 0 ]
	then
		printf \\xff
	else
		printf \\x82
	fi
	# Filename
	printf %-13.13s "$realfilename" | tr " ." \\0/
	# Compressed file length
	print4 $length
	# File date stamp
	print2 0
	# File time stamp
	print2 0
	# CRC
	if [ "$compressed" -ne 0 ]
	then
		print2 `${TOOL_SPARKCRC} "$statfilename"`
	else
		print2 `${TOOL_SPARKCRC} "$filename"`
	fi
	# Original file length
	if [ "$compressed" -ne 0 ]
	then
		print4 $st_size
	else
		print4 $length
	fi
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
		temp=`${TOOL_MKTEMP} -t $progname` || exit 1
		trap "rm -f $temp" 0
		# Archive marker
		printf \\x1a
		if [ -f "$file" ]
		then
			case "$file" in
				-*)	echo "Invalid filename" >&2
					exit 1
					;;
				*,???)	type=`echo "$file" | \
					    sed "s/.*,\(...\)$/\1/"`
					filename=`echo "$file" | \
					    sed "s/,...$//"`
					;;
				*)	type=fff
					filename="$file"
					;;
			esac
			# The compressed data in a sparkive is the output from
			# compress, minus the two bytes of magic at the start.
			# Compress also uses the top bit of the first byte
			# to indicate its choice of algorithm. Spark doesn't
			# understand that, so it must be stripped.
			compress -c "$file" | tail -c +3 >"$temp"
			size1=`wc -c "$file" | awk '{print $1}'`
			size2=`wc -c "$temp" | awk '{print $1}'`
			if [ $size1 -ge $size2 ]
			then
				makeheader "$temp" "$file" "$filename" "$type" 1
				nbits=`dd if="$temp" bs=1 count=1 2>/dev/null| \
				    od -t d1 | awk '{print $2}'`
				if [ $nbits -ge 128 ]
				then
					nbits=`expr $nbits - 128`
				fi
				printf \\x`printf %02x $nbits`
				tail -c +2 "$temp"
			else
				makeheader "$file" "$file" "$filename" "$type" 0
				cat "$file"
			fi
		fi
		if [ -d "$file" ]
		then
			(
				cd "$file"
				makearchive `ls -A` >$temp
			)
			if [ $? -ne 0 ]
			then
				exit 1
			fi
			makeheader "$temp" "$file" "$file" ddc 0
			cat "$temp"
		fi
		rm -f "$temp"
	done

	# Archive marker
	printf \\x1a
	# Archive terminator
	printf \\x00
}

progname=`basename $0`

if [ $# -eq 0 ]
then
	echo "Usage: $progname filename"
	echo "$progname: Outputs an uncompressed sparkive to stdout."
fi

makearchive "$@"
