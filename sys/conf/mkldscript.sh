#!/bin/sh
#	$NetBSD: mkldscript.sh,v 1.2 2014/11/12 02:15:58 christos Exp $

TEMPLATE="$1"
shift

mksets() {
    "${OBJDUMP:-objdump}" -x "$@" | fgrep "RELOCATION RECORDS FOR [link_set" | \
        sort -u | sed 's/^.*\[\(.*\)\]:$/\1/'
}
SETS=$(mksets "$@")


grep -v '^}$' "$TEMPLATE"
for s in $SETS; do
        printf '   . = ALIGN(4);\n'
        printf '   PROVIDE (__start_%s = .);\n' $s
        printf '   %s : { *(%s) }\n' $s $s
        printf '   PROVIDE (__stop_%s = .);\n' $s
done
printf '}\n'
