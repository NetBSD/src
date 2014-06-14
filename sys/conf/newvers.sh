#!/bin/sh -
#
#	$NetBSD: newvers.sh,v 1.58 2014/06/14 12:35:18 apb Exp $
#
# Copyright (c) 1984, 1986, 1990, 1993
#	The Regents of the University of California.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University nor the names of its contributors
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
#	@(#)newvers.sh	8.1 (Berkeley) 4/20/94

if [ ! -e version ]; then
	echo 0 > version
fi

v=$(cat version)
t=$(LC_ALL=C date)
u=${USER-root}
h=$(hostname)
d=$(pwd)
cwd=$(dirname $0)
copyright=$(awk '{ printf("\"%s\\n\"", $0); }' ${cwd}/copyright)

while [ $# -gt 0 ]; do
	case "$1" in
	-r)
		rflag=true
		;;
	-i)
		id="$2"
		shift
		;;
	-n)
		nflag=true
		;;
	esac
	shift
done

if [ -z "${id}" ]; then
	if [ -f ident ]; then
		id="$(cat ident)"
	else
		id=$(basename ${d})
	fi
	# Append ".${BUILDID}" to the default value of <id>.
	# If the "-i <id>" command line option was used then this
	# branch is not taken, so the command-line value of <id>
	# is used without change.
	if [ -n "${BUILDID}" ]; then
		id="${id}.${BUILDID}"
	fi
fi

osrelcmd=${cwd}/osrelease.sh

ost="NetBSD"
osr=$(sh $osrelcmd)

if [ ! -z "${rflag}" ]; then
	fullversion="${ost} ${osr} (${id})\n"
else
	fullversion="${ost} ${osr} (${id}) #${v}: ${t}\n\t${u}@${h}:${d}\n"
fi

echo $(expr ${v} + 1) > version

cat << _EOF > vers.c
/*
 * Automatically generated file from $0
 * Do not edit.
 */
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

const char ostype[] = "${ost}";
const char osrelease[] = "${osr}";
const char sccs[] = "@(#)${fullversion}";
const char version[] = "${fullversion}";
const char kernel_ident[] = "${id}";
const char copyright[] =
${copyright}
"\n";
_EOF

[ ! -z "${nflag}" ] && exit 0

cat << _EOF >> vers.c

/*
 * NetBSD identity note.
 */
#ifdef __arm__
#define _SHT_NOTE	%note
#else
#define _SHT_NOTE	@note
#endif

#define	_S(TAG)	__STRING(TAG)
__asm(
	".section\t\".note.netbsd.ident\", \"\"," _S(_SHT_NOTE) "\n"
	"\t.p2align\t2\n"
	"\t.long\t" _S(ELF_NOTE_NETBSD_NAMESZ) "\n"
	"\t.long\t" _S(ELF_NOTE_NETBSD_DESCSZ) "\n"
	"\t.long\t" _S(ELF_NOTE_TYPE_NETBSD_TAG) "\n"
	"\t.ascii\t" _S(ELF_NOTE_NETBSD_NAME) "\n"
	"\t.long\t" _S(__NetBSD_Version__) "\n"
	"\t.p2align\t2\n"
);

_EOF
