#!/bin/sh

#	$NetBSD: RunList.sh,v 1.1.1.1.2.2 1997/12/25 20:31:50 perry Exp $

if [ "X$1" = "X-d" ]; then
	SHELLCMD=cat
	shift
else
	SHELLCMD="sh -e"
fi

cat "$@" |
awk -f ${TOPDIR}/common/RunList.awk |
${SHELLCMD}
