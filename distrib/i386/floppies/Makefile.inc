#	$NetBSD: Makefile.inc,v 1.13 2002/02/03 16:22:19 lukem Exp $

.include "../../../Makefile.inc"

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${KERNSRCDIR}/conf/osrelease.sh -s
VER!=sh ${KERNSRCDIR}/conf/osrelease.sh
