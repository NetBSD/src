#!/bin/sh

#	$NetBSD: cvs.sh.sh,v 1.3 2003/07/26 19:46:33 salo Exp $

# ensure that HOME is set for pserver modes
UID=`id -u`
export HOME=`awk -F: '$3=='"$UID"'{print$6}' /etc/passwd`
# supplement path to include ssh and cvs
export PATH=/usr/pkg/bin:$PATH
# use ssh instead of rsh for cvs connections
export CVS_RSH=ssh

case $1 in
	netbsd)
		export CVSROOT="anoncvs@anoncvs.NetBSD.org:/cvsroot"
		shift
		;;
	freebsd)
		export CVSROOT=":pserver:anoncvs@anoncvs.freebsd.org:/home/ncvs"
		shift
		;;
	openbsd)
		export CVSROOT="anoncvs@anoncvs.usa.openbsd.org:/cvs"
		shift
		;;
	*)
		echo "configuration not supported"
		exit 0
esac

for file; do
	if ( echo $file | egrep -qi '(.*),(-r)?([0-9\.]+|[-_a-z0-9]+)$' ); then
		rev="-r`expr "$file" : '.*,\(.*\)' | sed 's/^-r//'`"
		file="`expr "$file" : '\(.*\),.*'`"
	else
		rev=""
	fi
	cvs co -p $rev $file
done
