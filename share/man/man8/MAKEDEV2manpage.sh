#!/bin/sh
#
#       $NetBSD: MAKEDEV2manpage.sh,v 1.1 2002/04/17 23:42:27 dillo Exp $
#
# Copyright (c) 2002
#       Dieter Baron <dillo@netbsd.org>.  All rights reserved.
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
#
###########################################################################
#
# Convert src/etc/etc.${ARCH}/MAKEDEV and
# src/share/man/man8/MAKEDEV.8.template to
# src/share/man/man8/man8.${ARCH}/MAKEDEV.8, replacing
#  - @@@SPECIAL@@@ with all targets in the first section (all, std, ...)
#  - @@@DEVICES@@@ with the remaining targets
#  - @@@DATE@@@ with the date from the previous version, if found
#  - @@@ARCH@@@ with the architecture name
# using src/share/man/man8/MAKEDEV2manpage.awk
#

AWK=${AWK:-awk}
DIFF=${DIFF:-diff}

mkmanpage() {
	arch=$1;
	shift;
	manpage="man8.${arch}/MAKEDEV.8";
	tmpfile="${manpage}.$$";

	${AWK} -vARCH=${arch} -f MAKEDEV2manpage.awk MAKEDEV.8.template \
		> ${tmpfile} || { rm ${tmpfile}; exit 1; }
	if ${DIFF} -I'^\.Dd ' -I'^\.\\" $NetBSD' -q ${manpage} ${tmpfile} \
		>/dev/null
	then
		result='unchanged';
		rm ${tmpfile};
	else
		result='updated';
		if [ `wc -l < ${tmpfile}` -ne `wc -l < ${manpage}` ]
		then
			LC_ALL=C LC_CTYPE=C date=`date +"%B %d, %Y`
		else
			date=`sed -n 's/^\.Dd //p' ${manpage}`
		fi
		sed "s/@@@DATE@@@/$date/" ${tmpfile} > ${tmpfile}.2
		rm ${tmpfile}
		mv ${tmpfile}.2 ${manpage}
	fi
	printf "%-20s ${result}\n" ${arch}
}


if [ $# -ne 0 ]
then
	archs="$@"
else
	archs=`ls -d man8.* | sed 's/man8\.//'`
fi

for n in ${archs}
do
	mkmanpage $n
done
