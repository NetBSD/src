#!/bin/sh
#	$NetBSD: mkldscript.sh,v 1.1.20.2 2007/09/03 14:33:05 yamt Exp $

TEMPLATE=$1
shift

SETS=`$OBJDUMP -x $* | fgrep "RELOCATION RECORDS FOR [link_set" | \
        sort -u | sed 's/^.*\[\(.*\)\]:$/\1/'`

for s in $SETS; do
        printf "    . = ALIGN(4);\n"
        printf "    PROVIDE (__start_%s = .);\n" $s
        printf "    *(%s)\n" $s
        printf "    PROVIDE (__stop_%s = .);\n" $s
done 
