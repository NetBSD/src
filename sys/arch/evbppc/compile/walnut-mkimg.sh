#!/bin/sh
# $NetBSD: walnut-mkimg.sh,v 1.4 2010/11/06 16:23:35 uebayasi Exp $

# Convert an input to a TFTP image loadable by the IBM PowerPC OpenBIOS.

magic=5394511	# IBM OpenBIOS magic number 0x0052504f
start=0
size=0
overwrite=0

if [ $# -ne 2 ] ; then
	echo usage: $0 input image 1>&2
	exit 1
fi

input=$1; shift
output=$1; shift

: ${OBJDUMP=objdump}
: ${OBJCOPY=objcopy}

file=$( file $input )
case $file in
*:\ ELF\ *)
	start=`${OBJDUMP} -f ${input} | awk '/start address/ { print $NF }'`
	start=`printf "%d" $start`
	${OBJCOPY} -O binary ${input} ${input}.bin.$$
	;;
*)
	case $file in
	*\ [Ff]ile\ [Ss]ystem*|*\ [Ff]ilesystem*)
		overwrite=1
		;;
	esac
	cp ${input} ${input}.bin.$$
	;;
esac

size=`stat -f '%z' ${input}.bin.$$`
size=$(( ( $size + 511 ) / 512 ))

enc()
{
	local _x=$1; shift
	printf $( printf '\\x%x' $_x )
}

be32enc()
{
	local _x=$1; shift
	enc $(( ( $_x >> 24 ) & 0xff ))
	enc $(( ( $_x >> 16 ) & 0xff ))
	enc $(( ( $_x >>  8 ) & 0xff ))
	enc $(( ( $_x >>  0 ) & 0xff ))
}

{
	be32enc $magic
	be32enc $start
	be32enc $size
	be32enc 0
	be32enc $start
	be32enc 0
	be32enc 0
	be32enc 0
} > ${input}.hdr.$$

if [ $overwrite = 0 ]; then
	cat ${input}.hdr.$$ ${input}.bin.$$ > ${output}
else
	cp ${input}.bin.$$ ${output}
	dd if=${input}.hdr.$$ of=${output} conv=notrunc
fi

rm -f ${input}.hdr.$$ ${input}.bin.$$
exit
