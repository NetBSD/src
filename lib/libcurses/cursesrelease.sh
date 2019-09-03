#!/bin/sh

#	$NetBSD: cursesrelease.sh,v 1.1 2019/09/03 10:36:17 roy Exp $
#
# Copyright (c) 1997 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Luke Mewburn.
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

# We use the number specified in <curses.h>

path="$0"
[ "${path#/*}" = "$path" ] && path="./$path"
exec < ${path%/*}/curses.h

# Search for line
# #define __NetBSD_Curses_Version__ <ver_num> /* NetBSD-Curses <ver_text> */
#
# <ver_num> and <ver_text> should match!

while
	read define ver_tag rel_num comment_start NetBSD rel_text rest || exit 1
do
	[ "$define" = "#define" ] || continue;
	[ "$ver_tag" = "__NetBSD_Curses_Version__" ] || continue
	break
done

# ${rel_num} is [M]Mmm00pp00
rel_num=${rel_num%??}
rel_MMmm=${rel_num%????}
rel_MM=${rel_MMmm%??}
rel_mm=${rel_MMmm#${rel_MM}}
rel_pp=${rel_num#${rel_MMmm}00}

echo "${rel_MM#0}.${rel_mm#0}.${rel_pp#0}"
