#	$NetBSD: Makefile.inc,v 1.1.1.1 1998/06/06 05:29:50 mrg Exp $

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh -s
VER!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh 
