#!/bin/sh
# $NetBSD: mkimg.sh,v 1.1 2001/06/13 06:01:51 simonb Exp $

# Covert a kernel to an tftp image loadable by the walnut IBM openbios.

if [ $# -ne 2 ] ; then
	echo usage: $0 kernel image 1>&2
	exit 1
fi

kernel=$1; shift
output=$1; shift

: ${OBJDUMP=objdump}
: ${OBJCOPY=objcopy}

start=`${OBJDUMP} -f ${kernel} | awk '/start address/ { print $NF }'`
start=`printf "%d" $start`
${OBJCOPY} -O binary ${kernel} ${kernel}.bin.$$
size=`/bin/ls -l ${kernel}.bin.$$ | awk '{ printf "%d", ( $5 + 511 ) / 512 }'`

printf "%d\n%d\n0\n%d\n0\n0\n0\n" $start $size $start |
    awk 'BEGIN { printf "\x00\x52\x50\x4f" }
	{
		printf "%c", $0 / 256 / 256 / 256 ;
		printf "%c", $0 / 256 / 256 ;
		printf "%c", $0 / 256 ;
		printf "%c", $0 ;
	}
    ' > ${output}

cat ${kernel}.bin.$$ >> ${output}

rm -f ${kernel}.bin.$$
exit
