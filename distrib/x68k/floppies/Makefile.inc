#	$NetBSD: Makefile.inc,v 1.7 1998/10/04 14:41:12 minoura Exp $

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh -s
VER!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh 
