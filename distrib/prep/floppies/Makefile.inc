#	$NetBSD: Makefile.inc,v 1.1 2001/01/16 16:53:59 nonaka Exp $

.include "../../../Makefile.inc"

# REV is revision without dots, as in "15Q"
# VER is revision with dots, as in "1.5Q"
REV!=sh ${KERNSRCDIR}/conf/osrelease.sh -s
VER!=sh ${KERNSRCDIR}/conf/osrelease.sh 
