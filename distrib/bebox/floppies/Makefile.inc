#	$NetBSD: Makefile.inc,v 1.3 2002/04/10 15:05:41 lukem Exp $

.include "../../../Makefile.inc"

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${_SRC_TOP_}/sys/conf/osrelease.sh -s
VER!=sh ${_SRC_TOP_}/sys/conf/osrelease.sh 

