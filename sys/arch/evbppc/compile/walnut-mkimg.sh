#!/bin/sh
# $NetBSD: walnut-mkimg.sh,v 1.2 2004/03/27 01:47:46 simonb Exp $

# Convert a kernel to an tftp image loadable by the IBM PowerPC OpenBIOS.

magic=5394511	# IBM OpenBIOS magic number 0x0052504f

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

printf "%d\n%d\n%d\n0\n%d\n0\n0\n0\n" $magic $start $size $start |
    awk '{
		printf "%c", int($0 / 256 / 256 / 256) % 256;
		printf "%c", int($0 / 256 / 256      ) % 256;
		printf "%c", int($0 / 256            ) % 256;
		printf "%c", int($0                  ) % 256;
	}
    ' > ${output}

cat ${kernel}.bin.$$ >> ${output}

rm -f ${kernel}.bin.$$
exit
