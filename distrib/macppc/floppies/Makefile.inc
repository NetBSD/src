#	$NetBSD: Makefile.inc,v 1.4 2000/07/24 09:16:27 tsubai Exp $

.include "../../../Makefile.inc"

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${KERNSRCDIR}/conf/osrelease.sh -s
VER!=sh ${KERNSRCDIR}/conf/osrelease.sh 
