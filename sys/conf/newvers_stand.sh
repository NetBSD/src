#!/bin/sh -
#
# $NetBSD: newvers_stand.sh,v 1.6.8.1 2009/05/13 17:19:04 jym Exp $
#
# Copyright (c) 2000 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Jaromir Dolecek.
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

# Script for generating of vers.c file from given template. Used in
# bootblock build on various architectures.
#
# Called as:
#	sh ${S}/conf/newvers_stand.sh [-NDM] VERSION_FILE ARCH [EXTRA_MSG]

cwd=$(dirname $0)

add_name=yes
add_date=yes
add_maker=yes
add_kernrev=yes

# parse command args
while getopts "NDMK?" OPT; do
	case $OPT in
	N)	add_name=no;;
	D)	add_date=no;;
	M)	add_maker=no;;
	K)	add_kernrev=no;;
	?)	echo "Syntax: newvers_stand.sh [-NDMK] VERSION_TEMPLATE ARCH EXTRA_COMMENT" >&2
		exit 1;;
	esac
done

shift `expr $OPTIND - 1`

r=`awk -F: '$1 ~ /^[0-9.]*$/ { it = $1; } END { print it }' $1`

# always add revision info
echo "const char bootprog_rev[] = \"${r}\";" > vers.c

if [ $add_name = yes ]; then
	a="$2"		# architecture name
	extra=${3:+" $3"}

	echo "const char bootprog_name[] = \"NetBSD/${a}${extra}\";" >> vers.c
fi

if [ $add_date = yes ]; then
	t=`LC_ALL=C date`
	echo "const char bootprog_date[] = \"${t}\";" >> vers.c
fi

if [ $add_maker = yes ]; then
	u=${USER-root} h=`hostname`
	echo "const char bootprog_maker[] = \"${u}@${h}\";" >> vers.c
fi

if [ $add_kernrev = yes ]; then
	osr=$(sh ${cwd}/osrelease.sh)
	echo "const char bootprog_kernrev[] = \"${osr}\";" >> vers.c
fi
