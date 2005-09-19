#!/bin/sh
#
# $NetBSD: buildfloppies.sh,v 1.9.2.1 2005/09/19 20:59:41 tron Exp $
#
# Copyright (c) 2002-2003 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Luke Mewburn of Wasabi Systems.
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

# set defaults
#
: ${PAX=pax}
prog=${0##*/}


usage()
{
	cat 1>&2 << _USAGE_
Usage: ${prog} [-i instboot] [-m max] [-p] [-s suffix] base size file [...]
	-i instboot	run instboot to install a bootstrap on @IMAGE@
	-m max		maximum number of floppies to build
	-p		pad last floppy to floppy size
	-s suffix	suffix for floppies
	base		basename of generated floppies
	size		size of a floppy in 512 byte blocks
	file [...]	file(s) to build
_USAGE_
	exit 1
}

plural()
{
	[ $1 -ne 1 ] && echo "s"
}

roundup()
{
	echo $(( ( $1 + $2 - 1 ) / ( $2 ) ))
}


#	parse and check arguments
#

while getopts i:m:ps: opt; do
	case ${opt} in
	i)
		instboot=${OPTARG} ;;
	m)
		maxdisks=${OPTARG} ;;
	p)
		pad=1 ;;
	s)
		suffix=${OPTARG} ;;
	\?|*)
		usage
		;;
	esac
done

shift $(( ${OPTIND} - 1 ))
[ $# -lt 3 ] && usage
floppybase=$1
floppysize=$2
shift 2
files=$*

#	setup temp file, remove existing images
#
floppy=floppy.$$.tar
trap "rm -f ${floppy}" 0 1 2 3			# EXIT HUP INT QUIT
rm -f ${floppybase}?${suffix}

#	create tar file
#
dd if=/dev/zero of=${floppy} bs=8k count=1 2>/dev/null
${PAX} -O -w -b8k ${files} >> ${floppy} || exit 1
	# XXX: use pax metafile and set perms?
if [ -n "$instboot" ]; then
	instboot=$( echo $instboot | sed -e s/@IMAGE@/${floppy}/ )
	echo "Running instboot: ${instboot}"
	eval ${instboot} || exit 1
fi

#	check size against available number of disks
#
bytes=$( ls -ln "${floppy}" | awk '{print $5}' )
blocks=$(roundup ${bytes} 512)
	# when calculating numdisks, take into account:
	#	a) the image already has an 8K tar header prepended
	#	b) each floppy needs an 8K tar volume header
numdisks=$(roundup ${blocks}-16 ${floppysize}-16)
if [ -z "${maxdisks}" ]; then
	maxdisks=${numdisks}
fi

if [ ${numdisks} -gt ${maxdisks} ]; then
	excess=$(( (${blocks}-16 - (${floppysize}-16) * ${maxdisks}) * 512 ))
	echo 1>&2 \
	    "$prog: Image is ${excess} bytes ($(( ${excess} / 1024 )) KB)"\
	    "too big to fit on ${maxdisks} disk"$(plural ${maxdisks})
	exit 1
fi

padsize=$(( ${floppysize} * ${maxdisks} ))
padcount=$(( ${padsize} - ${blocks} ))
if [ -n "${pad}" ]; then
	echo \
	    "Writing $(( ${padsize} * 512 )) bytes ($(( ${padsize} / 2 )) KB)" \
	    "on ${numdisks} disk"$(plural ${numdisks})"," \
	    "padded by $(( ${padcount} * 512 )) bytes" \
	    "($(( ${padcount} / 2 )) KB)"
else
	echo "Writing ${bytes} bytes ($(( ${blocks} / 2 )) KB)"\
	    "on ${numdisks} disk"$(plural ${numdisks})"," \
	    "free space $(( ${padcount} * 512 )) bytes" \
	    "($(( ${padcount} / 2 )) KB)"
fi

#	write disks
#
curdisk=1
image=
seek=0
skip=0
floppysize8k=$(( ${floppysize} / 16 ))
while [ ${curdisk} -le ${numdisks} ]; do
	image="${floppybase}${curdisk}${suffix}"
	echo "Creating disk ${curdisk} to ${image}"
	if [ ${curdisk} -eq 1 ]; then
		: > ${image}
	else
		echo USTARFS ${curdisk} > ${image}
	fi
	count=$(( ${floppysize8k} - ${seek} ))
	dd bs=8k conv=sync seek=${seek} skip=${skip} count=${count} \
	    if=${floppy} of=${image} 2>/dev/null

	curdisk=$(( ${curdisk} + 1 ))
	skip=$(( $skip + $count ))
	seek=1
done

#	pad last disk if necessary
#
if [ -n "${pad}" ]; then
	dd if=$image of=$image conv=notrunc conv=sync bs=${floppysize}b count=1
fi


#	final status
#
echo "Final result:"
ls -l ${floppybase}?${suffix}

exit 0
