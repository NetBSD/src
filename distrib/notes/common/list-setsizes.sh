#!/bin/sh
#
# Copyright (c) 2004 Hubert Feyrer <hubert@feyrer.de>
# All rights reserved.
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
#          This product includes software developed by Hubert Feyrer
#          for the NetBSD Project.
# 4. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# Usage:
# 1) ssh releng.netbsd.org
# 2) cd /pub/NetBSD/netbsd-2-0
# 3) sh list-setsizes.sh base
# 4) paste values into s"Binary distribution sets" section of
#    src/distrib/common/notes/contents
#

ports="acorn26 acorn32 algor alpha amd64 amiga arc atari bebox cats cesfic
	cobalt dreamcast evbarm evbmips-eb evbppc evbsh3-eb
	hp300 hp700 hpcarm hpcmips hpcsh i386 ibmnws luna68k
	mac68k macppc mipsco mmeye mvme68k mvmeppc netwinder news68k newsmips
	next68k ofppc pmax pmppc prep sandpoint sbmips-eb sgimips
	shark sparc sparc64 sun2 sun3 vax x68k xen-i386"

set="$1"

if [ "$set" = "" ]; then
	echo "Usage: $0 setname"
	echo "e.g.   $0 base"
	exit 1
fi

for port in $ports
do
	set -- `gzip -l */$port/binary/sets/${set}.tgz | tail -2 | head -1`
	compressed=`expr \( "$1" + 1048575 \) / 1048576`
	uncompressed=`expr \( "$2" + 1048575 \) / 1048576`
	setfile=$4
	nport=`echo $port | sed s,-eb,,`

	if [ "$compressed"   = "0" ]; then compressed="-unknown-" ; fi
	if [ "$uncompressed" = "0" ]; then uncompressed="-unknown-" ; fi

	echo ".if \\n[$nport] .setsize $compressed $uncompressed"
done
