# $NetBSD: runlist.sh,v 1.1 1997/10/18 07:42:33 mark Exp $

if [ "X$1" = "X-d" ]; then
	SHELLCMD=cat
	shift
else
	SHELLCMD="sh -e"
fi

( while [ "X$1" != "X" ]; do
	cat $1
	shift
done ) | awk -f ${CURDIR}/list2sh.awk | ${SHELLCMD}
