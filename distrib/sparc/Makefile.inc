#	$NetBSD: Makefile.inc,v 1.3.2.2 1999/01/25 06:19:11 cgd Exp $

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${TOP}/../../sys/conf/osrelease.sh -s
VER!=sh ${TOP}/../../sys/conf/osrelease.sh 
