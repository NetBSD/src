#	$NetBSD: Makefile.inc,v 1.12.2.1 2002/03/25 17:32:47 he Exp $

.include "../../../Makefile.inc"

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh -s
VER!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh 
VERDEP= ${.CURDIR}/../../../../sys/conf/osrelease.sh 
