#!/bin/sh
# $NetBSD: elf2aout.sh,v 1.4 2002/03/05 21:26:11 thorpej Exp $
#
# Shell script to convert an ARM ELF kernel into a bootable a.out kernel by
# changing the header block on the kernel, and shuffling bits around in the
# file.  Care has to be taken with the sections as they need to be page
# aligned.
#
# XXX In a perfect world, objcopy -O a.out-arm-netbsd would work, but
# XXX bugs lurking in BFD prevent it from doing so.

AWKPROG='\
function x(v) { printf "%c%c%c\0", v, v / 256, v / 65536 } \
{ \
        printf "\0\217\01\013"; \
        x($1); \
        x($2); \
        x($3); \
        printf "\0\0\0\0"; \
        printf "\040\0\0\360"; \
        printf "\0\0\0\0"; \
        printf "\0\0\0\0" \
}'

infile=${1}
outfile=${2}

trap "rm -f ${infile}.text ${infile}.data" 0 1 2 3 15

${OBJCOPY} -O binary -j .text ${infile} ${infile}.text || exit 1
${OBJCOPY} -O binary -j .data ${infile} ${infile}.data || exit 1

set -- `${SIZE} ${infile} | tail +2`
TEXT=$1
DATA=$2
BSS=$3

TALIGN=$(( (($TEXT + 4095) / 4096) * 4096 ))
DALIGN=$(( (($DATA + 4095) / 4096) * 4096 ))
BALIGN=$(( (($BSS + 4095) / 4096) * 4096 ))

TPAD=$(( $TALIGN - $TEXT ))
DPAD=$(( $DALIGN - $DATA ))
BPAD=$(( $BALIGN - $BSS ))

DTMP=$(( $DATA + 32768 - $TPAD ))
DTALIGN=$(( (($DTMP + 4095) /4096) * 4096 ))

TDPAD=32768
DBPAD=$(( $DTALIGN - $DTMP ))

echo TEXT	= $TEXT
echo TPAD	= $TPAD
echo TDPAD	= $TDPAD

echo DATA	= $DATA
echo DPAD	= $DPAD
echo DBPAD	= $DBPAD

(
	echo $TALIGN $DTALIGN $BSS | awk "${AWKPROG}"; \
	cat ${infile}.text; \
	dd if=/dev/zero bs=$TDPAD count=1; \
	cat ${infile}.data; \
	dd if=/dev/zero bs=$DBPAD count=1 \
) > ${outfile}

${SIZE} ${outfile}
chmod 755 ${outfile}

exit 0
