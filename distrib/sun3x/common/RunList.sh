#!/bin/sh

#	$NetBSD: RunList.sh,v 1.1.1.1 1997/12/24 09:21:18 jeremy Exp $

if [ "X$1" = "X-d" ]; then
	SHELLCMD=cat
	shift
else
	SHELLCMD="sh -e"
fi

cat "$@" |
awk -f ${TOPDIR}/common/RunList.awk |
${SHELLCMD}
