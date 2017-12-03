#!/bin/sh
#	$NetBSD: mkldscript.sh,v 1.1.90.1 2017/12/03 11:36:57 jdolecek Exp $
#
#	This script is used by cats, luna68k and shark to produce
#	a kernel linker script that merges link sets for a.out kernels
#	(without -t). It is also used for the same reason by kernel modules
#	(with -t).

PROG="$(basename "$0")"
TEMPLATE=

mksets() {
    "${OBJDUMP:-objdump}" -x "$@" | fgrep "RELOCATION RECORDS FOR [link_set" | \
        sort -u | sed 's/^.*\[\(.*\)\]:$/\1/'
}

while getopts "t:" f; do
	case "$f" in
	t)
		TEMPLATE=${OPTARG};;
	*)
		echo "Usage: $PROG [-t <template>] objs" 1>^&2
		exit 1;;
	esac
done

shift $((OPTIND - 1))

SETS=$(mksets "$@")

if [ -n "${TEMPLATE}" ]; then
	grep -v '^}$' "${TEMPLATE}"
fi

for s in $SETS; do
        printf '   . = ALIGN(4);\n'
        printf '   PROVIDE (__start_%s = .);\n' $s
	if [ -n "${TEMPLATE}" ]; then
		printf '   %s : { *(%s) }\n' $s $s
	else
		printf '   *(%s)\n' $s
	fi
        printf '   PROVIDE (__stop_%s = .);\n' $s
done

if [ -n "${TEMPLATE}" ]; then
	printf '}\n'
fi
