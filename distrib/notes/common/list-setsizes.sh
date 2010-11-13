#!/bin/sh
#
# Copyright (c) 2004, 2008 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Hubert Feyrer <hubert@feyrer.de>.
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
# Usage:
# 1) ssh ftp.netbsd.org
# 2) cd /ftp/pub/NetBSD/<release>
# 3) sh list-setsizes.sh base
# 4) paste values into s"Binary distribution sets" section of
#    src/distrib/common/notes/contents
#

ports="acorn26 acorn32 algor alpha amd64 amiga arc atari bebox cats cesfic
	cobalt dreamcast evbarm evbmips evbppc evbsh3
	hp300 hp700 hpcarm hpcmips hpcsh i386 ibmnws iyonix landisk luna68k
	mac68k macppc mipsco mmeye mvme68k mvmeppc netwinder news68k newsmips
	next68k ofppc pmax prep sandpoint sbmips sgimips
	shark sparc sparc64 sun2 sun3 vax x68k"

set="$1"

if [ "$set" = "" ]; then
	echo "Usage: $0 setname"
	echo "e.g.   $0 base"
	exit 1
fi

for port in $ports
do
	set -- `gzip -l $port/binary/sets/${set}.tgz | tail -1`
	compressed=`expr \( "$1" + 1048575 \) / 1048576`
	uncompressed=`expr \( "$2" + 1048575 \) / 1048576`
	setfile=$4

	if [ "$compressed"   = "0" ]; then compressed="-unknown-" ; fi
	if [ "$uncompressed" = "0" ]; then uncompressed="-unknown-" ; fi

	echo ".if \\n[$port] .setsize $compressed $uncompressed"
done
