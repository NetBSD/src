#!/bin/sh
# $NetBSD: elf2aout.sh,v 1.1 2002/02/06 19:54:47 thorpej Exp $
# Shell script to convert an ARM ELF kernel into a bootable a.out kernel by
# changing the header block on the kernel, and shuffling bits around in the
# file.  Care has to be taken with the sections as they need to be page
# aligned.
#
# XXX In a perfect world, objcopy -O a.out-arm-netbsd would work, but
# XXX bugs lurking in BFD prevent it from doing so.

AWKPROG='\
function r(v) { return sprintf("%d", ((v + 4095) / 4096)) * 4096 } \
function x(v) { printf "%c%c%c\0", v, v / 256, v / 65536 } \
{ \
        printf "\0\217\01\013"; \
        x(r($1)); \
        x(r($2 + 32768 - (r($1) - $1))); \
        x($3); \
        printf "\0\0\0\0\040\0\0\360\0\0\0\0\0\0\0\0" \
}'

infile=${1}
outfile=${2}

trap "rm -f ${infile}.text ${infile}.data" 0 1 2 3 15

${OBJCOPY} -O binary -j .text ${infile} ${infile}.text || exit 1
${OBJCOPY} -O binary -j .data ${infile} ${infile}.data || exit 1

TEXT=`${SIZE} ${infile} | tail +2 | awk '
function r(v) { return sprintf("%d", ((v + 4095) / 4096)) * 4096 }
{print r($1)}'`
echo TEXT = $TEXT

TPAD=`${SIZE} ${infile} | tail +2 | awk '
	function r(v) { return sprintf("%d", ((v + 4095) / 4096)) * 4096 }
	{print r($1) - $1}'`
	echo TPAD = $TPAD

DATA=`${SIZE} ${infile} | tail +2 | awk '
function r(v) { return sprintf("%d", ((v + 4095) / 4096)) * 4096 }
{print r($2 + 32768 - (r($1) - $1))}'`
echo DATA = $DATA

DPAD=`${SIZE} ${infile} | tail +2 | awk '
function r(v) { return sprintf("%d", ((v + 4095) / 4096)) * 4096 }
{print r($2 + 32768 - (r($1) - $1)) - ($2 + 32768 - (r($1) - $1))}'`
echo DPAD = $DPAD

(${SIZE} ${infile} | tail +2 | awk "${AWKPROG}" ; \
  cat ${infile}.text ; dd if=/dev/zero bs=32k count=1; cat ${infile}.data; dd if=/dev/zero bs=$DPAD count=1 \
) > ${outfile}

${SIZE} ${outfile}
chmod 755 ${outfile}

exit 0
