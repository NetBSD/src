#	$NetBSD: Makefile.inc,v 1.1 2001/12/07 05:04:58 briggs Exp $

.include "../../../Makefile.inc"

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${KERNSRCDIR}/conf/osrelease.sh -s
VER!=sh ${KERNSRCDIR}/conf/osrelease.sh 
