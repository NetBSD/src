#	$NetBSD: Makefile.inc,v 1.1 1999/12/09 14:58:59 tsutsui Exp $

# REV is revision without dots, as in "14A"
# VER is revision with dots, as in "1.4A"
REV!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh -s
VER!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh 
