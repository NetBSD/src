#	$NetBSD: Makefile.inc,v 1.6.2.1 1998/10/24 01:03:43 cgd Exp $

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh -s
VER!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh
