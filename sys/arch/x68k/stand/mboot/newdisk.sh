#!/bin/sh

#
# Copyright (c) 1999 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Minoura Makoto.
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
#	This product includes software developed by the NetBSD
#	Foundation, Inc. and its contributors.
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

# Create the disk mark for x68k SCSI IPL.
#  Problem: The script requires awk, which is toooo large for miniroot.
#  Solution: Write C version in distrib/utils/sysinst/arch/x68k/md.c.

# Usage: /usr/mdec/newdisk [-vnfc] [-m /usr/mdec/mboot] /dev/rsd?c

prog=$0
verbose=0
dryrun=0
force=0
checkonly=0
Usage="Usage: $prog [-v] [-n] [-f] [-c] [-m /usr/mdec/mboot] /dev/rsdXc"
mboot=/usr/mdec/mboot
AWK=/usr/bin/awk
SED=/usr/bin/sed
DD=/bin/dd
DISKLABEL=/sbin/disklabel
C="NetBSD/x68k SCSI Primary Boot. \
(C) 1999 by The NetBSD Foundation, Inc."

vmessage () {
    if [ "$verbose" = "1" ]; then
	echo "$@" 1>&2
    fi
    return 0
}
message () {
    echo "$@" 1>&2
    return 0
}


##
## Process command line.
##
args=`getopt vnfcm: $*`
if [ $? != 0 ]; then
    message $Usage
    exit 1
fi

set -- $args
for i; do
    case "$i" in
    -v)
	verbose=1; shift;;
    -n)
	dryrun=1; shift;;
    -f)
	force=1; shift;;
    -c)
	checkonly=1; shift;;
    -m)
	mboot=$2; shift; shift;;
    --)
	shift; break;;
    esac
done
if [ $# != 1 ]; then
    message $Usage
    exit 1
fi
dev=$1
case $dev in
    /dev/rsd[0-9]c|/dev/rvnd[0-9]c)
	;;
    /dev/sd[0-9]c|/dev/vnd[0-9]c)
	dev=`echo $dev | $SED -e 's+/dev/+/dev/r+'`;;
    rsd[0-9]c|rvnd[0-9]c)
	dev=/dev/$dev;;
    rsd[0-9]|rvnd[0-9])
	dev=/dev/${dev}c;;
    sd[0-9]c|vnd[0-9]c)
	dev=/dev/r$dev;;
    sd[0-9]|vnd[0-9])
	dev=/dev/r${dev}c;;
    *)
	message $Usage
	exit 1;;
esac


if [ ! -f $mboot ]; then
    message "$prog: $mboot: No such file or directory."
    exit 1
fi
if [ \( ! -s $mboot \) -o `wc -c < $mboot` -gt 1024 ]; then
    message "$prog: $mboot: Invalid mboot."
    exit 1
fi
if [ ! -c $dev ]; then
    message "$prog: $dev: No such file or directory."
    exit 1
fi


##
## Collect disk information.
##
if [ X`$DD if=$dev | $DD bs=8 count=1 2> /dev/null` = X"X68SCSI1" ]; then
    if [ "$force" = "0" ]; then
	message "$dev is already marked.  Use -f to overwrite the existing mark."
	exit 1
    fi
fi
if [ "$checkonly" = "1" ]; then
    exit 0
fi
vmessage -n "Inspecting $dev... "
sect=`$DISKLABEL $dev 2> /dev/null | \
	$AWK -F: '/^bytes\/sector:/ \
		    {if ( $2 != 512 ) {
			print "This type of disk is not supported." \
				> "/dev/stderr"
			exit 1
		    }
		 }
		 /total sectors:/ \
		    {print $2}'` \
	|| exit 1
vmessage "total number of sector is $sect."

##
## Build the disk mark.
##
vmessage -n "Building disk mark... "
$AWK -v "n=$sect" -v "C=$C" '
	BEGIN {
	    printf "X68SCSI1%c%c%c%c%c%c%c%c%s", \
		2, 0, \
		(n/16777216)%256, (n/65536)%256, (n/256)%256, n%256, \
		1, 0, C
	}' < /dev/null \
    | $DD bs=1024 conv=sync > /tmp/diskmark$$ 2> /dev/null
vmessage "done."


##
## Copy mboot
##
vmessage -n "Merging $mboot... "
$DD if=$mboot bs=1024 count=1 conv=sync >> /tmp/diskmark$$ 2> /dev/null
vmessage "done."


##
## Create empty partition table.
##
vmessage -n "Creating an empty partition table... "
nblock=`expr $sect / 2`		# Assume $sect == 512
$AWK -v "n=$nblock" '
	BEGIN {
	    printf "X68K%c%c%c%c%c%c%c%c%c%c%c%c", \
		0, 0, 0, 32, \
		(n/16777215)%256, (n/65536)%256, (n/256)%256, n%256, \
		(n/16777215)%256, (n/65536)%256, (n/256)%256, n%256
	}' < /dev/null \
    | $DD bs=1024 conv=sync >> /tmp/diskmark$$ 2> /dev/null
vmessage done.


##
## Do it!
##
if [ $dryrun = "0" ]; then
    vmessage -n "Writing... "
    $DISKLABEL -W $dev
    $DD if=/tmp/diskmark$$ of=$dev conv=sync 2> /dev/null
    $DISKLABEL -N $dev
    rm /tmp/diskmark$$
    vmessage "done."
    vmessage -n "Labeling... "
    disklabel $dev 2> /dev/null | sed -n -e '1,/^$/p' > /tmp/disklabel$$
    echo "3 partitions:" >> /tmp/disklabel$$
    echo "c: $sect 0 unused 0 0" >> /tmp/disklabel$$
    disklabel -R $dev /tmp/disklabel$$
    rm /tmp/disklabel$$
    vmessage "done."
    echo "$dev is now ready for use."
    echo "Note $dev cannot be used with Human68k."
else
    message "Disk mark is kept in /tmp/diskmark$$."
fi
