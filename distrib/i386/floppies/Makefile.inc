#	$NetBSD: Makefile.inc,v 1.10 1997/06/14 18:56:01 perry Exp $

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh -s
VER!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh 
