#	$NetBSD: Makefile.inc,v 1.3 1998/08/28 02:47:58 mrg Exp $

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${TOP}/../../sys/conf/osrelease.sh -s
VER!=sh ${TOP}/../../sys/conf/osrelease.sh 
