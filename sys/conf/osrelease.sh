#!/bin/sh
#
#	$NetBSD: osrelease.sh,v 1.118 2009/11/15 13:39:00 dsl Exp $
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

# We use the number specified in <sys/param.h>

path="./$0"
exec < ${path%/*}/../sys/param.h

while
	read define ver_tag release comment || exit 1
do
	[ "$define" = "#define" ] || continue;
	[ "$ver_tag" = "__NetBSD_Version__" ] || continue
	break
done

# ${release} is [M]Mmm00pp00
#
# default: return MM.mm.pp
# -m: return MM, representing only the major number; however, for -current,
#     return the next major number (e.g. for 5.99.nn, return 6)
# -n: return MM.mm
# -s: return MMmmpp (no dots)

release=${release%??}

rel_MMmm=${release%????}
rel_MM=${rel_MMmm%??}
rel_mm=${rel_MMmm#${rel_MM}}
rel_pp=${release#${rel_MMmm}00}

case $1 in
-m)
	echo "$(((${rel_MMmm}+1)/100))"
	;;
-n)
	echo "$rel_MM.$rel_mm"
	;;
-s)
	echo "$rel_MM$rel_mm$rel_pp"
	;;
*)
	echo "$rel_MM.$rel_mm.$rel_pp"
	;;
esac
