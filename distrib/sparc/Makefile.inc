#	$NetBSD: Makefile.inc,v 1.2 1998/06/21 07:39:45 mrg Exp $

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${.CURDIR}/../../../sys/conf/osrelease.sh -s
VER!=sh ${.CURDIR}/../../../sys/conf/osrelease.sh 
