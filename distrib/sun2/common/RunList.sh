#!/bin/sh

#	$NetBSD: RunList.sh,v 1.1 2001/05/18 00:16:38 fredette Exp $

if [ "X$1" = "X-d" ]; then
	SHELLCMD=cat
	shift
else
	SHELLCMD="sh -e"
fi

cat "$@" |
awk -f ${TOPDIR}/common/RunList.awk |
${SHELLCMD}
