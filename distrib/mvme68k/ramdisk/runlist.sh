#!/bin/sh

#	$NetBSD: runlist.sh,v 1.1 1997/12/17 22:13:42 scw Exp $

if [ "X$1" = "X-d" ]; then
	SHELLCMD=cat
	shift
else
	SHELLCMD="sh -e"
fi

cat "$@" |
awk -f runlist.awk |
${SHELLCMD}
