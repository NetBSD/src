#!/bin/sh
# $NetBSD: elf2aout.sh,v 1.2 2002/01/31 21:50:06 chris Exp $
# Shell script to convert a cats ELF kernel into a bootable a.out kernel by
# changing the header block on the kernel, and shuffling bits around in the
# file.  Care has to be taken with the sections as they need to be page
# aligned.

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

if [ "x${TOOLDIR}" = "x" ]; then
OBJCOPY=objcopy
SIZE=size
else
OBJCOPY=${TOOLDIR}/bin/arm--netbsdelf-objcopy
SIZE=${TOOLDIR}/bin/arm--netbsdelf-size
fi

if [ ! -f netbsd ]; then 
echo "Missing netbsd kernel"
exit 1
fi

mv -f netbsd netbsd.elf
${OBJCOPY} -O binary -j .text netbsd.elf netbsd.text
${OBJCOPY} -O binary -j .data netbsd.elf netbsd.data

TEXT=`${SIZE} netbsd.elf | tail +2 | awk '
function r(v) { return sprintf("%d", ((v + 4095) / 4096)) * 4096 }
{print r($1)}'`
echo TEXT = $TEXT

TPAD=`${SIZE} netbsd.elf | tail +2 | awk '
	function r(v) { return sprintf("%d", ((v + 4095) / 4096)) * 4096 }
	{print r($1) - $1}'`
	echo TPAD = $TPAD

DATA=`${SIZE} netbsd.elf | tail +2 | awk '
function r(v) { return sprintf("%d", ((v + 4095) / 4096)) * 4096 }
{print r($2 + 32768 - (r($1) - $1))}'`
echo DATA = $DATA

DPAD=`${SIZE} netbsd.elf | tail +2 | awk '
function r(v) { return sprintf("%d", ((v + 4095) / 4096)) * 4096 }
{print r($2 + 32768 - (r($1) - $1)) - ($2 + 32768 - (r($1) - $1))}'`
echo DPAD = $DPAD

(${SIZE} netbsd.elf | tail +2 | awk "${AWKPROG}" ; \
  cat netbsd.text ; dd if=/dev/zero bs=32k count=1; cat netbsd.data; dd if=/dev/zero bs=$DPAD count=1 \
) > netbsd.aout

cp netbsd.aout netbsd
${SIZE} netbsd
chmod 755 netbsd

exit 0
