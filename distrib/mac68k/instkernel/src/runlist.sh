#	$NetBSD: runlist.sh,v 1.1 1999/03/26 08:34:38 ender Exp $

if [ "X$1" = "X-d" ]; then
	SHELLCMD=cat
	shift
else
	SHELLCMD="sh -e"
fi

( while [ "X$1" != "X" ]; do
	cat $1
	shift
done ) | awk -f ${TOPDIR}/src/list2sh.awk | ${SHELLCMD}
