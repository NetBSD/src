#!/bin/sh
# $NetBSD: mkbootinfo.sh,v 1.1 2008/01/23 23:09:43 garbled Exp $

OS=$1
BPART=$2
OUT=$3

if [ -z "$OS" ]; then
	OS="NetBSD/ofppc"
fi

if [ -z "$BPART" ]; then
	BPART=3
fi

if [ -z "$OUT" ]; then
	OUT="bootinfo.txt"
fi

echo "<chrp-boot>" > $OUT
echo "<description>${OS}</description>" >>$OUT
echo "<os-name>${OS}</os-name>" >>$OUT
echo "<boot-script>boot &device;:${BPART}</boot-script>" >>$OUT
if [ -f "./netbsd.chrp" ]; then
	/bin/cat ./netbsd.chrp >>$OUT
else
	/bin/cat /usr/mdec/netbsd.chrp >>$OUT
fi
echo "</chrp-boot>" >>$OUT
