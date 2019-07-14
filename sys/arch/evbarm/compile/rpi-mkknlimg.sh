#!/bin/sh

# $NetBSD: rpi-mkknlimg.sh,v 1.2 2019/07/14 20:12:22 thorpej Exp $
#
# Tag an RPI kernel so the firmware will load device tree

# https://github.com/raspberrypi/linux/commit/5b2523aae9c5beb443315a7814633fc740992d07

magic_rptl=1280594002		# 'RPTL'
magic_283x=2016622642		# '283x'
magic_ddtk=1263813700		# 'DDTK'
magic_dtok=1263490116		# 'DTOK'
magic_kver=

if [ $# -ne 2 ] ; then
	echo usage: $0 input output 1>&2
	exit 1
fi

input=$1; shift
output=$1; shift

enc()
{
	local _x=$1; shift
	printf $( printf '\\%o' $_x )
}

le32enc()
{
	local _x=$1; shift
	enc $(( ( $_x >>  0 ) & 0xff ))
	enc $(( ( $_x >>  8 ) & 0xff ))
	enc $(( ( $_x >> 16 ) & 0xff ))
	enc $(( ( $_x >> 24 ) & 0xff ))
}

{
	cat ${input}

	# marker
	le32enc 0
	le32enc 0

	le32enc 1
	le32enc 4
	le32enc $magic_283x
	le32enc 1
	le32enc 4
	le32enc $magic_dtok
	le32enc 1
	le32enc 4
	le32enc $magic_ddtk

	# length ( 11 * 4 + 12)
	le32enc 56
	le32enc 4
	le32enc $magic_rptl
} > ${output}

exit
