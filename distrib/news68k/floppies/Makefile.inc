#	$NetBSD: Makefile.inc,v 1.1.2.1 1999/12/27 18:28:26 wrstuden Exp $

# REV is revision without dots, as in "14A"
# VER is revision with dots, as in "1.4A"
REV!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh -s
VER!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh 
